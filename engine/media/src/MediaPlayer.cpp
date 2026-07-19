#include "media/MediaPlayer.h"

#include "audio/AudioEngine.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <thread>

namespace RBX::Media {

struct Player::Impl
{
    struct AudioChunk
    {
        std::vector<float> samples;
        std::size_t offsetFrames = 0;
    };

    std::filesystem::path path;
    std::shared_ptr<const std::vector<std::uint8_t>> bytes;
    mutable std::mutex mutex;
    std::condition_variable condition;
    std::thread worker;
    bool stopping = false;
    bool ready = false;
    bool loadReported = false;
    bool decodeFailed = false;
    bool failureReported = false;
    bool decodeEnded = false;
    bool seekPending = false;
    std::int64_t seekTargetMicroseconds = 0;
    std::uint64_t seekGeneration = 0;
    std::string failureText;
    StreamInfo streamInfo;
    std::deque<std::shared_ptr<VideoFrame>> videoQueue;
    std::deque<AudioChunk> audioQueue;
    std::size_t queuedAudioFrames = 0;
    std::shared_ptr<VideoFrame> displayedFrame;
    Audio::ClipHandle audioClip;
    Audio::VoiceHandle audioVoice;
    bool wantsPlayback = false;
    bool looping = false;
    float playbackVolume = 1.0f;
    double positionSeconds = 0.0;

    explicit Impl(std::filesystem::path source)
        : path(std::move(source))
        , worker([this] { decodeLoop(); })
    {
    }

    explicit Impl(std::shared_ptr<const std::vector<std::uint8_t>> source)
        : bytes(std::move(source))
        , worker([this] { decodeLoop(); })
    {
    }

    ~Impl()
    {
        {
            std::lock_guard lock(mutex);
            stopping = true;
        }
        condition.notify_all();
        if (worker.joinable())
            worker.join();
    }

    void decodeLoop() noexcept
    {
        try
        {
            std::unique_ptr<Decoder> decoder = bytes
                ? std::make_unique<Decoder>(bytes)
                : std::make_unique<Decoder>(path);
            {
                std::unique_lock lock(mutex);
                streamInfo = decoder->info();
                ready = true;
            }
            condition.notify_all();

            VideoFrame video;
            AudioFrame audio;
            std::uint64_t generation = 0;
            std::int64_t discardBefore = 0;
            while (true)
            {
                {
                    std::unique_lock lock(mutex);
                    condition.wait(lock, [this] {
                        return stopping || seekPending ||
                            (videoQueue.size() < 12 && queuedAudioFrames < 48000U * 8U);
                    });
                    if (stopping)
                        return;
                    if (seekPending)
                    {
                        discardBefore = seekTargetMicroseconds;
                        generation = seekGeneration;
                        seekPending = false;
                        decodeEnded = false;
                        lock.unlock();
                        decoder->seek(discardBefore);
                        continue;
                    }
                }

                const DecodeResult result = decoder->decodeNext(video, audio);
                std::unique_lock lock(mutex);
                if (generation != seekGeneration)
                    continue;
                if (result == DecodeResult::End)
                {
                    decodeEnded = true;
                    condition.wait(lock, [this] { return stopping || seekPending; });
                    continue;
                }
                if (result == DecodeResult::Video)
                {
                    if (video.timestampMicroseconds + 50000 < discardBefore)
                        continue;
                    videoQueue.push_back(std::make_shared<VideoFrame>(std::move(video)));
                    video = VideoFrame{};
                }
                else
                {
                    if (audio.timestampMicroseconds + 50000 < discardBefore)
                        continue;
                    queuedAudioFrames += audio.samples.size() / audio.channels;
                    audioQueue.push_back({std::move(audio.samples), 0});
                    audio = AudioFrame{};
                }
            }
        }
        catch (const std::exception& error)
        {
            std::lock_guard lock(mutex);
            decodeFailed = true;
            failureText = error.what();
        }
    }

    void destroyVoice(Audio::Engine& audio)
    {
        if (audioVoice)
        {
            audio.destroyVoice(audioVoice);
            audioVoice = {};
        }
    }

    void destroyAudio(Audio::Engine& audio)
    {
        destroyVoice(audio);
        if (audioClip)
        {
            audio.destroyClip(audioClip);
            audioClip = {};
        }
    }

    void createAudioIfNeeded(Audio::Engine& audio)
    {
        if (audioClip || !ready || !streamInfo.hasAudio)
            return;
        const std::uint64_t lengthFrames = std::max<std::uint64_t>(1,
            static_cast<std::uint64_t>(streamInfo.durationMicroseconds) * 48000U / 1000000U);
        audioClip = audio.createQueuedClip(48000, 2, lengthFrames, 48000U * 4U);
    }

    void submitAudio(Audio::Engine& audio)
    {
        createAudioIfNeeded(audio);
        if (!audioClip)
            return;
        while (!audioQueue.empty())
        {
            AudioChunk& chunk = audioQueue.front();
            const std::span<const float> remaining(chunk.samples.data() + chunk.offsetFrames * 2,
                chunk.samples.size() - chunk.offsetFrames * 2);
            const std::uint32_t submitted = audio.submitQueuedFrames(audioClip, remaining);
            if (submitted == 0)
                break;
            chunk.offsetFrames += submitted;
            queuedAudioFrames -= submitted;
            if (chunk.offsetFrames * 2 == chunk.samples.size())
                audioQueue.pop_front();
        }
        condition.notify_all();
    }

    void startVoiceIfReady(Audio::Engine& audio)
    {
        if (!wantsPlayback || audioVoice || !audioClip ||
            audio.queuedFramesAvailable(audioClip) < 4800)
            return;
        Audio::VoiceParameters parameters;
        parameters.volume = playbackVolume;
        audioVoice = audio.play(audioClip, parameters);
    }

    void requestSeek(double seconds, Audio::Engine& audio)
    {
        positionSeconds = std::clamp(seconds, 0.0,
            streamInfo.durationMicroseconds > 0
                ? streamInfo.durationMicroseconds / 1000000.0 : seconds);
        destroyVoice(audio);
        if (audioClip)
            audio.resetQueuedClip(audioClip);
        videoQueue.clear();
        audioQueue.clear();
        queuedAudioFrames = 0;
        displayedFrame.reset();
        decodeEnded = false;
        seekTargetMicroseconds = static_cast<std::int64_t>(positionSeconds * 1000000.0);
        ++seekGeneration;
        seekPending = true;
        condition.notify_all();
    }
};

Player::Player(const std::filesystem::path& path)
    : impl(std::make_unique<Impl>(path))
{
}

Player::Player(std::shared_ptr<const std::vector<std::uint8_t>> bytes)
    : impl(std::make_unique<Impl>(std::move(bytes)))
{
}

Player::~Player() = default;

void Player::close(Audio::Engine& audio)
{
    std::lock_guard lock(impl->mutex);
    impl->wantsPlayback = false;
    impl->destroyAudio(audio);
}

PlaybackEvents Player::update(Audio::Engine& audio, double elapsedSeconds)
{
    PlaybackEvents events;
    std::lock_guard lock(impl->mutex);
    events.failed = impl->decodeFailed && !impl->failureReported;
    impl->failureReported |= events.failed;
    if (impl->decodeFailed || !impl->ready)
        return events;

    impl->submitAudio(audio);
    impl->startVoiceIfReady(audio);
    if (impl->wantsPlayback)
        impl->positionSeconds += std::max(elapsedSeconds, 0.0);

    const double length = impl->streamInfo.durationMicroseconds / 1000000.0;
    if (impl->wantsPlayback && length > 0.0 && impl->positionSeconds >= length)
    {
        if (impl->looping)
        {
            impl->requestSeek(std::fmod(impl->positionSeconds, length), audio);
            events.looped = true;
        }
        else
        {
            impl->positionSeconds = length;
            impl->wantsPlayback = false;
            impl->destroyVoice(audio);
            events.ended = true;
        }
    }

    const std::int64_t position = static_cast<std::int64_t>(impl->positionSeconds * 1000000.0);
    while (!impl->videoQueue.empty() &&
        impl->videoQueue.front()->timestampMicroseconds <= position + 1000)
    {
        impl->displayedFrame = impl->videoQueue.front();
        impl->videoQueue.pop_front();
        events.frameChanged = true;
    }
    if (!impl->displayedFrame && !impl->videoQueue.empty())
    {
        impl->displayedFrame = impl->videoQueue.front();
        events.frameChanged = true;
    }
    events.loaded = impl->displayedFrame && !impl->loadReported;
    impl->loadReported |= events.loaded;
    impl->condition.notify_all();
    return events;
}

void Player::setPlaying(bool value, Audio::Engine& audio)
{
    std::lock_guard lock(impl->mutex);
    if (impl->wantsPlayback == value)
        return;
    impl->wantsPlayback = value;
    if (!value && impl->audioVoice)
        audio.pause(impl->audioVoice);
    else if (value && impl->audioVoice)
        audio.resume(impl->audioVoice);
    else if (value)
        impl->startVoiceIfReady(audio);
}

bool Player::playing() const noexcept
{
    std::lock_guard lock(impl->mutex);
    return impl->wantsPlayback;
}

void Player::setLooped(bool value) noexcept
{
    std::lock_guard lock(impl->mutex);
    impl->looping = value;
}

bool Player::looped() const noexcept
{
    std::lock_guard lock(impl->mutex);
    return impl->looping;
}

void Player::setVolume(float value, Audio::Engine& audio)
{
    std::lock_guard lock(impl->mutex);
    impl->playbackVolume = std::max(value, 0.0f);
    if (impl->audioVoice)
        audio.setVoiceVolume(impl->audioVoice, impl->playbackVolume);
}

float Player::volume() const noexcept
{
    std::lock_guard lock(impl->mutex);
    return impl->playbackVolume;
}

void Player::seek(double seconds, Audio::Engine& audio)
{
    std::lock_guard lock(impl->mutex);
    impl->requestSeek(seconds, audio);
}

double Player::timePosition() const noexcept
{
    std::lock_guard lock(impl->mutex);
    return impl->positionSeconds;
}

double Player::timeLength() const noexcept
{
    std::lock_guard lock(impl->mutex);
    return impl->streamInfo.durationMicroseconds / 1000000.0;
}

bool Player::loaded() const noexcept
{
    std::lock_guard lock(impl->mutex);
    return impl->ready;
}

bool Player::failed() const noexcept
{
    std::lock_guard lock(impl->mutex);
    return impl->decodeFailed;
}

std::string Player::failure() const
{
    std::lock_guard lock(impl->mutex);
    return impl->failureText;
}

StreamInfo Player::info() const
{
    std::lock_guard lock(impl->mutex);
    return impl->streamInfo;
}

std::shared_ptr<const VideoFrame> Player::currentFrame() const
{
    std::lock_guard lock(impl->mutex);
    return impl->displayedFrame;
}

} // namespace RBX::Media

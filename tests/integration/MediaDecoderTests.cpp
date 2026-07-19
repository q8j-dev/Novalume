#include "media/MediaDecoder.h"
#include "media/MediaPlayer.h"
#include "audio/AudioEngine.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace {

void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

} // namespace

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "expected one media fixture path");
        const std::filesystem::path fixture(argv[1]);
        require(std::filesystem::is_regular_file(fixture), "media fixture is missing");

        RBX::Media::Decoder decoder(fixture);
        const RBX::Media::StreamInfo& info = decoder.info();
        require(info.hasVideo, "video stream was not discovered");
        require(info.hasAudio, "audio stream was not discovered");
        require(info.width == 96 && info.height == 64, "video resolution is incorrect");
        require(info.durationMicroseconds >= 900000 && info.durationMicroseconds <= 1100000,
            "media duration is incorrect");

        RBX::Media::VideoFrame video;
        RBX::Media::AudioFrame audio;
        std::size_t videoFrames = 0;
        std::size_t audioFrames = 0;
        std::size_t coloredPixels = 0;
        std::size_t audioSamples = 0;
        while (true)
        {
            const RBX::Media::DecodeResult result = decoder.decodeNext(video, audio);
            if (result == RBX::Media::DecodeResult::End)
                break;
            if (result == RBX::Media::DecodeResult::Video)
            {
                ++videoFrames;
                require(video.rgba.size() == 96U * 64U * 4U,
                    "decoded RGBA frame has an invalid size");
                for (std::size_t index = 0; index < video.rgba.size(); index += 4)
                {
                    const int lowest = std::min({video.rgba[index], video.rgba[index + 1],
                        video.rgba[index + 2]});
                    const int highest = std::max({video.rgba[index], video.rgba[index + 1],
                        video.rgba[index + 2]});
                    coloredPixels += highest - lowest > 32;
                }
            }
            else
            {
                ++audioFrames;
                require(audio.sampleRate == 48000 && audio.channels == 2,
                    "decoded audio was not converted to the engine format");
                for (float sample : audio.samples)
                {
                    require(std::isfinite(sample), "decoded audio contains a non-finite sample");
                    audioSamples += std::abs(sample) > 0.001f;
                }
            }
        }
        require(videoFrames >= 20, "too few video frames were decoded");
        require(audioFrames > 0 && audioSamples > 1000, "decoded audio is silent or missing");
        require(coloredPixels > 1000, "decoded video is flat or missing color conversion");

        decoder.seek(500000);
        bool receivedSeekFrame = false;
        for (unsigned int index = 0; index < 100 && !receivedSeekFrame; ++index)
        {
            const RBX::Media::DecodeResult result = decoder.decodeNext(video, audio);
            require(result != RBX::Media::DecodeResult::End,
                "seek unexpectedly reached end of media");
            receivedSeekFrame = result == RBX::Media::DecodeResult::Video &&
                video.timestampMicroseconds >= 400000;
        }
        require(receivedSeekFrame, "seek did not resume near the requested timestamp");

        std::ifstream fixtureInput(fixture, std::ios::binary);
        auto fixtureBytes = std::make_shared<std::vector<std::uint8_t>>(
            std::istreambuf_iterator<char>(fixtureInput), std::istreambuf_iterator<char>());
        RBX::Media::Decoder memoryDecoder(fixtureBytes);
        require(memoryDecoder.info().width == 96 && memoryDecoder.info().hasAudio,
            "in-memory media input did not preserve stream metadata");
        require(memoryDecoder.decodeNext(video, audio) != RBX::Media::DecodeResult::End,
            "in-memory media input did not decode data");

        RBX::Audio::Engine audioEngine({.sampleRate = 48000, .channels = 2});
        RBX::Media::Player player(fixture);
        bool loaded = false;
        bool frameChanged = false;
        for (unsigned int index = 0; index < 500 && !loaded; ++index)
        {
            const RBX::Media::PlaybackEvents events = player.update(audioEngine, 0.0);
            loaded |= events.loaded;
            frameChanged |= events.frameChanged;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        require(loaded && frameChanged && player.loaded() && player.currentFrame(),
            "streaming player did not load a real first video frame");
        player.setVolume(0.5f, audioEngine);
        player.setPlaying(true, audioEngine);
        bool ended = false;
        float mixedEnergy = 0.0f;
        std::vector<float> mixed(480 * 2);
        for (unsigned int index = 0; index < 140 && !ended; ++index)
        {
            const RBX::Media::PlaybackEvents events = player.update(audioEngine, 0.01);
            ended |= events.ended;
            std::fill(mixed.begin(), mixed.end(), 0.0f);
            if (audioEngine.mix(mixed))
                for (float sample : mixed)
                    mixedEnergy += std::abs(sample);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        require(ended && !player.playing() && player.timePosition() >= 0.9,
            "streaming player did not reach its timed Ended state");
        require(mixedEnergy > 1.0f, "streaming player audio never reached the shared mixer");

        player.seek(0.8, audioEngine);
        player.setLooped(true);
        player.setPlaying(true, audioEngine);
        bool looped = false;
        for (unsigned int index = 0; index < 50 && !looped; ++index)
        {
            looped |= player.update(audioEngine, 0.01).looped;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        require(looped && player.playing() && player.timePosition() < 0.4,
            "looped playback did not seek and continue");
        player.setPlaying(false, audioEngine);
        require(!player.playing(), "Pause must stop the playback clock");
        player.close(audioEngine);
        std::cout << "FFmpeg " << RBX::Media::version() << " decoded " << videoFrames
                  << " video frames and " << audioFrames << " audio frames\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "MediaDecoderTests: " << error.what() << '\n';
        return 1;
    }
}

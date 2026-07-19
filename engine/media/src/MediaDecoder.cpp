#include "media/MediaDecoder.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <limits>
#include <stdexcept>

namespace RBX::Media {
namespace {

std::string errorText(int error)
{
    std::array<char, AV_ERROR_MAX_STRING_SIZE> result{};
    av_strerror(error, result.data(), result.size());
    return result.data();
}

void require(int result, const char* operation)
{
    if (result < 0)
        throw std::runtime_error(std::string(operation) + ": " + errorText(result));
}

std::int64_t timestampMicroseconds(std::int64_t timestamp, AVRational timeBase)
{
    if (timestamp == AV_NOPTS_VALUE)
        return 0;
    return av_rescale_q(timestamp, timeBase, AVRational{1, 1000000});
}

} // namespace

struct Decoder::Impl
{
    AVFormatContext* format = nullptr;
    AVIOContext* io = nullptr;
    std::shared_ptr<const std::vector<std::uint8_t>> inputBytes;
    std::size_t inputOffset = 0;
    AVCodecContext* videoCodec = nullptr;
    AVCodecContext* audioCodec = nullptr;
    SwsContext* scaler = nullptr;
    SwrContext* resampler = nullptr;
    AVPacket* packet = nullptr;
    AVFrame* frame = nullptr;
    StreamInfo streamInfo;
    int videoStream = -1;
    int audioStream = -1;
    std::uint32_t outputSampleRate = 48000;
    std::uint32_t outputChannels = 2;
    bool inputEnded = false;
    bool videoDrained = false;
    bool audioDrained = false;

    Impl(const std::filesystem::path& path, std::uint32_t sampleRate, std::uint32_t channels)
        : outputSampleRate(sampleRate)
        , outputChannels(channels)
    {
        if (path.empty() || sampleRate == 0 || channels == 0 || channels > 8)
            throw std::invalid_argument("media decoder requires a path and valid audio format");

        const std::string pathString = path.string();
        require(avformat_open_input(&format, pathString.c_str(), nullptr, nullptr), "could not open media");
        initialize();
    }

    Impl(std::shared_ptr<const std::vector<std::uint8_t>> bytes,
        std::uint32_t sampleRate, std::uint32_t channels)
        : inputBytes(std::move(bytes))
        , outputSampleRate(sampleRate)
        , outputChannels(channels)
    {
        if (!inputBytes || inputBytes->empty() || sampleRate == 0 || channels == 0 || channels > 8)
            throw std::invalid_argument("media decoder requires nonempty bytes and valid audio format");
        format = avformat_alloc_context();
        std::uint8_t* buffer = static_cast<std::uint8_t*>(av_malloc(32768));
        if (!format || !buffer)
        {
            av_free(buffer);
            throw std::bad_alloc();
        }
        io = avio_alloc_context(buffer, 32768, 0, this, &readInput, nullptr, &seekInput);
        if (!io)
        {
            av_free(buffer);
            throw std::bad_alloc();
        }
        format->pb = io;
        format->flags |= AVFMT_FLAG_CUSTOM_IO;
        try
        {
            require(avformat_open_input(&format, nullptr, nullptr, nullptr), "could not open media bytes");
            initialize();
        }
        catch (...)
        {
            close();
            throw;
        }
    }

    static int readInput(void* opaque, std::uint8_t* destination, int requested)
    {
        Impl* self = static_cast<Impl*>(opaque);
        const std::size_t remaining = self->inputBytes->size() - self->inputOffset;
        const std::size_t count = std::min<std::size_t>(remaining,
            static_cast<std::size_t>(std::max(requested, 0)));
        if (count == 0)
            return AVERROR_EOF;
        std::copy_n(self->inputBytes->data() + self->inputOffset, count, destination);
        self->inputOffset += count;
        return static_cast<int>(count);
    }

    static std::int64_t seekInput(void* opaque, std::int64_t offset, int whence)
    {
        Impl* self = static_cast<Impl*>(opaque);
        if (whence == AVSEEK_SIZE)
            return static_cast<std::int64_t>(self->inputBytes->size());
        const int origin = whence & ~AVSEEK_FORCE;
        std::int64_t base = 0;
        if (origin == SEEK_CUR)
            base = static_cast<std::int64_t>(self->inputOffset);
        else if (origin == SEEK_END)
            base = static_cast<std::int64_t>(self->inputBytes->size());
        else if (origin != SEEK_SET)
            return AVERROR(EINVAL);
        const std::int64_t target = base + offset;
        if (target < 0 || static_cast<std::uint64_t>(target) > self->inputBytes->size())
            return AVERROR(EINVAL);
        self->inputOffset = static_cast<std::size_t>(target);
        return target;
    }

    void initialize()
    {
        try
        {
            require(avformat_find_stream_info(format, nullptr), "could not inspect media streams");
            openStream(AVMEDIA_TYPE_VIDEO, videoStream, videoCodec);
            openStream(AVMEDIA_TYPE_AUDIO, audioStream, audioCodec);
            if (!videoCodec)
                throw std::runtime_error("media contains no decodable video stream");

            streamInfo.hasVideo = true;
            streamInfo.hasAudio = audioCodec != nullptr;
            streamInfo.width = static_cast<std::uint32_t>(videoCodec->width);
            streamInfo.height = static_cast<std::uint32_t>(videoCodec->height);
            if (streamInfo.width == 0 || streamInfo.height == 0 ||
                streamInfo.width > 16384 || streamInfo.height > 16384)
                throw std::runtime_error("video dimensions are invalid or exceed engine limits");
            streamInfo.durationMicroseconds = format->duration == AV_NOPTS_VALUE
                ? 0 : std::max<std::int64_t>(format->duration, 0);

            packet = av_packet_alloc();
            frame = av_frame_alloc();
            if (!packet || !frame)
                throw std::bad_alloc();

            scaler = sws_getContext(videoCodec->width, videoCodec->height,
                videoCodec->pix_fmt, videoCodec->width, videoCodec->height,
                AV_PIX_FMT_RGBA, SWS_BILINEAR, nullptr, nullptr, nullptr);
            if (!scaler)
                throw std::runtime_error("could not initialize video color conversion");

            if (audioCodec)
            {
                AVChannelLayout outputLayout{};
                av_channel_layout_default(&outputLayout, static_cast<int>(outputChannels));
                const int result = swr_alloc_set_opts2(&resampler, &outputLayout,
                    AV_SAMPLE_FMT_FLT, static_cast<int>(outputSampleRate),
                    &audioCodec->ch_layout, audioCodec->sample_fmt, audioCodec->sample_rate,
                    0, nullptr);
                av_channel_layout_uninit(&outputLayout);
                require(result, "could not configure audio conversion");
                if (!resampler)
                    throw std::runtime_error("could not allocate audio conversion");
                require(swr_init(resampler), "could not initialize audio conversion");
            }
        }
        catch (...)
        {
            close();
            throw;
        }
    }

    ~Impl()
    {
        close();
    }

    void openStream(AVMediaType type, int& streamIndex, AVCodecContext*& codecContext)
    {
        const int found = av_find_best_stream(format, type, -1, -1, nullptr, 0);
        if (found == AVERROR_STREAM_NOT_FOUND)
            return;
        require(found, "could not select media stream");
        streamIndex = found;
        AVStream* stream = format->streams[streamIndex];
        const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
        if (!codec)
            throw std::runtime_error("media stream uses an unavailable decoder");
        codecContext = avcodec_alloc_context3(codec);
        if (!codecContext)
            throw std::bad_alloc();
        require(avcodec_parameters_to_context(codecContext, stream->codecpar),
            "could not copy media codec parameters");
        require(avcodec_open2(codecContext, codec, nullptr), "could not open media decoder");
    }

    void close() noexcept
    {
        swr_free(&resampler);
        sws_freeContext(scaler);
        scaler = nullptr;
        av_frame_free(&frame);
        av_packet_free(&packet);
        avcodec_free_context(&audioCodec);
        avcodec_free_context(&videoCodec);
        avformat_close_input(&format);
        if (io)
            avio_context_free(&io);
    }

    bool receiveVideo(VideoFrame& output)
    {
        const int result = avcodec_receive_frame(videoCodec, frame);
        if (result == AVERROR(EAGAIN) || result == AVERROR_EOF)
        {
            videoDrained |= result == AVERROR_EOF;
            return false;
        }
        require(result, "video decode failed");

        output.width = streamInfo.width;
        output.height = streamInfo.height;
        const std::size_t byteCount = static_cast<std::size_t>(output.width) * output.height * 4;
        output.rgba.resize(byteCount);
        std::uint8_t* destination[] = {output.rgba.data(), nullptr, nullptr, nullptr};
        int destinationStride[] = {static_cast<int>(output.width * 4), 0, 0, 0};
        const int rows = sws_scale(scaler, frame->data, frame->linesize, 0,
            videoCodec->height, destination, destinationStride);
        if (rows != videoCodec->height)
            throw std::runtime_error("video color conversion returned an incomplete frame");
        output.timestampMicroseconds = timestampMicroseconds(
            frame->best_effort_timestamp, format->streams[videoStream]->time_base);
        av_frame_unref(frame);
        return true;
    }

    bool receiveAudio(AudioFrame& output)
    {
        const int result = avcodec_receive_frame(audioCodec, frame);
        if (result == AVERROR(EAGAIN) || result == AVERROR_EOF)
        {
            audioDrained |= result == AVERROR_EOF;
            return false;
        }
        require(result, "audio decode failed");

        const std::int64_t delayed = swr_get_delay(resampler, audioCodec->sample_rate);
        const std::int64_t capacity = av_rescale_rnd(delayed + frame->nb_samples,
            outputSampleRate, audioCodec->sample_rate, AV_ROUND_UP);
        if (capacity <= 0 || capacity > std::numeric_limits<int>::max())
            throw std::runtime_error("audio conversion requested an invalid frame count");
        output.samples.resize(static_cast<std::size_t>(capacity) * outputChannels);
        std::uint8_t* destination[] = {
            reinterpret_cast<std::uint8_t*>(output.samples.data()), nullptr};
        const int converted = swr_convert(resampler, destination, static_cast<int>(capacity),
            const_cast<const std::uint8_t**>(frame->extended_data), frame->nb_samples);
        require(converted, "audio conversion failed");
        output.samples.resize(static_cast<std::size_t>(converted) * outputChannels);
        output.sampleRate = outputSampleRate;
        output.channels = outputChannels;
        output.timestampMicroseconds = timestampMicroseconds(
            frame->best_effort_timestamp, format->streams[audioStream]->time_base);
        av_frame_unref(frame);
        return true;
    }

    DecodeResult next(VideoFrame& video, AudioFrame& audio)
    {
        while (true)
        {
            if (videoCodec && receiveVideo(video))
                return DecodeResult::Video;
            if (audioCodec && receiveAudio(audio))
                return DecodeResult::Audio;

            if (inputEnded)
            {
                if ((!videoCodec || videoDrained) && (!audioCodec || audioDrained))
                    return DecodeResult::End;
                continue;
            }

            const int read = av_read_frame(format, packet);
            if (read == AVERROR_EOF)
            {
                inputEnded = true;
                if (videoCodec)
                    require(avcodec_send_packet(videoCodec, nullptr), "video decoder drain failed");
                if (audioCodec)
                    require(avcodec_send_packet(audioCodec, nullptr), "audio decoder drain failed");
                continue;
            }
            require(read, "media demux failed");
            AVCodecContext* target = packet->stream_index == videoStream ? videoCodec
                : packet->stream_index == audioStream ? audioCodec : nullptr;
            if (target)
            {
                const int sent = avcodec_send_packet(target, packet);
                av_packet_unref(packet);
                if (sent != AVERROR(EAGAIN))
                    require(sent, "media packet submission failed");
            }
            else
                av_packet_unref(packet);
        }
    }

    void seekTo(std::int64_t timestamp)
    {
        timestamp = std::clamp<std::int64_t>(timestamp, 0,
            streamInfo.durationMicroseconds > 0 ? streamInfo.durationMicroseconds
                                                : std::numeric_limits<std::int64_t>::max());
        const int result = av_seek_frame(format, -1,
            av_rescale_q(timestamp, AVRational{1, 1000000}, AV_TIME_BASE_Q),
            AVSEEK_FLAG_BACKWARD);
        require(result, "media seek failed");
        if (videoCodec)
            avcodec_flush_buffers(videoCodec);
        if (audioCodec)
            avcodec_flush_buffers(audioCodec);
        if (resampler)
            swr_close(resampler), require(swr_init(resampler), "audio seek reset failed");
        inputEnded = videoDrained = audioDrained = false;
    }
};

Decoder::Decoder(const std::filesystem::path& path,
    std::uint32_t audioSampleRate, std::uint32_t audioChannels)
    : impl(std::make_unique<Impl>(path, audioSampleRate, audioChannels))
{
}

Decoder::Decoder(std::shared_ptr<const std::vector<std::uint8_t>> bytes,
    std::uint32_t audioSampleRate, std::uint32_t audioChannels)
    : impl(std::make_unique<Impl>(std::move(bytes), audioSampleRate, audioChannels))
{
}

Decoder::~Decoder() = default;

const StreamInfo& Decoder::info() const noexcept
{
    return impl->streamInfo;
}

DecodeResult Decoder::decodeNext(VideoFrame& video, AudioFrame& audio)
{
    return impl->next(video, audio);
}

void Decoder::seek(std::int64_t timestampMicroseconds)
{
    impl->seekTo(timestampMicroseconds);
}

std::string version()
{
    return av_version_info();
}

} // namespace RBX::Media

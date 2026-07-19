#include "FfmpegAudioDecoder.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/error.h>
#include <libswresample/swresample.h>
}

#include <algorithm>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace RBX::Audio {
namespace {

struct MemoryInput
{
    std::span<const std::byte> bytes;
    std::size_t position = 0;
};

int readMemory(void* opaque, std::uint8_t* destination, int size)
{
    MemoryInput& input = *static_cast<MemoryInput*>(opaque);
    const std::size_t count = std::min<std::size_t>(
        static_cast<std::size_t>(std::max(size, 0)),
        input.bytes.size() - input.position);
    if (count == 0)
        return AVERROR_EOF;
    std::memcpy(destination, input.bytes.data() + input.position, count);
    input.position += count;
    return static_cast<int>(count);
}

std::int64_t seekMemory(void* opaque, std::int64_t offset, int whence)
{
    MemoryInput& input = *static_cast<MemoryInput*>(opaque);
    if (whence == AVSEEK_SIZE)
        return static_cast<std::int64_t>(input.bytes.size());
    const int origin = whence & ~AVSEEK_FORCE;
    std::int64_t position = offset;
    if (origin == SEEK_CUR)
        position += static_cast<std::int64_t>(input.position);
    else if (origin == SEEK_END)
        position += static_cast<std::int64_t>(input.bytes.size());
    else if (origin != SEEK_SET)
        return AVERROR(EINVAL);
    if (position < 0 || static_cast<std::uint64_t>(position) > input.bytes.size())
        return AVERROR(EINVAL);
    input.position = static_cast<std::size_t>(position);
    return position;
}

[[noreturn]] void fail(const char* operation, int error)
{
    char text[AV_ERROR_MAX_STRING_SIZE] = {};
    av_strerror(error, text, sizeof(text));
    throw std::runtime_error(std::string(operation) + ": " + text);
}

} // namespace

PcmClip decodeFfmpegAudio(
    std::span<const std::byte> encodedData, std::uint64_t maxClipFrames)
{
    if (encodedData.empty())
        throw std::invalid_argument("FFmpeg audio input is empty");
    if (maxClipFrames == 0)
        throw std::invalid_argument("FFmpeg audio frame limit is zero");

    MemoryInput input{encodedData};
    constexpr int ioBufferSize = 32768;
    std::uint8_t* ioBuffer = static_cast<std::uint8_t*>(av_malloc(ioBufferSize));
    if (!ioBuffer)
        throw std::bad_alloc();
    AVIOContext* io = avio_alloc_context(
        ioBuffer, ioBufferSize, 0, &input, &readMemory, nullptr, &seekMemory);
    if (!io) {
        av_free(ioBuffer);
        throw std::bad_alloc();
    }
    AVFormatContext* format = avformat_alloc_context();
    if (!format) {
        avio_context_free(&io);
        throw std::bad_alloc();
    }
    format->pb = io;
    format->flags |= AVFMT_FLAG_CUSTOM_IO;

    AVCodecContext* codec = nullptr;
    AVPacket* packet = nullptr;
    AVFrame* frame = nullptr;
    SwrContext* resampler = nullptr;
    try {
        int result = avformat_open_input(&format, nullptr, nullptr, nullptr);
        if (result < 0)
            fail("FFmpeg could not open encoded audio", result);
        result = avformat_find_stream_info(format, nullptr);
        if (result < 0)
            fail("FFmpeg could not inspect encoded audio", result);
        const int streamIndex = av_find_best_stream(
            format, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
        if (streamIndex < 0)
            fail("FFmpeg found no audio stream", streamIndex);
        AVStream* stream = format->streams[streamIndex];
        const AVCodec* decoder = avcodec_find_decoder(stream->codecpar->codec_id);
        if (!decoder)
            throw std::runtime_error("FFmpeg has no decoder for the audio stream");
        codec = avcodec_alloc_context3(decoder);
        if (!codec)
            throw std::bad_alloc();
        result = avcodec_parameters_to_context(codec, stream->codecpar);
        if (result < 0)
            fail("FFmpeg could not configure the audio decoder", result);
        result = avcodec_open2(codec, decoder, nullptr);
        if (result < 0)
            fail("FFmpeg could not start the audio decoder", result);
        if (codec->sample_rate <= 0 || codec->ch_layout.nb_channels <= 0)
            throw std::runtime_error("FFmpeg audio stream has invalid dimensions");

        PcmClip pcm;
        pcm.sampleRate = static_cast<std::uint32_t>(codec->sample_rate);
        pcm.channels = static_cast<std::uint32_t>(codec->ch_layout.nb_channels);
        if (maxClipFrames >
            std::numeric_limits<std::size_t>::max() / pcm.channels)
            throw std::invalid_argument("FFmpeg audio frame limit is not addressable");
        const std::size_t maxSamples =
            static_cast<std::size_t>(maxClipFrames) * pcm.channels;
        AVChannelLayout outputLayout{};
        result = av_channel_layout_copy(&outputLayout, &codec->ch_layout);
        if (result < 0)
            fail("FFmpeg could not copy the audio channel layout", result);
        result = swr_alloc_set_opts2(&resampler,
            &outputLayout, AV_SAMPLE_FMT_FLT, codec->sample_rate,
            &codec->ch_layout, codec->sample_fmt, codec->sample_rate, 0, nullptr);
        av_channel_layout_uninit(&outputLayout);
        if (result < 0 || !resampler)
            fail("FFmpeg could not configure audio conversion", result);
        result = swr_init(resampler);
        if (result < 0)
            fail("FFmpeg could not initialize audio conversion", result);

        packet = av_packet_alloc();
        frame = av_frame_alloc();
        if (!packet || !frame)
            throw std::bad_alloc();
        const auto receiveFrames = [&]() {
            for (;;) {
                const int receive = avcodec_receive_frame(codec, frame);
                if (receive == AVERROR(EAGAIN) || receive == AVERROR_EOF)
                    break;
                if (receive < 0)
                    fail("FFmpeg audio decoding failed", receive);
                const int capacity = swr_get_out_samples(resampler, frame->nb_samples);
                if (capacity < 0)
                    fail("FFmpeg audio conversion capacity failed", capacity);
                const std::size_t start = pcm.samples.size();
                const std::size_t capacityFrames = static_cast<std::size_t>(capacity);
                if (capacityFrames > (maxSamples - start) / pcm.channels)
                    throw std::invalid_argument(
                        "FFmpeg audio exceeds the configured frame limit");
                pcm.samples.resize(start + capacityFrames * pcm.channels);
                std::uint8_t* output = reinterpret_cast<std::uint8_t*>(
                    pcm.samples.data() + start);
                const int converted = swr_convert(resampler, &output, capacity,
                    const_cast<const std::uint8_t**>(frame->extended_data),
                    frame->nb_samples);
                if (converted < 0)
                    fail("FFmpeg audio conversion failed", converted);
                if (static_cast<std::size_t>(converted) > capacityFrames)
                    throw std::runtime_error(
                        "FFmpeg audio conversion exceeded its output capacity");
                pcm.samples.resize(start +
                    static_cast<std::size_t>(converted) * pcm.channels);
                av_frame_unref(frame);
            }
        };

        while (av_read_frame(format, packet) >= 0) {
            if (packet->stream_index == streamIndex) {
                result = avcodec_send_packet(codec, packet);
                if (result < 0)
                    fail("FFmpeg rejected an audio packet", result);
                receiveFrames();
            }
            av_packet_unref(packet);
        }
        result = avcodec_send_packet(codec, nullptr);
        if (result < 0 && result != AVERROR_EOF)
            fail("FFmpeg could not flush the audio decoder", result);
        receiveFrames();
        if (pcm.samples.empty())
            throw std::runtime_error("FFmpeg decoded no audio samples");

        swr_free(&resampler);
        av_frame_free(&frame);
        av_packet_free(&packet);
        avcodec_free_context(&codec);
        avformat_close_input(&format);
        avio_context_free(&io);
        return pcm;
    }
    catch (...) {
        swr_free(&resampler);
        av_frame_free(&frame);
        av_packet_free(&packet);
        avcodec_free_context(&codec);
        avformat_close_input(&format);
        avio_context_free(&io);
        throw;
    }
}

} // namespace RBX::Audio

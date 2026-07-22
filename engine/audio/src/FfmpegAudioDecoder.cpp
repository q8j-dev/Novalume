#include "FfmpegAudioDecoder.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/error.h>
#include <libswresample/swresample.h>
}

#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <vector>

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

void writeLittleEndian16(std::ostream& output, std::uint16_t value)
{
    const std::array<char, 2> bytes = {
        static_cast<char>(value & 0xffu),
        static_cast<char>((value >> 8u) & 0xffu)};
    output.write(bytes.data(), bytes.size());
}

void writeLittleEndian32(std::ostream& output, std::uint32_t value)
{
    const std::array<char, 4> bytes = {
        static_cast<char>(value & 0xffu),
        static_cast<char>((value >> 8u) & 0xffu),
        static_cast<char>((value >> 16u) & 0xffu),
        static_cast<char>((value >> 24u) & 0xffu)};
    output.write(bytes.data(), bytes.size());
}

void writeFloatWavHeader(std::ostream& output, std::uint32_t sampleRate,
    std::uint16_t channels, std::uint32_t dataBytes)
{
    output.write("RIFF", 4);
    writeLittleEndian32(output, 36u + dataBytes);
    output.write("WAVEfmt ", 8);
    writeLittleEndian32(output, 16u);
    writeLittleEndian16(output, 3u);
    writeLittleEndian16(output, channels);
    writeLittleEndian32(output, sampleRate);
    writeLittleEndian32(output, sampleRate * channels * sizeof(float));
    writeLittleEndian16(output,
        static_cast<std::uint16_t>(channels * sizeof(float)));
    writeLittleEndian16(output, 32u);
    output.write("data", 4);
    writeLittleEndian32(output, dataBytes);
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

FfmpegStreamMetadata transcodeFfmpegAudioToFloatWav(
    const std::filesystem::path& inputPath,
    const std::filesystem::path& outputPath,
    std::uint64_t maxClipFrames)
{
    if (!std::filesystem::is_regular_file(inputPath) || maxClipFrames == 0)
        throw std::invalid_argument("FFmpeg streaming input is invalid");

    const std::u8string inputUtf8 = inputPath.u8string();
    const std::string inputName(
        reinterpret_cast<const char*>(inputUtf8.data()), inputUtf8.size());
    AVFormatContext* format = nullptr;
    AVCodecContext* codec = nullptr;
    AVPacket* packet = nullptr;
    AVFrame* frame = nullptr;
    SwrContext* resampler = nullptr;
    std::ofstream output;
    try
    {
        int result = avformat_open_input(&format, inputName.c_str(), nullptr,
            nullptr);
        if (result < 0)
            fail("FFmpeg could not open streaming audio", result);
        result = avformat_find_stream_info(format, nullptr);
        if (result < 0)
            fail("FFmpeg could not inspect streaming audio", result);
        const int streamIndex = av_find_best_stream(
            format, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
        if (streamIndex < 0)
            fail("FFmpeg found no streaming audio", streamIndex);
        AVStream* stream = format->streams[streamIndex];
        const AVCodec* decoder = avcodec_find_decoder(stream->codecpar->codec_id);
        if (!decoder)
            throw std::runtime_error(
                "FFmpeg has no decoder for the streaming audio");
        codec = avcodec_alloc_context3(decoder);
        if (!codec)
            throw std::bad_alloc();
        result = avcodec_parameters_to_context(codec, stream->codecpar);
        if (result < 0)
            fail("FFmpeg could not configure streaming audio", result);
        result = avcodec_open2(codec, decoder, nullptr);
        if (result < 0)
            fail("FFmpeg could not start streaming audio", result);
        if (codec->sample_rate <= 0 || codec->sample_rate > 768000 ||
            codec->ch_layout.nb_channels <= 0 ||
            codec->ch_layout.nb_channels > 8)
            throw std::runtime_error(
                "FFmpeg streaming audio has invalid dimensions");

        FfmpegStreamMetadata metadata;
        metadata.sampleRate = static_cast<std::uint32_t>(codec->sample_rate);
        metadata.channels = static_cast<std::uint32_t>(
            codec->ch_layout.nb_channels);
        if (metadata.channels > std::numeric_limits<std::uint16_t>::max() ||
            maxClipFrames > (std::numeric_limits<std::uint32_t>::max() - 36u) /
                (metadata.channels * sizeof(float)))
            throw std::invalid_argument(
                "FFmpeg streaming audio exceeds the WAV container limit");

        AVChannelLayout outputLayout{};
        result = av_channel_layout_copy(&outputLayout, &codec->ch_layout);
        if (result < 0)
            fail("FFmpeg could not copy streaming channel layout", result);
        result = swr_alloc_set_opts2(&resampler,
            &outputLayout, AV_SAMPLE_FMT_FLT, codec->sample_rate,
            &codec->ch_layout, codec->sample_fmt, codec->sample_rate, 0,
            nullptr);
        av_channel_layout_uninit(&outputLayout);
        if (result < 0 || !resampler)
            fail("FFmpeg could not configure streaming conversion", result);
        result = swr_init(resampler);
        if (result < 0)
            fail("FFmpeg could not initialize streaming conversion", result);

        output.open(outputPath,
            std::ios::binary | std::ios::trunc | std::ios::out);
        if (!output)
            throw std::runtime_error(
                "FFmpeg streaming WAV could not be created");
        writeFloatWavHeader(output, metadata.sampleRate,
            static_cast<std::uint16_t>(metadata.channels), 0);

        packet = av_packet_alloc();
        frame = av_frame_alloc();
        if (!packet || !frame)
            throw std::bad_alloc();
        std::vector<float> converted;
        const auto receiveFrames = [&]() {
            for (;;)
            {
                const int receive = avcodec_receive_frame(codec, frame);
                if (receive == AVERROR(EAGAIN) || receive == AVERROR_EOF)
                    break;
                if (receive < 0)
                    fail("FFmpeg streaming audio decoding failed", receive);
                const int capacity = swr_get_out_samples(
                    resampler, frame->nb_samples);
                if (capacity < 0)
                    fail("FFmpeg streaming conversion capacity failed",
                        capacity);
                if (capacity > 65536)
                    throw std::invalid_argument(
                        "FFmpeg streaming conversion frame is too large");
                if (static_cast<std::uint64_t>(capacity) >
                    maxClipFrames - metadata.frameCount)
                    throw std::invalid_argument(
                        "FFmpeg streaming audio exceeds the frame limit");
                converted.resize(static_cast<std::size_t>(capacity) *
                    metadata.channels);
                std::uint8_t* destination = reinterpret_cast<std::uint8_t*>(
                    converted.data());
                const int frames = swr_convert(resampler, &destination,
                    capacity,
                    const_cast<const std::uint8_t**>(frame->extended_data),
                    frame->nb_samples);
                if (frames < 0)
                    fail("FFmpeg streaming audio conversion failed", frames);
                if (frames > capacity)
                    throw std::runtime_error(
                        "FFmpeg streaming conversion exceeded its capacity");
                const std::size_t bytes = static_cast<std::size_t>(frames) *
                    metadata.channels * sizeof(float);
                output.write(reinterpret_cast<const char*>(converted.data()),
                    static_cast<std::streamsize>(bytes));
                if (!output)
                    throw std::runtime_error(
                        "FFmpeg streaming WAV write failed");
                metadata.frameCount += static_cast<std::uint64_t>(frames);
                av_frame_unref(frame);
            }
        };

        while (av_read_frame(format, packet) >= 0)
        {
            if (packet->stream_index == streamIndex)
            {
                result = avcodec_send_packet(codec, packet);
                if (result < 0)
                    fail("FFmpeg rejected a streaming audio packet", result);
                receiveFrames();
            }
            av_packet_unref(packet);
        }
        result = avcodec_send_packet(codec, nullptr);
        if (result < 0 && result != AVERROR_EOF)
            fail("FFmpeg could not flush streaming audio", result);
        receiveFrames();
        if (metadata.frameCount == 0)
            throw std::runtime_error(
                "FFmpeg streaming audio produced no frames");
        const std::uint64_t dataBytes64 = metadata.frameCount *
            metadata.channels * sizeof(float);
        if (dataBytes64 >
            std::numeric_limits<std::uint32_t>::max() - 36u)
            throw std::invalid_argument(
                "FFmpeg streaming WAV exceeds its container limit");
        const std::uint32_t dataBytes = static_cast<std::uint32_t>(dataBytes64);
        output.seekp(0);
        writeFloatWavHeader(output, metadata.sampleRate,
            static_cast<std::uint16_t>(metadata.channels), dataBytes);
        output.close();
        if (!output)
            throw std::runtime_error(
                "FFmpeg streaming WAV finalization failed");

        swr_free(&resampler);
        av_frame_free(&frame);
        av_packet_free(&packet);
        avcodec_free_context(&codec);
        avformat_close_input(&format);
        return metadata;
    }
    catch (...)
    {
        output.close();
        std::error_code removeError;
        std::filesystem::remove(outputPath, removeError);
        swr_free(&resampler);
        av_frame_free(&frame);
        av_packet_free(&packet);
        avcodec_free_context(&codec);
        avformat_close_input(&format);
        throw;
    }
}

} // namespace RBX::Audio

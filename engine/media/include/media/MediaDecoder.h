#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace RBX::Media {

struct VideoFrame
{
    std::int64_t timestampMicroseconds = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::vector<std::uint8_t> rgba;
};

struct AudioFrame
{
    std::int64_t timestampMicroseconds = 0;
    std::uint32_t sampleRate = 0;
    std::uint32_t channels = 0;
    std::vector<float> samples;
};

struct StreamInfo
{
    std::int64_t durationMicroseconds = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    bool hasVideo = false;
    bool hasAudio = false;
};

enum class DecodeResult
{
    Video,
    Audio,
    End,
};

// Stateful demuxer/decoder used by VideoFrame.  One Decoder is owned by one
// playback worker; it performs no rendering and exposes only owned RGBA/float
// buffers across the engine boundary.
class Decoder final
{
public:
    explicit Decoder(const std::filesystem::path& path,
        std::uint32_t audioSampleRate = 48000, std::uint32_t audioChannels = 2);
    explicit Decoder(std::shared_ptr<const std::vector<std::uint8_t>> bytes,
        std::uint32_t audioSampleRate = 48000, std::uint32_t audioChannels = 2);
    ~Decoder();

    Decoder(const Decoder&) = delete;
    Decoder& operator=(const Decoder&) = delete;

    const StreamInfo& info() const noexcept;
    DecodeResult decodeNext(VideoFrame& video, AudioFrame& audio);
    void seek(std::int64_t timestampMicroseconds);

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

std::string version();

} // namespace RBX::Media

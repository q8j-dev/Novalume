#pragma once

#include "audio/AudioEngine.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>

namespace RBX::Audio {

PcmClip decodeFfmpegAudio(
    std::span<const std::byte> encodedData, std::uint64_t maxClipFrames);

struct FfmpegStreamMetadata
{
    std::uint32_t sampleRate = 0;
    std::uint32_t channels = 0;
    std::uint64_t frameCount = 0;
};

FfmpegStreamMetadata transcodeFfmpegAudioToFloatWav(
    const std::filesystem::path& inputPath,
    const std::filesystem::path& outputPath,
    std::uint64_t maxClipFrames);

} // namespace RBX::Audio

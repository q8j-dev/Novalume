#pragma once

#include "audio/AudioEngine.h"

#include <cstddef>
#include <span>

namespace RBX::Audio {

PcmClip decodeFfmpegAudio(
    std::span<const std::byte> encodedData, std::uint64_t maxClipFrames);

} // namespace RBX::Audio

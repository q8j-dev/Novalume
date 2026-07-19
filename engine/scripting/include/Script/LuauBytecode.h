#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace RBX::LuauBytecode {

struct DecodedChunk
{
    std::vector<std::uint8_t> bytes;
    std::uint8_t bytecodeVersion = 0;
    std::uint8_t typeVersion = 0;
    std::size_t integrityTrailerSize = 0;
};

// Decodes the signed Player ModuleScript representation into a standard Luau
// bytecode chunk accepted by the pinned upstream VM. Throws on malformed,
// truncated, unsupported, or unexpectedly trailed input.
DecodedChunk decodeSignedChunk(std::span<const std::uint8_t> source);

} // namespace RBX::LuauBytecode

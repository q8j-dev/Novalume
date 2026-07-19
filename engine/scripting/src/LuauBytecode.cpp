#include "Script/LuauBytecode.h"

#include <array>
#include <stdexcept>
#include <string>

namespace RBX::LuauBytecode {
namespace {

class Reader
{
public:
    explicit Reader(std::vector<std::uint8_t>& bytes)
        : bytes(bytes)
    {
    }

    std::size_t position() const { return offset; }
    std::size_t remaining() const { return bytes.size() - offset; }

    std::uint8_t byte()
    {
        require(1, "byte");
        return bytes[offset++];
    }

    std::uint32_t varint()
    {
        std::uint32_t value = 0;
        for (unsigned shift = 0; shift < 35; shift += 7)
        {
            const std::uint8_t current = byte();
            if (shift == 28 && (current & 0xf0) != 0)
                throw std::runtime_error("Luau varint exceeds 32 bits");
            value |= std::uint32_t(current & 0x7f) << shift;
            if ((current & 0x80) == 0)
                return value;
        }
        throw std::runtime_error("Malformed Luau varint");
    }

    void varint64()
    {
        for (unsigned index = 0; index < 10; ++index)
        {
            const std::uint8_t current = byte();
            if (index == 9 && (current & 0xfe) != 0)
                throw std::runtime_error("Luau varint exceeds 64 bits");
            if ((current & 0x80) == 0)
                return;
        }
        throw std::runtime_error("Malformed Luau 64-bit varint");
    }

    void skip(std::size_t count, const char* field)
    {
        require(count, field);
        offset += count;
    }

    void skipStringReference()
    {
        (void)varint();
    }

    std::uint8_t& at(std::size_t index)
    {
        if (index >= bytes.size())
            throw std::runtime_error("Luau instruction extends past the chunk");
        return bytes[index];
    }

private:
    void require(std::size_t count, const char* field) const
    {
        if (count > bytes.size() - offset)
            throw std::runtime_error(std::string("Truncated Luau ") + field);
    }

    std::vector<std::uint8_t>& bytes;
    std::size_t offset = 0;
};

bool hasAuxiliaryWord(std::uint8_t opcode)
{
    constexpr std::array<std::uint8_t, 23> opcodes = {
        7, 8, 12, 15, 16, 20, 27, 28, 29, 30, 31, 32,
        53, 55, 58, 60, 66, 74, 75, 77, 78, 79, 80,
    };
    for (std::uint8_t value : opcodes)
        if (value == opcode)
            return true;
    return false;
}

void skipConstant(Reader& reader, std::uint8_t version)
{
    const std::uint8_t kind = reader.byte();
    switch (kind)
    {
    case 0:
        return;
    case 1:
        reader.skip(1, "boolean constant");
        return;
    case 2:
        reader.skip(8, "number constant");
        return;
    case 3:
        (void)reader.varint();
        return;
    case 4:
        reader.skip(4, "import constant");
        return;
    case 5:
    {
        const std::uint32_t count = reader.varint();
        for (std::uint32_t index = 0; index < count; ++index)
            (void)reader.varint();
        return;
    }
    case 6:
        (void)reader.varint();
        return;
    case 7:
        reader.skip(16, "vector constant");
        return;
    case 8:
        if (version < 7)
            break;
        for (std::uint32_t count = reader.varint(), index = 0; index < count; ++index)
        {
            (void)reader.varint();
            reader.skip(4, "table-value constant");
        }
        return;
    case 9:
        if (version < 8)
            break;
        reader.skip(1, "integer sign");
        reader.varint64();
        return;
    default:
        break;
    }
    throw std::runtime_error("Unsupported Luau constant kind " + std::to_string(kind));
}

} // namespace

DecodedChunk decodeSignedChunk(std::span<const std::uint8_t> source)
{
    constexpr std::array<std::uint8_t, 8> signedPrefix = {
        0x52, 0x7a, 0x10, 0x4a, 0x5b, 0x32, 0x09, 0xe0,
    };
    if (source.size() < signedPrefix.size())
        throw std::runtime_error("Signed ModuleScript source is too short");
    for (std::size_t index = 0; index < signedPrefix.size(); ++index)
        if (source[index] != signedPrefix[index])
            throw std::runtime_error("Unrecognized signed ModuleScript prefix");

    DecodedChunk result;
    result.bytes.assign(source.begin() + signedPrefix.size(), source.end());
    Reader reader(result.bytes);

    result.bytecodeVersion = reader.byte();
    if (result.bytecodeVersion == 0 || result.bytecodeVersion > 8)
        throw std::runtime_error("Unsupported Luau bytecode version " +
            std::to_string(result.bytecodeVersion));
    if (result.bytecodeVersion >= 4)
        result.typeVersion = reader.byte();

    for (std::uint32_t count = reader.varint(), index = 0; index < count; ++index)
        reader.skip(reader.varint(), "string table entry");

    if (result.typeVersion == 3)
    {
        while (reader.byte() != 0)
            reader.skipStringReference();
    }

    const std::uint32_t protoCount = reader.varint();
    for (std::uint32_t protoIndex = 0; protoIndex < protoCount; ++protoIndex)
    {
        reader.skip(4, "prototype header");
        if (result.bytecodeVersion >= 4)
        {
            reader.skip(1, "prototype flags");
            reader.skip(reader.varint(), "prototype type information");
        }

        const std::uint32_t codeSize = reader.varint();
        const std::size_t codeStart = reader.position();
        if (std::size_t(codeSize) > reader.remaining() / 4)
            throw std::runtime_error("Luau instruction stream exceeds the chunk");

        std::uint32_t pc = 0;
        while (pc < codeSize)
        {
            const std::size_t instructionOffset = codeStart + std::size_t(pc) * 4;
            const std::uint8_t decoded = std::uint8_t(reader.at(instructionOffset) * 203u);
            if (decoded > 82)
                throw std::runtime_error("Invalid decoded Luau opcode " + std::to_string(decoded));
            reader.at(instructionOffset) = decoded;
            pc += hasAuxiliaryWord(decoded) ? 2 : 1;
        }
        if (pc != codeSize)
            throw std::runtime_error("Luau auxiliary instruction extends past its prototype");
        reader.skip(std::size_t(codeSize) * 4, "instruction stream");

        for (std::uint32_t count = reader.varint(), index = 0; index < count; ++index)
            skipConstant(reader, result.bytecodeVersion);
        for (std::uint32_t count = reader.varint(), index = 0; index < count; ++index)
            (void)reader.varint();
        (void)reader.varint();
        reader.skipStringReference();

        const bool hasLines = reader.byte() != 0;
        if (hasLines)
        {
            const std::uint8_t lineGap = reader.byte();
            if (lineGap >= 32)
                throw std::runtime_error("Invalid Luau line-information gap");
            reader.skip(codeSize, "line information");
            const std::size_t intervals = codeSize ? ((codeSize - 1) >> lineGap) + 1 : 0;
            reader.skip(intervals * 4, "absolute line information");
        }

        const bool hasDebug = reader.byte() != 0;
        if (hasDebug)
        {
            for (std::uint32_t count = reader.varint(), index = 0; index < count; ++index)
            {
                reader.skipStringReference();
                (void)reader.varint();
                (void)reader.varint();
                reader.skip(1, "local register");
            }
            for (std::uint32_t count = reader.varint(), index = 0; index < count; ++index)
                reader.skipStringReference();
        }
    }

    (void)reader.varint();
    result.integrityTrailerSize = reader.remaining();
    if (result.integrityTrailerSize != 0 && result.integrityTrailerSize != 24)
        throw std::runtime_error("Unexpected signed ModuleScript trailer size " +
            std::to_string(result.integrityTrailerSize));
    result.bytes.resize(reader.position());
    return result;
}

} // namespace RBX::LuauBytecode

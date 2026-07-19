#pragma once

#include <bit>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <cmath>
#include <span>
#include <string>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace RBX::Network {

using BitCount = std::size_t;

class PacketBuffer {
public:
    static constexpr std::size_t MaximumPacketBytes = 64 * 1024 * 1024;

    PacketBuffer() = default;
    explicit PacketBuffer(std::size_t initialBytes) { bytes.reserve(initialBytes); }
    PacketBuffer(const unsigned char* data, std::size_t byteCount, bool)
    {
        if (byteCount > MaximumPacketBytes)
            throw std::length_error("packet exceeds the configured size limit");
        if (!data && byteCount != 0)
            throw std::invalid_argument("packet data is null");
        if (byteCount != 0)
            bytes.assign(data, data + byteCount);
        writeBitOffset = byteCount * 8;
    }

    const unsigned char* GetData() const { return bytes.data(); }
    unsigned char* GetData() { return bytes.data(); }
    BitCount GetNumberOfBitsUsed() const { return writeBitOffset; }
    std::size_t GetNumberOfBytesUsed() const { return (writeBitOffset + 7) / 8; }
    BitCount GetNumberOfBitsAllocated() const { return bytes.size() * 8; }
    BitCount GetReadOffset() const { return readBitOffset; }
    BitCount GetWriteOffset() const { return writeBitOffset; }

    void SetReadOffset(BitCount offset)
    {
        if (offset > writeBitOffset)
            throw std::out_of_range("packet read offset exceeds written data");
        readBitOffset = offset;
    }

    void SetWriteOffset(BitCount offset)
    {
        ensureBits(offset);
        writeBitOffset = offset;
        if (readBitOffset > writeBitOffset)
            readBitOffset = writeBitOffset;
    }

    void Reset()
    {
        bytes.clear();
        readBitOffset = 0;
        writeBitOffset = 0;
    }

    void AddBitsAndReallocate(BitCount count) { ensureBits(writeBitOffset + count); }

    void AlignWriteToByteBoundary()
    {
        const BitCount aligned = (writeBitOffset + 7) & ~BitCount(7);
        ensureBits(aligned);
        writeBitOffset = aligned;
    }

    void AlignReadToByteBoundary()
    {
        readBitOffset = std::min((readBitOffset + 7) & ~BitCount(7), writeBitOffset);
    }

    void IgnoreBits(BitCount count)
    {
        if (count > writeBitOffset - readBitOffset)
            throw std::out_of_range("packet ignore exceeds unread data");
        readBitOffset += count;
    }

    void Write0() { writeBit(false); }
    void Write1() { writeBit(true); }
    bool ReadBit()
    {
        bool value = false;
        if (!readBit(value))
            throw std::out_of_range("packet bit read exceeds written data");
        return value;
    }

    bool WriteBits(const unsigned char* input, BitCount bitCount, bool rightAligned = true)
    {
        if (!input && bitCount != 0)
            return false;
        ensureBits(writeBitOffset + bitCount);
        const BitCount fullBits = bitCount & ~BitCount(7);
        const BitCount tailBits = bitCount & 7;
        for (BitCount index = 0; index < bitCount; ++index) {
            BitCount sourceBit = index;
            if (rightAligned && tailBits && index >= fullBits)
                sourceBit += 8 - tailBits;
            const bool value = (input[sourceBit / 8] & (0x80u >> (sourceBit & 7))) != 0;
            writeBit(value);
        }
        return true;
    }

    bool ReadBits(unsigned char* output, BitCount bitCount, bool rightAligned = true)
    {
        if ((!output && bitCount != 0) || bitCount > writeBitOffset - readBitOffset)
            return false;
        const std::size_t outputBytes = (bitCount + 7) / 8;
        std::memset(output, 0, outputBytes);
        const BitCount fullBits = bitCount & ~BitCount(7);
        const BitCount tailBits = bitCount & 7;
        for (BitCount index = 0; index < bitCount; ++index) {
            BitCount destinationBit = index;
            if (rightAligned && tailBits && index >= fullBits)
                destinationBit += 8 - tailBits;
            bool value = false;
            readBit(value);
            if (value)
                output[destinationBit / 8] |= static_cast<unsigned char>(0x80u >> (destinationBit & 7));
        }
        return true;
    }

    bool Write(const char* data, std::size_t byteCount)
    {
        if (!data && byteCount != 0)
            return false;
        if ((writeBitOffset & 7) == 0) {
            if (byteCount > MaximumPacketBytes - GetNumberOfBytesUsed())
                throw std::length_error("packet exceeds the configured size limit");
            ensureBits(writeBitOffset + byteCount * 8);
            if (byteCount != 0)
                std::memcpy(bytes.data() + writeBitOffset / 8, data, byteCount);
            writeBitOffset += byteCount * 8;
            return true;
        }
        return WriteBits(reinterpret_cast<const unsigned char*>(data), byteCount * 8, false);
    }

    bool Read(char* data, std::size_t byteCount)
    {
        if ((!data && byteCount != 0) ||
            byteCount > (writeBitOffset - readBitOffset) / 8)
            return false;
        if ((readBitOffset & 7) == 0) {
            if (byteCount != 0)
                std::memcpy(data, bytes.data() + readBitOffset / 8, byteCount);
            readBitOffset += byteCount * 8;
            return true;
        }
        return ReadBits(reinterpret_cast<unsigned char*>(data), byteCount * 8, false);
    }

    bool WriteAlignedBytes(const unsigned char* data, std::size_t byteCount)
    {
        AlignWriteToByteBoundary();
        return Write(reinterpret_cast<const char*>(data), byteCount);
    }

    bool ReadAlignedBytes(unsigned char* data, std::size_t byteCount)
    {
        AlignReadToByteBoundary();
        return Read(reinterpret_cast<char*>(data), byteCount);
    }

    bool Write(PacketBuffer& source)
    {
        while (source.readBitOffset < source.writeBitOffset) {
            bool bit = false;
            source.readBit(bit);
            writeBit(bit);
        }
        return true;
    }

    bool Write(PacketBuffer& source, BitCount bitCount)
    {
        if (bitCount > source.writeBitOffset - source.readBitOffset)
            return false;
        for (BitCount index = 0; index < bitCount; ++index) {
            bool bit = false;
            source.readBit(bit);
            writeBit(bit);
        }
        return true;
    }

    bool Write(bool value)
    {
        writeBit(value);
        return true;
    }

    bool Read(bool& value) { return readBit(value); }

    bool Write(const std::string& value)
    {
        if (value.size() > MaximumPacketBytes ||
            value.size() > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()))
            return false;
        if (!Write(static_cast<std::uint32_t>(value.size())))
            return false;
        return Write(value.data(), value.size());
    }

    bool Read(std::string& value)
    {
        std::uint32_t size = 0;
        if (!Read(size) || size > MaximumPacketBytes ||
            static_cast<BitCount>(size) * 8 > writeBitOffset - readBitOffset)
            return false;
        value.resize(size);
        return Read(value.data(), size);
    }

    void WriteFloat16(float value, float minimum, float maximum)
    {
        if (!(maximum > minimum))
            throw std::invalid_argument("invalid float quantization range");
        const float normalized = std::clamp((value - minimum) / (maximum - minimum), 0.0f, 1.0f);
        Write(static_cast<std::uint16_t>(normalized * 65535.0f));
    }

    bool ReadFloat16(float& value, float minimum, float maximum)
    {
        if (!(maximum > minimum))
            return false;
        std::uint16_t encoded = 0;
        if (!Read(encoded))
            return false;
        value = minimum + (static_cast<float>(encoded) / 65535.0f) * (maximum - minimum);
        value = std::clamp(value, minimum, maximum);
        return true;
    }

    template<typename T>
    void WriteNormQuat(T w, T x, T y, T z)
    {
        Write(w < T(0));
        Write(x < T(0));
        Write(y < T(0));
        Write(z < T(0));
        Write(static_cast<std::uint16_t>(std::abs(x) * T(65535)));
        Write(static_cast<std::uint16_t>(std::abs(y) * T(65535)));
        Write(static_cast<std::uint16_t>(std::abs(z) * T(65535)));
    }

    template<typename T>
    bool ReadNormQuat(T& w, T& x, T& y, T& z)
    {
        bool wNegative = false;
        bool xNegative = false;
        bool yNegative = false;
        bool zNegative = false;
        std::uint16_t encodedX = 0;
        std::uint16_t encodedY = 0;
        std::uint16_t encodedZ = 0;
        if (!Read(wNegative) || !Read(xNegative) || !Read(yNegative) || !Read(zNegative) ||
            !Read(encodedX) || !Read(encodedY) || !Read(encodedZ))
            return false;
        x = static_cast<T>(encodedX / 65535.0);
        y = static_cast<T>(encodedY / 65535.0);
        z = static_cast<T>(encodedZ / 65535.0);
        if (xNegative) x = -x;
        if (yNegative) y = -y;
        if (zNegative) z = -z;
        w = static_cast<T>(std::sqrt(std::max<T>(T(0), T(1) - x * x - y * y - z * z)));
        if (wNegative) w = -w;
        return true;
    }

    template<typename T>
    void WriteVector(T x, T y, T z)
    {
        const T magnitude = static_cast<T>(std::sqrt(x * x + y * y + z * z));
        Write(static_cast<float>(magnitude));
        if (magnitude > T(0.00001)) {
            WriteFloat16(static_cast<float>(x / magnitude), -1.0f, 1.0f);
            WriteFloat16(static_cast<float>(y / magnitude), -1.0f, 1.0f);
            WriteFloat16(static_cast<float>(z / magnitude), -1.0f, 1.0f);
        }
    }

    template<typename T>
    bool ReadVector(T& x, T& y, T& z)
    {
        float magnitude = 0.0f;
        if (!Read(magnitude))
            return false;
        if (magnitude <= 0.00001f) {
            x = y = z = T(0);
            return true;
        }
        float normalizedX = 0.0f;
        float normalizedY = 0.0f;
        float normalizedZ = 0.0f;
        if (!ReadFloat16(normalizedX, -1.0f, 1.0f) ||
            !ReadFloat16(normalizedY, -1.0f, 1.0f) ||
            !ReadFloat16(normalizedZ, -1.0f, 1.0f))
            return false;
        x = static_cast<T>(normalizedX * magnitude);
        y = static_cast<T>(normalizedY * magnitude);
        z = static_cast<T>(normalizedZ * magnitude);
        return true;
    }

    template<typename T>
        requires ((std::is_integral_v<T> && !std::is_same_v<T, bool>) ||
            std::is_floating_point_v<T> || std::is_enum_v<T>)
    bool Write(T value)
    {
        using Stored = typename StoredType<T>::type;
        Stored stored{};
        if constexpr (std::is_enum_v<T>)
            stored = static_cast<Stored>(value);
        else
            std::memcpy(&stored, &value, sizeof(T));
        // RakNet's BitStream contract writes arithmetic values in network
        // byte order. PacketBufferFast and the historical replication codecs
        // decode that representation directly, including at unaligned bit
        // offsets.
        if constexpr (std::endian::native == std::endian::little)
            stored = byteSwap(stored);
        return Write(reinterpret_cast<const char*>(&stored), sizeof(stored));
    }

    template<typename T>
        requires ((std::is_integral_v<T> && !std::is_same_v<T, bool>) ||
            std::is_floating_point_v<T> || std::is_enum_v<T>)
    bool Read(T& value)
    {
        using Stored = typename StoredType<T>::type;
        Stored stored{};
        if (!Read(reinterpret_cast<char*>(&stored), sizeof(stored)))
            return false;
        if constexpr (std::endian::native == std::endian::little)
            stored = byteSwap(stored);
        if constexpr (std::is_enum_v<T>)
            value = static_cast<T>(stored);
        else
            std::memcpy(&value, &stored, sizeof(T));
        return true;
    }

private:
    template<typename T, bool = std::is_enum_v<T>>
    struct StoredTypeImpl { using type = T; };
    template<typename T>
    struct StoredTypeImpl<T, true> { using type = std::underlying_type_t<T>; };
    template<typename T>
    using StoredType = StoredTypeImpl<T>;

    template<typename T>
    static T byteSwap(T value)
    {
        auto* first = reinterpret_cast<unsigned char*>(&value);
        std::reverse(first, first + sizeof(T));
        return value;
    }

    void ensureBits(BitCount bitCount)
    {
        if (bitCount > MaximumPacketBytes * 8)
            throw std::length_error("packet exceeds the configured size limit");
        const std::size_t requiredBytes = (bitCount + 7) / 8;
        if (bytes.size() < requiredBytes)
            bytes.resize(requiredBytes, 0);
    }

    void writeBit(bool value)
    {
        ensureBits(writeBitOffset + 1);
        const unsigned char mask = static_cast<unsigned char>(0x80u >> (writeBitOffset & 7));
        if (value)
            bytes[writeBitOffset / 8] |= mask;
        else
            bytes[writeBitOffset / 8] &= static_cast<unsigned char>(~mask);
        ++writeBitOffset;
    }

    bool readBit(bool& value)
    {
        if (readBitOffset >= writeBitOffset)
            return false;
        value = (bytes[readBitOffset / 8] & (0x80u >> (readBitOffset & 7))) != 0;
        ++readBitOffset;
        return true;
    }

    std::vector<unsigned char> bytes;
    BitCount readBitOffset = 0;
    BitCount writeBitOffset = 0;
};

} // namespace RBX::Network

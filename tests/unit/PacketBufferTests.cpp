#include "network/PacketBuffer.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace {

int failures = 0;

void expect(bool condition, const char* message)
{
    if (!condition) {
        ++failures;
        std::cerr << "packet buffer test failed: " << message << '\n';
    }
}

} // namespace

int main()
{
    using RBX::Network::PacketBuffer;

    PacketBuffer values;
    values.Write(true);
    values.Write(false);
    values.Write<std::uint32_t>(0x12345678u);
    values.Write<float>(1.5f);
    bool first = false;
    bool second = true;
    std::uint32_t integer = 0;
    float number = 0.0f;
    expect(values.Read(first) && first, "boolean true round trip");
    expect(values.Read(second) && !second, "boolean false round trip");
    expect(values.Read(integer) && integer == 0x12345678u, "unaligned integer round trip");
    expect(values.Read(number) && number == 1.5f, "unaligned floating-point round trip");

    PacketBuffer endian;
    endian.Write<std::uint32_t>(0x12345678u);
    expect(endian.GetNumberOfBytesUsed() == 4, "integer byte count");
    expect(endian.GetData()[0] == 0x12 && endian.GetData()[1] == 0x34 &&
        endian.GetData()[2] == 0x56 && endian.GetData()[3] == 0x78,
        "RakNet-compatible network-order integer encoding");

    PacketBuffer arithmetic;
    arithmetic.Write<std::uint16_t>(0xCAFEu);
    arithmetic.Write<std::uint32_t>(0x12345678u);
    std::uint16_t shortValue = 0;
    std::uint32_t integerValue = 0;
    expect(arithmetic.Read(shortValue) && shortValue == 0xCAFEu,
        "16-bit arithmetic compatibility");
    expect(arithmetic.Read(integerValue) && integerValue == 0x12345678u,
        "32-bit arithmetic compatibility");

    PacketBuffer bits;
    const unsigned char fiveBits = 0x15;
    expect(bits.WriteBits(&fiveBits, 5), "right-aligned bit write");
    unsigned char decoded = 0;
    expect(bits.ReadBits(&decoded, 5), "right-aligned bit read");
    expect(decoded == fiveBits, "right-aligned bit round trip");

    const auto expectFastBits = [](std::uint16_t value, unsigned int count) {
        PacketBuffer stream;
        stream.WriteBits(reinterpret_cast<const unsigned char*>(&value), count, true);
        std::uint16_t decodedValue = 0;
        expect(stream.ReadBits(reinterpret_cast<unsigned char*>(&decodedValue), count, true),
            "partial-bit read");
        expect(decodedValue == value, "readFastN partial-bit compatibility");
    };
    expectFastBits(0x3u, 2);
    expectFastBits(0x15u, 5);
    expectFastBits(0x12Fu, 9);
    expectFastBits(0x5A3u, 11);

    PacketBuffer alignment;
    alignment.Write1();
    alignment.AlignWriteToByteBoundary();
    alignment.Write<std::uint8_t>(0xA5);
    expect(alignment.GetNumberOfBitsUsed() == 16, "write alignment");
    expect(alignment.ReadBit(), "aligned prefix bit");
    alignment.AlignReadToByteBoundary();
    std::uint8_t alignedByte = 0;
    expect(alignment.Read(alignedByte) && alignedByte == 0xA5, "read alignment");

    PacketBuffer strings;
    strings.Write(std::string("aligned string"));
    std::string alignedString;
    expect(strings.Read(alignedString) && alignedString == "aligned string",
        "aligned string round trip");

    PacketBuffer unalignedStringStream;
    unalignedStringStream.Write1();
    unalignedStringStream.Write(std::string("unaligned string"));
    expect(unalignedStringStream.ReadBit(), "unaligned string prefix");
    std::string unalignedString;
    expect(unalignedStringStream.Read(unalignedString) &&
        unalignedString == "unaligned string", "unaligned string round trip");

    bool caught = false;
    try {
        alignment.IgnoreBits(1);
    } catch (const std::out_of_range&) {
        caught = true;
    }
    expect(caught, "bounded read offset");

    return failures == 0 ? 0 : 1;
}

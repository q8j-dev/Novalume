#include "rbx/assets/AssetMountTable.h"
#include "rbx/assets/PlacePackage.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>

namespace {

void require(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

template<typename Value>
void writeLittleEndian(std::ostream& stream, Value value)
{
    for (unsigned int index = 0; index < sizeof(Value); ++index)
        stream.put(static_cast<char>((value >> (index * 8U)) & 0xffU));
}

std::uint32_t crc32(std::string_view bytes)
{
    std::uint32_t crc = 0xffffffffU;
    for (char value : bytes)
    {
        crc ^= static_cast<unsigned char>(value);
        for (unsigned int bit = 0; bit < 8; ++bit)
            crc = (crc >> 1U) ^ (0xedb88320U & (0U - (crc & 1U)));
    }
    return ~crc;
}

void writeEntry(std::ostream& stream, std::string_view name, std::string_view payload,
    bool corruptCrc = false)
{
    writeLittleEndian<std::uint16_t>(stream, static_cast<std::uint16_t>(name.size()));
    writeLittleEndian<std::uint64_t>(stream, payload.size());
    writeLittleEndian<std::uint32_t>(stream, crc32(payload) + (corruptCrc ? 1U : 0U));
    stream.write(name.data(), static_cast<std::streamsize>(name.size()));
    stream.write(payload.data(), static_cast<std::streamsize>(payload.size()));
}

void writePackage(const std::filesystem::path& path, bool unsafePath = false,
    bool corruptCrc = false)
{
    std::ofstream stream(path, std::ios::binary);
    const std::array<char, 8> magic = {'R', 'B', 'X', 'L', 'P', 'K', '1', '\0'};
    stream.write(magic.data(), static_cast<std::streamsize>(magic.size()));
    writeLittleEndian<std::uint32_t>(stream, 1);
    writeLittleEndian<std::uint32_t>(stream, 2);
    writeEntry(stream, unsafePath ? "../main.rbxlx" : "place/main.rbxlx", "<roblox/>");
    writeEntry(stream, "assets/1818.png", "png-payload", corruptCrc);
}

} // namespace

int main()
{
    namespace fs = std::filesystem;
    const fs::path root = fs::temp_directory_path() /
        ("rbx-place-package-test-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directories(root);

    const fs::path package = root / "place.rbxlp";
    writePackage(package);
    const RBX::Assets::MaterializedPlacePackage materialized =
        RBX::Assets::materializePlacePackage(package, root / "cache");
    require(materialized.place.filename() == "main.rbxlx",
        "RBXLP did not materialize its place payload");
    RBX::Assets::AssetMountTable mounts;
    mounts.addMount("embedded-place", materialized.root, 1000);
    const auto asset = mounts.resolveAssetId("1818");
    require(asset && asset->path.filename() == "1818.png",
        "RBXLP embedded numeric asset did not resolve through the mount table");

    bool unsafeRejected = false;
    try
    {
        const fs::path unsafe = root / "unsafe.rbxlp";
        writePackage(unsafe, true);
        (void)RBX::Assets::materializePlacePackage(unsafe, root / "unsafe-cache");
    }
    catch (const std::runtime_error&)
    {
        unsafeRejected = true;
    }
    require(unsafeRejected, "RBXLP path traversal was not rejected");

    bool corruptionRejected = false;
    try
    {
        const fs::path corrupt = root / "corrupt.rbxlp";
        writePackage(corrupt, false, true);
        (void)RBX::Assets::materializePlacePackage(corrupt, root / "corrupt-cache");
    }
    catch (const std::runtime_error&)
    {
        corruptionRejected = true;
    }
    require(corruptionRejected, "RBXLP payload corruption was not rejected");

    fs::remove_all(root);
    return 0;
}

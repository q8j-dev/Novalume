#include "rbx/assets/PlacePackage.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace RBX::Assets {
namespace {

constexpr std::array<char, 8> packageMagic = {'R', 'B', 'X', 'L', 'P', 'K', '1', '\0'};
constexpr std::uint32_t packageVersion = 1;
constexpr std::uint32_t maximumEntryCount = 100000;
constexpr std::uint64_t maximumEntrySize = 8ULL * 1024ULL * 1024ULL * 1024ULL;

template<typename Value>
Value readLittleEndian(std::istream& stream)
{
    static_assert(std::is_unsigned_v<Value>);
    Value value = 0;
    for (unsigned int index = 0; index < sizeof(Value); ++index)
    {
        const int byte = stream.get();
        if (byte == std::char_traits<char>::eof())
            throw std::runtime_error("RBXLP package ended inside an entry header");
        value |= static_cast<Value>(static_cast<unsigned char>(byte)) << (index * 8U);
    }
    return value;
}

std::uint32_t updateCrc32(std::uint32_t crc, const char* bytes, std::size_t size)
{
    crc = ~crc;
    for (std::size_t index = 0; index < size; ++index)
    {
        crc ^= static_cast<unsigned char>(bytes[index]);
        for (unsigned int bit = 0; bit < 8; ++bit)
            crc = (crc >> 1U) ^ (0xedb88320U & (0U - (crc & 1U)));
    }
    return ~crc;
}

bool validEntryPath(std::string_view name)
{
    if (name.empty() || name.front() == '/' || name.find('\\') != std::string_view::npos ||
        name.find('\0') != std::string_view::npos)
        return false;
    const std::filesystem::path path{std::string(name)};
    if (path.is_absolute() || path.has_root_name() || path.has_root_directory())
        return false;
    for (const std::filesystem::path& component : path)
    {
        if (component.empty() || component == "." || component == "..")
            return false;
    }
    for (char value : name)
    {
        const unsigned char character = static_cast<unsigned char>(value);
        if (character < 0x20 || character == 0x7f)
            return false;
    }
    return true;
}

std::uint64_t packageIdentity(const std::filesystem::path& package)
{
    std::ifstream stream(package, std::ios::binary);
    if (!stream)
        throw std::runtime_error("could not open RBXLP package: " + package.string());
    std::uint64_t hash = 1469598103934665603ULL;
    std::array<char, 65536> buffer{};
    while (stream)
    {
        stream.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const std::streamsize count = stream.gcount();
        for (std::streamsize index = 0; index < count; ++index)
        {
            hash ^= static_cast<unsigned char>(buffer[static_cast<std::size_t>(index)]);
            hash *= 1099511628211ULL;
        }
    }
    return hash;
}

std::string identityString(std::uint64_t value)
{
    std::ostringstream stream;
    stream << std::hex << std::setw(16) << std::setfill('0') << value;
    return stream.str();
}

std::filesystem::path findPlace(const std::filesystem::path& root)
{
    const std::filesystem::path placeRoot = root / "place";
    std::error_code error;
    if (!std::filesystem::is_directory(placeRoot, error) || error)
        return {};
    std::vector<std::filesystem::path> places;
    for (std::filesystem::directory_iterator iterator(placeRoot, error), end;
         !error && iterator != end; iterator.increment(error))
    {
        if (!iterator->is_regular_file(error) || error)
            continue;
        const std::string extension = iterator->path().extension().string();
        if (extension == ".rbxl" || extension == ".rbxlx")
            places.push_back(iterator->path());
    }
    if (error || places.size() != 1)
        return {};
    return places.front();
}

} // namespace

bool isPlacePackage(const std::filesystem::path& path)
{
    return path.extension() == ".rbxlp";
}

MaterializedPlacePackage materializePlacePackage(
    const std::filesystem::path& package, const std::filesystem::path& cacheRoot)
{
    if (!std::filesystem::is_regular_file(package))
        throw std::runtime_error("RBXLP package does not exist: " + package.string());

    const std::filesystem::path destination =
        cacheRoot / identityString(packageIdentity(package));
    const std::filesystem::path readyMarker = destination / ".complete";
    if (std::filesystem::is_regular_file(readyMarker))
    {
        const std::filesystem::path place = findPlace(destination);
        if (!place.empty())
            return {destination, place};
    }

    std::filesystem::create_directories(cacheRoot);
    const std::filesystem::path temporary = cacheRoot /
        (destination.filename().string() + ".tmp-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(temporary);
    try
    {
        std::ifstream stream(package, std::ios::binary);
        std::array<char, packageMagic.size()> magic{};
        stream.read(magic.data(), static_cast<std::streamsize>(magic.size()));
        if (!stream || magic != packageMagic)
            throw std::runtime_error("file is not an RBXLP package");
        if (readLittleEndian<std::uint32_t>(stream) != packageVersion)
            throw std::runtime_error("unsupported RBXLP package version");
        const std::uint32_t entryCount = readLittleEndian<std::uint32_t>(stream);
        if (entryCount == 0 || entryCount > maximumEntryCount)
            throw std::runtime_error("RBXLP package has an invalid entry count");

        std::array<char, 65536> buffer{};
        for (std::uint32_t entryIndex = 0; entryIndex < entryCount; ++entryIndex)
        {
            const std::uint16_t nameLength = readLittleEndian<std::uint16_t>(stream);
            const std::uint64_t size = readLittleEndian<std::uint64_t>(stream);
            const std::uint32_t expectedCrc = readLittleEndian<std::uint32_t>(stream);
            if (nameLength == 0 || size > maximumEntrySize)
                throw std::runtime_error("RBXLP package has invalid entry metadata");
            std::string name(nameLength, '\0');
            stream.read(name.data(), static_cast<std::streamsize>(name.size()));
            if (!stream || !validEntryPath(name))
                throw std::runtime_error("RBXLP package contains an unsafe entry path");

            const std::filesystem::path outputPath = temporary / name;
            std::filesystem::create_directories(outputPath.parent_path());
            std::ofstream output(outputPath, std::ios::binary | std::ios::trunc);
            if (!output)
                throw std::runtime_error("could not materialize RBXLP entry");
            std::uint64_t remaining = size;
            std::uint32_t crc = 0;
            while (remaining != 0)
            {
                const std::size_t chunk = static_cast<std::size_t>(
                    std::min<std::uint64_t>(remaining, buffer.size()));
                stream.read(buffer.data(), static_cast<std::streamsize>(chunk));
                if (stream.gcount() != static_cast<std::streamsize>(chunk))
                    throw std::runtime_error("RBXLP package ended inside an entry payload");
                output.write(buffer.data(), static_cast<std::streamsize>(chunk));
                if (!output)
                    throw std::runtime_error("could not write materialized RBXLP entry");
                crc = updateCrc32(crc, buffer.data(), chunk);
                remaining -= chunk;
            }
            if (crc != expectedCrc)
                throw std::runtime_error("RBXLP entry failed its CRC-32 integrity check: " + name);
        }
        if (stream.peek() != std::char_traits<char>::eof())
            throw std::runtime_error("RBXLP package has unindexed trailing bytes");
        const std::filesystem::path place = findPlace(temporary);
        if (place.empty())
            throw std::runtime_error("RBXLP package must contain exactly one RBXL or RBXLX place");
        std::ofstream(temporary / ".complete", std::ios::binary) << "RBXLP1\n";

        std::error_code error;
        if (std::filesystem::exists(destination, error))
            std::filesystem::remove_all(destination, error);
        std::filesystem::rename(temporary, destination);
        return {destination, destination / "place" / place.filename()};
    }
    catch (...)
    {
        std::error_code error;
        std::filesystem::remove_all(temporary, error);
        throw;
    }
}

} // namespace RBX::Assets

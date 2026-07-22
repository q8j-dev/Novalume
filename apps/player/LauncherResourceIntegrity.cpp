#include "LauncherResourceIntegrity.h"

#include "LauncherResourceManifest.generated.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <fstream>
#include <limits>
#include <set>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace rbx::player
{
namespace
{

constexpr std::string_view kManifestHeader =
    "ROBLOX_DURANGO_LAUNCHER_MANIFEST_V1";
constexpr std::string_view kEntryCountPrefix = "entries\t";
constexpr std::size_t kSha256HexLength = 64;
constexpr std::uintmax_t kMaximumManifestBytes = 4 * 1024 * 1024;

constexpr std::array<std::uint32_t, 64> kSha256RoundConstants = {
    0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U,
    0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
    0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U,
    0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
    0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU,
    0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
    0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U,
    0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
    0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U,
    0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
    0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U,
    0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
    0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U,
    0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
    0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
    0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U};

constexpr std::uint32_t rotateRight(std::uint32_t value, unsigned int count)
{
    return (value >> count) | (value << (32U - count));
}

class Sha256 final
{
public:
    void update(std::span<const std::uint8_t> bytes)
    {
        if (bytes.size() > std::numeric_limits<std::uint64_t>::max() - totalBytes)
            throw std::runtime_error("launcher resource is too large to hash");
        totalBytes += static_cast<std::uint64_t>(bytes.size());

        while (!bytes.empty())
        {
            const std::size_t amount =
                std::min(bytes.size(), block.size() - blockSize);
            std::copy_n(bytes.begin(), amount, block.begin() + blockSize);
            blockSize += amount;
            bytes = bytes.subspan(amount);
            if (blockSize == block.size())
            {
                transform(block.data());
                blockSize = 0;
            }
        }
    }

    std::array<std::uint8_t, 32> finish()
    {
        if (totalBytes > std::numeric_limits<std::uint64_t>::max() / 8U)
            throw std::runtime_error("launcher resource is too large to hash");
        const std::uint64_t bitLength = totalBytes * 8U;

        block[blockSize++] = 0x80U;
        if (blockSize > 56)
        {
            std::fill(block.begin() + blockSize, block.end(), 0U);
            transform(block.data());
            blockSize = 0;
        }
        std::fill(block.begin() + blockSize, block.begin() + 56, 0U);
        for (std::size_t index = 0; index < 8; ++index)
        {
            block[63 - index] = static_cast<std::uint8_t>(
                bitLength >> static_cast<unsigned int>(index * 8));
        }
        transform(block.data());

        std::array<std::uint8_t, 32> digest{};
        for (std::size_t wordIndex = 0; wordIndex < state.size(); ++wordIndex)
        {
            for (std::size_t byteIndex = 0; byteIndex < 4; ++byteIndex)
            {
                digest[wordIndex * 4 + byteIndex] =
                    static_cast<std::uint8_t>(state[wordIndex] >>
                        static_cast<unsigned int>((3 - byteIndex) * 8));
            }
        }
        return digest;
    }

private:
    void transform(const std::uint8_t* input)
    {
        std::array<std::uint32_t, 64> schedule{};
        for (std::size_t index = 0; index < 16; ++index)
        {
            const std::size_t offset = index * 4;
            schedule[index] =
                (static_cast<std::uint32_t>(input[offset]) << 24U) |
                (static_cast<std::uint32_t>(input[offset + 1]) << 16U) |
                (static_cast<std::uint32_t>(input[offset + 2]) << 8U) |
                static_cast<std::uint32_t>(input[offset + 3]);
        }
        for (std::size_t index = 16; index < schedule.size(); ++index)
        {
            const std::uint32_t first = schedule[index - 15];
            const std::uint32_t second = schedule[index - 2];
            const std::uint32_t sigma0 =
                rotateRight(first, 7) ^ rotateRight(first, 18) ^ (first >> 3U);
            const std::uint32_t sigma1 =
                rotateRight(second, 17) ^ rotateRight(second, 19) ^ (second >> 10U);
            schedule[index] = schedule[index - 16] + sigma0 +
                schedule[index - 7] + sigma1;
        }

        std::uint32_t a = state[0];
        std::uint32_t b = state[1];
        std::uint32_t c = state[2];
        std::uint32_t d = state[3];
        std::uint32_t e = state[4];
        std::uint32_t f = state[5];
        std::uint32_t g = state[6];
        std::uint32_t h = state[7];
        for (std::size_t index = 0; index < schedule.size(); ++index)
        {
            const std::uint32_t sum1 =
                rotateRight(e, 6) ^ rotateRight(e, 11) ^ rotateRight(e, 25);
            const std::uint32_t choice = (e & f) ^ (~e & g);
            const std::uint32_t temporary1 = h + sum1 + choice +
                kSha256RoundConstants[index] + schedule[index];
            const std::uint32_t sum0 =
                rotateRight(a, 2) ^ rotateRight(a, 13) ^ rotateRight(a, 22);
            const std::uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
            const std::uint32_t temporary2 = sum0 + majority;

            h = g;
            g = f;
            f = e;
            e = d + temporary1;
            d = c;
            c = b;
            b = a;
            a = temporary1 + temporary2;
        }

        state[0] += a;
        state[1] += b;
        state[2] += c;
        state[3] += d;
        state[4] += e;
        state[5] += f;
        state[6] += g;
        state[7] += h;
    }

    std::array<std::uint32_t, 8> state = {
        0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU,
        0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U};
    std::array<std::uint8_t, 64> block{};
    std::size_t blockSize = 0;
    std::uint64_t totalBytes = 0;
};

std::string toHex(std::span<const std::uint8_t> bytes)
{
    constexpr char digits[] = "0123456789abcdef";
    std::string result(bytes.size() * 2, '0');
    for (std::size_t index = 0; index < bytes.size(); ++index)
    {
        result[index * 2] = digits[bytes[index] >> 4U];
        result[index * 2 + 1] = digits[bytes[index] & 0x0fU];
    }
    return result;
}

std::string sha256File(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error(
            "cannot open packaged launcher resource for SHA-256 verification: " +
            path.generic_string());

    Sha256 hash;
    std::array<std::uint8_t, 64 * 1024> buffer{};
    while (input)
    {
        input.read(reinterpret_cast<char*>(buffer.data()),
            static_cast<std::streamsize>(buffer.size()));
        const std::streamsize count = input.gcount();
        if (count > 0)
            hash.update(std::span(buffer.data(), static_cast<std::size_t>(count)));
    }
    if (!input.eof())
        throw std::runtime_error(
            "cannot read packaged launcher resource for SHA-256 verification: " +
            path.generic_string());
    const std::array<std::uint8_t, 32> digest = hash.finish();
    return toHex(digest);
}

bool isLowerHexSha256(std::string_view value)
{
    if (value.size() != kSha256HexLength)
        return false;
    for (const char character : value)
    {
        if (!((character >= '0' && character <= '9') ||
              (character >= 'a' && character <= 'f')))
            return false;
    }
    return true;
}

template<typename Integer>
Integer parseUnsigned(std::string_view value, std::string_view field)
{
    Integer result = 0;
    const auto [end, error] = std::from_chars(
        value.data(), value.data() + value.size(), result);
    if (error != std::errc() || end != value.data() + value.size())
        throw std::runtime_error(
            "invalid " + std::string(field) + " in packaged launcher manifest");
    return result;
}

std::string validateLogicalPath(std::string_view value)
{
    if (value.empty() || value.front() == '/' || value.back() == '/' ||
        value.find('\\') != std::string_view::npos ||
        value.find('\t') != std::string_view::npos ||
        value.find('\r') != std::string_view::npos ||
        value.find('\n') != std::string_view::npos)
        throw std::runtime_error("unsafe path in packaged launcher manifest");

    std::filesystem::path parsed(value);
    if (parsed.is_absolute() || parsed.has_root_path() ||
        parsed.generic_string() != value)
        throw std::runtime_error("non-canonical path in packaged launcher manifest");
    for (const std::filesystem::path& component : parsed)
    {
        if (component == "." || component == ".." || component.empty())
            throw std::runtime_error("traversal path in packaged launcher manifest");
    }
    return parsed.generic_string();
}

struct ManifestEntry final
{
    std::string logicalPath;
    std::string sha256;
    std::uintmax_t size = 0;
};

std::vector<ManifestEntry> parseManifest(const std::filesystem::path& path)
{
    const std::uintmax_t manifestSize = std::filesystem::file_size(path);
    if (manifestSize == 0 || manifestSize > kMaximumManifestBytes)
        throw std::runtime_error("packaged launcher manifest has an invalid size");

    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("cannot open packaged launcher manifest");

    std::string line;
    if (!std::getline(input, line) || line != kManifestHeader)
        throw std::runtime_error("packaged launcher manifest has an invalid header");
    if (!std::getline(input, line) ||
        !std::string_view(line).starts_with(kEntryCountPrefix))
        throw std::runtime_error("packaged launcher manifest has no entry count");
    const std::size_t declaredCount = parseUnsigned<std::size_t>(
        std::string_view(line).substr(kEntryCountPrefix.size()), "entry count");
    if (declaredCount != launcher_manifest::entryCount)
        throw std::runtime_error(
            "packaged launcher manifest entry count does not match the Player build");

    std::vector<ManifestEntry> entries;
    entries.reserve(declaredCount);
    std::set<std::string, std::less<>> logicalPaths;
    while (std::getline(input, line))
    {
        const std::size_t firstSeparator = line.find('\t');
        const std::size_t secondSeparator = firstSeparator == std::string::npos
            ? std::string::npos
            : line.find('\t', firstSeparator + 1);
        if (firstSeparator == std::string::npos ||
            secondSeparator == std::string::npos ||
            line.find('\t', secondSeparator + 1) != std::string::npos)
            throw std::runtime_error("malformed entry in packaged launcher manifest");

        ManifestEntry entry;
        entry.sha256 = line.substr(0, firstSeparator);
        if (!isLowerHexSha256(entry.sha256))
            throw std::runtime_error("invalid SHA-256 in packaged launcher manifest");
        entry.size = parseUnsigned<std::uintmax_t>(
            std::string_view(line).substr(firstSeparator + 1,
                secondSeparator - firstSeparator - 1), "file size");
        entry.logicalPath = validateLogicalPath(
            std::string_view(line).substr(secondSeparator + 1));
        if (!logicalPaths.insert(entry.logicalPath).second)
            throw std::runtime_error(
                "duplicate path in packaged launcher manifest: " + entry.logicalPath);
        entries.push_back(std::move(entry));
    }
    if (!input.eof())
        throw std::runtime_error("cannot read packaged launcher manifest");
    if (entries.size() != declaredCount)
        throw std::runtime_error(
            "packaged launcher manifest contains the wrong number of entries");
    if (!logicalPaths.contains("ScaledWorldv4.7.rbxl"))
        throw std::runtime_error(
            "packaged launcher manifest does not cover the live ScaledWorld place");
    return entries;
}

std::filesystem::file_status requireStatus(
    const std::filesystem::path& path, std::string_view description)
{
    std::error_code error;
    const std::filesystem::file_status status =
        std::filesystem::symlink_status(path, error);
    if (error)
        throw std::runtime_error(
            "cannot inspect packaged launcher " + std::string(description) + ": " +
            path.generic_string());
    return status;
}

}

void verifyLauncherResourceIntegrity(const std::filesystem::path& resourceRoot)
{
    const std::filesystem::path launcherRoot = resourceRoot / "launcher";
    const std::filesystem::path contentRoot = launcherRoot / "content";
    const std::filesystem::path manifestPath =
        launcherRoot / "launcher-manifest.v1";

    const std::filesystem::file_status launcherStatus =
        requireStatus(launcherRoot, "root");
    const std::filesystem::file_status contentStatus =
        requireStatus(contentRoot, "content root");
    const std::filesystem::file_status manifestStatus =
        requireStatus(manifestPath, "manifest");
    if (std::filesystem::is_symlink(launcherStatus) ||
        !std::filesystem::is_directory(launcherStatus) ||
        std::filesystem::is_symlink(contentStatus) ||
        !std::filesystem::is_directory(contentStatus) ||
        std::filesystem::is_symlink(manifestStatus) ||
        !std::filesystem::is_regular_file(manifestStatus))
        throw std::runtime_error(
            "packaged Durango launcher roots and manifest must be regular non-symlink paths");

    for (const std::filesystem::directory_entry& item :
         std::filesystem::directory_iterator(launcherRoot))
    {
        const std::filesystem::file_status status = item.symlink_status();
        if (std::filesystem::is_symlink(status))
            throw std::runtime_error(
                "packaged Durango launcher contains a symlink: " +
                item.path().generic_string());

        const std::filesystem::path filename = item.path().filename();
        const bool isContentRoot = filename == "content";
        const bool isManifest = filename == "launcher-manifest.v1";
        if ((!isContentRoot && !isManifest) ||
            (isContentRoot && !std::filesystem::is_directory(status)) ||
            (isManifest && !std::filesystem::is_regular_file(status)))
            throw std::runtime_error(
                "packaged Durango launcher contains an unexpected root entry: " +
                filename.generic_string());
    }

    const std::uintmax_t manifestSize = std::filesystem::file_size(manifestPath);
    if (manifestSize == 0 || manifestSize > kMaximumManifestBytes)
        throw std::runtime_error("packaged launcher manifest has an invalid size");
    if (sha256File(manifestPath) != launcher_manifest::sha256)
        throw std::runtime_error(
            "packaged Durango launcher manifest does not match this Player build");

    const std::vector<ManifestEntry> entries = parseManifest(manifestPath);
    std::set<std::string, std::less<>> expectedPaths;
    std::set<std::string, std::less<>> expectedDirectories;
    for (const ManifestEntry& entry : entries)
    {
        const std::filesystem::path path = contentRoot / entry.logicalPath;
        const std::filesystem::file_status status =
            requireStatus(path, "resource");
        if (std::filesystem::is_symlink(status) ||
            !std::filesystem::is_regular_file(status))
            throw std::runtime_error(
                "packaged Durango launcher resource is missing or not a regular file: " +
                entry.logicalPath);
        if (std::filesystem::file_size(path) != entry.size)
            throw std::runtime_error(
                "packaged Durango launcher resource size mismatch: " +
                entry.logicalPath);
        if (sha256File(path) != entry.sha256)
            throw std::runtime_error(
                "packaged Durango launcher resource SHA-256 mismatch: " +
                entry.logicalPath);
        expectedPaths.insert(entry.logicalPath);
        for (std::filesystem::path parent =
                 std::filesystem::path(entry.logicalPath).parent_path();
             !parent.empty(); parent = parent.parent_path())
            expectedDirectories.insert(parent.generic_string());
    }

    std::size_t packagedFileCount = 0;
    for (const std::filesystem::directory_entry& item :
         std::filesystem::recursive_directory_iterator(contentRoot))
    {
        const std::filesystem::file_status status = item.symlink_status();
        if (std::filesystem::is_symlink(status))
            throw std::runtime_error(
                "packaged Durango launcher contains a symlink: " +
                item.path().generic_string());

        const std::filesystem::path relative =
            item.path().lexically_relative(contentRoot);
        const std::string logicalPath = validateLogicalPath(relative.generic_string());
        if (std::filesystem::is_directory(status))
        {
            if (!expectedDirectories.contains(logicalPath))
                throw std::runtime_error(
                    "packaged Durango launcher contains an unexpected directory: " +
                    logicalPath);
            continue;
        }
        if (!std::filesystem::is_regular_file(status))
            throw std::runtime_error(
                "packaged Durango launcher contains a non-regular entry: " +
                item.path().generic_string());
        if (!expectedPaths.contains(logicalPath))
            throw std::runtime_error(
                "packaged Durango launcher contains an unexpected resource: " +
                logicalPath);
        ++packagedFileCount;
    }
    if (packagedFileCount != entries.size())
        throw std::runtime_error(
            "packaged Durango launcher file count does not match its manifest");
}

}

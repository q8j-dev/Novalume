#include "rbx/SystemUtil.h"

#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <charconv>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace {

constexpr std::uint64_t bytesPerMiB = 1024ULL * 1024ULL;

std::string trim(std::string value)
{
    const auto isSpace = [](char character) {
        return std::isspace(static_cast<unsigned char>(character)) != 0;
    };
    value.erase(value.begin(), std::find_if_not(value.begin(), value.end(), isSpace));
    value.erase(std::find_if_not(value.rbegin(), value.rend(), isSpace).base(), value.end());
    return value;
}

std::optional<std::string> readTextFile(
    const std::filesystem::path& path, std::size_t maximumBytes = 4096)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
        return std::nullopt;

    std::string value;
    value.resize(maximumBytes);
    stream.read(value.data(), static_cast<std::streamsize>(value.size()));
    value.resize(static_cast<std::size_t>(stream.gcount()));
    if (stream.bad())
        return std::nullopt;
    return trim(std::move(value));
}

std::optional<std::uint64_t> parseUnsigned(std::string_view value)
{
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())))
        value.remove_prefix(1);
    const char* begin = value.data();
    const char* end = value.data() + value.size();
    std::uint64_t result = 0;
    const auto parsed = std::from_chars(begin, end, result);
    if (parsed.ec != std::errc() || parsed.ptr == begin)
        return std::nullopt;
    return result;
}

std::optional<std::uint64_t> readUnsignedFile(const std::filesystem::path& path)
{
    const std::optional<std::string> value = readTextFile(path, 128);
    return value ? parseUnsigned(*value) : std::nullopt;
}

std::optional<std::string> cpuInfoValue(std::initializer_list<std::string_view> keys)
{
    std::ifstream stream("/proc/cpuinfo");
    if (!stream)
        return std::nullopt;

    std::string line;
    while (std::getline(stream, line))
    {
        const std::size_t separator = line.find(':');
        if (separator == std::string::npos)
            continue;
        const std::string key = trim(line.substr(0, separator));
        for (std::string_view candidate : keys)
        {
            if (key == candidate)
            {
                std::string value = trim(line.substr(separator + 1));
                if (!value.empty())
                    return value;
            }
        }
    }
    return std::nullopt;
}

std::uint64_t positiveProcessorCount(long value)
{
    return value > 0 ? static_cast<std::uint64_t>(value) : 1;
}

bool isCpuDirectory(const std::filesystem::path& path)
{
    const std::string name = path.filename().string();
    return name.size() > 3 && name.starts_with("cpu") &&
        std::all_of(name.begin() + 3, name.end(), [](char character) {
            return std::isdigit(static_cast<unsigned char>(character)) != 0;
        });
}

struct CpuTopology
{
    std::set<std::pair<std::int64_t, std::int64_t>> cores;
    std::set<std::int64_t> packages;
};

std::optional<std::int64_t> readSignedFile(const std::filesystem::path& path)
{
    const std::optional<std::string> value = readTextFile(path, 128);
    if (!value)
        return std::nullopt;
    std::int64_t result = 0;
    const char* begin = value->data();
    const char* end = value->data() + value->size();
    const auto parsed = std::from_chars(begin, end, result);
    if (parsed.ec != std::errc() || parsed.ptr == begin)
        return std::nullopt;
    return result;
}

CpuTopology readCpuTopology()
{
    CpuTopology topology;
    std::error_code error;
    const std::filesystem::path root("/sys/devices/system/cpu");
    for (std::filesystem::directory_iterator iterator(root, error), end;
         !error && iterator != end; iterator.increment(error))
    {
        if (!isCpuDirectory(iterator->path()))
            continue;
        const std::filesystem::path topologyRoot = iterator->path() / "topology";
        const std::optional<std::int64_t> package =
            readSignedFile(topologyRoot / "physical_package_id");
        const std::optional<std::int64_t> core = readSignedFile(topologyRoot / "core_id");
        if (package)
            topology.packages.insert(*package);
        if (package && core)
            topology.cores.emplace(*package, *core);
    }
    return topology;
}

std::uint64_t scaledBytes(std::uint64_t value, std::uint64_t multiplier)
{
    if (multiplier != 0 && value > std::numeric_limits<std::uint64_t>::max() / multiplier)
        return std::numeric_limits<std::uint64_t>::max();
    return value * multiplier;
}

std::optional<std::uint64_t> memoryInfoKiB(std::string_view requestedKey)
{
    std::ifstream stream("/proc/meminfo");
    if (!stream)
        return std::nullopt;

    std::string key;
    std::uint64_t value = 0;
    std::string unit;
    while (stream >> key >> value >> unit)
    {
        if (!key.empty() && key.back() == ':')
            key.pop_back();
        if (key == requestedKey && unit == "kB")
            return value;
    }
    return std::nullopt;
}

std::string unquoteOsReleaseValue(std::string value)
{
    value = trim(std::move(value));
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
    {
        value = value.substr(1, value.size() - 2);
        std::string decoded;
        decoded.reserve(value.size());
        bool escaped = false;
        for (char character : value)
        {
            if (escaped)
            {
                decoded.push_back(character);
                escaped = false;
            }
            else if (character == '\\')
            {
                escaped = true;
            }
            else
            {
                decoded.push_back(character);
            }
        }
        if (escaped)
            decoded.push_back('\\');
        return decoded;
    }
    return value;
}

std::optional<std::string> osReleaseValue(std::string_view requestedKey)
{
    std::ifstream stream("/etc/os-release");
    if (!stream)
        return std::nullopt;

    std::string line;
    while (std::getline(stream, line))
    {
        const std::size_t separator = line.find('=');
        if (separator == std::string::npos || line.substr(0, separator) != requestedKey)
            continue;
        std::string value = unquoteOsReleaseValue(line.substr(separator + 1));
        if (!value.empty())
            return value;
    }
    return std::nullopt;
}

std::optional<std::filesystem::path> primaryDrmCard()
{
    std::error_code error;
    const std::filesystem::path root("/sys/class/drm");
    std::optional<std::filesystem::path> lowestIndexedCard;
    std::uint64_t lowestCardIndex = std::numeric_limits<std::uint64_t>::max();
    for (std::filesystem::directory_iterator iterator(root, error), end;
         !error && iterator != end; iterator.increment(error))
    {
        const std::string name = iterator->path().filename().string();
        if (name.size() <= 4 || !name.starts_with("card") ||
            !std::all_of(name.begin() + 4, name.end(), [](char character) {
                return std::isdigit(static_cast<unsigned char>(character)) != 0;
            }))
        {
            continue;
        }
        const std::optional<std::uint64_t> cardIndex =
            parseUnsigned(std::string_view(name).substr(4));
        if (!cardIndex)
            continue;
        if (readUnsignedFile(iterator->path() / "device/boot_vga").value_or(0) == 1)
            return iterator->path();
        if (*cardIndex < lowestCardIndex)
        {
            lowestCardIndex = *cardIndex;
            lowestIndexedCard = iterator->path();
        }
    }
    return lowestIndexedCard;
}

std::string drmDeviceDescription(const std::filesystem::path& card)
{
    const std::optional<std::string> vendor = readTextFile(card / "device/vendor", 64);
    const std::optional<std::string> device = readTextFile(card / "device/device", 64);
    const std::optional<std::string> uevent = readTextFile(card / "device/uevent", 4096);
    std::string driver;
    if (uevent)
    {
        std::istringstream lines(*uevent);
        std::string line;
        while (std::getline(lines, line))
        {
            if (line.starts_with("DRIVER="))
            {
                driver = line.substr(7);
                break;
            }
        }
    }

    std::ostringstream result;
    if (!driver.empty())
        result << driver;
    if (vendor && device)
    {
        if (!driver.empty())
            result << ' ';
        result << *vendor << ':' << *device;
    }
    return result.str();
}

}

namespace RBX::SystemUtil {

std::string getCPUMake()
{
    if (const std::optional<std::string> value =
            cpuInfoValue({"model name", "Hardware", "Processor"}))
    {
        return *value;
    }
    struct utsname information = {};
    return uname(&information) == 0 ? information.machine : std::string();
}

std::uint64_t getCPUSpeed()
{
    if (const std::optional<std::uint64_t> kilohertz = readUnsignedFile(
            "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq"))
    {
        return *kilohertz / 1000;
    }
    if (const std::optional<std::string> value = cpuInfoValue({"cpu MHz"}))
    {
        char* end = nullptr;
        const double megahertz = std::strtod(value->c_str(), &end);
        if (end != value->c_str() && megahertz > 0 &&
            megahertz <= static_cast<double>(std::numeric_limits<std::uint64_t>::max()))
        {
            return static_cast<std::uint64_t>(megahertz);
        }
    }
    return 0;
}

std::uint64_t getCPULogicalCount()
{
    return positiveProcessorCount(sysconf(_SC_NPROCESSORS_ONLN));
}

std::uint64_t getCPUCoreCount()
{
    const CpuTopology topology = readCpuTopology();
    return topology.cores.empty()
        ? getCPULogicalCount()
        : static_cast<std::uint64_t>(topology.cores.size());
}

std::uint64_t getCPUPhysicalCount()
{
    const CpuTopology topology = readCpuTopology();
    return topology.packages.empty()
        ? 1
        : static_cast<std::uint64_t>(topology.packages.size());
}

bool isCPU64Bit()
{
    return sizeof(void*) == 8;
}

std::uint64_t getMBSysRAM()
{
    if (const std::optional<std::uint64_t> totalKiB = memoryInfoKiB("MemTotal"))
        return *totalKiB / 1024;

    struct sysinfo information = {};
    if (sysinfo(&information) != 0)
        return 0;
    return scaledBytes(information.totalram, information.mem_unit) / bytesPerMiB;
}

std::uint64_t getMBSysAvailableRAM()
{
    if (const std::optional<std::uint64_t> availableKiB = memoryInfoKiB("MemAvailable"))
        return *availableKiB / 1024;

    struct sysinfo information = {};
    if (sysinfo(&information) != 0)
        return 0;
    const std::uint64_t availablePages = information.freeram + information.bufferram;
    return scaledBytes(availablePages, information.mem_unit) / bytesPerMiB;
}

std::uint64_t getVideoMemory()
{
    const std::optional<std::filesystem::path> card = primaryDrmCard();
    if (!card)
        return 0;
    return readUnsignedFile(*card / "device/mem_info_vram_total").value_or(0);
}

std::string osPlatform()
{
    return "Linux";
}

int osPlatformId()
{
    return 0;
}

std::string osVer()
{
    if (const std::optional<std::string> prettyName = osReleaseValue("PRETTY_NAME"))
        return *prettyName;
    struct utsname information = {};
    if (uname(&information) != 0)
        return std::string();
    return std::string(information.sysname) + ' ' + information.release;
}

std::string deviceName()
{
    std::array<char, 256> name = {};
    if (gethostname(name.data(), name.size() - 1) == 0)
        return name.data();
    struct utsname information = {};
    return uname(&information) == 0 ? information.nodename : std::string();
}

std::string getGPUMake()
{
    const std::optional<std::filesystem::path> card = primaryDrmCard();
    return card ? drmDeviceDescription(*card) : std::string();
}

std::string getMaxRes()
{
    std::uint64_t bestPixels = 0;
    unsigned int bestWidth = 0;
    unsigned int bestHeight = 0;
    std::error_code error;
    const std::filesystem::path root("/sys/class/drm");
    for (std::filesystem::directory_iterator iterator(root, error), end;
         !error && iterator != end; iterator.increment(error))
    {
        const std::string name = iterator->path().filename().string();
        if (!name.starts_with("card") || name.find('-') == std::string::npos)
            continue;
        std::ifstream modes(iterator->path() / "modes");
        std::string mode;
        while (std::getline(modes, mode))
        {
            unsigned int width = 0;
            unsigned int height = 0;
            const std::size_t separator = mode.find('x');
            if (separator == std::string::npos)
                continue;
            const auto widthResult = std::from_chars(
                mode.data(), mode.data() + separator, width);
            const auto heightResult = std::from_chars(
                mode.data() + separator + 1, mode.data() + mode.size(), height);
            if (widthResult.ec != std::errc() || heightResult.ec != std::errc())
                continue;
            const std::uint64_t pixels =
                static_cast<std::uint64_t>(width) * static_cast<std::uint64_t>(height);
            if (pixels > bestPixels)
            {
                bestPixels = pixels;
                bestWidth = width;
                bestHeight = height;
            }
        }
    }
    if (bestPixels == 0)
        return std::string();
    return std::to_string(bestWidth) + 'x' + std::to_string(bestHeight);
}

}

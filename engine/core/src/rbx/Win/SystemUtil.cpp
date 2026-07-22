#include "rbx/SystemUtil.h"

#include <windows.h>
#include <dxgi1_1.h>
#include <winternl.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace {

constexpr std::uint64_t bytesPerMiB = 1024ULL * 1024ULL;

std::string wideToUtf8(const wchar_t* value)
{
    if (!value || *value == L'\0')
        return std::string();
    const int required = WideCharToMultiByte(
        CP_UTF8, WC_ERR_INVALID_CHARS, value, -1, nullptr, 0, nullptr, nullptr);
    if (required <= 1)
        return std::string();
    std::string result(static_cast<std::size_t>(required), '\0');
    if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value, -1,
            result.data(), required, nullptr, nullptr) == 0)
    {
        return std::string();
    }
    result.resize(static_cast<std::size_t>(required - 1));
    return result;
}

std::optional<std::string> readRegistryString(const wchar_t* name)
{
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
            L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0,
            KEY_QUERY_VALUE | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS)
    {
        return std::nullopt;
    }

    DWORD type = 0;
    DWORD byteCount = 0;
    const LSTATUS sizeStatus = RegQueryValueExW(
        key, name, nullptr, &type, nullptr, &byteCount);
    constexpr DWORD maximumRegistryStringBytes = 64 * 1024;
    if (sizeStatus != ERROR_SUCCESS ||
        (type != REG_SZ && type != REG_EXPAND_SZ) ||
        byteCount < sizeof(wchar_t) || byteCount > maximumRegistryStringBytes)
    {
        RegCloseKey(key);
        return std::nullopt;
    }

    std::vector<wchar_t> value(
        static_cast<std::size_t>(byteCount / sizeof(wchar_t)) + 1, L'\0');
    const LSTATUS valueStatus = RegQueryValueExW(key, name, nullptr, &type,
        reinterpret_cast<BYTE*>(value.data()), &byteCount);
    RegCloseKey(key);
    if (valueStatus != ERROR_SUCCESS)
        return std::nullopt;
    return wideToUtf8(value.data());
}

std::optional<DWORD> readRegistryDword(const wchar_t* name)
{
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
            L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0,
            KEY_QUERY_VALUE | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS)
    {
        return std::nullopt;
    }
    DWORD type = 0;
    DWORD value = 0;
    DWORD size = sizeof(value);
    const LSTATUS status = RegQueryValueExW(key, name, nullptr, &type,
        reinterpret_cast<BYTE*>(&value), &size);
    RegCloseKey(key);
    if (status != ERROR_SUCCESS || type != REG_DWORD || size != sizeof(value))
        return std::nullopt;
    return value;
}

std::uint64_t logicalProcessorCount()
{
    const DWORD count = GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
    if (count != 0)
        return count;
    SYSTEM_INFO information = {};
    GetNativeSystemInfo(&information);
    return std::max<DWORD>(information.dwNumberOfProcessors, 1);
}

std::uint64_t processorRelationshipCount(LOGICAL_PROCESSOR_RELATIONSHIP relationship)
{
    DWORD size = 0;
    if (GetLogicalProcessorInformationEx(relationship, nullptr, &size) != FALSE ||
        GetLastError() != ERROR_INSUFFICIENT_BUFFER || size == 0 ||
        size > 16 * 1024 * 1024)
    {
        return 0;
    }

    std::vector<unsigned char> bytes(size);
    if (GetLogicalProcessorInformationEx(relationship,
            reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(bytes.data()),
            &size) == FALSE)
    {
        return 0;
    }

    std::uint64_t count = 0;
    std::size_t offset = 0;
    while (offset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX) <= size)
    {
        const auto* information =
            reinterpret_cast<const SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(
                bytes.data() + offset);
        if (information->Size < sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX) ||
            information->Size > size - offset)
        {
            return 0;
        }
        ++count;
        offset += information->Size;
    }
    return offset == size ? count : 0;
}

struct GraphicsAdapterInformation
{
    std::string description;
    std::uint64_t memoryBytes = 0;
};

GraphicsAdapterInformation primaryGraphicsAdapter()
{
    IDXGIFactory1* factory = nullptr;
    if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1),
            reinterpret_cast<void**>(&factory))))
    {
        return {};
    }

    GraphicsAdapterInformation result;
    for (UINT index = 0;; ++index)
    {
        IDXGIAdapter1* adapter = nullptr;
        const HRESULT enumerate = factory->EnumAdapters1(index, &adapter);
        if (enumerate == DXGI_ERROR_NOT_FOUND)
            break;
        if (FAILED(enumerate) || !adapter)
            continue;

        DXGI_ADAPTER_DESC1 description = {};
        if (SUCCEEDED(adapter->GetDesc1(&description)) &&
            (description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0)
        {
            result.description = wideToUtf8(description.Description);
            result.memoryBytes = description.DedicatedVideoMemory;
            adapter->Release();
            break;
        }
        adapter->Release();
    }
    factory->Release();
    return result;
}

const GraphicsAdapterInformation& graphicsAdapter()
{
    static const GraphicsAdapterInformation information = primaryGraphicsAdapter();
    return information;
}

std::string processorArchitecture()
{
    SYSTEM_INFO information = {};
    GetNativeSystemInfo(&information);
    switch (information.wProcessorArchitecture)
    {
    case PROCESSOR_ARCHITECTURE_AMD64:
        return "x86-64";
    case PROCESSOR_ARCHITECTURE_ARM64:
        return "ARM64";
    case PROCESSOR_ARCHITECTURE_INTEL:
        return "x86";
    case PROCESSOR_ARCHITECTURE_ARM:
        return "ARM";
    default:
        return std::string();
    }
}

}

namespace RBX::SystemUtil {

std::string getCPUMake()
{
    const std::optional<std::string> name = readRegistryString(L"ProcessorNameString");
    return name && !name->empty() ? *name : processorArchitecture();
}

std::uint64_t getCPUSpeed()
{
    const std::optional<DWORD> megahertz = readRegistryDword(L"~MHz");
    return megahertz.value_or(0);
}

std::uint64_t getCPULogicalCount()
{
    return logicalProcessorCount();
}

std::uint64_t getCPUCoreCount()
{
    const std::uint64_t count = processorRelationshipCount(RelationProcessorCore);
    return count == 0 ? logicalProcessorCount() : count;
}

std::uint64_t getCPUPhysicalCount()
{
    const std::uint64_t count = processorRelationshipCount(RelationProcessorPackage);
    return count == 0 ? 1 : count;
}

bool isCPU64Bit()
{
#if defined(_WIN64)
    return true;
#else
    BOOL isWow64 = FALSE;
    return IsWow64Process(GetCurrentProcess(), &isWow64) != FALSE && isWow64 != FALSE;
#endif
}

std::uint64_t getMBSysRAM()
{
    MEMORYSTATUSEX memory = {};
    memory.dwLength = sizeof(memory);
    return GlobalMemoryStatusEx(&memory) != FALSE
        ? memory.ullTotalPhys / bytesPerMiB
        : 0;
}

std::uint64_t getMBSysAvailableRAM()
{
    MEMORYSTATUSEX memory = {};
    memory.dwLength = sizeof(memory);
    return GlobalMemoryStatusEx(&memory) != FALSE
        ? memory.ullAvailPhys / bytesPerMiB
        : 0;
}

std::uint64_t getVideoMemory()
{
    return graphicsAdapter().memoryBytes;
}

std::string osPlatform()
{
    return "Win32";
}

int osPlatformId()
{
    return VER_PLATFORM_WIN32_NT;
}

std::string osVer()
{
    using RtlGetVersionFunction = LONG (WINAPI*)(PRTL_OSVERSIONINFOW);
    const HMODULE module = GetModuleHandleW(L"ntdll.dll");
    const auto function = module
        ? reinterpret_cast<RtlGetVersionFunction>(
              GetProcAddress(module, "RtlGetVersion"))
        : nullptr;
    RTL_OSVERSIONINFOW version = {};
    version.dwOSVersionInfoSize = sizeof(version);
    if (!function || function(&version) != 0)
        return std::string();
    return std::to_string(version.dwMajorVersion) + '.' +
        std::to_string(version.dwMinorVersion) + '.' +
        std::to_string(version.dwBuildNumber);
}

std::string deviceName()
{
    DWORD size = 0;
    GetComputerNameExW(ComputerNamePhysicalDnsHostname, nullptr, &size);
    if (size == 0 || size > 32768)
        return std::string();
    std::vector<wchar_t> name(static_cast<std::size_t>(size) + 1, L'\0');
    if (GetComputerNameExW(ComputerNamePhysicalDnsHostname, name.data(), &size) == FALSE)
        return std::string();
    return wideToUtf8(name.data());
}

std::string getGPUMake()
{
    return graphicsAdapter().description;
}

std::string getMaxRes()
{
    DWORD bestWidth = 0;
    DWORD bestHeight = 0;
    std::uint64_t bestPixels = 0;

    for (DWORD displayIndex = 0;; ++displayIndex)
    {
        DISPLAY_DEVICEW display = {};
        display.cb = sizeof(display);
        if (EnumDisplayDevicesW(nullptr, displayIndex, &display, 0) == FALSE)
            break;
        if ((display.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) == 0)
            continue;

        for (DWORD modeIndex = 0;; ++modeIndex)
        {
            DEVMODEW mode = {};
            mode.dmSize = sizeof(mode);
            if (EnumDisplaySettingsExW(
                    display.DeviceName, modeIndex, &mode, EDS_RAWMODE) == FALSE)
            {
                break;
            }
            if (mode.dmPelsWidth == 0 || mode.dmPelsHeight == 0)
                continue;

            const std::uint64_t pixels =
                static_cast<std::uint64_t>(mode.dmPelsWidth) *
                static_cast<std::uint64_t>(mode.dmPelsHeight);
            if (pixels > bestPixels ||
                (pixels == bestPixels && mode.dmPelsWidth > bestWidth))
            {
                bestPixels = pixels;
                bestWidth = mode.dmPelsWidth;
                bestHeight = mode.dmPelsHeight;
            }
        }
    }

    if (bestPixels == 0)
        return std::string();
    return std::to_string(bestWidth) + 'x' + std::to_string(bestHeight);
}

}

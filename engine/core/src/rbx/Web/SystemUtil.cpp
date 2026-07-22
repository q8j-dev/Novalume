#include "rbx/SystemUtil.h"

#include <emscripten.h>

#include <algorithm>
#include <cstdint>
#include <string>

namespace {

EM_JS(int, browserLogicalProcessorCount, (), {
    return Math.max(1, navigator.hardwareConcurrency || 1);
});

EM_JS(int, browserMemoryMiB, (), {
    return Math.max(256, Math.round((navigator.deviceMemory || 4) * 1024));
});

std::string scriptString(const char* script)
{
    const char* value = emscripten_run_script_string(script);
    return value ? value : "";
}

}

namespace RBX::SystemUtil {

std::string getCPUMake()
{
    return scriptString("navigator.userAgentData?.platform || navigator.platform || 'WebAssembly'");
}

std::uint64_t getCPUSpeed()
{
    return 0;
}

std::uint64_t getCPULogicalCount()
{
    return static_cast<std::uint64_t>(browserLogicalProcessorCount());
}

std::uint64_t getCPUCoreCount()
{
    return getCPULogicalCount();
}

std::uint64_t getCPUPhysicalCount()
{
    return getCPULogicalCount();
}

bool isCPU64Bit()
{
    return false;
}

std::uint64_t getMBSysRAM()
{
    return static_cast<std::uint64_t>(browserMemoryMiB());
}

std::uint64_t getMBSysAvailableRAM()
{
    return getMBSysRAM() / 2;
}

std::uint64_t getVideoMemory()
{
    return 256ULL * 1024ULL * 1024ULL;
}

std::string osPlatform()
{
    return "Web";
}

int osPlatformId()
{
    return 0;
}

std::string osVer()
{
    return scriptString("navigator.userAgent");
}

std::string deviceName()
{
    return "Browser";
}

std::string getGPUMake()
{
    return scriptString("navigator.gpu ? 'WebGPU' : 'WebGL'");
}

std::string getMaxRes()
{
    return scriptString("typeof screen === 'undefined' ? '0x0' : String(screen.width) + 'x' + String(screen.height)");
}

}

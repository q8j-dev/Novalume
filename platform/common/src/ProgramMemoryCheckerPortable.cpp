#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "Util/ProgramMemoryChecker.h"

namespace RBX {

using namespace Hasher;

#if defined(_WIN32)
__declspec(align(8)) const char* const maskAddr = "portable-player-mask";
__declspec(align(8)) const char* const goldHash = "portable-player-hash";
#else
__attribute__((aligned(8))) const char* const maskAddr = "portable-player-mask";
__attribute__((aligned(8))) const char* const goldHash = "portable-player-hash";
#endif

PmcHashContainer pmcHash;

namespace Security {
volatile const size_t rbxGoldHash = 0;
volatile const uintptr_t rbxLowerBase = 0;
volatile const size_t rbxLowerSize = 0;
volatile const uintptr_t rbxUpperBase = 0;
volatile const size_t rbxUpperSize = 0;
volatile const uintptr_t rbxRdataBase = 0;
volatile const size_t rbxRdataSize = 0;
volatile const uintptr_t rbxVmpBase = 0;
volatile const size_t rbxVmpSize = 0;
volatile const uintptr_t rbxIatBase = 0;
volatile const size_t rbxIatSize = 0;
volatile const uintptr_t rbxVmpPlainBase = 0;
volatile const size_t rbxVmpPlainSize = 0;
volatile const uintptr_t rbxVmpMutantBase = 0;
volatile const size_t rbxVmpMutantSize = 0;
volatile const uintptr_t rbxVmp0MiscBase = 0;
volatile const size_t rbxVmp0MiscSize = 0;
volatile const uintptr_t rbxVmp1MiscBase = 0;
volatile const size_t rbxVmp1MiscSize = 0;
volatile const uintptr_t rbxRdataNoIatBase = 0;
volatile const size_t rbxRdataNoIatSize = 0;
} // namespace Security

ScanRegion ScanRegion::getScanRegion(const char*, const char*)
{
    return {};
}

PmcHashContainer::PmcHashContainer(const PmcHashContainer& value)
    : nonce(value.nonce)
    , hash(value.hash)
{
}

ProgramMemoryChecker::ProgramMemoryChecker()
    : hsceHashOrReduced(0)
    , hsceHashAndReduced(0)
    , bytesPerStep(0)
    , currentRegion(0)
    , currentMemory(nullptr)
    , lastCompletedHash(0)
    , lastGoldenHash(0)
    , lastCompletedTime(Time::nowFast())
{
}

bool ProgramMemoryChecker::areMemoryPagePermissionsSetupForHacking()
{
    return false;
}

unsigned int ProgramMemoryChecker::step()
{
    lastCompletedTime = Time::nowFast();
    return kAllDone;
}

int ProgramMemoryChecker::isLuaLockOk() const
{
    return kLuaLockOk;
}

unsigned int ProgramMemoryChecker::getLastCompletedHash() const
{
    return lastCompletedHash;
}

unsigned int ProgramMemoryChecker::getLastGoldenHash() const
{
    return lastGoldenHash;
}

Time ProgramMemoryChecker::getLastCompletedTime() const
{
    return lastCompletedTime;
}

void ProgramMemoryChecker::getLastHashes(PmcHashContainer::HashVector& output) const
{
    output.clear();
}

unsigned int ProgramMemoryChecker::hashScanningRegions(size_t) const
{
    return 0;
}

unsigned int ProgramMemoryChecker::updateHsceHash()
{
    return 0;
}

unsigned int ProgramMemoryChecker::getHsceOrHash() const
{
    return hsceHashOrReduced;
}

unsigned int ProgramMemoryChecker::getHsceAndHash() const
{
    return hsceHashAndReduced;
}

} // namespace RBX

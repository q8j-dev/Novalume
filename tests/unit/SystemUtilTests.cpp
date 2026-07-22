#include "g3d/System.h"
#include "rbx/Crypt.h"
#include "rbx/SystemUtil.h"
#include "rbx/atomic.h"
#include "rbx/rbxTime.h"

#include <chrono>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

namespace {

void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

template <typename Function>
void requireThrows(Function&& function, const char* message)
{
    try
    {
        function();
    }
    catch (const std::exception&)
    {
        return;
    }
    throw std::runtime_error(message);
}

}

int main()
{
    try
    {
        rbx::atomic<int> atomicValue(1);
        require(atomicValue.compare_and_swap(2, 1) == 1 && atomicValue == 2,
            "atomic compare-and-swap must return and replace the prior value");
        require(atomicValue++ == 2 && atomicValue == 3,
            "atomic post-increment must preserve its historical contract");
        require(atomicValue.swap(7) == 3 && atomicValue == 7,
            "atomic exchange must return the prior value");

        const long long tickBefore = RBX::Time::getTickCount();
        const std::uint64_t cycleBefore = G3D::System::getCycleCount();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        const long long tickAfter = RBX::Time::getTickCount();
        const std::uint64_t cycleAfter = G3D::System::getCycleCount();
        require(tickAfter > tickBefore,
            "the native monotonic clock must advance");
        require(cycleAfter > cycleBefore,
            "the native high-resolution counter must advance");

        const std::uint64_t logicalProcessors =
            RBX::SystemUtil::getCPULogicalCount();
        const std::uint64_t processorCores =
            RBX::SystemUtil::getCPUCoreCount();
        const std::uint64_t processorPackages =
            RBX::SystemUtil::getCPUPhysicalCount();
        require(logicalProcessors >= 1, "logical processor count must be positive");
        require(processorCores >= 1, "physical core count must be positive");
        require(processorPackages >= 1, "processor package count must be positive");
        require(processorPackages <= processorCores,
            "processor packages cannot outnumber physical cores");
        require(!RBX::SystemUtil::getCPUMake().empty(),
            "CPU identity must come from the host");
        if (sizeof(void*) == 8)
            require(RBX::SystemUtil::isCPU64Bit(),
                "a 64-bit build must report 64-bit CPU support");

        const std::uint64_t totalMemory = RBX::SystemUtil::getMBSysRAM();
        const std::uint64_t availableMemory =
            RBX::SystemUtil::getMBSysAvailableRAM();
        require(totalMemory > 0, "physical memory query must succeed");
        require(availableMemory <= totalMemory,
            "available physical memory cannot exceed total memory");
        require(totalMemory == RBX::SystemUtil::getMBSysRAM(),
            "total physical memory must be stable");

        require(!RBX::SystemUtil::osPlatform().empty(),
            "OS platform must be identified");
        require(!RBX::SystemUtil::osVer().empty(),
            "OS version must be identified");
        require(!RBX::SystemUtil::deviceName().empty(),
            "device name must come from the host");

        (void)RBX::SystemUtil::getVideoMemory();
        (void)RBX::SystemUtil::getGPUMake();
        (void)RBX::SystemUtil::getMaxRes();

        constexpr const char* message = "Hello World!";
        constexpr const char* signature =
            "ZLrv3Sy5zh08/+tec0tw2dMJ1JkhX/TcItuo/IuYPkP3muftzYEU3mt+uU9236H"
            "dh2RQlUIw3me/hI06aj9KVAbS8dHSzHbF6GpkhhmwmiLW4v8XfSFb//XujR4nEa"
            "dWi21mzWTVDEySJA66uotV63r3jvYmVHC+o35dBN0h5Jw=";
        RBX::Crypt crypt;
        crypt.verifySignatureBase64(message, signature);
        requireThrows(
            [&crypt] {
                crypt.verifySignatureBase64("Tampered message", signature);
            },
            "tampered signed content must be rejected");
        requireThrows(
            [&crypt] {
                crypt.verifySignatureBase64(message, "not valid base64");
            },
            "malformed signatures must be rejected");

        std::cout << "system platform=" << RBX::SystemUtil::osPlatform()
                  << " version=" << RBX::SystemUtil::osVer()
                  << " logical=" << logicalProcessors
                  << " cores=" << processorCores
                  << " packages=" << processorPackages
                  << " memory-mib=" << totalMemory
                  << " available-mib=" << availableMemory << '\n';
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "system-util contract failed: " << error.what() << '\n';
        return 1;
    }
}

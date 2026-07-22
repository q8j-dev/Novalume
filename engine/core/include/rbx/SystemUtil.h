#pragma once

#include <cstdint>
#include <string>

namespace RBX
{
    namespace SystemUtil
	{
		/// CPU Related
		std::string getCPUMake();		
		std::uint64_t getCPUSpeed();
		std::uint64_t getCPULogicalCount();
		std::uint64_t getCPUCoreCount();
		std::uint64_t getCPUPhysicalCount();
		bool isCPU64Bit();
		
		/// Memory Related
		std::uint64_t getMBSysRAM();
		std::uint64_t getMBSysAvailableRAM();
		std::uint64_t getVideoMemory();

		/// OS Related
        std::string osPlatform();
        int osPlatformId();
        std::string osVer();
		std::string deviceName();
		
		/// GPU Related
		std::string getGPUMake();
		
		// Display Resolution
		std::string getMaxRes();
    }
}

#pragma once

#include "network/NetworkTypes.h"

#include "network/PacketBuffer.h"

#include "NetworkSettings.h"
#include "Util.h"

#ifdef NETWORK_PROFILER
#define NETPROFILE_LOG(typeStr, packetPtr) RBX::Network::NetworkProfiler::singleton()->logPacket(typeStr, packetPtr)
#define NETPROFILE_START(dataBlobNameStr, bitStreamPtr) RBX::Network::NetworkProfiler::singleton()->startProfiling(dataBlobNameStr, bitStreamPtr)
#define NETPROFILE_END(dataBlobNameStr, bitStreamPtr) RBX::Network::NetworkProfiler::singleton()->endProfiling(dataBlobNameStr, bitStreamPtr)
#define CPUPROFILER_START(tag) RBX::Network::NetworkProfiler::singleton()->startCpuProfiling(tag);
#define CPUPROFILER_STEP(tag) RBX::Network::NetworkProfiler::singleton()->stepCpuProfiling(tag);
#define CPUPROFILER_OUTPUT() RBX::Network::NetworkProfiler::singleton()->outputCpuProfiling();
#else
#define NETPROFILE_LOG(typeStr, packetPtr)
#define NETPROFILE_START(dataBlobNameStr, bitStreamPtr)
#define NETPROFILE_END(dataBlobNameStr, bitStreamPtr)
#define CPUPROFILER_START(tag)
#define CPUPROFILER_STEP(tag)
#define CPUPROFILER_OUTPUT()
#endif

#ifdef NETWORK_PROFILER

namespace RBX
{
namespace Network
{

class NetworkProfiler
{
public:
	class DataBlobInfo
	{
	public:
		DataBlobInfo(const std::string& _name, RBX::Network::BitCount _bitStreamOffset)
		{
			name = _name;
			bitStreamOffset = _bitStreamOffset; 
		}
		std::string name;
		RBX::Network::BitCount bitStreamOffset;
	};

    enum ProfilerTags
    {
        PROFILER_streamOutPart,
        PROFILER_jointRemoval,
        PROFILER_gcStep,
        PROFILER_TAG_3,
        PROFILER_TAG_4,
        PROFILER_TAG_5,
        PROFILER_TAG_6,
        PROFILER_TAG_7,
        PROFILER_TAG_8,
        PROFILER_TAG_9,
        PROFILER_TAG_COUNT
    };

    class CpuProfilingStat
    {
        int currentStep;
        int numSample;
        RBX::Timer<RBX::Time::Precise> timer;

    public:
        double stepDelta[256]; // maximum 256 steps
        CpuProfilingStat()
        {
            reset();
        }
        inline void newSample() { timer.reset(); numSample++; currentStep = 0; }
        inline void step()
        {
            RBXASSERT(numSample>0);
            RBXASSERT(currentStep < 256);
            if (numSample <= 0 || currentStep >= 256)
                return;
            stepDelta[currentStep] = (stepDelta[currentStep] * (numSample-1) + timer.delta().seconds())/numSample;
            currentStep++;
        }
        int getNumSample() {return numSample;}
        void reset()
        {
            for (double& value : stepDelta)
                value = 0.0;
            currentStep = 0;
            numSample = 0;
        }
    };

    CpuProfilingStat cpuProfilingStats[PROFILER_TAG_COUNT]; 

	static NetworkProfiler* singleton();
	void logPacket(const std::string& type, const Packet* packet);

	void startProfiling(const std::string& dataBlobName, const RBX::Network::PacketBuffer* bitStream);
	void endProfiling(const std::string& dataBlobName, const RBX::Network::PacketBuffer* bitStream);

    void startCpuProfiling(int);
    void stepCpuProfiling(int);
    void outputCpuProfiling();

	virtual ~NetworkProfiler(void);
private:
	// members
	std::vector<DataBlobInfo> dataBlobStack; // use vector to make use of its iterator
	std::size_t deepestLayer;
	RBX::Timer<RBX::Time::Fast> profilerTimer;
	NetworkSettings* networkSettings;
	bool profilingActive;

	// functions
	NetworkProfiler(void);
	bool CanProfile();
};

}
}

#endif

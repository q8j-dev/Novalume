#include "NetworkProfiler.h"

#ifdef NETWORK_PROFILER

#include "rbx/Debug.h"
#include "util/standardout.h"

#include <algorithm>
#include <sstream>

namespace RBX::Network {

NetworkProfiler::NetworkProfiler()
    : deepestLayer(0)
    , networkSettings(&NetworkSettings::singleton())
    , profilingActive(false)
{
}

NetworkProfiler::~NetworkProfiler() = default;

NetworkProfiler* NetworkProfiler::singleton()
{
    static NetworkProfiler profiler;
    return &profiler;
}

bool NetworkProfiler::CanProfile()
{
    if (!networkSettings->profiling) {
        profilingActive = false;
        dataBlobStack.clear();
        deepestLayer = 0;
        return false;
    }
    if (!profilingActive) {
        profilingActive = true;
        profilerTimer.reset();
    }
    if (networkSettings->profilerTimedSeconds > 0.0f &&
        profilerTimer.delta().seconds() > networkSettings->profilerTimedSeconds) {
        networkSettings->profiling = false;
        profilingActive = false;
        dataBlobStack.clear();
        deepestLayer = 0;
        return false;
    }
    return true;
}

void NetworkProfiler::logPacket(const std::string& type, const Packet* packet)
{
    if (!CanProfile() || !packet)
        return;
    StandardOut::singleton()->printf(MESSAGE_INFO,
        "Network profile [%s] packet=%s bits=%zu",
        networkSettings->profilerTag.c_str(), type.c_str(),
        static_cast<std::size_t>(packet->length) * 8);
}

void NetworkProfiler::startProfiling(const std::string& dataBlobName,
    const PacketBuffer* bitStream)
{
    if (!CanProfile() || !bitStream)
        return;
    dataBlobStack.emplace_back(dataBlobName, bitStream->GetReadOffset());
    deepestLayer = std::max(deepestLayer, dataBlobStack.size());
}

void NetworkProfiler::endProfiling(const std::string& dataBlobName,
    const PacketBuffer* bitStream)
{
    if (!CanProfile() || !bitStream || dataBlobStack.empty())
        return;

    const DataBlobInfo data = dataBlobStack.back();
    RBXASSERT(dataBlobName == data.name);
    if (dataBlobStack.size() == deepestLayer) {
        std::ostringstream layers;
        for (const DataBlobInfo& layer : dataBlobStack) {
            if (layers.tellp() > 0)
                layers << '.';
            layers << layer.name;
        }
        const BitCount bitSize = bitStream->GetReadOffset() - data.bitStreamOffset;
        StandardOut::singleton()->printf(MESSAGE_INFO,
            "Network profile [%s] offset=%zu bits=%zu path=%s",
            networkSettings->profilerTag.c_str(), data.bitStreamOffset,
            bitSize, layers.str().c_str());
    }
    dataBlobStack.pop_back();
    if (dataBlobStack.empty())
        deepestLayer = 0;
}

void NetworkProfiler::startCpuProfiling(int tag)
{
    if (networkSettings->profilecpu && tag >= 0 && tag < PROFILER_TAG_COUNT)
        cpuProfilingStats[tag].newSample();
}

void NetworkProfiler::stepCpuProfiling(int tag)
{
    if (networkSettings->profilecpu && tag >= 0 && tag < PROFILER_TAG_COUNT)
        cpuProfilingStats[tag].step();
}

void NetworkProfiler::outputCpuProfiling()
{
    for (int tag = 0; tag < PROFILER_TAG_COUNT; ++tag) {
        const int samples = cpuProfilingStats[tag].getNumSample();
        if (samples <= 0)
            continue;
        StandardOut::singleton()->printf(MESSAGE_INFO,
            "Network CPU profile [%d] samples=%d", tag, samples);
        double previous = 0.0;
        for (int step = 0; step < 256 && cpuProfilingStats[tag].stepDelta[step] > 0.0; ++step) {
            const double total = cpuProfilingStats[tag].stepDelta[step];
            StandardOut::singleton()->printf(MESSAGE_INFO,
                "Network CPU profile [%d] step=%d delta=%f total=%f",
                tag, step + 1, total - previous, total);
            previous = total;
        }
    }
}

} // namespace RBX::Network

#endif

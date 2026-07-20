#include "util/MemoryStats.h"

#include <fstream>

#include <sys/sysinfo.h>
#include <unistd.h>

namespace RBX::MemoryStats {

memsize_t usedMemoryBytes()
{
    std::ifstream status("/proc/self/statm");
    memsize_t totalPages = 0;
    memsize_t residentPages = 0;
    if (!(status >> totalPages >> residentPages))
        return 0;
    return residentPages * static_cast<memsize_t>(sysconf(_SC_PAGESIZE));
}

memsize_t freeMemoryBytes()
{
    struct sysinfo info {};
    if (sysinfo(&info) != 0)
        return 0;
    return static_cast<memsize_t>(info.freeram + info.bufferram) * info.mem_unit;
}

memsize_t totalMemoryBytes()
{
    struct sysinfo info {};
    if (sysinfo(&info) != 0)
        return 0;
    return static_cast<memsize_t>(info.totalram) * info.mem_unit;
}

} // namespace RBX::MemoryStats

#include "v8datamodel/HeapProfilerService.h"

namespace RBX {

const char* const sHeapProfilerService = "HeapProfilerService";

HeapProfilerService::HeapProfilerService()
    : Service(true)
{
    setName(sHeapProfilerService);
    setRobloxLocked(true);
}

} // namespace RBX

#pragma once

#include "v8tree/Service.h"

namespace RBX {

extern const char* const sHeapProfilerService;

class HeapProfilerService
    : public DescribedNonCreatable<HeapProfilerService, Instance, sHeapProfilerService>
    , public Service
{
public:
    HeapProfilerService();
};

} // namespace RBX

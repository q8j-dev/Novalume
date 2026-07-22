#pragma once

#include "v8tree/Service.h"

namespace RBX {

extern const char* const sScriptProfilerService;

class ScriptProfilerService
    : public DescribedNonCreatable<ScriptProfilerService, Instance, sScriptProfilerService>
    , public Service
{
public:
    ScriptProfilerService();

    rbx::signal<void(shared_ptr<Instance>, std::string)> onNewDataSignal;
};

} // namespace RBX

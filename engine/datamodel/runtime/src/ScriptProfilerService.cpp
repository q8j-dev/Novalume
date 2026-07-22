#include "v8datamodel/ScriptProfilerService.h"

namespace RBX {

const char* const sScriptProfilerService = "ScriptProfilerService";

REFLECTION_BEGIN();
static Reflection::EventDesc<ScriptProfilerService,
    void(shared_ptr<Instance>, std::string)> eventOnNewData(
        &ScriptProfilerService::onNewDataSignal, "OnNewData", "player", "data",
        Security::Plugin);
REFLECTION_END();

ScriptProfilerService::ScriptProfilerService()
    : Service(true)
{
    setName(sScriptProfilerService);
    setRobloxLocked(true);
}

} // namespace RBX

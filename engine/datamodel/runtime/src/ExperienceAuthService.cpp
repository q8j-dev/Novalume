#include "V8DataModel/ExperienceAuthService.h"

namespace RBX {

const char* const sExperienceAuthService = "ExperienceAuthService";

REFLECTION_BEGIN();
static Reflection::EventDesc<ExperienceAuthService,
    void(std::string, shared_ptr<const Reflection::ValueArray>,
        shared_ptr<const Reflection::ValueTable>)> eventOpenAuthPrompt(
            &ExperienceAuthService::openAuthPromptSignal, "OpenAuthPrompt",
            "guid", "scopes", "metadata", Security::RobloxScript);
static Reflection::BoundFuncDesc<ExperienceAuthService,
    void(std::string, shared_ptr<const Reflection::ValueArray>,
        Enums::ScopeCheckResult, shared_ptr<const Reflection::ValueTable>)>
    funcScopeCheckUIComplete(&ExperienceAuthService::scopeCheckUIComplete,
        "ScopeCheckUIComplete", "guid", "scopes", "result", "metadata",
        Security::RobloxScript);
REFLECTION_END();

ExperienceAuthService::ExperienceAuthService()
{
    setName(sExperienceAuthService);
    setRobloxLocked(true);
}

void ExperienceAuthService::scopeCheckUIComplete(std::string guid,
    shared_ptr<const Reflection::ValueArray> scopes,
    Enums::ScopeCheckResult result,
    shared_ptr<const Reflection::ValueTable> metadata)
{
    if (guid.empty() || !scopes)
        throw std::runtime_error("ScopeCheckUIComplete requires a guid and scopes");
    if (!metadata)
        metadata.reset(new Reflection::ValueTable());
    scopeCheckUICompletedSignal(guid, scopes, result, metadata);
}

void ExperienceAuthService::openAuthPrompt(std::string guid,
    shared_ptr<const Reflection::ValueArray> scopes,
    shared_ptr<const Reflection::ValueTable> metadata)
{
    if (guid.empty() || !scopes)
        throw std::runtime_error("OpenAuthPrompt requires a guid and scopes");
    if (!metadata)
        metadata.reset(new Reflection::ValueTable());
    openAuthPromptSignal(guid, scopes, metadata);
}

}

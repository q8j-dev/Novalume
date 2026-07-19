#pragma once

#include "V8Tree/Service.h"
#include "V8DataModel/InteractionEnums.h"

namespace RBX {

extern const char* const sExperienceAuthService;

class ExperienceAuthService
    : public DescribedNonCreatable<ExperienceAuthService, Instance, sExperienceAuthService>
    , public Service
{
public:
    ExperienceAuthService();

    rbx::signal<void(std::string, shared_ptr<const Reflection::ValueArray>,
        shared_ptr<const Reflection::ValueTable>)> openAuthPromptSignal;
    rbx::signal<void(std::string, shared_ptr<const Reflection::ValueArray>,
        Enums::ScopeCheckResult, shared_ptr<const Reflection::ValueTable>)>
        scopeCheckUICompletedSignal;

    void scopeCheckUIComplete(std::string guid,
        shared_ptr<const Reflection::ValueArray> scopes,
        Enums::ScopeCheckResult result,
        shared_ptr<const Reflection::ValueTable> metadata);
    void openAuthPrompt(std::string guid,
        shared_ptr<const Reflection::ValueArray> scopes,
        shared_ptr<const Reflection::ValueTable> metadata);
};

}

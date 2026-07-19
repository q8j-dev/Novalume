#include "V8DataModel/ExperienceNotificationService.h"

#include "Network/Players.h"

#include <stdexcept>

namespace RBX {

const char* const sExperienceNotificationService = "ExperienceNotificationService";

REFLECTION_BEGIN();
static Reflection::BoundYieldFuncDesc<ExperienceNotificationService, bool()>
    funcCanPromptOptInAsync(
        &ExperienceNotificationService::canPromptOptInAsync,
        "CanPromptOptInAsync", Security::None);
static Reflection::BoundFuncDesc<ExperienceNotificationService, void()>
    funcPromptOptIn(&ExperienceNotificationService::promptOptIn,
        "PromptOptIn", Security::None);
static Reflection::BoundFuncDesc<ExperienceNotificationService, void()>
    funcInvokeOptInPromptClosed(
        &ExperienceNotificationService::invokeOptInPromptClosed,
        "InvokeOptInPromptClosed", Security::RobloxScript);
static Reflection::EventDesc<ExperienceNotificationService, void()>
    eventPromptOptInRequested(
        &ExperienceNotificationService::promptOptInRequestedSignal,
        "PromptOptInRequested", Security::RobloxScript);
REFLECTION_END();

ExperienceNotificationService::ExperienceNotificationService()
    : Service(true)
    , eligibilityInitialized(false)
    , canPromptOptIn(false)
    , promptPending(false)
{
    setName(sExperienceNotificationService);
    setRobloxLocked(true);
}

void ExperienceNotificationService::requireFrontend(const char* operation) const
{
    if (!Network::Players::frontendProcessing(this))
        throw std::runtime_error(std::string("ExperienceNotificationService:") +
            operation + " can only be executed by a local script");
}

void ExperienceNotificationService::initializePromptEligibility(bool eligible)
{
    eligibilityInitialized = true;
    canPromptOptIn = eligible;
    if (!eligible)
        promptPending = false;
}

void ExperienceNotificationService::canPromptOptInAsync(
    boost::function<void(bool)> resumeFunction,
    boost::function<void(std::string)> errorFunction)
{
    try
    {
        requireFrontend("CanPromptOptInAsync");
        if (!eligibilityInitialized)
            throw std::runtime_error(
                "ExperienceNotificationService:CanPromptOptInAsync "
                "ExperienceNotificationService is not initialized.");
        resumeFunction(canPromptOptIn);
    }
    catch (const std::exception& error)
    {
        errorFunction(error.what());
    }
}

void ExperienceNotificationService::promptOptIn()
{
    requireFrontend("PromptOptIn");
    if (!eligibilityInitialized)
        throw std::runtime_error(
            "ExperienceNotificationService:PromptOptIn "
            "ExperienceNotificationService is not initialized.");
    if (!canPromptOptIn)
        throw std::runtime_error(
            "The player cannot be prompted. Please check "
            "ExperienceNotificationService:CanPromptOptIn first before calling "
            "ExperienceNotificationService:PromptOptIn");
    if (promptPending)
        return;

    promptPending = true;
    promptOptInRequestedSignal();
}

void ExperienceNotificationService::invokeOptInPromptClosed()
{
    requireFrontend("InvokeOptInPromptClosed");
    if (!eligibilityInitialized)
        throw std::runtime_error(
            "ExperienceNotificationService:InvokeOptInPromptClosed "
            "ExperienceNotificationService is not initialized.");
    if (!promptPending)
        return;

    promptPending = false;
    optInPromptClosedSignal();
}

} // namespace RBX

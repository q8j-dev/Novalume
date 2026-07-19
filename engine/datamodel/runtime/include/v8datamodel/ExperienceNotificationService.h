#pragma once

#include "V8Tree/Service.h"

#include "rbx/signal.h"

#include <string>

namespace RBX {

extern const char* const sExperienceNotificationService;

class ExperienceNotificationService
    : public DescribedNonCreatable<ExperienceNotificationService, Instance,
          sExperienceNotificationService>
    , public Service
{
public:
    ExperienceNotificationService();

    void canPromptOptInAsync(boost::function<void(bool)> resumeFunction,
        boost::function<void(std::string)> errorFunction);
    void promptOptIn();
    void invokeOptInPromptClosed();

    void initializePromptEligibility(bool eligible);
    bool getPromptEligibilityInitialized() const { return eligibilityInitialized; }
    bool getCanPromptOptIn() const { return canPromptOptIn; }
    bool getPromptPending() const { return promptPending; }

    rbx::signal<void()> promptOptInRequestedSignal;
    rbx::signal<void()> optInPromptClosedSignal;

private:
    void requireFrontend(const char* operation) const;

    bool eligibilityInitialized;
    bool canPromptOptIn;
    bool promptPending;
};

} // namespace RBX

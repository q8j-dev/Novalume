#pragma once

#include "V8Tree/Service.h"
#include "rbx/signal.h"

namespace RBX {

class InputObject;
class SelectionBox;

extern const char* const sExperienceStateCaptureService;

class ExperienceStateCaptureService
    : public DescribedNonCreatable<ExperienceStateCaptureService, Instance,
          sExperienceStateCaptureService>
    , public Service
{
public:
    typedef DescribedNonCreatable<ExperienceStateCaptureService, Instance,
        sExperienceStateCaptureService> Super;

    enum SelectionMode
    {
        SELECTION_MODE_DEFAULT = 0,
        SELECTION_MODE_SAFETY_HIGHLIGHT = 1
    };

    ExperienceStateCaptureService();

    bool getHiddenSelectionEnabled() const { return hiddenSelectionEnabled; }
    void setHiddenSelectionEnabled(bool value);
    bool getIsInBackground() const { return isInBackground; }
    bool getIsInCaptureMode() const { return isInCaptureMode; }
    SelectionMode getSelectionMode() const { return selectionMode; }
    void setSelectionMode(SelectionMode value);

    bool canEnterCaptureMode();
    void resetHighlight();
    void toggleCaptureMode();

    rbx::signal<void(shared_ptr<Instance>)> itemSelectedInCaptureModeSignal;

protected:
    void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider) override;

private:
    void setCaptureState(bool active);
    void onInputBegan(shared_ptr<Instance> input, bool gameProcessed);

    bool hiddenSelectionEnabled;
    bool isInBackground;
    bool isInCaptureMode;
    SelectionMode selectionMode;
    shared_ptr<SelectionBox> selectionBox;
    rbx::signals::scoped_connection inputBeganConnection;
};

} // namespace RBX

namespace RBX { namespace Reflection {
template<> EnumDesc<RBX::ExperienceStateCaptureService::SelectionMode>::EnumDesc();
} }

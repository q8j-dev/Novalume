#include "v8datamodel/ExperienceStateCaptureService.h"

#include "v8datamodel/DataModel.h"
#include "v8datamodel/InputObject.h"
#include "v8datamodel/Mouse.h"
#include "v8datamodel/PartInstance.h"
#include "v8datamodel/SelectionBox.h"
#include "v8datamodel/UserInputService.h"
#include "network/Player.h"
#include "network/Players.h"

namespace RBX {

const char* const sExperienceStateCaptureService = "ExperienceStateCaptureService";

REFLECTION_BEGIN();
static Reflection::PropDescriptor<ExperienceStateCaptureService, bool> propHiddenSelectionEnabled(
    "HiddenSelectionEnabled", category_Data,
    &ExperienceStateCaptureService::getHiddenSelectionEnabled,
    &ExperienceStateCaptureService::setHiddenSelectionEnabled,
    Reflection::PropertyDescriptor::HIDDEN_SCRIPTING, Security::RobloxScript);
static Reflection::PropDescriptor<ExperienceStateCaptureService, bool> propIsInBackground(
    "IsInBackground", category_Data,
    &ExperienceStateCaptureService::getIsInBackground, NULL,
    Reflection::PropertyDescriptor::HIDDEN_SCRIPTING, Security::RobloxScript);
static Reflection::PropDescriptor<ExperienceStateCaptureService, bool> propIsInCaptureMode(
    "IsInCaptureMode", category_Data,
    &ExperienceStateCaptureService::getIsInCaptureMode, NULL,
    Reflection::PropertyDescriptor::HIDDEN_SCRIPTING, Security::RobloxScript);
static Reflection::EnumPropDescriptor<ExperienceStateCaptureService,
    ExperienceStateCaptureService::SelectionMode> propSelectionMode(
        "SelectionMode", category_Data,
        &ExperienceStateCaptureService::getSelectionMode,
        &ExperienceStateCaptureService::setSelectionMode,
        Reflection::PropertyDescriptor::HIDDEN_SCRIPTING, Security::RobloxScript);
static Reflection::BoundFuncDesc<ExperienceStateCaptureService, bool()> funcCanEnterCaptureMode(
    &ExperienceStateCaptureService::canEnterCaptureMode, "CanEnterCaptureMode",
    Security::RobloxScript);
static Reflection::BoundFuncDesc<ExperienceStateCaptureService, void()> funcResetHighlight(
    &ExperienceStateCaptureService::resetHighlight, "ResetHighlight",
    Security::RobloxScript);
static Reflection::BoundFuncDesc<ExperienceStateCaptureService, void()> funcToggleCaptureMode(
    &ExperienceStateCaptureService::toggleCaptureMode, "ToggleCaptureMode",
    Security::RobloxScript);
static Reflection::EventDesc<ExperienceStateCaptureService, void(shared_ptr<Instance>)>
    eventItemSelectedInCaptureMode(
        &ExperienceStateCaptureService::itemSelectedInCaptureModeSignal,
        "ItemSelectedInCaptureMode", "instance", Security::RobloxScript);
REFLECTION_END();

namespace Reflection {
template<>
EnumDesc<ExperienceStateCaptureService::SelectionMode>::EnumDesc()
    : EnumDescriptor("ExperienceStateCaptureSelectionMode")
{
    addPair(ExperienceStateCaptureService::SELECTION_MODE_DEFAULT, "Default");
    addPair(ExperienceStateCaptureService::SELECTION_MODE_SAFETY_HIGHLIGHT,
        "SafetyHighlightMode");
}
}

ExperienceStateCaptureService::ExperienceStateCaptureService()
    : Service(true)
    , hiddenSelectionEnabled(false)
    , isInBackground(false)
    , isInCaptureMode(false)
    , selectionMode(SELECTION_MODE_DEFAULT)
{
    setName(sExperienceStateCaptureService);
    setRobloxLocked(true);
}

void ExperienceStateCaptureService::onServiceProvider(ServiceProvider* oldProvider,
    ServiceProvider* newProvider)
{
    inputBeganConnection.disconnect();
    resetHighlight();
    setCaptureState(false);
    Super::onServiceProvider(oldProvider, newProvider);

    if (newProvider)
    {
        UserInputService* input = ServiceProvider::create<UserInputService>(newProvider);
        inputBeganConnection = input->coreInputBeganEvent.connect(
            boost::bind(&ExperienceStateCaptureService::onInputBegan, this, _1, _2));
    }
}

void ExperienceStateCaptureService::setHiddenSelectionEnabled(bool value)
{
    if (hiddenSelectionEnabled == value)
        return;
    hiddenSelectionEnabled = value;
    if (selectionBox)
        selectionBox->setVisible(!value);
    raisePropertyChanged(propHiddenSelectionEnabled);
}

void ExperienceStateCaptureService::setSelectionMode(SelectionMode value)
{
    if (selectionMode == value)
        return;
    selectionMode = value;
    resetHighlight();
    raisePropertyChanged(propSelectionMode);
}

bool ExperienceStateCaptureService::canEnterCaptureMode()
{
    return DataModel::get(this) != NULL && Network::Players::findLocalPlayer(this) != NULL;
}

void ExperienceStateCaptureService::setCaptureState(bool active)
{
    if (isInCaptureMode != active)
    {
        isInCaptureMode = active;
        raisePropertyChanged(propIsInCaptureMode);
    }
    if (isInBackground != active)
    {
        isInBackground = active;
        raisePropertyChanged(propIsInBackground);
    }
    if (!active)
        resetHighlight();
}

void ExperienceStateCaptureService::toggleCaptureMode()
{
    if (!isInCaptureMode && !canEnterCaptureMode())
        return;
    setCaptureState(!isInCaptureMode);
}

void ExperienceStateCaptureService::resetHighlight()
{
    if (selectionBox)
        selectionBox->setAdornee(NULL);
}

void ExperienceStateCaptureService::onInputBegan(shared_ptr<Instance> input,
    bool gameProcessed)
{
    if (!isInCaptureMode || gameProcessed)
        return;

    InputObject* inputObject = Instance::fastDynamicCast<InputObject>(input.get());
    if (!inputObject || (inputObject->getUserInputType() != InputObject::TYPE_MOUSEBUTTON1
        && inputObject->getUserInputType() != InputObject::TYPE_TOUCH))
        return;

    Network::Player* player = Network::Players::findLocalPlayer(this);
    shared_ptr<Mouse> mouse = player ? player->getMouse() : shared_ptr<Mouse>();
    PartInstance* target = mouse ? mouse->getTarget() : NULL;
    if (!target)
        return;

    if (!selectionBox)
    {
        selectionBox = Creatable<Instance>::create<SelectionBox>();
        selectionBox->setName("ExperienceStateCaptureHighlight");
        selectionBox->setColor(Color3(0.0f, 0.635f, 1.0f));
        selectionBox->setSurfaceColor(Color3(0.0f, 0.635f, 1.0f));
        selectionBox->setTransparency(0.0f);
        selectionBox->setSurfaceTransparency(0.75f);
        selectionBox->setLineThickness(
            selectionMode == SELECTION_MODE_SAFETY_HIGHLIGHT ? 0.08f : 0.15f);
        selectionBox->setParent(this);
    }
    selectionBox->setVisible(!hiddenSelectionEnabled);
    selectionBox->setAdornee(target);

    shared_ptr<Instance> selected = shared_from(static_cast<Instance*>(target));
    setCaptureState(false);
    itemSelectedInCaptureModeSignal(selected);
}

} // namespace RBX

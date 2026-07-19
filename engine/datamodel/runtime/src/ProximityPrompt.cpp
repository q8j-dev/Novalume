#include "V8DataModel/ProximityPrompt.h"

#include "Network/Player.h"
#include "Network/Players.h"
#include "Util/RunStateOwner.h"
#include "V8DataModel/Attachment.h"
#include "V8DataModel/DataModel.h"
#include "V8DataModel/ModelInstance.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/Raycast.h"
#include "V8DataModel/UserInputService.h"
#include "V8DataModel/Workspace.h"

#include <algorithm>
#include <limits>

namespace RBX {

const char* const sProximityPrompt = "ProximityPrompt";
const char* const sProximityPromptService = "ProximityPromptService";

REFLECTION_BEGIN();
static Reflection::PropDescriptor<ProximityPrompt, std::string> propActionText(
    "ActionText", category_Data, &ProximityPrompt::getActionText, &ProximityPrompt::setActionText);
static Reflection::PropDescriptor<ProximityPrompt, std::string> propObjectText(
    "ObjectText", category_Data, &ProximityPrompt::getObjectText, &ProximityPrompt::setObjectText);
static Reflection::PropDescriptor<ProximityPrompt, bool> propPromptEnabled(
    "Enabled", category_Data, &ProximityPrompt::getEnabled, &ProximityPrompt::setEnabled);
static Reflection::PropDescriptor<ProximityPrompt, bool> propClickablePrompt(
    "ClickablePrompt", category_Data, &ProximityPrompt::getClickablePrompt, &ProximityPrompt::setClickablePrompt);
static Reflection::PropDescriptor<ProximityPrompt, bool> propRequiresLineOfSight(
    "RequiresLineOfSight", category_Data, &ProximityPrompt::getRequiresLineOfSight, &ProximityPrompt::setRequiresLineOfSight);
static Reflection::PropDescriptor<ProximityPrompt, bool> propAutoLocalize(
    "AutoLocalize", category_Data, &ProximityPrompt::getAutoLocalize, &ProximityPrompt::setAutoLocalize);
static Reflection::PropDescriptor<ProximityPrompt, float> propHoldDuration(
    "HoldDuration", category_Data, &ProximityPrompt::getHoldDuration, &ProximityPrompt::setHoldDuration);
static Reflection::PropDescriptor<ProximityPrompt, float> propMaxActivationDistance(
    "MaxActivationDistance", category_Data, &ProximityPrompt::getMaxActivationDistance, &ProximityPrompt::setMaxActivationDistance);
static Reflection::PropDescriptor<ProximityPrompt, float> propMaxIndicatorDistance(
    "MaxIndicatorDistance", category_Data, &ProximityPrompt::getMaxIndicatorDistance, &ProximityPrompt::setMaxIndicatorDistance);
static Reflection::EnumPropDescriptor<ProximityPrompt, KeyCode> propKeyboardKeyCode(
    "KeyboardKeyCode", category_Data, &ProximityPrompt::getKeyboardKeyCode, &ProximityPrompt::setKeyboardKeyCode);
static Reflection::EnumPropDescriptor<ProximityPrompt, KeyCode> propGamepadKeyCode(
    "GamepadKeyCode", category_Data, &ProximityPrompt::getGamepadKeyCode, &ProximityPrompt::setGamepadKeyCode);
static Reflection::EnumPropDescriptor<ProximityPrompt, ProximityPrompt::Style> propPromptStyle(
    "Style", category_Data, &ProximityPrompt::getStyle, &ProximityPrompt::setStyle);
static Reflection::EnumPropDescriptor<ProximityPrompt, ProximityPrompt::Exclusivity> propPromptExclusivity(
    "Exclusivity", category_Data, &ProximityPrompt::getExclusivity, &ProximityPrompt::setExclusivity);
static Reflection::PropDescriptor<ProximityPrompt, Vector2> propUIOffset(
    "UIOffset", category_Data, &ProximityPrompt::getUIOffset, &ProximityPrompt::setUIOffset);
static Reflection::EventDesc<ProximityPrompt, void(shared_ptr<Instance>)> eventTriggered(
    &ProximityPrompt::triggeredSignal, "Triggered", "playerWhoTriggered", Security::None);
static Reflection::EventDesc<ProximityPrompt, void(shared_ptr<Instance>)> eventTriggerEnded(
    &ProximityPrompt::triggerEndedSignal, "TriggerEnded", "playerWhoTriggered", Security::None);
static Reflection::EventDesc<ProximityPrompt, void(InputObject::UserInputType)> eventPromptShown(
    &ProximityPrompt::promptShownSignal, "PromptShown", "inputType", Security::None);
static Reflection::EventDesc<ProximityPrompt, void()> eventPromptHidden(
    &ProximityPrompt::promptHiddenSignal, "PromptHidden", Security::None);
static Reflection::EventDesc<ProximityPrompt, void()> eventPromptButtonHoldBegan(
    &ProximityPrompt::promptButtonHoldBeganSignal, "PromptButtonHoldBegan", Security::None);
static Reflection::EventDesc<ProximityPrompt, void()> eventPromptButtonHoldEnded(
    &ProximityPrompt::promptButtonHoldEndedSignal, "PromptButtonHoldEnded", Security::None);

static Reflection::PropDescriptor<ProximityPromptService, bool> propServiceEnabled(
    "Enabled", category_Data, &ProximityPromptService::getEnabled, &ProximityPromptService::setEnabled);
static Reflection::PropDescriptor<ProximityPromptService, int> propMaxPromptsVisible(
    "MaxPromptsVisible", category_Data, &ProximityPromptService::getMaxPromptsVisible, &ProximityPromptService::setMaxPromptsVisible);
static Reflection::PropDescriptor<ProximityPromptService, int> propMaxIndicatorsVisible(
    "MaxIndicatorsVisible", category_Data, &ProximityPromptService::getMaxIndicatorsVisible, &ProximityPromptService::setMaxIndicatorsVisible);
static Reflection::EventDesc<ProximityPromptService, void(shared_ptr<Instance>, InputObject::UserInputType)> eventServicePromptShown(
    &ProximityPromptService::promptShownSignal, "PromptShown", "prompt", "inputType", Security::None);
static Reflection::EventDesc<ProximityPromptService, void(shared_ptr<Instance>)> eventServicePromptHidden(
    &ProximityPromptService::promptHiddenSignal, "PromptHidden", "prompt", Security::None);
static Reflection::EventDesc<ProximityPromptService, void(shared_ptr<Instance>, shared_ptr<Instance>)> eventServicePromptTriggered(
    &ProximityPromptService::promptTriggeredSignal, "PromptTriggered", "prompt", "playerWhoTriggered", Security::None);
static Reflection::EventDesc<ProximityPromptService, void(shared_ptr<Instance>, shared_ptr<Instance>)> eventServicePromptTriggerEnded(
    &ProximityPromptService::promptTriggerEndedSignal, "PromptTriggerEnded", "prompt", "playerWhoTriggered", Security::None);
static Reflection::EventDesc<ProximityPromptService, void(shared_ptr<Instance>, shared_ptr<Instance>)> eventServicePromptButtonHoldBegan(
    &ProximityPromptService::promptButtonHoldBeganSignal, "PromptButtonHoldBegan", "prompt", "playerWhoTriggered", Security::None);
static Reflection::EventDesc<ProximityPromptService, void(shared_ptr<Instance>, shared_ptr<Instance>)> eventServicePromptButtonHoldEnded(
    &ProximityPromptService::promptButtonHoldEndedSignal, "PromptButtonHoldEnded", "prompt", "playerWhoTriggered", Security::None);
REFLECTION_END();

namespace Reflection {
template<> EnumDesc<ProximityPrompt::Style>::EnumDesc() : EnumDescriptor("ProximityPromptStyle")
{
    addPair(ProximityPrompt::STYLE_DEFAULT, "Default");
    addPair(ProximityPrompt::STYLE_CUSTOM, "Custom");
}
template<> EnumDesc<ProximityPrompt::Exclusivity>::EnumDesc() : EnumDescriptor("ProximityPromptExclusivity")
{
    addPair(ProximityPrompt::EXCLUSIVITY_ONE_PER_BUTTON, "OnePerButton");
    addPair(ProximityPrompt::EXCLUSIVITY_ONE_GLOBALLY, "OneGlobally");
    addPair(ProximityPrompt::EXCLUSIVITY_ALWAYS_SHOW, "AlwaysShow");
}
}

ProximityPrompt::ProximityPrompt()
    : actionText("Interact"), enabled(true), clickablePrompt(true), requiresLineOfSight(true)
    , autoLocalize(true), holdDuration(0), maxActivationDistance(10), maxIndicatorDistance(0)
    , keyboardKeyCode(SDLK_e), gamepadKeyCode(SDLK_UNKNOWN), style(STYLE_DEFAULT)
    , exclusivity(EXCLUSIVITY_ONE_PER_BUTTON), uiOffset(Vector2::zero())
{
    setName(sProximityPrompt);
}

bool ProximityPrompt::askSetParent(const Instance* parent) const
{
    return parent == nullptr || Instance::fastDynamicCast<PartInstance>(parent) ||
        Instance::fastDynamicCast<Attachment>(parent) ||
        Instance::fastDynamicCast<ModelInstance>(parent);
}

#define RBX_PROMPT_SETTER(Type, Name, Field, Prop) \
void ProximityPrompt::set##Name(Type value) { if (Field == value) return; Field = value; raisePropertyChanged(Prop); }
RBX_PROMPT_SETTER(bool, Enabled, enabled, propPromptEnabled)
RBX_PROMPT_SETTER(bool, ClickablePrompt, clickablePrompt, propClickablePrompt)
RBX_PROMPT_SETTER(bool, RequiresLineOfSight, requiresLineOfSight, propRequiresLineOfSight)
RBX_PROMPT_SETTER(bool, AutoLocalize, autoLocalize, propAutoLocalize)
RBX_PROMPT_SETTER(KeyCode, KeyboardKeyCode, keyboardKeyCode, propKeyboardKeyCode)
RBX_PROMPT_SETTER(KeyCode, GamepadKeyCode, gamepadKeyCode, propGamepadKeyCode)
RBX_PROMPT_SETTER(ProximityPrompt::Style, Style, style, propPromptStyle)
RBX_PROMPT_SETTER(ProximityPrompt::Exclusivity, Exclusivity, exclusivity, propPromptExclusivity)
RBX_PROMPT_SETTER(Vector2, UIOffset, uiOffset, propUIOffset)
#undef RBX_PROMPT_SETTER

void ProximityPrompt::setHoldDuration(float value) { value = std::max(0.0f, value); if (holdDuration != value) { holdDuration = value; raisePropertyChanged(propHoldDuration); } }
void ProximityPrompt::setMaxActivationDistance(float value) { value = std::max(0.0f, value); if (maxActivationDistance != value) { maxActivationDistance = value; raisePropertyChanged(propMaxActivationDistance); } }
void ProximityPrompt::setMaxIndicatorDistance(float value) { value = std::max(0.0f, value); if (maxIndicatorDistance != value) { maxIndicatorDistance = value; raisePropertyChanged(propMaxIndicatorDistance); } }
void ProximityPrompt::setActionText(const std::string& value) { if (actionText != value) { actionText = value; raisePropertyChanged(propActionText); } }
void ProximityPrompt::setObjectText(const std::string& value) { if (objectText != value) { objectText = value; raisePropertyChanged(propObjectText); } }

ProximityPromptService::ProximityPromptService()
    : Service(true), enabled(true), maxPromptsVisible(16), maxIndicatorsVisible(16)
    , heldSeconds(0), heldTriggered(false), heldInputType(InputObject::TYPE_NONE)
{
    setName(sProximityPromptService);
}

void ProximityPromptService::setEnabled(bool value) { if (enabled != value) { enabled = value; raisePropertyChanged(propServiceEnabled); } }
void ProximityPromptService::setMaxPromptsVisible(int value) { value = std::max(0, value); if (maxPromptsVisible != value) { maxPromptsVisible = value; raisePropertyChanged(propMaxPromptsVisible); } }
void ProximityPromptService::setMaxIndicatorsVisible(int value) { value = std::max(0, value); if (maxIndicatorsVisible != value) { maxIndicatorsVisible = value; raisePropertyChanged(propMaxIndicatorsVisible); } }

void ProximityPromptService::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
    heartbeatConnection.disconnect();
    inputBeganConnection.disconnect();
    inputEndedConnection.disconnect();
    visiblePrompt.reset();
    heldPrompt.reset();
    Super::onServiceProvider(oldProvider, newProvider);
    if (!newProvider) return;
    RunService* runService = ServiceProvider::create<RunService>(newProvider);
    UserInputService* input = ServiceProvider::create<UserInputService>(newProvider);
    heartbeatConnection = runService->heartbeatSignal.connect(boost::bind(&ProximityPromptService::onHeartbeat, this, _1));
    inputBeganConnection = input->inputBeganEvent.connect(boost::bind(&ProximityPromptService::onInputBegan, this, _1, _2));
    inputEndedConnection = input->inputEndedEvent.connect(boost::bind(&ProximityPromptService::onInputEnded, this, _1, _2));
}

static void collectPrompt(shared_ptr<Instance> instance, std::vector<shared_ptr<ProximityPrompt> >* prompts)
{
    if (shared_ptr<ProximityPrompt> prompt = dynamic_pointer_cast<ProximityPrompt>(instance)) prompts->push_back(prompt);
}

static bool promptWorldPosition(ProximityPrompt& prompt, Vector3* position)
{
    Instance* parent = prompt.getParent();
    if (PartInstance* part = Instance::fastDynamicCast<PartInstance>(parent)) { *position = part->getCoordinateFrame().translation; return true; }
    if (Attachment* attachment = Instance::fastDynamicCast<Attachment>(parent)) { *position = attachment->getPivotInWorld(); return true; }
    if (ModelInstance* model = Instance::fastDynamicCast<ModelInstance>(parent)) {
        if (PartInstance* primary = model->getPrimaryPartSetByUser()) { *position = primary->getCoordinateFrame().translation; return true; }
    }
    return false;
}

shared_ptr<ProximityPrompt> ProximityPromptService::nearestEligiblePrompt()
{
    if (!enabled || maxPromptsVisible <= 0) return shared_ptr<ProximityPrompt>();
    DataModel* dm = DataModel::get(this);
    Network::Player* player = Network::Players::findLocalPlayer(this);
    ModelInstance* character = player ? player->getCharacter() : NULL;
    PartInstance* root = character ? Instance::fastDynamicCast<PartInstance>(character->findFirstChildByName("HumanoidRootPart")) : NULL;
    if (!dm || !root) return shared_ptr<ProximityPrompt>();

    std::vector<shared_ptr<ProximityPrompt> > prompts;
    dm->visitDescendants(boost::bind(&collectPrompt, _1, &prompts));
    shared_ptr<ProximityPrompt> nearest;
    float nearestDistance = std::numeric_limits<float>::max();
    const Vector3 playerPosition = root->getCoordinateFrame().translation;
    Workspace* workspace = dm->getWorkspace();
    for (const shared_ptr<ProximityPrompt>& prompt : prompts) {
        Vector3 position;
        if (!prompt->getEnabled() || !promptWorldPosition(*prompt, &position)) continue;
        const float distance = (position - playerPosition).magnitude();
        if (distance <= prompt->getMaxActivationDistance() &&
            prompt->getRequiresLineOfSight() && workspace)
        {
            RaycastParams params;
            shared_ptr<Instances> excluded(new Instances());
            excluded->push_back(shared_from(character));
            params.filterDescendantsInstances = excluded;
            Reflection::Variant hit = workspace->raycast(
                playerPosition, position - playerPosition, params);
            if (hit.isType<RaycastResult>()) {
                const shared_ptr<Instance> hitInstance = hit.cast<RaycastResult>().instance;
                Instance* promptParent = prompt->getParent();
                if (Instance::fastDynamicCast<Attachment>(promptParent))
                    promptParent = promptParent->getParent();
                if (hitInstance && hitInstance.get() != promptParent &&
                    (!promptParent || !hitInstance->isDescendantOf(promptParent)))
                    continue;
            }
        }
        if (distance <= prompt->getMaxActivationDistance() && distance < nearestDistance) {
            nearest = prompt;
            nearestDistance = distance;
        }
    }
    return nearest;
}

void ProximityPromptService::showPrompt(const shared_ptr<ProximityPrompt>& prompt, InputObject::UserInputType inputType)
{
    visiblePrompt = prompt;
    prompt->promptShownSignal(inputType);
    promptShownSignal(prompt, inputType);
}

void ProximityPromptService::hidePrompt(const shared_ptr<ProximityPrompt>& prompt)
{
    if (!prompt) return;
    prompt->promptHiddenSignal();
    promptHiddenSignal(prompt);
    visiblePrompt.reset();
}

void ProximityPromptService::onHeartbeat(const Heartbeat& heartbeat)
{
    shared_ptr<ProximityPrompt> next = nearestEligiblePrompt();
    shared_ptr<ProximityPrompt> current = visiblePrompt.lock();
    if (current != next) { hidePrompt(current); if (next) showPrompt(next, InputObject::TYPE_KEYBOARD); }

    if (shared_ptr<ProximityPrompt> held = heldPrompt.lock()) {
        if (held != next) { onInputEnded(shared_ptr<Instance>(), false); return; }
        heldSeconds += heartbeat.gameStep;
        if (!heldTriggered && heldSeconds >= held->getHoldDuration()) {
            triggerPrompt(held);
            heldTriggered = true;
        }
    }
}

bool ProximityPromptService::inputMatches(const ProximityPrompt& prompt, const InputObject& input) const
{
    if (input.getUserInputType() == InputObject::TYPE_KEYBOARD) return input.getKeyCode() == prompt.getKeyboardKeyCode();
    if (input.getUserInputType() >= InputObject::TYPE_GAMEPAD1 && input.getUserInputType() <= InputObject::TYPE_GAMEPAD8)
        return input.getKeyCode() == prompt.getGamepadKeyCode();
    return prompt.getClickablePrompt() && input.getUserInputType() == InputObject::TYPE_MOUSEBUTTON1;
}

void ProximityPromptService::onInputBegan(shared_ptr<Instance> instance, bool processed)
{
    if (processed || heldPrompt.lock()) return;
    shared_ptr<InputObject> input = dynamic_pointer_cast<InputObject>(instance);
    shared_ptr<ProximityPrompt> prompt = visiblePrompt.lock();
    if (!input || !prompt || !inputMatches(*prompt, *input)) return;
    if (prompt->getHoldDuration() <= 0) { triggerPrompt(prompt); return; }
    heldPrompt = prompt;
    heldSeconds = 0;
    heldTriggered = false;
    heldInputType = input->getUserInputType();
    shared_ptr<Instance> player = shared_from(Network::Players::findLocalPlayer(this));
    prompt->promptButtonHoldBeganSignal();
    promptButtonHoldBeganSignal(prompt, player);
}

void ProximityPromptService::onInputEnded(shared_ptr<Instance> instance, bool)
{
    shared_ptr<ProximityPrompt> prompt = heldPrompt.lock();
    if (!prompt) return;
    shared_ptr<InputObject> input = dynamic_pointer_cast<InputObject>(instance);
    if (input && !inputMatches(*prompt, *input)) return;
    shared_ptr<Instance> player = shared_from(Network::Players::findLocalPlayer(this));
    prompt->promptButtonHoldEndedSignal();
    promptButtonHoldEndedSignal(prompt, player);
    heldPrompt.reset();
    heldSeconds = 0;
    heldTriggered = false;
}

void ProximityPromptService::triggerPrompt(const shared_ptr<ProximityPrompt>& prompt)
{
    shared_ptr<Instance> player = shared_from(Network::Players::findLocalPlayer(this));
    prompt->triggeredSignal(player);
    promptTriggeredSignal(prompt, player);
    prompt->triggerEndedSignal(player);
    promptTriggerEndedSignal(prompt, player);
}

} // namespace RBX

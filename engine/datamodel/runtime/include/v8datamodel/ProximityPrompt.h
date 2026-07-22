#pragma once

#include "v8tree/Service.h"
#include "v8datamodel/InputObject.h"
#include "util/KeyCode.h"
#include "util/UDim.h"

namespace RBX {

class Heartbeat;
namespace Network { class Player; }

extern const char* const sProximityPrompt;
extern const char* const sProximityPromptService;

class ProximityPrompt
    : public DescribedCreatable<ProximityPrompt, Instance, sProximityPrompt>
{
public:
    enum Style { STYLE_DEFAULT = 0, STYLE_CUSTOM = 1 };
    enum Exclusivity { EXCLUSIVITY_ONE_PER_BUTTON = 0, EXCLUSIVITY_ONE_GLOBALLY = 1, EXCLUSIVITY_ALWAYS_SHOW = 2 };

private:
    typedef DescribedCreatable<ProximityPrompt, Instance, sProximityPrompt> Super;
    std::string actionText;
    std::string objectText;
    bool enabled;
    bool clickablePrompt;
    bool requiresLineOfSight;
    bool autoLocalize;
    float holdDuration;
    float maxActivationDistance;
    float maxIndicatorDistance;
    KeyCode keyboardKeyCode;
    KeyCode gamepadKeyCode;
    Style style;
    Exclusivity exclusivity;
    Vector2 uiOffset;

public:
    ProximityPrompt();
    bool askSetParent(const Instance* parent) const override;

    bool getEnabled() const { return enabled; }
    void setEnabled(bool value);
    bool getClickablePrompt() const { return clickablePrompt; }
    void setClickablePrompt(bool value);
    bool getRequiresLineOfSight() const { return requiresLineOfSight; }
    void setRequiresLineOfSight(bool value);
    bool getAutoLocalize() const { return autoLocalize; }
    void setAutoLocalize(bool value);
    float getHoldDuration() const { return holdDuration; }
    void setHoldDuration(float value);
    float getMaxActivationDistance() const { return maxActivationDistance; }
    void setMaxActivationDistance(float value);
    float getMaxIndicatorDistance() const { return maxIndicatorDistance; }
    void setMaxIndicatorDistance(float value);
    const std::string& getActionText() const { return actionText; }
    void setActionText(const std::string& value);
    const std::string& getObjectText() const { return objectText; }
    void setObjectText(const std::string& value);
    KeyCode getKeyboardKeyCode() const { return keyboardKeyCode; }
    void setKeyboardKeyCode(KeyCode value);
    KeyCode getGamepadKeyCode() const { return gamepadKeyCode; }
    void setGamepadKeyCode(KeyCode value);
    Style getStyle() const { return style; }
    void setStyle(Style value);
    Exclusivity getExclusivity() const { return exclusivity; }
    void setExclusivity(Exclusivity value);
    Vector2 getUIOffset() const { return uiOffset; }
    void setUIOffset(Vector2 value);

    rbx::signal<void(shared_ptr<Instance>)> triggeredSignal;
    rbx::signal<void(shared_ptr<Instance>)> triggerEndedSignal;
    rbx::signal<void(InputObject::UserInputType)> promptShownSignal;
    rbx::signal<void()> promptHiddenSignal;
    rbx::signal<void()> promptButtonHoldBeganSignal;
    rbx::signal<void()> promptButtonHoldEndedSignal;
};

class ProximityPromptService
    : public DescribedCreatable<ProximityPromptService, Instance, sProximityPromptService,
          Reflection::ClassDescriptor::INTERNAL>
    , public Service
{
private:
    typedef DescribedCreatable<ProximityPromptService, Instance, sProximityPromptService,
        Reflection::ClassDescriptor::INTERNAL> Super;
    bool enabled;
    int maxPromptsVisible;
    int maxIndicatorsVisible;
    weak_ptr<ProximityPrompt> visiblePrompt;
    weak_ptr<ProximityPrompt> heldPrompt;
    double heldSeconds;
    bool heldTriggered;
    InputObject::UserInputType heldInputType;
    rbx::signals::scoped_connection heartbeatConnection;
    rbx::signals::scoped_connection inputBeganConnection;
    rbx::signals::scoped_connection inputEndedConnection;

    void onHeartbeat(const Heartbeat& heartbeat);
    void onInputBegan(shared_ptr<Instance> input, bool processed);
    void onInputEnded(shared_ptr<Instance> input, bool processed);
    void showPrompt(const shared_ptr<ProximityPrompt>& prompt, InputObject::UserInputType inputType);
    void hidePrompt(const shared_ptr<ProximityPrompt>& prompt);
    void triggerPrompt(const shared_ptr<ProximityPrompt>& prompt);
    bool inputMatches(const ProximityPrompt& prompt, const InputObject& input) const;
    shared_ptr<ProximityPrompt> nearestEligiblePrompt();

protected:
    void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider) override;

public:
    ProximityPromptService();
    bool getEnabled() const { return enabled; }
    void setEnabled(bool value);
    int getMaxPromptsVisible() const { return maxPromptsVisible; }
    void setMaxPromptsVisible(int value);
    int getMaxIndicatorsVisible() const { return maxIndicatorsVisible; }
    void setMaxIndicatorsVisible(int value);

    rbx::signal<void(shared_ptr<Instance>, InputObject::UserInputType)> promptShownSignal;
    rbx::signal<void(shared_ptr<Instance>)> promptHiddenSignal;
    rbx::signal<void(shared_ptr<Instance>, shared_ptr<Instance>)> promptTriggeredSignal;
    rbx::signal<void(shared_ptr<Instance>, shared_ptr<Instance>)> promptTriggerEndedSignal;
    rbx::signal<void(shared_ptr<Instance>, shared_ptr<Instance>)> promptButtonHoldBeganSignal;
    rbx::signal<void(shared_ptr<Instance>, shared_ptr<Instance>)> promptButtonHoldEndedSignal;
};

} // namespace RBX

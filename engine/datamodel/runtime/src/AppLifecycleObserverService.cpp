#include "v8datamodel/AppLifecycleObserverService.h"

namespace RBX {

const char* const sAppLifecycleObserverService = "AppLifecycleObserverService";

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<AppLifecycleObserverService,
    Enums::AppLifecycleManagerState()> funcGetCurrentState(
        &AppLifecycleObserverService::getCurrentState, "GetCurrentState",
        Security::RobloxScript);
static Reflection::BoundFuncDesc<AppLifecycleObserverService, bool()>
    funcIsDidDetachSupported(
        &AppLifecycleObserverService::isDidDetachSupported,
        "IsDidDetachSupported", Security::RobloxScript);
static Reflection::BoundFuncDesc<AppLifecycleObserverService, void()>
    funcTriggerOnLandingPageMount(
        &AppLifecycleObserverService::triggerOnLandingPageMount,
        "TriggerOnLandingPageMount", Security::RobloxScript);
static Reflection::BoundFuncDesc<AppLifecycleObserverService, void()>
    funcTriggerOnLuaAppInteractive(
        &AppLifecycleObserverService::triggerOnLuaAppInteractive,
        "TriggerOnLuaAppInteractive", Security::RobloxScript);
static Reflection::BoundFuncDesc<AppLifecycleObserverService, void()>
    funcTriggerOnLuaAppReadyToRender(
        &AppLifecycleObserverService::triggerOnLuaAppReadyToRender,
        "TriggerOnLuaAppReadyToRender", Security::RobloxScript);
static Reflection::EventDesc<AppLifecycleObserverService, void()>
    eventOnBecomeActive(&AppLifecycleObserverService::onBecomeActiveSignal,
        "OnBecomeActive", Security::RobloxScript);
static Reflection::EventDesc<AppLifecycleObserverService, void()>
    eventOnDetach(&AppLifecycleObserverService::onDetachSignal, "OnDetach",
        Security::RobloxScript);
static Reflection::EventDesc<AppLifecycleObserverService, void()>
    eventOnHide(&AppLifecycleObserverService::onHideSignal, "OnHide",
        Security::RobloxScript);
static Reflection::EventDesc<AppLifecycleObserverService, void()>
    eventOnResignActive(&AppLifecycleObserverService::onResignActiveSignal,
        "OnResignActive", Security::RobloxScript);
static Reflection::EventDesc<AppLifecycleObserverService, void()>
    eventOnStart(&AppLifecycleObserverService::onStartSignal, "OnStart",
        Security::RobloxScript);
static Reflection::EventDesc<AppLifecycleObserverService, void()>
    eventOnUnhide(&AppLifecycleObserverService::onUnhideSignal, "OnUnhide",
        Security::RobloxScript);
REFLECTION_END();

AppLifecycleObserverService::AppLifecycleObserverService()
    : Service(true)
    , currentState(Enums::APP_LIFECYCLE_ACTIVE)
    , didDetachSupported(false)
{
    setName(sAppLifecycleObserverService);
    setRobloxLocked(true);
}

void AppLifecycleObserverService::triggerOnLandingPageMount()
{
    landingPageMountSignal();
}

void AppLifecycleObserverService::triggerOnLuaAppInteractive()
{
    luaAppInteractiveSignal();
}

void AppLifecycleObserverService::triggerOnLuaAppReadyToRender()
{
    luaAppReadyToRenderSignal();
}

void AppLifecycleObserverService::setCurrentState(
    Enums::AppLifecycleManagerState value)
{
    if (currentState == value)
        return;
    const Enums::AppLifecycleManagerState previous = currentState;
    currentState = value;
    if (value == Enums::APP_LIFECYCLE_DETACHED)
        onDetachSignal();
    else if (value == Enums::APP_LIFECYCLE_HIDDEN)
        onHideSignal();
    else if (value == Enums::APP_LIFECYCLE_INACTIVE)
        onResignActiveSignal();
    else if (value == Enums::APP_LIFECYCLE_ACTIVE)
    {
        if (previous == Enums::APP_LIFECYCLE_HIDDEN)
            onUnhideSignal();
        else
            onBecomeActiveSignal();
    }
}

} // namespace RBX

#pragma once

#include "V8DataModel/InteractionEnums.h"
#include "V8Tree/Service.h"

#include "rbx/signal.h"

namespace RBX {

extern const char* const sAppLifecycleObserverService;

class AppLifecycleObserverService
    : public DescribedNonCreatable<AppLifecycleObserverService, Instance,
          sAppLifecycleObserverService>
    , public Service
{
public:
    AppLifecycleObserverService();

    Enums::AppLifecycleManagerState getCurrentState() { return currentState; }
    bool isDidDetachSupported() { return didDetachSupported; }
    void triggerOnLandingPageMount();
    void triggerOnLuaAppInteractive();
    void triggerOnLuaAppReadyToRender();

    void setCurrentState(Enums::AppLifecycleManagerState value);
    void setDidDetachSupported(bool value) { didDetachSupported = value; }

    rbx::signal<void()> onBecomeActiveSignal;
    rbx::signal<void()> onDetachSignal;
    rbx::signal<void()> onHideSignal;
    rbx::signal<void()> onResignActiveSignal;
    rbx::signal<void()> onStartSignal;
    rbx::signal<void()> onUnhideSignal;
    rbx::signal<void()> landingPageMountSignal;
    rbx::signal<void()> luaAppInteractiveSignal;
    rbx::signal<void()> luaAppReadyToRenderSignal;

private:
    Enums::AppLifecycleManagerState currentState;
    bool didDetachSupported;
};

} // namespace RBX

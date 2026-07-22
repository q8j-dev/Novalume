#pragma once

#include "v8tree/Service.h"

#include "rbx/signal.h"

#include <string>

namespace RBX {

extern const char* const sBrowserService;

class BrowserService
    : public DescribedNonCreatable<BrowserService, Instance, sBrowserService>
    , public Service
{
public:
    BrowserService();

    void closeBrowserWindow();
    void copyAuthCookieFromBrowserToEngine();
    void emitHybridEvent(std::string moduleName, std::string eventName, std::string params);
    void executeJavaScript(std::string javascript);
    void openBrowserWindow(std::string url);
    void openNativeOverlay(std::string title, std::string url);
    void openWeChatAuthWindow();
    void returnToJavaScript(std::string callbackId, bool success, std::string params);
    void sendCommand(std::string command);

    rbx::signal<void()> authCookieCopiedToEngineSignal;
    rbx::signal<void()> browserWindowClosedSignal;
    rbx::signal<void(std::string)> browserWindowWillNavigateSignal;
    rbx::signal<void(std::string)> javaScriptCallbackSignal;

    // Native hosts subscribe to these requests to supply their platform web view.
    rbx::signal<void(std::string, std::string, std::string)> hybridEventRequested;
    rbx::signal<void(std::string)> javaScriptRequested;
    rbx::signal<void(std::string, bool, std::string)> javaScriptReturnRequested;
    rbx::signal<void(std::string)> commandRequested;
    rbx::signal<void(std::string, std::string)> nativeOverlayRequested;
    rbx::signal<void()> weChatAuthRequested;

private:
    bool windowOpen;
    std::string currentUrl;
};

} // namespace RBX

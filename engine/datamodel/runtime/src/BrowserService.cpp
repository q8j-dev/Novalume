#include "V8DataModel/BrowserService.h"

#include "V8DataModel/GuiService.h"

namespace RBX {

const char* const sBrowserService = "BrowserService";

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<BrowserService, void()> funcCloseBrowserWindow(
    &BrowserService::closeBrowserWindow, "CloseBrowserWindow", Security::RobloxScript);
static Reflection::BoundFuncDesc<BrowserService, void()> funcCopyAuthCookieFromBrowserToEngine(
    &BrowserService::copyAuthCookieFromBrowserToEngine, "CopyAuthCookieFromBrowserToEngine",
    Security::RobloxScript);
static Reflection::BoundFuncDesc<BrowserService, void(std::string, std::string, std::string)>
    funcEmitHybridEvent(&BrowserService::emitHybridEvent, "EmitHybridEvent", "moduleName",
        "eventName", "params", Security::RobloxScript);
static Reflection::BoundFuncDesc<BrowserService, void(std::string)> funcExecuteJavaScript(
    &BrowserService::executeJavaScript, "ExecuteJavaScript", "javascript",
    Security::RobloxScript);
static Reflection::BoundFuncDesc<BrowserService, void(std::string)> funcOpenBrowserWindow(
    &BrowserService::openBrowserWindow, "OpenBrowserWindow", "url", Security::RobloxScript);
static Reflection::BoundFuncDesc<BrowserService, void(std::string, std::string)> funcOpenNativeOverlay(
    &BrowserService::openNativeOverlay, "OpenNativeOverlay", "title", "url",
    Security::RobloxScript);
static Reflection::BoundFuncDesc<BrowserService, void()> funcOpenWeChatAuthWindow(
    &BrowserService::openWeChatAuthWindow, "OpenWeChatAuthWindow", Security::RobloxScript);
static Reflection::BoundFuncDesc<BrowserService, void(std::string, bool, std::string)>
    funcReturnToJavaScript(&BrowserService::returnToJavaScript, "ReturnToJavaScript",
        "callbackId", "success", "params", Security::RobloxScript);
static Reflection::BoundFuncDesc<BrowserService, void(std::string)> funcSendCommand(
    &BrowserService::sendCommand, "SendCommand", "command", Security::RobloxScript);

static Reflection::EventDesc<BrowserService, void()> eventAuthCookieCopiedToEngine(
    &BrowserService::authCookieCopiedToEngineSignal, "AuthCookieCopiedToEngine",
    Security::RobloxScript);
static Reflection::EventDesc<BrowserService, void()> eventBrowserWindowClosed(
    &BrowserService::browserWindowClosedSignal, "BrowserWindowClosed", Security::RobloxScript);
static Reflection::EventDesc<BrowserService, void(std::string)> eventBrowserWindowWillNavigate(
    &BrowserService::browserWindowWillNavigateSignal, "BrowserWindowWillNavigate", "url",
    Security::RobloxScript);
static Reflection::EventDesc<BrowserService, void(std::string)> eventJavaScriptCallback(
    &BrowserService::javaScriptCallbackSignal, "JavaScriptCallback", "content",
    Security::RobloxScript);
REFLECTION_END();

BrowserService::BrowserService()
    : Service(true)
    , windowOpen(false)
{
    setName(sBrowserService);
    setRobloxLocked(true);
}

void BrowserService::closeBrowserWindow()
{
    if (!windowOpen)
        return;
    windowOpen = false;
    currentUrl.clear();
    browserWindowClosedSignal();
    if (GuiService* guiService = ServiceProvider::find<GuiService>(this))
        guiService->urlWindowClosed();
}

void BrowserService::copyAuthCookieFromBrowserToEngine()
{
    authCookieCopiedToEngineSignal();
}

void BrowserService::emitHybridEvent(std::string moduleName, std::string eventName,
    std::string params)
{
    hybridEventRequested(moduleName, eventName, params);
}

void BrowserService::executeJavaScript(std::string javascript)
{
    javaScriptRequested(javascript);
}

void BrowserService::openBrowserWindow(std::string url)
{
    if (url.empty())
        return;
    currentUrl = url;
    windowOpen = true;
    browserWindowWillNavigateSignal(url);
    if (GuiService* guiService = ServiceProvider::find<GuiService>(this))
        guiService->openUrlWindow(url);
}

void BrowserService::openNativeOverlay(std::string title, std::string url)
{
    nativeOverlayRequested(title, url);
    openBrowserWindow(url);
}

void BrowserService::openWeChatAuthWindow()
{
    weChatAuthRequested();
}

void BrowserService::returnToJavaScript(std::string callbackId, bool success,
    std::string params)
{
    javaScriptReturnRequested(callbackId, success, params);
}

void BrowserService::sendCommand(std::string command)
{
    commandRequested(command);
}

} // namespace RBX

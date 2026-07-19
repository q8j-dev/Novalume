#include "V8DataModel/WebViewService.h"

namespace RBX {

const char* const sWebViewService = "WebViewService";

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<WebViewService, void()> funcCloseWindow(
    &WebViewService::closeWindow, "CloseWindow", Security::RobloxScript);
static Reflection::BoundFuncDesc<WebViewService,
    void(std::string, std::string, bool, std::string, std::string, bool, bool)>
        funcMutateWindow(&WebViewService::mutateWindow, "MutateWindow",
            "url", "title", std::string(), "isVisible", true,
            "searchType", std::string(), "transitionAnimation", std::string(),
            "showDomainAsTitle", false, "backButtonVisible", true,
            Security::RobloxScript);
static Reflection::BoundFuncDesc<WebViewService,
    void(std::string, std::string, bool, std::string, std::string, bool, bool)>
        funcOpenWindow(&WebViewService::openWindow, "OpenWindow",
            "url", "title", std::string(), "isVisible", true,
            "searchType", std::string(), "transitionAnimation", std::string(),
            "showDomainAsTitle", false, "backButtonVisible", true,
            Security::RobloxScript);
static Reflection::BoundFuncDesc<WebViewService, void(std::string, Reflection::Variant)>
    funcOpenWindowV2(&WebViewService::openWindowV2, "OpenWindowV2", "url", "params",
        Reflection::Variant(), Security::RobloxScript);
static Reflection::BoundYieldFuncDesc<WebViewService, bool()> funcIsAvailable(
    &WebViewService::isAvailable, "IsAvailable", Security::RobloxScript);
static Reflection::EventDesc<WebViewService, void(std::string)> eventOnJavaScriptCall(
    &WebViewService::onJavaScriptCallSignal, "OnJavaScriptCall", "content",
    Security::RobloxScript);
static Reflection::EventDesc<WebViewService, void()> eventOnWindowClosed(
    &WebViewService::onWindowClosedSignal, "OnWindowClosed", Security::RobloxScript);
REFLECTION_END();

WebViewService::WebViewService()
    : Service(true)
    , windowOpen(false)
{
    setName(sWebViewService);
    setRobloxLocked(true);
}

void WebViewService::closeWindow()
{
    if (!windowOpen)
        return;
    windowOpen = false;
    closeWindowRequested();
    onWindowClosedSignal();
}

void WebViewService::mutateWindow(std::string url, std::string title, bool isVisible,
    std::string searchType, std::string transitionAnimation,
    bool showDomainAsTitle, bool backButtonVisible)
{
    state.url = url;
    state.title = title;
    state.visible = isVisible;
    state.searchType = searchType;
    state.transitionAnimation = transitionAnimation;
    state.showDomainAsTitle = showDomainAsTitle;
    state.backButtonVisible = backButtonVisible;
    mutateWindowRequested(state);
}

void WebViewService::openWindow(std::string url, std::string title, bool isVisible,
    std::string searchType, std::string transitionAnimation,
    bool showDomainAsTitle, bool backButtonVisible)
{
    mutateWindow(url, title, isVisible, searchType, transitionAnimation,
        showDomainAsTitle, backButtonVisible);
    windowOpen = true;
    openWindowRequested(state);
}

void WebViewService::openWindowV2(std::string url, Reflection::Variant params)
{
    state.url = url;
    windowOpen = true;
    openWindowV2Requested(url, params);
}

void WebViewService::isAvailable(boost::function<void(bool)> resumeFunction,
    boost::function<void(std::string)> errorFunction)
{
    resumeFunction(!openWindowRequested.empty() || !openWindowV2Requested.empty());
}

} // namespace RBX

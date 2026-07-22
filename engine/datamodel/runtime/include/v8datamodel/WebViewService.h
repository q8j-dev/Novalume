#pragma once

#include "v8tree/Service.h"

#include "rbx/signal.h"

#include <string>

namespace RBX {

extern const char* const sWebViewService;

class WebViewService
    : public DescribedNonCreatable<WebViewService, Instance, sWebViewService>
    , public Service
{
public:
    WebViewService();

    void closeWindow();
    void mutateWindow(std::string url, std::string title, bool isVisible,
        std::string searchType, std::string transitionAnimation,
        bool showDomainAsTitle, bool backButtonVisible);
    void openWindow(std::string url, std::string title, bool isVisible,
        std::string searchType, std::string transitionAnimation,
        bool showDomainAsTitle, bool backButtonVisible);
    void openWindowV2(std::string url, Reflection::Variant params);
    void isAvailable(boost::function<void(bool)> resumeFunction,
        boost::function<void(std::string)> errorFunction);

    rbx::signal<void(std::string)> onJavaScriptCallSignal;
    rbx::signal<void()> onWindowClosedSignal;

    struct WindowState
    {
        std::string url;
        std::string title;
        bool visible;
        std::string searchType;
        std::string transitionAnimation;
        bool showDomainAsTitle;
        bool backButtonVisible;

        WindowState()
            : visible(true)
            , showDomainAsTitle(false)
            , backButtonVisible(true)
        {}
    };

    // The platform web-view host consumes these state transitions.
    rbx::signal<void(const WindowState&)> openWindowRequested;
    rbx::signal<void(const WindowState&)> mutateWindowRequested;
    rbx::signal<void()> closeWindowRequested;
    rbx::signal<void(std::string, Reflection::Variant)> openWindowV2Requested;

private:
    WindowState state;
    bool windowOpen;
};

} // namespace RBX

#pragma once

#include "V8Tree/Service.h"

#include "rbx/signal.h"

#include <string>
#include <vector>

namespace RBX {

extern const char* const sLinkingService;

class LinkingService
    : public DescribedNonCreatable<LinkingService, Instance, sLinkingService>
    , public Service
{
public:
    LinkingService();

    void detectUrl(std::string url);
    shared_ptr<const Reflection::ValueTable> getAndClearLastPendingUrl();
    Reflection::Variant getLastLuaUrl();
    bool isUrlRegistered(std::string url);
    void registerLuaUrl(std::string url);
    shared_ptr<const Reflection::ValueTable> startLuaUrlDelivery();
    void stopLuaUrlDelivery();
    void openUrl(std::string url, boost::function<void(bool)> resumeFunction,
        boost::function<void(std::string)> errorFunction);
    void supportsSwitchToSettingsApp(boost::function<void(bool)> resumeFunction,
        boost::function<void(std::string)> errorFunction);
    void switchToSettingsApp(std::string route,
        boost::function<void()> resumeFunction,
        boost::function<void(std::string)> errorFunction);

    rbx::signal<void(std::string, std::string, std::string)> onLuaUrlSignal;

private:
    struct PendingUrl
    {
        std::string url;
        std::string matchedUrl;
        std::string attributionUrl;
        bool valid;

        PendingUrl() : valid(false) {}
    };

    bool matchRegisteredUrl(const std::string& url, std::string& matchedUrl) const;
    static bool isExternalUrlAllowed(const std::string& url);
    bool openUrlInternal(const std::string& url);
    bool supportsSwitchToSettingsAppInternal() const;
    static shared_ptr<const Reflection::ValueTable> toTable(const PendingUrl& pending);

    std::vector<std::string> registeredUrls;
    PendingUrl pendingUrl;
    std::string lastLuaUrl;
    bool delivering;
};

} // namespace RBX

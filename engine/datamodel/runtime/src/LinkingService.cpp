#include "v8datamodel/LinkingService.h"

#include "v8datamodel/GuiService.h"

#include <algorithm>
#include <cctype>
#include <regex>

namespace RBX {

const char* const sLinkingService = "LinkingService";

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<LinkingService, void(std::string)> funcDetectUrl(
    &LinkingService::detectUrl, "DetectUrl", "url", Security::RobloxScript);
static Reflection::BoundFuncDesc<LinkingService,
    shared_ptr<const Reflection::ValueTable>()> funcGetAndClearLastPendingUrl(
        &LinkingService::getAndClearLastPendingUrl,
        "GetAndClearLastPendingUrl", Security::RobloxScript);
static Reflection::BoundFuncDesc<LinkingService, Reflection::Variant()>
    funcGetLastLuaUrl(&LinkingService::getLastLuaUrl, "GetLastLuaUrl",
        Security::RobloxScript);
static Reflection::BoundFuncDesc<LinkingService, bool(std::string)>
    funcIsUrlRegistered(&LinkingService::isUrlRegistered, "IsUrlRegistered",
        "url", Security::RobloxScript);
static Reflection::BoundFuncDesc<LinkingService, void(std::string)>
    funcRegisterLuaUrl(&LinkingService::registerLuaUrl, "RegisterLuaUrl", "url",
        Security::RobloxScript);
static Reflection::BoundFuncDesc<LinkingService,
    shared_ptr<const Reflection::ValueTable>()> funcStartLuaUrlDelivery(
        &LinkingService::startLuaUrlDelivery, "StartLuaUrlDelivery",
        Security::RobloxScript);
static Reflection::BoundFuncDesc<LinkingService, void()> funcStopLuaUrlDelivery(
    &LinkingService::stopLuaUrlDelivery, "StopLuaUrlDelivery",
    Security::RobloxScript);
static Reflection::BoundYieldFuncDesc<LinkingService, bool(std::string)>
    funcOpenUrl(&LinkingService::openUrl, "OpenUrl", "url",
        Security::RobloxScript);
static Reflection::BoundYieldFuncDesc<LinkingService, bool()>
    funcSupportsSwitchToSettingsApp(&LinkingService::supportsSwitchToSettingsApp,
        "SupportsSwitchToSettingsApp", Security::RobloxScript);
static Reflection::BoundYieldFuncDesc<LinkingService, void(std::string)>
    funcSwitchToSettingsApp(&LinkingService::switchToSettingsApp,
        "SwitchToSettingsApp", "route", std::string(), Security::RobloxScript);
static Reflection::EventDesc<LinkingService,
    void(std::string, std::string, std::string)> eventOnLuaUrl(
        &LinkingService::onLuaUrlSignal, "OnLuaUrl", "url", "matchedUrl",
        "attributionUrl", Security::RobloxScript);
REFLECTION_END();

LinkingService::LinkingService()
    : Service(true)
    , delivering(false)
{
    setName(sLinkingService);
    setRobloxLocked(true);
}

bool LinkingService::matchRegisteredUrl(
    const std::string& url, std::string& matchedUrl) const
{
    for (std::vector<std::string>::const_iterator it = registeredUrls.begin();
         it != registeredUrls.end(); ++it)
    {
        try
        {
            if (std::regex_search(url, std::regex(*it,
                    std::regex_constants::ECMAScript |
                        std::regex_constants::icase)))
            {
                matchedUrl = *it;
                return true;
            }
        }
        catch (const std::regex_error&)
        {
            if (url.compare(0, it->size(), *it) == 0)
            {
                matchedUrl = *it;
                return true;
            }
        }
    }
    return false;
}

void LinkingService::detectUrl(std::string url)
{
    std::string matchedUrl;
    if (!matchRegisteredUrl(url, matchedUrl))
        return;

    lastLuaUrl = url;
    if (delivering)
    {
        onLuaUrlSignal(url, matchedUrl, std::string());
        return;
    }

    pendingUrl.url = url;
    pendingUrl.matchedUrl = matchedUrl;
    pendingUrl.attributionUrl.clear();
    pendingUrl.valid = true;
}

shared_ptr<const Reflection::ValueTable> LinkingService::toTable(
    const PendingUrl& pending)
{
    shared_ptr<Reflection::ValueTable> result(new Reflection::ValueTable());
    (*result)["hasPendingUrl"] = Reflection::Variant(pending.valid);
    if (pending.valid)
    {
        (*result)["url"] = Reflection::Variant(pending.url);
        (*result)["matchedUrl"] = Reflection::Variant(pending.matchedUrl);
        if (!pending.attributionUrl.empty())
            (*result)["attributionUrl"] = Reflection::Variant(pending.attributionUrl);
    }
    return result;
}

shared_ptr<const Reflection::ValueTable>
LinkingService::getAndClearLastPendingUrl()
{
    shared_ptr<const Reflection::ValueTable> result = toTable(pendingUrl);
    pendingUrl = PendingUrl();
    return result;
}

Reflection::Variant LinkingService::getLastLuaUrl()
{
    return lastLuaUrl.empty() ? Reflection::Variant() : Reflection::Variant(lastLuaUrl);
}

bool LinkingService::isUrlRegistered(std::string url)
{
    std::string matchedUrl;
    return matchRegisteredUrl(url, matchedUrl);
}

void LinkingService::registerLuaUrl(std::string url)
{
    if (url.empty())
        return;
    if (std::find(registeredUrls.begin(), registeredUrls.end(), url) ==
        registeredUrls.end())
        registeredUrls.push_back(url);
}

shared_ptr<const Reflection::ValueTable> LinkingService::startLuaUrlDelivery()
{
    delivering = true;
    shared_ptr<const Reflection::ValueTable> result = toTable(pendingUrl);
    pendingUrl = PendingUrl();
    return result;
}

void LinkingService::stopLuaUrlDelivery()
{
    delivering = false;
}

bool LinkingService::isExternalUrlAllowed(const std::string& url)
{
    if (url.empty() || std::find_if(url.begin(), url.end(), [](unsigned char c) {
            return c < 0x20 || c == 0x7f;
        }) != url.end())
        return false;

    const std::string::size_type colon = url.find(':');
    if (colon == std::string::npos)
        return false;
    std::string scheme = url.substr(0, colon);
    std::transform(scheme.begin(), scheme.end(), scheme.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return scheme == "http" || scheme == "https" || scheme == "roblox" ||
        scheme == "roblox-player";
}

bool LinkingService::openUrlInternal(const std::string& url)
{
    if (!isExternalUrlAllowed(url))
        return false;
    GuiService* guiService = ServiceProvider::find<GuiService>(this);
    if (!guiService)
        return false;
    guiService->openUrlWindow(url);
    return true;
}

void LinkingService::openUrl(std::string url,
    boost::function<void(bool)> resumeFunction,
    boost::function<void(std::string)> errorFunction)
{
    resumeFunction(openUrlInternal(url));
}

bool LinkingService::supportsSwitchToSettingsAppInternal() const
{
#if defined(__APPLE__) || defined(_WIN32)
    return true;
#else
    return false;
#endif
}

void LinkingService::supportsSwitchToSettingsApp(
    boost::function<void(bool)> resumeFunction,
    boost::function<void(std::string)> errorFunction)
{
    resumeFunction(supportsSwitchToSettingsAppInternal());
}

void LinkingService::switchToSettingsApp(std::string route,
    boost::function<void()> resumeFunction,
    boost::function<void(std::string)> errorFunction)
{
    if (!supportsSwitchToSettingsAppInternal())
    {
        resumeFunction();
        return;
    }
#if defined(__APPLE__)
    std::string url = "x-apple.systempreferences:";
    if (!route.empty())
        url += route;
#else
    std::string url = "ms-settings:";
    if (!route.empty())
        url += route;
#endif
    if (GuiService* guiService = ServiceProvider::find<GuiService>(this))
        guiService->openUrlWindow(url);
    resumeFunction();
}

} // namespace RBX

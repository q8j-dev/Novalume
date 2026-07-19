#include "V8DataModel/RbxAnalyticsService.h"

#include "Util/Analytics.h"
#include "Util/Guid.h"
#include "Util/RobloxGoogleAnalytics.h"
#include "Util/Statistics.h"
#include "V8DataModel/EventIngestService.h"
#include "V8DataModel/GameBasicSettings.h"

#include <rapidjson/document.h>

#include <stdexcept>

namespace RBX {

const char* const sRbxAnalyticsService = "RbxAnalyticsService";

namespace {

std::string newSessionId()
{
    std::string result;
    Guid::generateStandardGUID(result);
    if (result.size() > 2 && result.front() == '{' && result.back() == '}')
        result = result.substr(1, result.size() - 2);
    return result;
}

std::string scalarString(const Reflection::Variant& value)
{
    if (value.isVoid())
        return std::string();
    try
    {
        return value.get<std::string>();
    }
    catch (const std::exception&)
    {
        throw std::runtime_error("RbxAnalyticsService dictionaries must contain scalar values");
    }
}

void addInfluxPoint(boost::unordered_set<Analytics::InfluxDb::Point>& result,
    const std::string& key, const Reflection::Variant& value)
{
    rapidjson::Document owner;
    rapidjson::Value json;
    if (value.isType<bool>())
        json.SetBool(value.cast<bool>());
    else if (value.isType<int>())
        json.SetInt(value.cast<int>());
    else if (value.isType<long long>())
        json.SetInt64(value.cast<long long>());
    else if (value.isType<float>())
        json.SetDouble(value.cast<float>());
    else if (value.isType<double>())
        json.SetDouble(value.cast<double>());
    else
    {
        const std::string text = scalarString(value);
        json.SetString(text.c_str(), static_cast<rapidjson::SizeType>(text.size()),
            owner.GetAllocator());
    }
    result.insert(Analytics::InfluxDb::Point(key, json));
}

} // namespace

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<RbxAnalyticsService, void(std::string, int)>
    funcAddGlobalPointsField(&RbxAnalyticsService::addGlobalPointsField,
        "AddGlobalPointsField", "key", "value", Security::RobloxScript);
static Reflection::BoundFuncDesc<RbxAnalyticsService, void(std::string, std::string)>
    funcAddGlobalPointsTag(&RbxAnalyticsService::addGlobalPointsTag,
        "AddGlobalPointsTag", "key", "value", Security::RobloxScript);
static Reflection::BoundFuncDesc<RbxAnalyticsService, void(std::string)>
    funcRemoveGlobalPointsField(&RbxAnalyticsService::removeGlobalPointsField,
        "RemoveGlobalPointsField", "key", Security::RobloxScript);
static Reflection::BoundFuncDesc<RbxAnalyticsService, void(std::string)>
    funcRemoveGlobalPointsTag(&RbxAnalyticsService::removeGlobalPointsTag,
        "RemoveGlobalPointsTag", "key", Security::RobloxScript);
static Reflection::BoundFuncDesc<RbxAnalyticsService, std::string()>
    funcGetClientId(&RbxAnalyticsService::getClientId, "GetClientId",
        Security::RobloxScript);
static Reflection::BoundFuncDesc<RbxAnalyticsService, std::string()>
    funcGetPlaySessionId(&RbxAnalyticsService::getPlaySessionId, "GetPlaySessionId",
        Security::RobloxScript);
static Reflection::BoundFuncDesc<RbxAnalyticsService, std::string()>
    funcGetSessionId(&RbxAnalyticsService::getSessionId, "GetSessionId",
        Security::RobloxScript);
static Reflection::BoundFuncDesc<RbxAnalyticsService, void(std::string)>
    funcReleaseRBXEventStream(&RbxAnalyticsService::releaseRBXEventStream,
        "ReleaseRBXEventStream", "target", Security::RobloxScript);
static Reflection::BoundFuncDesc<RbxAnalyticsService, void(std::string, int)>
    funcReportCounter(&RbxAnalyticsService::reportCounter, "ReportCounter",
        "counterName", "amount", 1, Security::RobloxScript);
static Reflection::BoundFuncDesc<RbxAnalyticsService,
    void(std::string, shared_ptr<const Reflection::ValueTable>, int)>
    funcReportInfluxSeries(&RbxAnalyticsService::reportInfluxSeries,
        "ReportInfluxSeries", "seriesName", "points", "throttlingPercentage",
        Security::RobloxScript);
static Reflection::BoundFuncDesc<RbxAnalyticsService, void(std::string, float)>
    funcReportStats(&RbxAnalyticsService::reportStats, "ReportStats", "category",
        "value", Security::RobloxScript);
static Reflection::BoundFuncDesc<RbxAnalyticsService,
    void(std::string, std::string, double)> funcReportToDiagByCountryCode(
        &RbxAnalyticsService::reportToDiagByCountryCode, "ReportToDiagByCountryCode",
        "featureName", "measureName", "seconds", Security::RobloxScript);

typedef void (RbxAnalyticsService::*EventFunction)(std::string, std::string,
    std::string, shared_ptr<const Reflection::ValueTable>);
static Reflection::BoundFuncDesc<RbxAnalyticsService,
    void(std::string, std::string, std::string,
        shared_ptr<const Reflection::ValueTable>)> funcSetRBXEvent(
            &RbxAnalyticsService::setRBXEvent, "SetRBXEvent", "target",
            "eventContext", "eventName", "additionalArgs", Security::RobloxScript);
static Reflection::BoundFuncDesc<RbxAnalyticsService,
    void(std::string, std::string, std::string,
        shared_ptr<const Reflection::ValueTable>)> funcSetRBXEventStream(
            &RbxAnalyticsService::setRBXEventStream, "SetRBXEventStream", "target",
            "eventContext", "eventName", "additionalArgs", Security::RobloxScript);
static Reflection::BoundFuncDesc<RbxAnalyticsService,
    void(std::string, std::string, std::string,
        shared_ptr<const Reflection::ValueTable>)> funcSendEventImmediately(
            &RbxAnalyticsService::sendEventImmediately, "SendEventImmediately", "target",
            "eventContext", "eventName", "additionalArgs", Security::RobloxScript);
static Reflection::BoundFuncDesc<RbxAnalyticsService,
    void(std::string, std::string, std::string,
        shared_ptr<const Reflection::ValueTable>)> funcSendEventDeferred(
            &RbxAnalyticsService::sendEventDeferred, "SendEventDeferred", "target",
            "eventContext", "eventName", "additionalArgs", Security::RobloxScript);

static Reflection::BoundFuncDesc<RbxAnalyticsService,
    void(std::string, std::string, std::string, long long)> funcTrackEvent(
        &RbxAnalyticsService::trackEvent, "TrackEvent", "category", "action", "label",
        "value", static_cast<long long>(0), Security::RobloxScript);
static Reflection::BoundFuncDesc<RbxAnalyticsService,
    void(std::string, std::string, std::string, long long)> funcDeprecatedTrackEvent(
        &RbxAnalyticsService::trackEvent, "DEPRECATED_TrackEvent", "category", "action",
        "label", "value", static_cast<long long>(0), Security::RobloxScript);
static Reflection::BoundFuncDesc<RbxAnalyticsService,
    void(std::string, std::string, std::string,
        shared_ptr<const Reflection::ValueTable>, long long)> funcTrackEventWithArgs(
            &RbxAnalyticsService::trackEventWithArgs, "TrackEventWithArgs", "category",
            "action", "label", "args", "value", static_cast<long long>(0),
            Security::RobloxScript);
static Reflection::BoundFuncDesc<RbxAnalyticsService,
    void(std::string, std::string, std::string,
        shared_ptr<const Reflection::ValueTable>, long long)> funcDeprecatedTrackEventWithArgs(
            &RbxAnalyticsService::trackEventWithArgs, "DEPRECATED_TrackEventWithArgs",
            "category", "action", "label", "args", "value",
            static_cast<long long>(0), Security::RobloxScript);
static Reflection::BoundFuncDesc<RbxAnalyticsService,
    void(shared_ptr<const Reflection::ValueTable>)> funcUpdateHeartbeatObject(
        &RbxAnalyticsService::updateHeartbeatObject, "UpdateHeartbeatObject", "args",
        Security::RobloxScript);
REFLECTION_END();

RbxAnalyticsService::RbxAnalyticsService()
    : Service(true)
    , playSessionId(newSessionId())
{
    setName(sRbxAnalyticsService);
    setRobloxLocked(true);
}

EventIngestService* RbxAnalyticsService::eventIngest() const
{
    EventIngestService* result = ServiceProvider::find<EventIngestService>(this);
    if (!result)
        throw std::runtime_error("RbxAnalyticsService requires EventIngestService");
    return result;
}

void RbxAnalyticsService::addGlobalPointsField(std::string key, int value)
{ std::lock_guard<std::mutex> guard(mutex); globalPointsFields[std::move(key)] = value; }
void RbxAnalyticsService::addGlobalPointsTag(std::string key, std::string value)
{ std::lock_guard<std::mutex> guard(mutex); globalPointsTags[std::move(key)] = std::move(value); }
void RbxAnalyticsService::removeGlobalPointsField(std::string key)
{ std::lock_guard<std::mutex> guard(mutex); globalPointsFields.erase(key); }
void RbxAnalyticsService::removeGlobalPointsTag(std::string key)
{ std::lock_guard<std::mutex> guard(mutex); globalPointsTags.erase(key); }

std::string RbxAnalyticsService::getClientId()
{
    std::string result = GameBasicSettings::singleton().getGoogleAnalyticsClientId();
    if (result.empty())
    {
        result = newSessionId();
        GameBasicSettings::singleton().setGoogleAnalyticsClientId(result);
    }
    return result;
}

std::string RbxAnalyticsService::getPlaySessionId() { return playSessionId; }
std::string RbxAnalyticsService::getSessionId() { return eventIngest()->getSessionId(); }
void RbxAnalyticsService::releaseRBXEventStream(std::string target)
{ eventIngest()->releaseRBXEventStream(target); }
void RbxAnalyticsService::reportCounter(std::string counterName, int amount)
{
    {
        std::lock_guard<std::mutex> guard(mutex);
        reportedCounters[counterName] += amount;
    }
    if (!GetBaseURL().empty())
        Analytics::EphemeralCounter::reportCounter(counterName, amount);
}

long long RbxAnalyticsService::getReportedCounter(
    const std::string& counterName) const
{
    std::lock_guard<std::mutex> guard(mutex);
    const std::map<std::string, long long>::const_iterator found =
        reportedCounters.find(counterName);
    return found == reportedCounters.end() ? 0 : found->second;
}

void RbxAnalyticsService::reportInfluxSeries(std::string seriesName,
    shared_ptr<const Reflection::ValueTable> points, int throttlingPercentage)
{
    boost::unordered_set<Analytics::InfluxDb::Point> values;
    if (points)
        for (Reflection::ValueTable::const_iterator it = points->begin(); it != points->end(); ++it)
            addInfluxPoint(values, it->first, it->second);
    {
        std::lock_guard<std::mutex> guard(mutex);
        for (std::map<std::string, int>::const_iterator it = globalPointsFields.begin();
             it != globalPointsFields.end(); ++it)
            addInfluxPoint(values, it->first, Reflection::Variant(it->second));
        for (std::map<std::string, std::string>::const_iterator it = globalPointsTags.begin();
             it != globalPointsTags.end(); ++it)
            addInfluxPoint(values, it->first, Reflection::Variant(it->second));
    }
    Analytics::InfluxDb::reportPoints(seriesName, values, throttlingPercentage);
}

void RbxAnalyticsService::reportStats(std::string category, float value)
{
    {
        std::lock_guard<std::mutex> guard(mutex);
        reportedStats[category] = value;
    }
    if (!GetBaseURL().empty())
        Analytics::EphemeralCounter::reportStats(category, value);
}
void RbxAnalyticsService::reportToDiagByCountryCode(std::string featureName,
    std::string measureName, double seconds)
{ Analytics::EphemeralCounter::reportStats(featureName + "." + measureName,
    static_cast<float>(seconds)); }

void RbxAnalyticsService::setRBXEvent(std::string a, std::string b, std::string c,
    shared_ptr<const Reflection::ValueTable> d)
{ eventIngest()->setRBXEvent(std::move(a), std::move(b), std::move(c), d); }
void RbxAnalyticsService::setRBXEventStream(std::string a, std::string b, std::string c,
    shared_ptr<const Reflection::ValueTable> d)
{ eventIngest()->setRBXEventStream(std::move(a), std::move(b), std::move(c), d); }
void RbxAnalyticsService::sendEventImmediately(std::string a, std::string b, std::string c,
    shared_ptr<const Reflection::ValueTable> d)
{ eventIngest()->sendEventImmediately(std::move(a), std::move(b), std::move(c), d); }
void RbxAnalyticsService::sendEventDeferred(std::string a, std::string b, std::string c,
    shared_ptr<const Reflection::ValueTable> d)
{ eventIngest()->sendEventDeferred(std::move(a), std::move(b), std::move(c), d); }

void RbxAnalyticsService::trackEvent(std::string category, std::string action,
    std::string label, long long value)
{
    if (RobloxGoogleAnalytics::isInitialized())
        RobloxGoogleAnalytics::trackEvent(category.c_str(), action.c_str(), label.c_str(),
            static_cast<int>(value));
}

void RbxAnalyticsService::trackEventWithArgs(std::string category, std::string action,
    std::string label, shared_ptr<const Reflection::ValueTable> args, long long value)
{
    trackEvent(category, action, label, value);
    if (args)
        sendEventDeferred("client", category, action, args);
}

void RbxAnalyticsService::updateHeartbeatObject(
    shared_ptr<const Reflection::ValueTable> args)
{
    std::lock_guard<std::mutex> guard(mutex);
    heartbeatFields.clear();
    if (args)
        for (Reflection::ValueTable::const_iterator it = args->begin(); it != args->end(); ++it)
            heartbeatFields[it->first] = scalarString(it->second);
}

} // namespace RBX

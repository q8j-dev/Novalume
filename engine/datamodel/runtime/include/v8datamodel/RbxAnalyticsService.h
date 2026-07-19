#pragma once

#include "V8Tree/Service.h"

#include <map>
#include <mutex>
#include <string>

namespace RBX {

class EventIngestService;
extern const char* const sRbxAnalyticsService;

class RbxAnalyticsService
    : public DescribedNonCreatable<RbxAnalyticsService, Instance, sRbxAnalyticsService>
    , public Service
{
public:
    RbxAnalyticsService();

    void addGlobalPointsField(std::string key, int value);
    void addGlobalPointsTag(std::string key, std::string value);
    void removeGlobalPointsField(std::string key);
    void removeGlobalPointsTag(std::string key);

    std::string getClientId();
    std::string getPlaySessionId();
    std::string getSessionId();

    void releaseRBXEventStream(std::string target);
    void reportCounter(std::string counterName, int amount);
    long long getReportedCounter(const std::string& counterName) const;
    void reportInfluxSeries(std::string seriesName,
        shared_ptr<const Reflection::ValueTable> points, int throttlingPercentage);
    void reportStats(std::string category, float value);
    void reportToDiagByCountryCode(std::string featureName,
        std::string measureName, double seconds);

    void setRBXEvent(std::string target, std::string eventContext,
        std::string eventName, shared_ptr<const Reflection::ValueTable> additionalArgs);
    void setRBXEventStream(std::string target, std::string eventContext,
        std::string eventName, shared_ptr<const Reflection::ValueTable> additionalArgs);
    void sendEventImmediately(std::string target, std::string eventContext,
        std::string eventName, shared_ptr<const Reflection::ValueTable> additionalArgs);
    void sendEventDeferred(std::string target, std::string eventContext,
        std::string eventName, shared_ptr<const Reflection::ValueTable> additionalArgs);

    void trackEvent(std::string category, std::string action,
        std::string label, long long value);
    void trackEventWithArgs(std::string category, std::string action,
        std::string label, shared_ptr<const Reflection::ValueTable> args,
        long long value);
    void updateHeartbeatObject(shared_ptr<const Reflection::ValueTable> args);

private:
    EventIngestService* eventIngest() const;

    mutable std::mutex mutex;
    std::map<std::string, int> globalPointsFields;
    std::map<std::string, std::string> globalPointsTags;
    std::map<std::string, std::string> heartbeatFields;
    std::map<std::string, long long> reportedCounters;
    std::map<std::string, float> reportedStats;
    std::string playSessionId;
};

} // namespace RBX

#include "v8datamodel/TelemetryService.h"

namespace RBX {

const char* const sTelemetryService = "TelemetryService";

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<TelemetryService,
    Reflection::Variant(shared_ptr<const Reflection::ValueTable>,
        Reflection::Variant, float)> funcLogCounter(
            &TelemetryService::logCounter, "LogCounter", "config", "data",
            Reflection::Variant(), "value", 1.0f, Security::RobloxScript);
static Reflection::BoundFuncDesc<TelemetryService,
    Reflection::Variant(std::string)> funcLogDuration(
        &TelemetryService::logDurationEvent, "LogDurationEvent", "key",
        Security::RobloxScript);
static Reflection::BoundFuncDesc<TelemetryService,
    Reflection::Variant(std::string, double)> funcLogDurationTimestamp(
        &TelemetryService::logDurationEventWithTimestamp,
        "LogDurationEventWithTimestamp", "key", "timestamp",
        Security::RobloxScript);
static Reflection::BoundFuncDesc<TelemetryService,
    Reflection::Variant(shared_ptr<const Reflection::ValueTable>, Reflection::Variant)>
        funcLogEvent(&TelemetryService::logEvent, "LogEvent", "config", "data",
            Reflection::Variant(), Security::RobloxScript);
static Reflection::BoundFuncDesc<TelemetryService,
    Reflection::Variant(shared_ptr<const Reflection::ValueTable>,
        Reflection::Variant, float)> funcLogStat(
            &TelemetryService::logStat, "LogStat", "config", "data", "value",
            Security::RobloxScript);
REFLECTION_END();

TelemetryService::TelemetryService()
    : Service(true)
{
    setName(sTelemetryService);
    setRobloxLocked(true);
}

std::string TelemetryService::eventName(
    const shared_ptr<const Reflection::ValueTable>& config)
{
    if (!config)
        throw runtime_error("TelemetryService config is required");
    Reflection::ValueTable::const_iterator found = config->find("eventName");
    if (found == config->end() || !found->second.isType<std::string>() ||
        found->second.cast<std::string>().empty())
        throw runtime_error("TelemetryService eventName is required");
    return found->second.cast<std::string>();
}

Reflection::Variant TelemetryService::record(std::string kind,
    shared_ptr<const Reflection::ValueTable> config,
    Reflection::Variant data, double value)
{
    Record entry;
    entry.kind = std::move(kind);
    entry.eventName = eventName(config);
    entry.config = config;
    entry.data = std::move(data);
    entry.value = value;
    entry.recordedAt = std::chrono::steady_clock::now();
    if (records.size() == maxRecords)
        records.pop_front();
    records.push_back(std::move(entry));
    return Reflection::Variant(true);
}

Reflection::Variant TelemetryService::logCounter(
    shared_ptr<const Reflection::ValueTable> config,
    Reflection::Variant data, float value)
{
    return record("counter", config, std::move(data), value);
}

Reflection::Variant TelemetryService::logStat(
    shared_ptr<const Reflection::ValueTable> config,
    Reflection::Variant data, float value)
{
    return record("stat", config, std::move(data), value);
}

Reflection::Variant TelemetryService::logEvent(
    shared_ptr<const Reflection::ValueTable> config, Reflection::Variant data)
{
    return record("event", config, std::move(data), 1.0);
}

Reflection::Variant TelemetryService::logDurationEvent(std::string key)
{
    const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    std::map<std::string, std::chrono::steady_clock::time_point>::iterator found =
        durationStarts.find(key);
    if (found == durationStarts.end())
    {
        durationStarts[key] = now;
        return Reflection::Variant(0.0);
    }
    const double milliseconds =
        std::chrono::duration<double, std::milli>(now - found->second).count();
    found->second = now;
    return Reflection::Variant(milliseconds);
}

Reflection::Variant TelemetryService::logDurationEventWithTimestamp(
    std::string key, double timestamp)
{
    Record entry;
    entry.kind = "duration";
    entry.eventName = std::move(key);
    entry.value = timestamp;
    entry.recordedAt = std::chrono::steady_clock::now();
    if (records.size() == maxRecords)
        records.pop_front();
    records.push_back(std::move(entry));
    return Reflection::Variant(timestamp);
}

} // namespace RBX

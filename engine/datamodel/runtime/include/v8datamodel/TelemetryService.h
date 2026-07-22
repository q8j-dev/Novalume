#pragma once

#include "v8tree/Service.h"

#include <chrono>
#include <deque>
#include <map>
#include <string>

namespace RBX {

extern const char* const sTelemetryService;

class TelemetryService
    : public DescribedNonCreatable<TelemetryService, Instance, sTelemetryService>
    , public Service
{
public:
    TelemetryService();

    Reflection::Variant logCounter(shared_ptr<const Reflection::ValueTable> config,
        Reflection::Variant data, float value);
    Reflection::Variant logDurationEvent(std::string key);
    Reflection::Variant logDurationEventWithTimestamp(std::string key, double timestamp);
    Reflection::Variant logEvent(shared_ptr<const Reflection::ValueTable> config,
        Reflection::Variant data);
    Reflection::Variant logStat(shared_ptr<const Reflection::ValueTable> config,
        Reflection::Variant data, float value);

private:
    struct Record
    {
        std::string kind;
        std::string eventName;
        shared_ptr<const Reflection::ValueTable> config;
        Reflection::Variant data;
        double value;
        std::chrono::steady_clock::time_point recordedAt;
    };

    Reflection::Variant record(std::string kind,
        shared_ptr<const Reflection::ValueTable> config,
        Reflection::Variant data, double value);
    static std::string eventName(const shared_ptr<const Reflection::ValueTable>& config);

    static const std::size_t maxRecords = 2048;
    std::deque<Record> records;
    std::map<std::string, std::chrono::steady_clock::time_point> durationStarts;
};

} // namespace RBX

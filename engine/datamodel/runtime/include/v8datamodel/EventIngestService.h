#pragma once

#include "V8Tree/Service.h"

#include <chrono>
#include <condition_variable>
#include <deque>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace RBX {

extern const char* const sEventIngestService;

class EventIngestService
    : public DescribedNonCreatable<EventIngestService, Instance, sEventIngestService>
    , public Service
{
public:
    struct Event
    {
        enum Delivery
        {
            RbxEvent,
            RbxEventStream,
            Immediate,
            Deferred
        };

        Delivery delivery;
        std::string target;
        std::string context;
        std::string name;
        std::map<std::string, std::string> parameters;
        std::chrono::system_clock::time_point timestamp;
    };

    EventIngestService();
    ~EventIngestService();

    void setRBXEvent(std::string target, std::string eventContext,
        std::string eventName,
        shared_ptr<const Reflection::ValueTable> additionalArgs);
    void setRBXEventStream(std::string target, std::string eventContext,
        std::string eventName,
        shared_ptr<const Reflection::ValueTable> additionalArgs);
    void sendEventImmediately(std::string target, std::string eventContext,
        std::string eventName,
        shared_ptr<const Reflection::ValueTable> additionalArgs);
    void sendEventDeferred(std::string target, std::string eventContext,
        std::string eventName,
        shared_ptr<const Reflection::ValueTable> additionalArgs);
    void releaseRBXEventStream(const std::string& target);
    std::string getSessionId() const;

    void setTransportEnabled(bool value);
    std::size_t pendingEventCount() const;
    std::vector<std::string> drainSerializedEventsForTesting();

private:
    static Event makeEvent(Event::Delivery delivery, std::string target,
        std::string eventContext, std::string eventName,
        const shared_ptr<const Reflection::ValueTable>& additionalArgs);
    static std::string serializeEvent(const Event& event);
    static std::string serializeBatch(const std::vector<Event>& events);

    void submit(Event event);
    void reporterMain();
    void flushDeferred();
    void post(std::vector<Event> events);

    mutable std::mutex mutex;
    std::condition_variable reporterWake;
    std::deque<Event> deferredEvents;
    std::deque<Event> streamEvents;
    std::deque<Event> capturedEvents;
    std::thread reporter;
    bool stopping;
    bool transportEnabled;
};

} // namespace RBX

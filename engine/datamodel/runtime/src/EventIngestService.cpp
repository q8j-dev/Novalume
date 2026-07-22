#include "v8datamodel/EventIngestService.h"

#include "util/Guid.h"
#include "util/Http.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace RBX {

const char* const sEventIngestService = "EventIngestService";

namespace {

const char* const kEventIngestEndpoint = "https://ecsv2.roblox.com/client/pbe";
const std::size_t kMaximumDeferredEvents = 100;
const std::chrono::seconds kDeferredReportingInterval(30);

std::string sessionId()
{
    static const std::string value = [] {
        std::string result;
        Guid::generateStandardGUID(result);
        if (result.size() > 2 && result.front() == '{' && result.back() == '}')
            result = result.substr(1, result.size() - 2);
        return result;
    }();
    return value;
}

std::string variantString(const Reflection::Variant& value)
{
    if (value.isVoid())
        return std::string();
    try
    {
        return value.get<std::string>();
    }
    catch (const std::exception&)
    {
        throw std::runtime_error("EventIngestService additionalArgs values must be scalar");
    }
}

void discardHttpResult(std::string*, std::exception*)
{
}

} // namespace

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<EventIngestService,
    void(std::string, std::string, std::string,
        shared_ptr<const Reflection::ValueTable>)> funcSetRBXEvent(
            &EventIngestService::setRBXEvent, "SetRBXEvent", "target",
            "eventContext", "eventName", "additionalArgs",
            Security::RobloxScript);
static Reflection::BoundFuncDesc<EventIngestService,
    void(std::string, std::string, std::string,
        shared_ptr<const Reflection::ValueTable>)> funcSetRBXEventStream(
            &EventIngestService::setRBXEventStream, "SetRBXEventStream", "target",
            "eventContext", "eventName", "additionalArgs",
            Security::RobloxScript);
static Reflection::BoundFuncDesc<EventIngestService,
    void(std::string, std::string, std::string,
        shared_ptr<const Reflection::ValueTable>)> funcSendEventImmediately(
            &EventIngestService::sendEventImmediately, "SendEventImmediately", "target",
            "eventContext", "eventName", "additionalArgs",
            Security::RobloxScript);
static Reflection::BoundFuncDesc<EventIngestService,
    void(std::string, std::string, std::string,
        shared_ptr<const Reflection::ValueTable>)> funcSendEventDeferred(
            &EventIngestService::sendEventDeferred, "SendEventDeferred", "target",
            "eventContext", "eventName", "additionalArgs",
            Security::RobloxScript);
REFLECTION_END();

EventIngestService::EventIngestService()
    : Service(true)
    , stopping(false)
    , transportEnabled(true)
    , reporter(&EventIngestService::reporterMain, this)
{
    setName(sEventIngestService);
    setRobloxLocked(true);
}

EventIngestService::~EventIngestService()
{
    {
        std::lock_guard<std::mutex> guard(mutex);
        stopping = true;
    }
    reporterWake.notify_one();
    if (reporter.joinable())
        reporter.join();
    flushDeferred();
    releaseRBXEventStream(std::string());
}

EventIngestService::Event EventIngestService::makeEvent(Event::Delivery delivery,
    std::string target, std::string eventContext, std::string eventName,
    const shared_ptr<const Reflection::ValueTable>& additionalArgs)
{
    if (target.empty() || eventContext.empty() || eventName.empty())
        throw std::runtime_error("EventIngestService target, eventContext, and eventName are required");

    Event event;
    event.delivery = delivery;
    event.target = std::move(target);
    event.context = std::move(eventContext);
    event.name = std::move(eventName);
    event.timestamp = std::chrono::system_clock::now();

    if (additionalArgs)
    {
        for (Reflection::ValueTable::const_iterator it = additionalArgs->begin();
             it != additionalArgs->end(); ++it)
        {
            if (it->first == "ctx" || it->first == "evt" || it->first == "lt" ||
                it->first == "target" || it->first == "sessionId")
                throw std::runtime_error("EventIngestService additionalArgs contains a reserved field");
            event.parameters[it->first] = variantString(it->second);
        }
    }

    event.parameters["sessionId"] = sessionId();
    return event;
}

std::string EventIngestService::serializeEvent(const Event& event)
{
    const long long timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        event.timestamp.time_since_epoch()).count();
    std::ostringstream result;
    result << "target=" << Http::urlEncode(event.target)
           << "&ctx=" << Http::urlEncode(event.context)
           << "&evt=" << Http::urlEncode(event.name)
           << "&lt=" << timestamp;
    for (std::map<std::string, std::string>::const_iterator it = event.parameters.begin();
         it != event.parameters.end(); ++it)
        result << '&' << Http::urlEncode(it->first) << '=' << Http::urlEncode(it->second);
    return result.str();
}

std::string EventIngestService::serializeBatch(const std::vector<Event>& events)
{
    std::ostringstream result;
    for (std::size_t i = 0; i < events.size(); ++i)
    {
        if (i)
            result << '\n';
        result << serializeEvent(events[i]);
    }
    return result.str();
}

void EventIngestService::post(std::vector<Event> events)
{
    if (events.empty())
        return;

    bool enabled;
    {
        std::lock_guard<std::mutex> guard(mutex);
        enabled = transportEnabled;
        if (!enabled)
            capturedEvents.insert(capturedEvents.end(), events.begin(), events.end());
    }
    if (!enabled)
        return;

    Http(kEventIngestEndpoint).post(serializeBatch(events), Http::kContentTypeUrlEncoded,
        true, &discardHttpResult, false);
}

void EventIngestService::submit(Event event)
{
    if (event.delivery == Event::RbxEventStream)
    {
        std::lock_guard<std::mutex> guard(mutex);
        streamEvents.push_back(std::move(event));
        return;
    }
    if (event.delivery != Event::Deferred)
    {
        post(std::vector<Event>(1, std::move(event)));
        return;
    }

    bool full;
    {
        std::lock_guard<std::mutex> guard(mutex);
        deferredEvents.push_back(std::move(event));
        full = deferredEvents.size() >= kMaximumDeferredEvents;
    }
    if (full)
        reporterWake.notify_one();
}

void EventIngestService::releaseRBXEventStream(const std::string& target)
{
    std::vector<Event> events;
    {
        std::lock_guard<std::mutex> guard(mutex);
        for (std::deque<Event>::iterator it = streamEvents.begin();
             it != streamEvents.end();)
        {
            if (target.empty() || it->target == target)
            {
                events.push_back(std::move(*it));
                it = streamEvents.erase(it);
            }
            else
                ++it;
        }
    }
    post(std::move(events));
}

std::string EventIngestService::getSessionId() const
{
    return sessionId();
}

void EventIngestService::reporterMain()
{
    std::unique_lock<std::mutex> lock(mutex);
    while (!stopping)
    {
        reporterWake.wait_for(lock, kDeferredReportingInterval, [this] {
            return stopping || deferredEvents.size() >= kMaximumDeferredEvents;
        });
        lock.unlock();
        flushDeferred();
        lock.lock();
    }
}

void EventIngestService::flushDeferred()
{
    std::vector<Event> events;
    {
        std::lock_guard<std::mutex> guard(mutex);
        events.assign(std::make_move_iterator(deferredEvents.begin()),
            std::make_move_iterator(deferredEvents.end()));
        deferredEvents.clear();
    }
    post(std::move(events));
}

void EventIngestService::setRBXEvent(std::string target, std::string eventContext,
    std::string eventName, shared_ptr<const Reflection::ValueTable> additionalArgs)
{
    submit(makeEvent(Event::RbxEvent, std::move(target), std::move(eventContext),
        std::move(eventName), additionalArgs));
}

void EventIngestService::setRBXEventStream(std::string target,
    std::string eventContext, std::string eventName,
    shared_ptr<const Reflection::ValueTable> additionalArgs)
{
    submit(makeEvent(Event::RbxEventStream, std::move(target), std::move(eventContext),
        std::move(eventName), additionalArgs));
}

void EventIngestService::sendEventImmediately(std::string target,
    std::string eventContext, std::string eventName,
    shared_ptr<const Reflection::ValueTable> additionalArgs)
{
    submit(makeEvent(Event::Immediate, std::move(target), std::move(eventContext),
        std::move(eventName), additionalArgs));
}

void EventIngestService::sendEventDeferred(std::string target,
    std::string eventContext, std::string eventName,
    shared_ptr<const Reflection::ValueTable> additionalArgs)
{
    submit(makeEvent(Event::Deferred, std::move(target), std::move(eventContext),
        std::move(eventName), additionalArgs));
}

void EventIngestService::setTransportEnabled(bool value)
{
    std::lock_guard<std::mutex> guard(mutex);
    transportEnabled = value;
}

std::size_t EventIngestService::pendingEventCount() const
{
    std::lock_guard<std::mutex> guard(mutex);
    return deferredEvents.size() + streamEvents.size() + capturedEvents.size();
}

std::vector<std::string> EventIngestService::drainSerializedEventsForTesting()
{
    flushDeferred();
    std::deque<Event> captured;
    {
        std::lock_guard<std::mutex> guard(mutex);
        captured.swap(capturedEvents);
    }
    std::vector<std::string> result;
    result.reserve(captured.size());
    for (std::deque<Event>::const_iterator it = captured.begin(); it != captured.end(); ++it)
        result.push_back(serializeEvent(*it));
    return result;
}

} // namespace RBX

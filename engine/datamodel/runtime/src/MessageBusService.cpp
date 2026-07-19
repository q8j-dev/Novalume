#include "V8DataModel/MessageBusService.h"

#include "Script/ScriptContext.h"
#include "Util/standardout.h"

#include <vector>

namespace RBX {

const char* const sMessageBusService = "MessageBusService";

namespace MessageBusNs {
const char* const sMessageBusConnection = "MessageBusConnection";
}

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<MessageBusNs::Connection, void()> funcDisconnect(
    &MessageBusNs::Connection::disconnect, "Disconnect", Security::RobloxScript);

static Reflection::BoundFuncDesc<MessageBusService,
    Reflection::Variant(std::string, Reflection::Variant)> funcCall(
        &MessageBusService::call, "Call", "key", "input", Security::RobloxScript);
static Reflection::BoundFuncDesc<MessageBusService,
    Reflection::Variant(std::string)> funcGetLast(
        &MessageBusService::getLast, "GetLast", "mid", Security::RobloxScript);
static Reflection::BoundFuncDesc<MessageBusService,
    std::string(std::string, std::string)> funcGetMessageId(
        &MessageBusService::getMessageId, "GetMessageId", "domainName", "messageName",
        Security::RobloxScript);
static Reflection::BoundFuncDesc<MessageBusService,
    std::string(std::string, std::string)> funcGetRequestMessageId(
        &MessageBusService::getProtocolMethodRequestMessageId,
        "GetProtocolMethodRequestMessageId", "protocolName", "methodName",
        Security::RobloxScript);
static Reflection::BoundFuncDesc<MessageBusService,
    std::string(std::string, std::string)> funcGetResponseMessageId(
        &MessageBusService::getProtocolMethodResponseMessageId,
        "GetProtocolMethodResponseMessageId", "protocolName", "methodName",
        Security::RobloxScript);
static Reflection::BoundFuncDesc<MessageBusService,
    void(std::string, Reflection::Variant)> funcPublish(
        &MessageBusService::publish, "Publish", "mid", "params", Security::RobloxScript);
static Reflection::BoundFuncDesc<MessageBusService,
    void(std::string, std::string, Reflection::Variant, Reflection::Variant)>
        funcPublishRequest(&MessageBusService::publishProtocolMethodRequest,
            "PublishProtocolMethodRequest", "protocolName", "methodName", "message",
            "customTelemetryData", Security::RobloxScript);
static Reflection::BoundFuncDesc<MessageBusService,
    void(std::string, std::string, Reflection::Variant, int, Reflection::Variant)>
        funcPublishResponse(&MessageBusService::publishProtocolMethodResponse,
            "PublishProtocolMethodResponse", "protocolName", "methodName", "message",
            "responseCode", "customTelemetryData", Security::RobloxScript);
static Reflection::BoundFuncDesc<MessageBusService,
    void(std::string, std::string, Reflection::Variant, Lua::WeakFunctionRef,
        Reflection::Variant)> funcMakeRequest(
            &MessageBusService::makeRequest, "MakeRequest", "protocolName", "methodName",
            "message", "callback", "customTelemetryData", Security::RobloxScript);
static Reflection::BoundFuncDesc<MessageBusService,
    void(std::string, std::string, Lua::WeakFunctionRef)> funcSetRequestHandler(
        &MessageBusService::setRequestHandler, "SetRequestHandler", "protocolName",
        "methodName", "callback", Security::RobloxScript);
static Reflection::BoundFuncDesc<MessageBusService,
    shared_ptr<Instance>(std::string, Lua::WeakFunctionRef, bool, bool)> funcSubscribe(
        &MessageBusService::subscribe, "Subscribe", "mid", "callback", "once", "sticky",
        Security::RobloxScript);
static Reflection::BoundFuncDesc<MessageBusService,
    shared_ptr<Instance>(std::string, std::string, Lua::WeakFunctionRef, bool, bool)>
        funcSubscribeRequest(&MessageBusService::subscribeToProtocolMethodRequest,
            "SubscribeToProtocolMethodRequest", "protocolName", "methodName", "callback",
            "once", "sticky", Security::RobloxScript);
static Reflection::BoundFuncDesc<MessageBusService,
    shared_ptr<Instance>(std::string, std::string, Lua::WeakFunctionRef, bool, bool)>
        funcSubscribeResponse(&MessageBusService::subscribeToProtocolMethodResponse,
            "SubscribeToProtocolMethodResponse", "protocolName", "methodName", "callback",
            "once", "sticky", Security::RobloxScript);
REFLECTION_END();

MessageBusNs::Connection::Connection(MessageBusService* service, std::size_t id,
    std::string messageId, Lua::WeakFunctionRef callback, bool once)
    : service(service)
    , id(id)
    , messageId(std::move(messageId))
    , callback(callback)
    , once(once)
    , connected(true)
{
    setName(sMessageBusConnection);
    setRobloxLocked(true);
}

void MessageBusNs::Connection::disconnect()
{
    if (!connected)
        return;
    connected = false;
    if (service)
        service->disconnect(id);
    service = NULL;
}

MessageBusService::MessageBusService()
    : Service(true)
    , nextConnectionId(1)
{
    setName(sMessageBusService);
    setRobloxLocked(true);
}

Reflection::Variant MessageBusService::call(std::string key, Reflection::Variant input)
{
    publish(key, input);
    return getLast(key);
}

Reflection::Variant MessageBusService::getLast(std::string messageId)
{
    std::map<std::string, Reflection::Variant>::const_iterator found =
        lastMessages.find(messageId);
    return found == lastMessages.end() ? Reflection::Variant() : found->second;
}

std::string MessageBusService::getMessageId(std::string domainName,
    std::string messageName)
{
    return domainName + "." + messageName;
}

std::string MessageBusService::getProtocolMethodRequestMessageId(
    std::string protocolName, std::string methodName)
{
    return protocolName + "." + methodName + ".Request";
}

std::string MessageBusService::getProtocolMethodResponseMessageId(
    std::string protocolName, std::string methodName)
{
    return protocolName + "." + methodName + ".Response";
}

void MessageBusService::invoke(const shared_ptr<MessageBusNs::Connection>& connection,
    const Reflection::Variant& value)
{
    if (!connection || !connection->connected)
        return;
    if (connection->once)
        connection->disconnect();
    if (!connection->callback.lock())
        return;
    ScriptContext* context = ServiceProvider::create<ScriptContext>(this);
    Reflection::Tuple arguments;
    arguments.values.push_back(value);
    try
    {
        context->callInNewThread(connection->callback, arguments);
    }
    catch (const base_exception& error)
    {
        StandardOut::singleton()->printf(MESSAGE_ERROR,
            "MessageBusService callback failed: %s", error.what());
    }
}

void MessageBusService::publish(std::string messageId, Reflection::Variant params)
{
    lastMessages[messageId] = params;
    std::vector<shared_ptr<MessageBusNs::Connection> > pending;
    for (std::map<std::size_t, weak_ptr<MessageBusNs::Connection> >::iterator iterator =
             connections.begin(); iterator != connections.end(); ++iterator)
    {
        if (shared_ptr<MessageBusNs::Connection> connection = iterator->second.lock())
        {
            if (connection->connected && connection->messageId == messageId)
                pending.push_back(connection);
        }
    }
    for (std::vector<shared_ptr<MessageBusNs::Connection> >::iterator iterator =
             pending.begin(); iterator != pending.end(); ++iterator)
        invoke(*iterator, params);
}

void MessageBusService::publishProtocolMethodRequest(std::string protocolName,
    std::string methodName, Reflection::Variant message, Reflection::Variant)
{
    publish(getProtocolMethodRequestMessageId(protocolName, methodName), message);
}

void MessageBusService::publishProtocolMethodResponse(std::string protocolName,
    std::string methodName, Reflection::Variant message, int, Reflection::Variant)
{
    publish(getProtocolMethodResponseMessageId(protocolName, methodName), message);
}

void MessageBusService::makeRequest(std::string protocolName,
    std::string methodName, Reflection::Variant message,
    Lua::WeakFunctionRef callback, Reflection::Variant customTelemetryData)
{
    subscribeToProtocolMethodResponse(protocolName, methodName, callback, true, false);
    publishProtocolMethodRequest(protocolName, methodName, message, customTelemetryData);
}

void MessageBusService::setRequestHandler(std::string protocolName,
    std::string methodName, Lua::WeakFunctionRef callback)
{
    subscribeToProtocolMethodRequest(protocolName, methodName, callback, false, false);
}

shared_ptr<Instance> MessageBusService::subscribe(std::string messageId,
    Lua::WeakFunctionRef callback, bool once, bool sticky)
{
    shared_ptr<MessageBusNs::Connection> connection(new MessageBusNs::Connection(
        this, nextConnectionId++, messageId, callback, once));
    connections[connection->id] = connection;
    connection->setParent(this);
    if (sticky)
    {
        std::map<std::string, Reflection::Variant>::const_iterator found =
            lastMessages.find(messageId);
        if (found != lastMessages.end())
            invoke(connection, found->second);
    }
    return connection;
}

shared_ptr<Instance> MessageBusService::subscribeToProtocolMethodRequest(
    std::string protocolName, std::string methodName,
    Lua::WeakFunctionRef callback, bool once, bool sticky)
{
    return subscribe(getProtocolMethodRequestMessageId(protocolName, methodName),
        callback, once, sticky);
}

shared_ptr<Instance> MessageBusService::subscribeToProtocolMethodResponse(
    std::string protocolName, std::string methodName,
    Lua::WeakFunctionRef callback, bool once, bool sticky)
{
    return subscribe(getProtocolMethodResponseMessageId(protocolName, methodName),
        callback, once, sticky);
}

void MessageBusService::disconnect(std::size_t id)
{
    connections.erase(id);
}

} // namespace RBX

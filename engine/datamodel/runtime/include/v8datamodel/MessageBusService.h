#pragma once

#include "Script/ThreadRef.h"
#include "V8Tree/Service.h"

#include <cstddef>
#include <map>
#include <string>

namespace RBX {

extern const char* const sMessageBusService;

class MessageBusService;

namespace MessageBusNs {

extern const char* const sMessageBusConnection;

class Connection
    : public DescribedNonCreatable<Connection, Instance, sMessageBusConnection>
{
public:
    Connection(MessageBusService* service, std::size_t id, std::string messageId,
        Lua::WeakFunctionRef callback, bool once);

    void disconnect();
    bool isConnected() const { return connected; }

private:
    friend class RBX::MessageBusService;

    MessageBusService* service;
    std::size_t id;
    std::string messageId;
    Lua::WeakFunctionRef callback;
    bool once;
    bool connected;
};

} // namespace MessageBusNs

class MessageBusService
    : public DescribedNonCreatable<MessageBusService, Instance, sMessageBusService>
    , public Service
{
public:
    MessageBusService();

    Reflection::Variant call(std::string key, Reflection::Variant input);
    Reflection::Variant getLast(std::string messageId);
    std::string getMessageId(std::string domainName, std::string messageName);
    std::string getProtocolMethodRequestMessageId(std::string protocolName,
        std::string methodName);
    std::string getProtocolMethodResponseMessageId(std::string protocolName,
        std::string methodName);

    void publish(std::string messageId, Reflection::Variant params);
    void publishProtocolMethodRequest(std::string protocolName,
        std::string methodName, Reflection::Variant message,
        Reflection::Variant customTelemetryData);
    void publishProtocolMethodResponse(std::string protocolName,
        std::string methodName, Reflection::Variant message,
        int responseCode, Reflection::Variant customTelemetryData);
    void makeRequest(std::string protocolName, std::string methodName,
        Reflection::Variant message, Lua::WeakFunctionRef callback,
        Reflection::Variant customTelemetryData);
    void setRequestHandler(std::string protocolName, std::string methodName,
        Lua::WeakFunctionRef callback);

    shared_ptr<Instance> subscribe(std::string messageId,
        Lua::WeakFunctionRef callback, bool once, bool sticky);
    shared_ptr<Instance> subscribeToProtocolMethodRequest(std::string protocolName,
        std::string methodName, Lua::WeakFunctionRef callback, bool once, bool sticky);
    shared_ptr<Instance> subscribeToProtocolMethodResponse(std::string protocolName,
        std::string methodName, Lua::WeakFunctionRef callback, bool once, bool sticky);

    void disconnect(std::size_t id);

private:
    void invoke(const shared_ptr<MessageBusNs::Connection>& connection,
        const Reflection::Variant& value);

    std::size_t nextConnectionId;
    std::map<std::size_t, weak_ptr<MessageBusNs::Connection> > connections;
    std::map<std::string, Reflection::Variant> lastMessages;
};

} // namespace RBX

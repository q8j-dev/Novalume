#include "v8datamodel/MemStorageService.h"

#include "Script/ScriptContext.h"
#include "util/standardout.h"

#include <vector>

namespace RBX {

const char* const sMemStorageService = "MemStorageService";

namespace MemStorage {
const char* const sMemStorageConnection = "MemStorageConnection";
}

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<MemStorage::Connection, void()> funcDisconnect(
    &MemStorage::Connection::disconnect, "Disconnect", Security::Plugin);
static Reflection::BoundFuncDesc<MemStorageService,
    shared_ptr<Instance>(std::string, Lua::WeakFunctionRef)> funcBind(
        &MemStorageService::bind, "Bind", "key", "callback", Security::RobloxScript);
static Reflection::BoundFuncDesc<MemStorageService,
    shared_ptr<Instance>(std::string, Lua::WeakFunctionRef)> funcBindAndFire(
        &MemStorageService::bindAndFire, "BindAndFire", "key", "callback",
        Security::RobloxScript);
static Reflection::BoundFuncDesc<MemStorageService,
    Reflection::Variant(std::string, Reflection::Variant)> funcCall(
        &MemStorageService::call, "Call", "key", "input", Security::RobloxScript);
static Reflection::BoundFuncDesc<MemStorageService, void(std::string, std::string)>
    funcFire(&MemStorageService::fire, "Fire", "key", "value", std::string(),
        Security::RobloxScript);
static Reflection::BoundFuncDesc<MemStorageService,
    std::string(std::string, std::string)> funcGetItem(
        &MemStorageService::getItem, "GetItem", "key", "defaultValue", std::string(),
        Security::RobloxScript);
static Reflection::BoundFuncDesc<MemStorageService, bool(std::string)> funcHasItem(
    &MemStorageService::hasItem, "HasItem", "key", Security::RobloxScript);
static Reflection::BoundFuncDesc<MemStorageService, bool(std::string)> funcRemoveItem(
    &MemStorageService::removeItem, "RemoveItem", "key", Security::RobloxScript);
static Reflection::BoundFuncDesc<MemStorageService, void(std::string, std::string)>
    funcSetItem(&MemStorageService::setItem, "SetItem", "key", "value", std::string(),
        Security::RobloxScript);
REFLECTION_END();

MemStorage::Connection::Connection(MemStorageService* service, std::size_t id,
    std::string key, Lua::WeakFunctionRef callback)
    : service(service)
    , id(id)
    , key(std::move(key))
    , callback(callback)
    , connected(true)
{
    setName(sMemStorageConnection);
    setRobloxLocked(true);
}

void MemStorage::Connection::disconnect()
{
    if (!connected)
        return;
    connected = false;
    if (service)
        service->disconnect(id);
    service = NULL;
}

MemStorageService::MemStorageService()
    : Service(true)
    , nextConnectionId(1)
{
    setName(sMemStorageService);
    setRobloxLocked(true);
}

shared_ptr<MemStorage::Connection> MemStorageService::createConnection(
    std::string key, Lua::WeakFunctionRef callback)
{
    shared_ptr<MemStorage::Connection> connection(new MemStorage::Connection(
        this, nextConnectionId++, std::move(key), callback));
    connections[connection->id] = connection;
    connection->setParent(this);
    return connection;
}

Reflection::Tuple MemStorageService::invoke(
    const shared_ptr<MemStorage::Connection>& connection,
    const Reflection::Variant& value)
{
    if (!connection || !connection->connected || !connection->callback.lock())
        return Reflection::Tuple();

    Reflection::Tuple arguments;
    arguments.values.push_back(value);
    try
    {
        ScriptContext* context = ServiceProvider::create<ScriptContext>(this);
        return context->callInNewThread(connection->callback, arguments);
    }
    catch (const base_exception& error)
    {
        StandardOut::singleton()->printf(MESSAGE_ERROR,
            "MemStorageService callback failed: %s", error.what());
        return Reflection::Tuple();
    }
}

shared_ptr<Instance> MemStorageService::bind(std::string key,
    Lua::WeakFunctionRef callback)
{
    return createConnection(std::move(key), callback);
}

shared_ptr<Instance> MemStorageService::bindAndFire(std::string key,
    Lua::WeakFunctionRef callback)
{
    shared_ptr<MemStorage::Connection> connection = createConnection(key, callback);
    invoke(connection, Reflection::Variant(getItem(key, std::string())));
    return connection;
}

Reflection::Variant MemStorageService::call(std::string key,
    Reflection::Variant input)
{
    std::vector<shared_ptr<MemStorage::Connection> > pending;
    for (std::map<std::size_t, weak_ptr<MemStorage::Connection> >::iterator iterator =
             connections.begin(); iterator != connections.end(); ++iterator)
    {
        if (shared_ptr<MemStorage::Connection> connection = iterator->second.lock())
            if (connection->connected && connection->key == key)
                pending.push_back(connection);
    }

    Reflection::Variant result;
    for (std::vector<shared_ptr<MemStorage::Connection> >::iterator iterator =
             pending.begin(); iterator != pending.end(); ++iterator)
    {
        Reflection::Tuple values = invoke(*iterator, input);
        if (!values.values.empty())
            result = values.values.front();
    }
    return result;
}

void MemStorageService::fire(std::string key, std::string value)
{
    call(std::move(key), Reflection::Variant(std::move(value)));
}

std::string MemStorageService::getItem(std::string key,
    std::string defaultValue)
{
    std::map<std::string, std::string>::const_iterator found = items.find(key);
    return found == items.end() ? defaultValue : found->second;
}

bool MemStorageService::hasItem(std::string key)
{
    return items.find(key) != items.end();
}

bool MemStorageService::removeItem(std::string key)
{
    return items.erase(key) != 0;
}

void MemStorageService::setItem(std::string key, std::string value)
{
    items[std::move(key)] = std::move(value);
}

void MemStorageService::disconnect(std::size_t id)
{
    connections.erase(id);
}

} // namespace RBX

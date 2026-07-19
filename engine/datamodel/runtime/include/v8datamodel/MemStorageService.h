#pragma once

#include "Script/ThreadRef.h"
#include "V8Tree/Service.h"

#include <cstddef>
#include <map>
#include <string>

namespace RBX {

extern const char* const sMemStorageService;

class MemStorageService;

namespace MemStorage {

extern const char* const sMemStorageConnection;

class Connection
    : public DescribedNonCreatable<Connection, Instance, sMemStorageConnection>
{
public:
    Connection(MemStorageService* service, std::size_t id, std::string key,
        Lua::WeakFunctionRef callback);

    void disconnect();

private:
    friend class RBX::MemStorageService;

    MemStorageService* service;
    std::size_t id;
    std::string key;
    Lua::WeakFunctionRef callback;
    bool connected;
};

} // namespace MemStorage

class MemStorageService
    : public DescribedNonCreatable<MemStorageService, Instance, sMemStorageService>
    , public Service
{
public:
    MemStorageService();

    shared_ptr<Instance> bind(std::string key, Lua::WeakFunctionRef callback);
    shared_ptr<Instance> bindAndFire(std::string key, Lua::WeakFunctionRef callback);
    Reflection::Variant call(std::string key, Reflection::Variant input);
    void fire(std::string key, std::string value);
    std::string getItem(std::string key, std::string defaultValue);
    bool hasItem(std::string key);
    bool removeItem(std::string key);
    void setItem(std::string key, std::string value);

    void disconnect(std::size_t id);

private:
    shared_ptr<MemStorage::Connection> createConnection(std::string key,
        Lua::WeakFunctionRef callback);
    Reflection::Tuple invoke(const shared_ptr<MemStorage::Connection>& connection,
        const Reflection::Variant& value);

    std::size_t nextConnectionId;
    std::map<std::string, std::string> items;
    std::map<std::size_t, weak_ptr<MemStorage::Connection> > connections;
};

} // namespace RBX

#include "V8DataModel/LocalStorageService.h"

#include "Script/ScriptContext.h"
#include "Util/standardout.h"

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <fstream>

namespace RBX {

const char* const sLocalStorageService = "LocalStorageService";
const char* const sAppStorageService = "AppStorageService";
std::filesystem::path LocalStorageService::storageRoot;

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<LocalStorageService, void()> funcFlush(
    &LocalStorageService::flush, "Flush", Security::RobloxScript);
static Reflection::BoundFuncDesc<LocalStorageService, std::string(std::string)> funcGetItem(
    &LocalStorageService::getItem, "GetItem", "key", Security::RobloxScript);
static Reflection::BoundFuncDesc<LocalStorageService,
    void(std::string, std::string)> funcSetItem(
        &LocalStorageService::setItem, "SetItem", "key", "value",
        Security::RobloxScript);
static Reflection::BoundFuncDesc<LocalStorageService,
    void(Lua::WeakFunctionRef)> funcWhenLoaded(
        &LocalStorageService::whenLoaded, "WhenLoaded", "callback",
        Security::RobloxScript);
static Reflection::EventDesc<LocalStorageService,
    void(std::string, std::string)> eventItemWasSet(
        &LocalStorageService::itemWasSet, "ItemWasSet", "key", "value",
        Security::RobloxScript);
static Reflection::EventDesc<LocalStorageService, void()> eventStoreWasCleared(
    &LocalStorageService::storeWasCleared, "StoreWasCleared",
    Security::RobloxScript);
REFLECTION_END();

LocalStorageService::LocalStorageService(std::string fileName)
    : Service(true)
    , fileName(std::move(fileName))
    , loaded(false)
    , dirty(false)
{
    setName(sLocalStorageService);
    setRobloxLocked(true);
    load();
}

void LocalStorageService::setStorageRoot(std::filesystem::path root)
{
    storageRoot = std::move(root);
}

std::filesystem::path LocalStorageService::path() const
{
    return storageRoot.empty() ? std::filesystem::path() : storageRoot / fileName;
}

void LocalStorageService::load()
{
    items.clear();
    const std::filesystem::path source = path();
    if (!source.empty() && std::filesystem::is_regular_file(source))
    {
        try
        {
            boost::property_tree::ptree tree;
            boost::property_tree::read_json(source.string(), tree);
            for (boost::property_tree::ptree::const_iterator iterator = tree.begin();
                 iterator != tree.end(); ++iterator)
                items[iterator->first] = iterator->second.get_value<std::string>();
        }
        catch (const std::exception& error)
        {
            StandardOut::singleton()->printf(MESSAGE_WARNING,
                "LocalStorageService could not load %s: %s",
                source.string().c_str(), error.what());
        }
    }
    loaded = true;
    dirty = false;
}

void LocalStorageService::flush()
{
    if (!dirty)
        return;
    const std::filesystem::path destination = path();
    if (destination.empty())
        throw runtime_error("LocalStorageService has no writable storage root");

    std::filesystem::create_directories(destination.parent_path());
    boost::property_tree::ptree tree;
    for (std::map<std::string, std::string>::const_iterator iterator = items.begin();
         iterator != items.end(); ++iterator)
        tree.put(iterator->first, iterator->second);

    const std::filesystem::path temporary = destination.string() + ".tmp";
    boost::property_tree::write_json(temporary.string(), tree, std::locale(), false);
    std::error_code error;
    std::filesystem::rename(temporary, destination, error);
    if (error)
    {
        std::filesystem::remove(destination, error);
        error.clear();
        std::filesystem::rename(temporary, destination, error);
    }
    if (error)
        throw runtime_error("LocalStorageService failed to replace %s: %s",
            destination.string().c_str(), error.message().c_str());
    dirty = false;
}

std::string LocalStorageService::getItem(std::string key)
{
    std::map<std::string, std::string>::const_iterator found = items.find(key);
    return found == items.end() ? std::string() : found->second;
}

void LocalStorageService::setItem(std::string key, std::string value)
{
    items[key] = value;
    dirty = true;
    itemWasSet(std::move(key), std::move(value));
}

void LocalStorageService::whenLoaded(Lua::WeakFunctionRef callback)
{
    if (!loaded || !callback.lock())
        return;
    Reflection::Tuple arguments;
    ScriptContext* context = ServiceProvider::create<ScriptContext>(this);
    context->callInNewThread(callback, arguments);
}

AppStorageService::AppStorageService()
    : Super("appStorage.json")
{
    setName(sAppStorageService);
}

} // namespace RBX


#include "V8DataModel/CollectionService.h"
#include "V8DataModel/Bindable.h"
#include "V8DataModel/Configuration.h"
#include "V8DataModel/StyleSheet.h"
#include "Script/LuaSignalBridge.h"

namespace RBX
{


const char* const sCollectionService = "CollectionService";

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<CollectionService, shared_ptr<const Instances>(std::string)> func_GetCollection(&CollectionService::getCollection, "GetCollection", "class", Security::None);
static Reflection::BoundFuncDesc<CollectionService, void(shared_ptr<Instance>, std::string)> func_AddTag(&CollectionService::addTag, "AddTag", "instance", "tag", Security::None);
static Reflection::BoundFuncDesc<CollectionService, void(shared_ptr<Instance>, std::string)> func_RemoveTag(&CollectionService::removeTag, "RemoveTag", "instance", "tag", Security::None);
static Reflection::BoundFuncDesc<CollectionService, bool(shared_ptr<Instance>, std::string)> func_HasTag(&CollectionService::hasTag, "HasTag", "instance", "tag", Security::None);
static Reflection::BoundFuncDesc<CollectionService, shared_ptr<const Reflection::ValueArray>(shared_ptr<Instance>)> func_GetTags(&CollectionService::getTags, "GetTags", "instance", Security::None);
static Reflection::BoundFuncDesc<CollectionService, shared_ptr<const Reflection::ValueArray>()> func_GetAllTags(&CollectionService::getAllTags, "GetAllTags", Security::None);
static Reflection::BoundFuncDesc<CollectionService, shared_ptr<const Instances>(std::string)> func_GetTagged(&CollectionService::getTagged, "GetTagged", "tag", Security::None);
static Reflection::CustomBoundFuncDesc<CollectionService, Reflection::Variant(std::string)> func_GetInstanceAddedSignal(&CollectionService::getInstanceAddedSignal, "GetInstanceAddedSignal", "tag", Security::None);
static Reflection::CustomBoundFuncDesc<CollectionService, Reflection::Variant(std::string)> func_GetInstanceRemovedSignal(&CollectionService::getInstanceRemovedSignal, "GetInstanceRemovedSignal", "tag", Security::None);
Reflection::EventDesc<CollectionService, void(shared_ptr<Instance>)> event_collectionItemAdded(&CollectionService::itemAddedSignal, "ItemAdded", "instance");
Reflection::EventDesc<CollectionService, void(shared_ptr<Instance>)> event_collectionItemRemoved(&CollectionService::itemRemovedSignal, "ItemRemoved", "instance");
static Reflection::EventDesc<CollectionService, void(std::string)> event_TagAdded(&CollectionService::tagAddedSignal, "TagAdded", "tag");
static Reflection::EventDesc<CollectionService, void(std::string)> event_TagRemoved(&CollectionService::tagRemovedSignal, "TagRemoved", "tag");
static Reflection::EventDesc<CollectionService, void(std::string, shared_ptr<Instance>)> event_TaggedInstanceAdded(&CollectionService::taggedInstanceAddedSignal, "_TaggedInstanceAdded", "tag", "instance", Security::RobloxScript);
static Reflection::EventDesc<CollectionService, void(std::string, shared_ptr<Instance>)> event_TaggedInstanceRemoved(&CollectionService::taggedInstanceRemovedSignal, "_TaggedInstanceRemoved", "tag", "instance", Security::RobloxScript);
REFLECTION_END();


CollectionService::CollectionService()
	:DescribedNonCreatable<CollectionService, Instance, sCollectionService>(sCollectionService)
{

}

const Reflection::EventDescriptor* CollectionService::getInstanceAddedEventDescriptor() { return &event_TaggedInstanceAdded; }
const Reflection::EventDescriptor* CollectionService::getInstanceRemovedEventDescriptor() { return &event_TaggedInstanceRemoved; }

namespace {

std::string readCollectionTag(lua_State* state)
{
	if (lua_gettop(state) < 2 || !lua_isstring(state, 2))
		throw RBX::runtime_error("Tag must be a string");
	size_t length = 0;
	const char* value = lua_tolstring(state, 2, &length);
	if (length == 0 || length > 100)
		throw RBX::runtime_error("Tag must be between 1 and 100 characters");
	return std::string(value, length);
}

}

int CollectionService::getInstanceAddedSignal(lua_State* state)
{
	const std::string tag = readCollectionTag(state);
	shared_ptr<BindableEvent>& source = instanceAddedSignals[tag];
	if (!source)
		source = Creatable<Instance>::create<BindableEvent>();
	Lua::EventInstance event = { source->findSignalDescriptor("Event"), source, std::string() };
	Lua::EventBridge::pushNewObject(state, event);
	return 1;
}

int CollectionService::getInstanceRemovedSignal(lua_State* state)
{
	const std::string tag = readCollectionTag(state);
	shared_ptr<BindableEvent>& source = instanceRemovedSignals[tag];
	if (!source)
		source = Creatable<Instance>::create<BindableEvent>();
	Lua::EventInstance event = { source->findSignalDescriptor("Event"), source, std::string() };
	Lua::EventBridge::pushNewObject(state, event);
	return 1;
}

void CollectionService::fireTaggedSignal(
	std::map<std::string, shared_ptr<BindableEvent> >& signals,
	const std::string& tag, shared_ptr<Instance> instance)
{
	const std::map<std::string, shared_ptr<BindableEvent> >::iterator found =
		signals.find(tag);
	if (found == signals.end())
		return;
	shared_ptr<Reflection::Tuple> arguments(new Reflection::Tuple());
	arguments->values.push_back(instance);
	found->second->fire(arguments);
}

void CollectionService::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	descendantAddedConnection.disconnect();
	descendantRemovingConnection.disconnect();
	if (newProvider)
	{
		descendantAddedConnection = newProvider->getOrCreateDescendantAddedSignal()->connect(
			boost::bind(&CollectionService::onTaggedDescendantAdded, this, _1));
		descendantRemovingConnection = newProvider->getOrCreateDescendantRemovingSignal()->connect(
			boost::bind(&CollectionService::onTaggedDescendantRemoving, this, _1));
	}
}

bool CollectionService::isInProvider(const Instance* instance)
{
	const ServiceProvider* provider = ServiceProvider::findServiceProvider(instance);
	return provider && provider == getParent();
}

void CollectionService::onTaggedDescendantAdded(shared_ptr<Instance> instance)
{
	for (const std::string& tag : instance->getTagsInternal())
		if (tagged[tag].insert(instance.get()).second)
		{
			taggedInstanceAddedSignal(tag, instance);
			fireTaggedSignal(instanceAddedSignals, tag, instance);
		}
}

void CollectionService::onTaggedDescendantRemoving(shared_ptr<Instance> instance)
{
	for (const std::string& tag : instance->getTagsInternal())
	{
		TaggedMap::iterator found = tagged.find(tag);
		if (found != tagged.end() && found->second.erase(instance.get()))
		{
			taggedInstanceRemovedSignal(tag, instance);
			fireTaggedSignal(instanceRemovedSignals, tag, instance);
		}
	}
}

void CollectionService::addTag(shared_ptr<Instance> instance, std::string tag)
{
	if (!instance || tag.empty() || tag.size() > 100)
		throw runtime_error("CollectionService:AddTag requires an instance and a tag between 1 and 100 characters");
	if (!instance->addTagInternal(tag))
		return;
	if (++tagCounts[tag] == 1)
		tagAddedSignal(tag);
	if (isInProvider(instance.get()) && tagged[tag].insert(instance.get()).second)
	{
		taggedInstanceAddedSignal(tag, instance);
		fireTaggedSignal(instanceAddedSignals, tag, instance);
	}
	applyResolvedStyles(instance.get());
}

void CollectionService::removeTag(shared_ptr<Instance> instance, std::string tag)
{
	if (!instance || !instance->removeTagInternal(tag))
		return;
	TaggedMap::iterator found = tagged.find(tag);
	if (found != tagged.end() && found->second.erase(instance.get()))
	{
		taggedInstanceRemovedSignal(tag, instance);
		fireTaggedSignal(instanceRemovedSignals, tag, instance);
	}
	std::map<std::string, size_t>::iterator count = tagCounts.find(tag);
	if (count != tagCounts.end() && --count->second == 0)
	{
		tagCounts.erase(count);
		tagRemovedSignal(tag);
	}
}

bool CollectionService::hasTag(shared_ptr<Instance> instance, std::string tag)
{
	return instance && instance->hasTagInternal(tag);
}

shared_ptr<const Reflection::ValueArray> CollectionService::getTags(shared_ptr<Instance> instance)
{
	shared_ptr<Reflection::ValueArray> result(new Reflection::ValueArray());
	if (instance)
		for (const std::string& tag : instance->getTagsInternal())
			result->push_back(tag);
	return result;
}

shared_ptr<const Reflection::ValueArray> CollectionService::getAllTags()
{
	shared_ptr<Reflection::ValueArray> result(new Reflection::ValueArray());
	for (const auto& item : tagCounts)
		result->push_back(item.first);
	return result;
}

shared_ptr<const Instances> CollectionService::getTagged(std::string tag)
{
	shared_ptr<Instances> result(new Instances());
	TaggedMap::const_iterator found = tagged.find(tag);
	if (found != tagged.end())
		for (Instance* value : found->second)
			if (value && isInProvider(value))
				result->push_back(shared_from(value));
	return result;
}

shared_ptr<const Instances> CollectionService::getCollection(const Name& className)
{
	return getCollection(className.toString());
}

shared_ptr<const Instances> CollectionService::getCollection(std::string className)
{
	const CollectionMap::iterator iter = collections.find(className);
	if(iter != collections.end()){
		return iter->second->read();
	}
	return shared_ptr<const Instances>();
}

void CollectionService::removeInstance(shared_ptr<Instance> instance)
{
	const std::string& className = instance->getDescriptor().name.toString();

	CollectionMap::iterator collectionIter = collections.find(className);

	RBXASSERT(collectionIter != collections.end());
	Instances::iterator iter;
	shared_ptr<Instances> c(collectionIter->second->write());

	iter = std::find( c->begin(), c->end(), instance);
	if(iter != c->end()){
		// Fast-remove. This can make a huge speed improvement over regular remove
		*iter = c->back();
		c->pop_back();

		itemRemovedSignal(instance);
	}
	else{
		RBXASSERT(0);
	}
}
void CollectionService::addInstance(shared_ptr<Instance> instance)
{
	const std::string& className = instance->getDescriptor().name.toString();

	CollectionMap::iterator collectionIter = collections.find(className);
	if(collectionIter == collections.end()){
		collections[className].reset(new copy_on_write_ptr<Instances>());
	}

	shared_ptr<Instances> c(collections[className]->write());
	c->push_back(instance);

	itemAddedSignal(instance);
}



}

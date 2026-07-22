#pragma once

#include "v8tree/Service.h"
#include <queue>
#include <set>

namespace RBX {

extern const char* const sCollectionService;
class BindableEvent;
class CollectionService
	: public DescribedNonCreatable<CollectionService, Instance, sCollectionService>
	, public Service
{
public:
	CollectionService();

	rbx::signal<void(shared_ptr<Instance>)> itemAddedSignal;
	rbx::signal<void(shared_ptr<Instance>)> itemRemovedSignal;

	shared_ptr<const Instances> getCollection(std::string type);
	shared_ptr<const Instances> getCollection(const Name& className);
	template<class T>
	shared_ptr<const Instances> getCollection()
	{
		return getCollection(T::classDescriptor());
	}

	void removeInstance(shared_ptr<Instance> instance);
	void addInstance(shared_ptr<Instance> instance);
	void addTag(shared_ptr<Instance> instance, std::string tag);
	void removeTag(shared_ptr<Instance> instance, std::string tag);
	bool hasTag(shared_ptr<Instance> instance, std::string tag);
	shared_ptr<const Reflection::ValueArray> getTags(shared_ptr<Instance> instance);
	shared_ptr<const Reflection::ValueArray> getAllTags();
	shared_ptr<const Instances> getTagged(std::string tag);
	int getInstanceAddedSignal(lua_State* state);
	int getInstanceRemovedSignal(lua_State* state);
	static const Reflection::EventDescriptor* getInstanceAddedEventDescriptor();
	static const Reflection::EventDescriptor* getInstanceRemovedEventDescriptor();

	rbx::signal<void(std::string)> tagAddedSignal;
	rbx::signal<void(std::string)> tagRemovedSignal;
	rbx::signal<void(std::string, shared_ptr<Instance>)> taggedInstanceAddedSignal;
	rbx::signal<void(std::string, shared_ptr<Instance>)> taggedInstanceRemovedSignal;

protected:
	void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider) override;

private:
	// TODO: Lookup by const RBX::Name*
	typedef std::map<std::string, shared_ptr<copy_on_write_ptr<Instances> > > CollectionMap;
	CollectionMap collections;
	typedef std::map<std::string, std::set<Instance*> > TaggedMap;
	TaggedMap tagged;
	std::map<std::string, size_t> tagCounts;
	std::map<std::string, shared_ptr<BindableEvent> > instanceAddedSignals;
	std::map<std::string, shared_ptr<BindableEvent> > instanceRemovedSignals;
	rbx::signals::scoped_connection descendantAddedConnection;
	rbx::signals::scoped_connection descendantRemovingConnection;

	void onTaggedDescendantAdded(shared_ptr<Instance> instance);
	void onTaggedDescendantRemoving(shared_ptr<Instance> instance);
	void fireTaggedSignal(std::map<std::string, shared_ptr<BindableEvent> >& signals,
		const std::string& tag, shared_ptr<Instance> instance);
	bool isInProvider(const Instance* instance);

};

}

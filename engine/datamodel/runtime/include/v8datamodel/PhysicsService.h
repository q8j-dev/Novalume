#pragma once

#include "v8tree/Service.h"
#include "v8world/SendPhysics.h"
#include "util/ConcurrencyValidator.h"
#include "rbx/Intrusive/Set.h"
#include "v8datamodel/PartInstance.h"
#include "v8datamodel/Workspace.h"
#include "util/BinaryString.h"

#include <array>
#include <string>

namespace RBX {

	extern const char *const sPhysicsService;

	class PhysicsService 
		: public DescribedNonCreatable<PhysicsService, Instance, sPhysicsService>
		, public Service
		, public HeartbeatInstance
	{
	private:
		typedef DescribedNonCreatable<PhysicsService, Instance, sPhysicsService> Super;

	public:
		typedef RBX::Intrusive::Set< PartInstance, PhysicsService > Parts;
		typedef Parts::Iterator PartsIt;

	private:

        bool iAmServer;

		Parts parts;

		typedef boost::unordered_set<TouchPair> PartPairs;
		PartPairs touchesSendList;		// used by physics senders, swaps with receive list
		PartPairs touchesReceiveList;	// from physics receiver and world
		rbx::signals::scoped_connection touchesConnection;
		rbx::atomic<int> touchSentCounter;
		int touchResetCount;
		int touchSendListId;

		// used to determine number of physics senders
		rbx::signals::connection playersChangedConnection;

		ConcurrencyValidator concurrencyValidator;

		static const unsigned int kMaxCollisionGroups = 32;
		std::array<std::string, kMaxCollisionGroups> collisionGroupNames;
		std::array<unsigned int, kMaxCollisionGroups> collisionGroupMasks;
		std::array<bool, kMaxCollisionGroups> collisionGroupRegistered;

		unsigned int requireCollisionGroup(const std::string& name) const;
		void refreshWorldCollisionGroups();
		void validateCollisionGroupName(const std::string& name) const;

		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

		rbx::signals::connection assemblyPhysicsOnConnection;
		rbx::signals::connection assemblyPhysicsOffConnection;
		void onAssemblyPhysicsOn(Primitive* primitive);		// added by engine
		void onAssemblyPhysicsOff(Primitive* primitive);

	public:
		PhysicsService();
		~PhysicsService();

		void registerCollisionGroup(std::string name);
		int createCollisionGroup(std::string name);
		void unregisterCollisionGroup(std::string name);
		void renameCollisionGroup(std::string from, std::string to);
		bool isCollisionGroupRegistered(std::string name);
		int getCollisionGroupId(std::string name);
		std::string getCollisionGroupName(int id);
		int getMaxCollisionGroups() { return kMaxCollisionGroups; }
		void setCollisionGroupsCollidable(std::string first, std::string second, bool collidable);
		bool collisionGroupsAreCollidable(std::string first, std::string second);
		shared_ptr<const Reflection::ValueArray> getRegisteredCollisionGroups();
		shared_ptr<const Reflection::ValueArray> getCollisionGroups();
		void setPartCollisionGroup(shared_ptr<Instance> instance, std::string name);
		bool collisionGroupContainsPart(std::string name, shared_ptr<Instance> instance);
		unsigned int getCollisionGroupMask(unsigned int id) const;
		BinaryString getCollisionGroupData() const;
		void setCollisionGroupData(const BinaryString& data);

		// Interface
		rbx::signal<void(shared_ptr<Instance>)> assemblyAddingSignal;				// state change scripts			
		rbx::signal<void(shared_ptr<Instance>)> assemblyRemovedSignal;			// state change scripts

		int numSenders() {
			return parts.size();
		}

		PartsIt begin() {
			return parts.begin();
		}

		PartsIt end() {
			return parts.end();
		}

		/*override*/ void onHeartbeat(const Heartbeat& event);
 
		void onTouchStep(const TouchPair& other);
		size_t pendingTouchCount() { return touchesSendList.size(); }
		int getTouchesId() { return touchSendListId; }
		void getTouches(std::list<TouchPair>& out);
		void onTouchesSent();

		void onPlayersChanged(Instance::CombinedSignalType type, const ICombinedSignalData* data);
	};

} // namespace

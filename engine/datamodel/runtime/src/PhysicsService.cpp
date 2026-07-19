/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#include "V8DataModel/PhysicsService.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/Workspace.h"
#include "V8World/World.h"
#include "V8World/Assembly.h"
#include "Network/Players.h"
#include "rbx/Debug.h"
#include "reflection/Function.h"

#include <algorithm>
#include <set>
#include <vector>

namespace RBX {

const char *const sPhysicsService = "PhysicsService";

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<PhysicsService, void(std::string)>
	physics_RegisterCollisionGroup(&PhysicsService::registerCollisionGroup, "RegisterCollisionGroup", "name", Security::None);
static Reflection::BoundFuncDesc<PhysicsService, int(std::string)>
	physics_CreateCollisionGroup(&PhysicsService::createCollisionGroup, "CreateCollisionGroup", "name", Security::None);
static Reflection::BoundFuncDesc<PhysicsService, void(std::string)>
	physics_UnregisterCollisionGroup(&PhysicsService::unregisterCollisionGroup, "UnregisterCollisionGroup", "name", Security::None);
static Reflection::BoundFuncDesc<PhysicsService, void(std::string)>
	physics_RemoveCollisionGroup(&PhysicsService::unregisterCollisionGroup, "RemoveCollisionGroup", "name", Security::None);
static Reflection::BoundFuncDesc<PhysicsService, void(std::string, std::string)>
	physics_RenameCollisionGroup(&PhysicsService::renameCollisionGroup, "RenameCollisionGroup", "from", "to", Security::None);
static Reflection::BoundFuncDesc<PhysicsService, bool(std::string)>
	physics_IsCollisionGroupRegistered(&PhysicsService::isCollisionGroupRegistered, "IsCollisionGroupRegistered", "name", Security::None);
static Reflection::BoundFuncDesc<PhysicsService, int(std::string)>
	physics_GetCollisionGroupId(&PhysicsService::getCollisionGroupId, "GetCollisionGroupId", "name", Security::None);
static Reflection::BoundFuncDesc<PhysicsService, std::string(int)>
	physics_GetCollisionGroupName(&PhysicsService::getCollisionGroupName, "GetCollisionGroupName", "name", Security::None);
static Reflection::BoundFuncDesc<PhysicsService, int()>
	physics_GetMaxCollisionGroups(&PhysicsService::getMaxCollisionGroups, "GetMaxCollisionGroups", Security::None);
static Reflection::BoundFuncDesc<PhysicsService, void(std::string, std::string, bool)>
	physics_CollisionGroupSetCollidable(&PhysicsService::setCollisionGroupsCollidable, "CollisionGroupSetCollidable", "name1", "name2", "collidable", Security::None);
static Reflection::BoundFuncDesc<PhysicsService, bool(std::string, std::string)>
	physics_CollisionGroupsAreCollidable(&PhysicsService::collisionGroupsAreCollidable, "CollisionGroupsAreCollidable", "name1", "name2", Security::None);
static Reflection::BoundFuncDesc<PhysicsService, shared_ptr<const Reflection::ValueArray>()>
	physics_GetRegisteredCollisionGroups(&PhysicsService::getRegisteredCollisionGroups, "GetRegisteredCollisionGroups", Security::None);
static Reflection::BoundFuncDesc<PhysicsService, shared_ptr<const Reflection::ValueArray>()>
	physics_GetCollisionGroups(&PhysicsService::getCollisionGroups, "GetCollisionGroups", Security::None);
static Reflection::BoundFuncDesc<PhysicsService, void(shared_ptr<Instance>, std::string)>
	physics_SetPartCollisionGroup(&PhysicsService::setPartCollisionGroup, "SetPartCollisionGroup", "part", "name", Security::None);
static Reflection::BoundFuncDesc<PhysicsService, bool(std::string, shared_ptr<Instance>)>
	physics_CollisionGroupContainsPart(&PhysicsService::collisionGroupContainsPart, "CollisionGroupContainsPart", "name", "part", Security::None);
REFLECTION_END();

PhysicsService::PhysicsService()
	: iAmServer(false)
	, touchResetCount(1)
	, touchSendListId(0)
{
	setName("PhysicsService");
	collisionGroupNames.fill(std::string());
	collisionGroupMasks.fill(0xffffffffu);
	collisionGroupRegistered.fill(false);
	collisionGroupNames[0] = "Default";
	collisionGroupRegistered[0] = true;
}

void PhysicsService::validateCollisionGroupName(const std::string& name) const
{
	if (name.empty())
		throw runtime_error("Collision group name must not be empty");
	if (name.size() > 100)
		throw runtime_error("Collision group name must not exceed 100 characters");
}

bool PhysicsService::isCollisionGroupRegistered(std::string name)
{
	for (unsigned int id = 0; id < kMaxCollisionGroups; ++id)
		if (collisionGroupRegistered[id] && collisionGroupNames[id] == name)
			return true;
	return false;
}

unsigned int PhysicsService::requireCollisionGroup(const std::string& name) const
{
	for (unsigned int id = 0; id < kMaxCollisionGroups; ++id)
		if (collisionGroupRegistered[id] && collisionGroupNames[id] == name)
			return id;
	throw runtime_error("Collision group '%s' is not registered", name.c_str());
}

int PhysicsService::getCollisionGroupId(std::string name)
{
	return static_cast<int>(requireCollisionGroup(name));
}

std::string PhysicsService::getCollisionGroupName(int id)
{
	if (id < 0 || id >= static_cast<int>(kMaxCollisionGroups) ||
		!collisionGroupRegistered[id])
		throw runtime_error("Collision group id %d is not registered", id);
	return collisionGroupNames[id];
}

unsigned int PhysicsService::getCollisionGroupMask(unsigned int id) const
{
	if (id >= kMaxCollisionGroups || !collisionGroupRegistered[id])
		throw runtime_error("Collision group id %u is not registered", id);
	return collisionGroupMasks[id];
}

BinaryString PhysicsService::getCollisionGroupData() const
{
	std::string result;
	result.reserve(2 + kMaxCollisionGroups * 8);
	result.push_back(static_cast<char>(1));

	unsigned int count = 0;
	for (unsigned int id = 0; id < kMaxCollisionGroups; ++id)
		if (collisionGroupRegistered[id])
			++count;
	result.push_back(static_cast<char>(count));

	for (unsigned int id = 0; id < kMaxCollisionGroups; ++id)
	{
		if (!collisionGroupRegistered[id])
			continue;
		result.push_back(static_cast<char>(id));
		result.push_back(static_cast<char>(4));
		const unsigned int mask = collisionGroupMasks[id];
		for (unsigned int byte = 0; byte < 4; ++byte)
			result.push_back(static_cast<char>((mask >> (byte * 8)) & 0xff));
		const std::string& name = collisionGroupNames[id];
		result.push_back(static_cast<char>(name.size()));
		result.append(name);
	}
	return BinaryString(result);
}

void PhysicsService::setCollisionGroupData(const BinaryString& data)
{
	const std::string& bytes = data.value();
	std::size_t offset = 0;
	const std::size_t size = bytes.size();
	const unsigned int maskWidth = 4;
	if (size < 2)
		throw runtime_error("CollisionGroupData is truncated");

	const unsigned int version = static_cast<unsigned char>(bytes[offset++]);
	if (version != 1)
		throw runtime_error("CollisionGroupData version %u is unsupported", version);
	const unsigned int count = static_cast<unsigned char>(bytes[offset++]);
	if (count == 0 || count > kMaxCollisionGroups)
		throw runtime_error("CollisionGroupData contains invalid group count %u", count);

	std::array<std::string, kMaxCollisionGroups> names;
	std::array<unsigned int, kMaxCollisionGroups> masks;
	std::array<bool, kMaxCollisionGroups> registered;
	names.fill(std::string());
	masks.fill(0xffffffffu);
	registered.fill(false);
	std::set<std::string> uniqueNames;

	for (unsigned int group = 0; group < count; ++group)
	{
		if (offset + 2 > size)
			throw runtime_error("CollisionGroupData is truncated before group %u", group);
		const unsigned int id = static_cast<unsigned char>(bytes[offset++]);
		const unsigned int encodedMaskWidth = static_cast<unsigned char>(bytes[offset++]);
		if (id >= kMaxCollisionGroups || registered[id])
			throw runtime_error("CollisionGroupData contains invalid or duplicate id %u", id);
		if (encodedMaskWidth != maskWidth)
			throw runtime_error("CollisionGroupData group %u has invalid mask width %u", id, encodedMaskWidth);
		if (offset + maskWidth + 1 > size)
			throw runtime_error("CollisionGroupData is truncated in group %u", id);

		unsigned int mask = 0;
		for (unsigned int byte = 0; byte < maskWidth; ++byte)
			mask |= static_cast<unsigned int>(static_cast<unsigned char>(bytes[offset++])) << (byte * 8);
		const unsigned int nameLength = static_cast<unsigned char>(bytes[offset++]);
		if (nameLength == 0 || nameLength > 100 || offset + nameLength > size)
			throw runtime_error("CollisionGroupData group %u has an invalid name length", id);
		const std::string name = bytes.substr(offset, nameLength);
		offset += nameLength;
		validateCollisionGroupName(name);
		if (!uniqueNames.insert(name).second)
			throw runtime_error("CollisionGroupData contains duplicate name '%s'", name.c_str());

		registered[id] = true;
		names[id] = name;
		masks[id] = mask;
	}

	if (offset != size)
		throw runtime_error("CollisionGroupData contains %u trailing bytes", static_cast<unsigned int>(size - offset));
	if (!registered[0] || names[0] != "Default")
		throw runtime_error("CollisionGroupData must define Default as group 0");
	for (unsigned int first = 0; first < kMaxCollisionGroups; ++first)
	{
		if (!registered[first])
			continue;
		for (unsigned int second = 0; second < kMaxCollisionGroups; ++second)
		{
			if (!registered[second])
				continue;
			const bool forward = (masks[first] & (1u << second)) != 0;
			const bool reverse = (masks[second] & (1u << first)) != 0;
			if (forward != reverse)
				throw runtime_error("CollisionGroupData matrix is asymmetric for groups %u and %u", first, second);
		}
	}

	std::vector<std::pair<shared_ptr<PartInstance>, std::string> > existingParts;
	if (Instance* root = getRootAncestor())
	{
		shared_ptr<const Instances> descendants = root->getDescendants();
		for (Instances::const_iterator it = descendants->begin(); it != descendants->end(); ++it)
		{
			if (PartInstance* part = Instance::fastDynamicCast<PartInstance>(it->get()))
				existingParts.push_back(std::make_pair(shared_from<PartInstance>(part), part->getCollisionGroup()));
		}
	}

	collisionGroupNames = names;
	collisionGroupMasks = masks;
	collisionGroupRegistered = registered;
	for (std::vector<std::pair<shared_ptr<PartInstance>, std::string> >::iterator it = existingParts.begin();
		it != existingParts.end(); ++it)
	{
		const std::string& previousName = it->second;
		it->first->setCollisionGroup(isCollisionGroupRegistered(previousName) ? previousName : "Default");
	}
}

void PhysicsService::registerCollisionGroup(std::string name)
{
	validateCollisionGroupName(name);
	if (isCollisionGroupRegistered(name))
		throw runtime_error("Collision group '%s' is already registered", name.c_str());
	for (unsigned int id = 1; id < kMaxCollisionGroups; ++id)
	{
		if (!collisionGroupRegistered[id])
		{
			collisionGroupRegistered[id] = true;
			collisionGroupNames[id] = name;
			collisionGroupMasks[id] = 0xffffffffu;
			refreshWorldCollisionGroups();
			return;
		}
	}
	throw runtime_error("The maximum of %u collision groups has been reached", kMaxCollisionGroups);
}

int PhysicsService::createCollisionGroup(std::string name)
{
	registerCollisionGroup(name);
	return getCollisionGroupId(name);
}

void PhysicsService::unregisterCollisionGroup(std::string name)
{
	const unsigned int id = requireCollisionGroup(name);
	if (id == 0)
		throw runtime_error("The Default collision group cannot be unregistered");
	if (Instance* root = getRootAncestor())
	{
		shared_ptr<const Instances> descendants = root->getDescendants();
		for (Instances::const_iterator it = descendants->begin(); it != descendants->end(); ++it)
		{
			PartInstance* part = Instance::fastDynamicCast<PartInstance>(it->get());
			if (part && static_cast<unsigned int>(part->getCollisionGroupId()) == id)
				part->setCollisionGroup("Default");
		}
	}
	for (unsigned int other = 0; other < kMaxCollisionGroups; ++other)
		collisionGroupMasks[other] |= (1u << id);
	collisionGroupNames[id].clear();
	collisionGroupMasks[id] = 0xffffffffu;
	collisionGroupRegistered[id] = false;
	refreshWorldCollisionGroups();
}

void PhysicsService::renameCollisionGroup(std::string from, std::string to)
{
	validateCollisionGroupName(to);
	const unsigned int id = requireCollisionGroup(from);
	if (id == 0)
		throw runtime_error("The Default collision group cannot be renamed");
	if (isCollisionGroupRegistered(to))
		throw runtime_error("Collision group '%s' is already registered", to.c_str());
	collisionGroupNames[id] = to;
	refreshWorldCollisionGroups();
}

void PhysicsService::setCollisionGroupsCollidable(
	std::string first, std::string second, bool collidable)
{
	const unsigned int firstId = requireCollisionGroup(first);
	const unsigned int secondId = requireCollisionGroup(second);
	const unsigned int firstBit = 1u << firstId;
	const unsigned int secondBit = 1u << secondId;
	if (collidable)
	{
		collisionGroupMasks[firstId] |= secondBit;
		collisionGroupMasks[secondId] |= firstBit;
	}
	else
	{
		collisionGroupMasks[firstId] &= ~secondBit;
		collisionGroupMasks[secondId] &= ~firstBit;
	}
	refreshWorldCollisionGroups();
}

bool PhysicsService::collisionGroupsAreCollidable(
	std::string first, std::string second)
{
	const unsigned int firstId = requireCollisionGroup(first);
	const unsigned int secondId = requireCollisionGroup(second);
	return (collisionGroupMasks[firstId] & (1u << secondId)) != 0 &&
		(collisionGroupMasks[secondId] & (1u << firstId)) != 0;
}

shared_ptr<const Reflection::ValueArray> PhysicsService::getRegisteredCollisionGroups()
{
	shared_ptr<Reflection::ValueArray> result(new Reflection::ValueArray());
	for (unsigned int id = 0; id < kMaxCollisionGroups; ++id)
	{
		if (!collisionGroupRegistered[id])
			continue;
		shared_ptr<Reflection::ValueTable> group(new Reflection::ValueTable());
		(*group)["name"] = collisionGroupNames[id];
		(*group)["mask"] = static_cast<int>(collisionGroupMasks[id]);
		result->push_back(shared_ptr<const Reflection::ValueTable>(group));
	}
	return result;
}

shared_ptr<const Reflection::ValueArray> PhysicsService::getCollisionGroups()
{
	return getRegisteredCollisionGroups();
}

void PhysicsService::setPartCollisionGroup(
	shared_ptr<Instance> instance, std::string name)
{
	PartInstance* part = Instance::fastDynamicCast<PartInstance>(instance.get());
	if (!part)
		throw runtime_error("part must be a BasePart");
	part->setCollisionGroup(name);
}

bool PhysicsService::collisionGroupContainsPart(
	std::string name, shared_ptr<Instance> instance)
{
	PartInstance* part = Instance::fastDynamicCast<PartInstance>(instance.get());
	if (!part)
		throw runtime_error("part must be a BasePart");
	return static_cast<unsigned int>(part->getCollisionGroupId()) == requireCollisionGroup(name);
}

void PhysicsService::refreshWorldCollisionGroups()
{
	Instance* root = getRootAncestor();
	if (!root)
		return;
	shared_ptr<const Instances> descendants = root->getDescendants();
	for (Instances::const_iterator it = descendants->begin(); it != descendants->end(); ++it)
	{
		PartInstance* part = Instance::fastDynamicCast<PartInstance>(it->get());
		if (!part)
			continue;
		const unsigned int id = static_cast<unsigned int>(part->getCollisionGroupId());
		if (id < kMaxCollisionGroups && collisionGroupRegistered[id])
			part->setCollisionGroupInternal(id, collisionGroupMasks[id]);
	}
}

PhysicsService::~PhysicsService()
{
	RBXASSERT(parts.empty());
	RBXASSERT(!assemblyPhysicsOnConnection.connected());
	RBXASSERT(!assemblyPhysicsOffConnection.connected());
}


void PhysicsService::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	assemblyPhysicsOnConnection.disconnect();
	assemblyPhysicsOffConnection.disconnect();
	
	touchesConnection.disconnect();
	playersChangedConnection.disconnect();

	Super::onServiceProvider(oldProvider, newProvider);
	
	onServiceProviderHeartbeatInstance(oldProvider, newProvider);		// hooks up heartbeat

	if (Workspace* w = ServiceProvider::find<Workspace>(newProvider)) 
	{

		touchesConnection = w->stepTouch.connect(boost::bind(&PhysicsService::onTouchStep, this, _1));
		Network::Players* players = ServiceProvider::find<Network::Players>(this);
		playersChangedConnection = players->combinedSignal.connect(boost::bind(&PhysicsService::onPlayersChanged, this, _1, _2));

		SendPhysics* sendPhysics = w->getWorld()->getSendPhysics();
		if (sendPhysics)
		{
			assemblyPhysicsOnConnection = sendPhysics->assemblyPhysicsOnSignal.connect(boost::bind(&PhysicsService::onAssemblyPhysicsOn, this, _1));
			assemblyPhysicsOffConnection = sendPhysics->assemblyPhysicsOffSignal.connect(boost::bind(&PhysicsService::onAssemblyPhysicsOff, this, _1));
		}
	}
}


void PhysicsService::onAssemblyPhysicsOn(Primitive* primitive)
{
	WriteValidator writeValidator(concurrencyValidator);

	RBXASSERT(primitive->getWorld());		// confirm the Part is in Workspace / primitive is in World
	PartInstance* part = PartInstance::fromPrimitive(primitive);

	RBXASSERT(Assembly::isAssemblyRootPrimitive(part->getPartPrimitive()));

	// force update assembly radius so farther usage is not dirty
	primitive->getAssembly()->computeMaxRadius();

	shared_ptr<PartInstance> sharedPart = shared_from<PartInstance>(part);
	assemblyAddingSignal(sharedPart);

	RBXASSERT(!part->PhysicsServiceHook::is_linked());
	parts.insert(*part);

	if (!iAmServer)
	{
		iAmServer = Workspace::serverIsPresent(this);
	}
	if (iAmServer)
	{
		part->addMovementNode(part->getCoordinateFrame(), part->getVelocity(), Time::nowFast());
	}
}



void PhysicsService::onAssemblyPhysicsOff(Primitive* primitive)
{
	WriteValidator writeValidator(concurrencyValidator);

	RBXASSERT(primitive->getWorld());		// confirm the Part is in Workspace / primitive is in World
	PartInstance* part = PartInstance::fromPrimitive(primitive);

	RBXASSERT(Assembly::isAssemblyRootPrimitive(part->getPartPrimitive()));

	RBXASSERT(part->PhysicsServiceHook::is_linked());
	parts.remove_element(*part);

	assemblyRemovedSignal(shared_from<PartInstance>(part));
}

void PhysicsService::onHeartbeat(const Heartbeat& event)
{	
	if ((touchesSendList.empty() && !touchesReceiveList.empty()) || touchSentCounter >= touchResetCount)
	{
		touchesSendList.clear();
		touchesSendList.swap(touchesReceiveList);
		touchSentCounter = 0;
		touchSendListId++;
	}
}

void PhysicsService::onTouchStep(const TouchPair& tp)
{
	touchesReceiveList.insert(tp);
}

void PhysicsService::getTouches(std::list<TouchPair>& out)
{
	out.insert(out.end(), touchesSendList.begin(), touchesSendList.end());
}

void PhysicsService::onTouchesSent()
{
	touchSentCounter++;
}

void PhysicsService::onPlayersChanged(Instance::CombinedSignalType type, const ICombinedSignalData* data)
{
	if (Network::Players::backendProcessing(this))
	{
		if (type == Instance::CHILD_ADDED || type == Instance::CHILD_REMOVED)
			touchResetCount = Network::Players::getPlayerCount(this);
	}
}

} // namespace RBX

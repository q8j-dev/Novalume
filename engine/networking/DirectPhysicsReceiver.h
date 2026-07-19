#pragma once

#include "network/NetworkTypes.h"

#include "network/PacketBuffer.h"

#include "PhysicsReceiver.h"
#include "ReplicatorStats.h"

namespace RBX { 

	namespace Network {

	class DirectPhysicsReceiver : public PhysicsReceiver
	{
	private:
		MechanismItem tempItem;

	public:
		DirectPhysicsReceiver(Replicator* replicator, bool isServer): PhysicsReceiver(replicator, isServer) {}
		virtual void receivePacket(RBX::Network::PacketBuffer& bitsream, NetworkTime timeStamp, ReplicatorStats::PhysicsReceiverStats* stats);
	};

} }
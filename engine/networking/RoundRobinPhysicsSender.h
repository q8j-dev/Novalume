#pragma once

#include "network/NetworkTypes.h"

#include "network/PacketBuffer.h"

#include "PhysicsSender.h"
#include "V8World/SimJob.h"
#include "rbx/rbxTime.h"

namespace RBX { 

	namespace Network {

	class RoundRobinPhysicsSender : public PhysicsSender
	{
	private:
		class JobSender;
		friend class JobSender;

		SimJobTracker jobStagePos;
		Time lastCharacterSendTime;
		
		Time lastBufferCheckTime;

		const SimJob* findTargetPlayerCharacterSimJob();

    protected:

	public:
		RoundRobinPhysicsSender(Replicator& replicator);

		virtual int sendPacket(int maxPackets, PacketPriority packetPriority,
			ReplicatorStats::PhysicsSenderStats* stats);
		
		virtual void step();
		void sendPhysicsData(RBX::Network::PacketBuffer& bitStream, const Assembly* assembly);
	};

} }
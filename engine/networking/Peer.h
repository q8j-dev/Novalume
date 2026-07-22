#pragma once

#include "network/PacketBuffer.h"

#include "PacketIds.h"
#include "Streaming.h"
#include "NetworkSettings.h"
#include "Item.h"
#include "network/api.h"
#include "network/NetworkTypes.h"
#include "v8tree/Instance.h"
#include "util/RunStateOwner.h"
#include "util/Region2.h"
#include "v8datamodel/DataModelJob.h"
#include "queue"

namespace RBX { 

namespace Network {

	class ConcurrentPeer;
	class Replicator;
	class PacketReceiveJob;

	// Client and Server descend from this class
	extern const char* const sPeer;
	class Peer 
		: public Reflection::Described<Peer, sPeer, Instance >
		, public PacketHandler
	{
	private:
		typedef Reflection::Described<Peer, sPeer, Instance > Super;
		shared_ptr<PacketReceiveJob> receiveJob;
	public:
		RunningAverageDutyCycle<> networkDutyCycle;
		boost::shared_ptr<ConcurrentPeer> networkPeer;
		void setOutgoingKBPSLimit(int limit);

		void encryptDataPart(RBX::Network::PacketBuffer& bitStream);
		static void decryptDataPart(RBX::Network::PacketBuffer& bitStream);

	protected:
		
		int protocolVersion;	// default network protocol, shared between client and server
		
		Peer();
		~Peer();
		virtual void onCreatePeer();
		bool askAddChild(const Instance* instance) const;
		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);
	};


} }

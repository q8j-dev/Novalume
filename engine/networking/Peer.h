#pragma once

#include "network/PacketBuffer.h"

#include "PacketIds.h"
#include "Streaming.h"
#include "NetworkSettings.h"
#include "Item.h"
#include "Network/api.h"
#include "network/NetworkTypes.h"
#include "V8Tree/Instance.h"
#include "Util/RunStateOwner.h"
#include "Util/Region2.h"
#include "V8DataModel/DataModelJob.h"
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

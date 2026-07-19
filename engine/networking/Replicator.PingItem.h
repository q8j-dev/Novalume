#pragma once

#include "network/NetworkTypes.h"

#include "network/PacketBuffer.h"

#include "Item.h"
#include "Replicator.h"

#include "network/NetworkTypes.h"

namespace RBX {
namespace Network {

class DeserializedPingItem : public DeserializedItem
{
public:
	bool pingBack;
	NetworkTime time;
	unsigned int sendStats;
	unsigned int extraStats;

	DeserializedPingItem();
	~DeserializedPingItem() {}

	/*implement*/ void process(Replicator& replicator);
};


class Replicator::PingItem : public PooledItem
{
	NetworkTime time;
    unsigned int extraStats;
public:
	PingItem(Replicator* replicator, NetworkTime time, unsigned int extraStats);

	/*implement*/ virtual bool write(RBX::Network::PacketBuffer& bitStream);
	static shared_ptr<DeserializedItem> read(Replicator& replicator, RBX::Network::PacketBuffer& bitStream);
};

}
}

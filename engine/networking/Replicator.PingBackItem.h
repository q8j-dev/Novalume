#pragma once

#include "network/NetworkTypes.h"

#include "network/PacketBuffer.h"

#include "Item.h"
#include "Replicator.h"

#include "network/NetworkTypes.h"

namespace RBX {
namespace Network {

class Replicator::PingBackItem : public PooledItem
{
	NetworkTime time;
    unsigned int extraStats;
public:
	PingBackItem(Replicator* replicator, NetworkTime time, unsigned int extraStats);

	/*implement*/ virtual bool write(RBX::Network::PacketBuffer& bitStream);
};

}
}

#pragma once

#include "Replicator.h"

#include "network/PacketBuffer.h"

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

namespace RBX {
namespace Network {

class ConcurrentPeer;
class Item;

class Replicator::ItemSender : boost::noncopyable
{
	Replicator& replicator;
	ConcurrentPeer* networkPeer;
	shared_ptr<RBX::Network::PacketBuffer> bitStream;
	PacketPriority packetPriority;

	void openPacket();
	void closePacket();
	const unsigned int maxStreamSize;
public:
	bool sentItems;
	ItemSender(Replicator& replicator, ConcurrentPeer *networkPeer);
	~ItemSender();

	typedef enum {SEND_BITSTREAM_FULL = 0, SEND_OK} SendStatus;

	SendStatus send(Item& item);
	int getNumberOfBytesUsed() const;
};

}
}
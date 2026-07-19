#include "Replicator.MarkerItem.h"

#include "Item.h"
#include "Replicator.h"
#include "Util.h"
#include "Replicator.StreamJob.h"

#include "network/PacketBuffer.h"
#include <string>

DYNAMIC_LOGGROUP(NetworkJoin)

namespace RBX {
namespace Network {

Replicator::MarkerItem::MarkerItem(Replicator* replicator, int id)
		: Item(*replicator), id(id) 
	{}

bool Replicator::MarkerItem::write(RBX::Network::PacketBuffer& bitStream) {
	if(!replicator.isInitialDataSent())
		return false;

	writeItemType(bitStream, ItemTypeMarker);

	bitStream << id;
	if (replicator.settings().printInstances) {
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE, 
		"Replication: Sending marker %d to %s", 
		id, 
		peerAddressToString(replicator.remotePlayerId).c_str()
		);
	}

	replicator.onSentMarker(id);
	FASTLOG1F(DFLog::NetworkJoin, "MarkerItem %ld sent", id);

	return true;
}

shared_ptr<DeserializedItem> Replicator::MarkerItem::read(Replicator& replicator, RBX::Network::PacketBuffer& bitStream)
{
	shared_ptr<DeserializedMarkerItem> deserializedData(new DeserializedMarkerItem());

	bitStream >> deserializedData->id;

	if (replicator.settings().printInstances) {
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE,
			"Received marker %d from %s", deserializedData->id,
			peerAddressToString(replicator.remotePlayerId).c_str());
	}

	return deserializedData;
}

}
}

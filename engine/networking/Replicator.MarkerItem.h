#pragma once

#include "network/PacketBuffer.h"

#include "Item.h"
#include "Replicator.h"

namespace RBX {
namespace Network {

class DeserializedMarkerItem : public DeserializedItem
{
public:
	int id;

	DeserializedMarkerItem() { type = Item::ItemTypeMarker; }
	~DeserializedMarkerItem() {}
	/*implement*/ void process(Replicator& replicator) { replicator.readMarkerItem(this); }
};

class Replicator::MarkerItem : public Item
{
	int id;
public:

	MarkerItem(Replicator* replicator, int id);

	/*implement*/virtual bool write(RBX::Network::PacketBuffer& bitStream);
	
	static shared_ptr<DeserializedItem> read(Replicator& replicator, RBX::Network::PacketBuffer& bitStream);
};

}
}

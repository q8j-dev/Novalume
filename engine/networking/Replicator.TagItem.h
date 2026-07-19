#pragma once

#include "network/PacketBuffer.h"

#include "Item.h"
#include "Replicator.h"

namespace RBX { namespace Network {

class DeserializedTagItem : public DeserializedItem
{
public:
	int id;

	DeserializedTagItem();
	~DeserializedTagItem() {}

	/*implement*/ void process(Replicator& replicator);
};

class Replicator::TagItem : public Item
{
	int id;
	boost::function<bool()> readyCallback;
public:
	TagItem(Replicator* replicator, int id, boost::function<bool()> readyCallback);

	/*implement*/ virtual bool write(RBX::Network::PacketBuffer& bitStream);
	static shared_ptr<DeserializedItem> read(Replicator& replicator, RBX::Network::PacketBuffer& bitStream);
};

	
}}

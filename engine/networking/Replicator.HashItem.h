#pragma once

#include "network/PacketBuffer.h"

#include "Item.h"
#include "Replicator.h"
#include "util/ProgramMemoryChecker.h"

namespace RBX {
    namespace Network {

        class Replicator::HashItem : public Item
        {
            PmcHashContainer hashes;
            unsigned long long fuzzyToken;
            unsigned long long apiToken;
            unsigned long long prevApiToken;
        public:
            HashItem(Replicator* replicator, const PmcHashContainer* const hashes, unsigned long long fuzzyToken,
                unsigned long long apiToken, unsigned long long prevApiToken);

            /*implement*/ virtual bool write(RBX::Network::PacketBuffer& bitStream);
        };

    }
}

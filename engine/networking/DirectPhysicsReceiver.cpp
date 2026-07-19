/* Copyright 2003-2008 ROBLOX Corporation, All Rights Reserved */

#include "DirectPhysicsReceiver.h"
#include "Compressor.h"
#include "network/NetworkTypes.h"
#include "NetworkSettings.h"

#include "Streaming.h"
#include "Replicator.h"

#include "V8DataModel/PartInstance.h"
#include "V8DataModel/Workspace.h"

#include "V8World/World.h"
#include "V8World/Primitive.h"
#include "V8World/Assembly.h"
#include "util/standardout.h"

#include <boost/utility/enable_if.hpp>
#include <boost/type_traits.hpp>
#include "NetworkProfiler.h"

DYNAMIC_FASTFLAG(PhysicsSenderUseOwnerTimestamp)
DYNAMIC_FASTFLAG(CleanUpInterpolationTimestamps)
SYNCHRONIZED_FASTFLAG(PhysicsPacketSendWorldStepTimestamp)

using namespace RBX;
using namespace RBX::Network;

void DirectPhysicsReceiver::receivePacket(RBX::Network::PacketBuffer& inBitstream, NetworkTime timeStamp, ReplicatorStats::PhysicsReceiverStats* _stats)
{
	NetworkTime interpolationTimestamp;
	RBX::RemoteTime remoteSendTime;
	if (SFFlag::getPhysicsPacketSendWorldStepTimestamp())
	{
		inBitstream >> interpolationTimestamp;
		remoteSendTime = ((double)interpolationTimestamp / 1000.0f);
	}
	else
	{
		remoteSendTime = ((double)timeStamp / 1000.0f);
	}

	stats = _stats;

	NetworkTime now = GetTime(); 	

	//RBXASSERT(now >= timeStamp); // Defect filed: DE6810
	if (!SFFlag::getPhysicsPacketSendWorldStepTimestamp() && DFFlag::PhysicsSenderUseOwnerTimestamp && timeStamp > now)
	{
		timeStamp = now;
	}
	
	NetworkTime deltaTime = SFFlag::getPhysicsPacketSendWorldStepTimestamp() ? now - interpolationTimestamp : now - timeStamp;//

	// local RBX time when the packet was sent
	RBX::Time localTime = replicator->networkTimeToLocalTime(timeStamp);

	while (true)
	{
		shared_ptr<PartInstance> part;

		if (replicator->isStreamingEnabled())
		{
			bool done;
			inBitstream >> done;
			if (done)
				break; // packet ended

			bool cframeOnly;
			inBitstream >> cframeOnly;
			if (cframeOnly)
			{
				receiveMechanismCFrames(inBitstream, timeStamp, remoteSendTime);
				continue;
			}
			else if (!receiveRootPart(part, inBitstream)) {
				continue; // mechanism ended
			}
		}
		else  // non-streaming
		{
			if (!receiveRootPart(part, inBitstream)) {
				break; // packet ended
			}
			BOOST_STATIC_ASSERT(sizeof(NetworkTime) == sizeof(part->networkTime));
		}

		// read mechanism if non-streaming OR (streaming and not cframe only)
		if (part) 
		{
            if (!iAmServer && localTime < part->getLastUpdateTime())
            {
                // old packet, discard
                if (replicator->settings().printPhysicsErrors) {
                    RBX::StandardOut::singleton()->print(RBX::MESSAGE_INFO, "Discard old packet");
                }
                part.reset();
            }

			if (part)
			{
                NetworkTime& partTime = part->networkTime;
                if (partTime > (SFFlag::getPhysicsPacketSendWorldStepTimestamp() ? interpolationTimestamp : timeStamp) ) {
                    if (replicator->settings().printPhysicsErrors) {
                        RBX::StandardOut::singleton()->print(RBX::MESSAGE_INFO, "Physics-in old packet");
                    }
                    part.reset();
                }
                else 
				{
					if (SFFlag::getPhysicsPacketSendWorldStepTimestamp())
					{
						// Store the adjusted physics timestamp used by the network protocol.
						partTime = interpolationTimestamp;
					}
					else
					{
						partTime = timeStamp;
					}
                }
            }
		}

		RBX::Network::BitCount characterBits = inBitstream.GetReadOffset();
        int numNodesInHistory;
		receiveMechanism(inBitstream, part.get(), tempItem, remoteSendTime, numNodesInHistory);
		if (stats && part && "Torso" == part->getName()) {
			stats->details.characterAnim.increment();
			stats->details.characterAnimSize.sample((inBitstream.GetReadOffset() - characterBits) / 8);
		}

        if (part) {
            setPhysics(tempItem, remoteSendTime, deltaTime, numNodesInHistory);

            if (!iAmServer)
            {
				if (DFFlag::CleanUpInterpolationTimestamps)
				{
					// Interpolation Delay has been re-purposed for use with relation to NetworkPing
					// Interpolation Delay depends greatly on NetworkPing and Frequency of Updates.
					part->setInterpolationDelay( (double)deltaTime/1000 );
				}
				else
				{
					part->setInterpolationDelay((localTime - part->getLastUpdateTime()).seconds());
				}
                part->setLastUpdateTime(localTime);
            }
        }
	}
}

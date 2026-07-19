#include "Peer.h"

#include "Replicator.h"
#include "network/NetworkTypes.h"
#include "ConcurrentPeer.h"
#include "rbx/Log.h"
#include "rbx/Debug.h"
#include "V8DataModel/Stats.h"
#include "V8DataModel/DataModelJob.h"
#include "V8DataModel/PhysicsService.h"
#include "V8DataModel/DataModel.h"
#include "util/ObscureValue.h"
#include "util/standardout.h"

const char* const RBX::Network::sPeer = "NetworkPeer";

namespace RBX {
	namespace Network {

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<Peer, void(int)> func_SetOutgoingKBPSLimit(&Peer::setOutgoingKBPSLimit, "SetOutgoingKBPSLimit", "limit", Security::Plugin);
REFLECTION_END();

static double lerp = 0.05;

Peer::Peer()
	:networkDutyCycle(lerp)
{
	protocolVersion = NETWORK_PROTOCOL_VERSION;
}

Peer::~Peer()
{
}

void Peer::onCreatePeer()
{
	networkPeer->rawPeer()->SetOccasionalPing(true);
}

void Peer::encryptDataPart(RBX::Network::PacketBuffer& bitStream)
{
	// GameNetworkingSockets authenticates and encrypts the complete transport payload.
	(void)bitStream;
}


void Peer::decryptDataPart(RBX::Network::PacketBuffer& inBitstream)
{
	(void)inBitstream;
}

bool Peer::askAddChild(const Instance* instance) const
{
	return Instance::fastDynamicCast<Replicator>(instance)!=NULL;
}

class PeerStatsItem : public Stats::Item
{
	Peer* peer;
	Stats::Item* networkRate;
	Stats::Item* networkActivity;
	Stats::Item* physicsSenders;
	Stats::Item* bufferHealth;

public:
	PeerStatsItem(Peer* peer):peer(peer)
	{
		Stats::Item* item = createChildItem("Packets Thread");	// TODO: Rename this when possible. Unfortunately, Game service requires this to be here
		networkRate = item->createChildItem("Rate");
		networkActivity = item->createChildItem("Activity");
		physicsSenders = item->createChildItem("Physics Senders");	// TODO: More this out of "Packets Thread" when we refactor Game Service
		bufferHealth = item->createChildItem("Send Buffer Health");
	}

	/* override */ void update()
	{
		networkActivity->formatPercent(peer->networkDutyCycle.dutyCycle());
		double rate = peer->networkDutyCycle.rate();
		networkRate->formatValue(rate, "%.2g/s", rate);

		PhysicsService* physicsService = ServiceProvider::find<PhysicsService>(this);
		physicsSenders->formatValue(physicsService ? physicsService->numSenders() : 0);

		double health = peer->networkPeer->GetBufferHealth();
		bufferHealth->formatValue(health, "%.4g", health);
	}
};


class PacketReceiveJob : public DataModelJob
{
	weak_ptr<DataModel> dataModel;
	ObscureValue<double> receiveRate;
public:
	weak_ptr<ConcurrentPeer> networkPeer;
	PacketReceiveJob(shared_ptr<ConcurrentPeer> networkPeer, DataModel* dataModel)
		:DataModelJob("Net PacketReceive", DataModelJob::DataIn, false, shared_from(dataModel), Time::Interval(0))
		,networkPeer(networkPeer)
		,dataModel(shared_from(dataModel))
		,receiveRate(NetworkSettings::singleton().getReceiveRate())
	{
		// Transport polling must continue while the DataModel is otherwise idle;
		// the cyclic executive can stop scheduling a sleeping receive job after
		// the join burst, leaving GNS datagrams unread indefinitely.
		cyclicExecutive = false;
	}

private:
	Time::Interval sleepTime(const Stats& stats)
	{
		return computeStandardSleepTime(stats, receiveRate);
	}

	virtual Job::Error error(const Stats& stats)
	{
		if (TaskScheduler::singleton().isCyclicExecutive() && cyclicExecutive)
			return computeStandardErrorCyclicExecutiveSleeping(stats, receiveRate);
		return computeStandardError(stats, receiveRate);
	}


	virtual TaskScheduler::StepResult stepDataModelJob(const Stats& stats) 
	{
		if (shared_ptr<DataModel> safeDataModel = dataModel.lock())
		{
			FASTLOG1(FLog::DataModelJobs, "Packet receive start, data model: %p", safeDataModel.get());
			DataModel::scoped_write_request request(safeDataModel.get());

			if(shared_ptr<ConcurrentPeer> safeNetworkPeer = networkPeer.lock())
				while (Packet* packet = safeNetworkPeer->rawPeer()->Receive())
					safeNetworkPeer->DeallocatePacket(packet);

			FASTLOG1(FLog::DataModelJobs, "Packet receive finish, data model: %p", safeDataModel.get());
			return TaskScheduler::Stepped;
		}
		return TaskScheduler::Done;
	}
};


void Peer::setOutgoingKBPSLimit(int limit)
{
	if (limit<=0)
		networkPeer->rawPeer()->SetPerConnectionOutgoingBandwidthLimit(0);
	else
		networkPeer->rawPeer()->SetPerConnectionOutgoingBandwidthLimit(1000 * G3D::iClamp(limit, 10, 10000));
}

void Peer::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	RBX::Stats::StatsService* stats = ServiceProvider::find<RBX::Stats::StatsService>(oldProvider);
	if (stats)
	{ 
		shared_ptr<Stats::Item> network = shared_from_polymorphic_downcast<Stats::Item>(stats->findFirstChildByName("Network"));
		if (network)
			network->setParent(NULL);
	}

	if (receiveJob)
	{
		receiveJob->networkPeer.reset();	// Make sure it doesn't try to run from now on. (The concurrency is such that it isn't running now)
		TaskScheduler::singleton().remove(receiveJob);
		receiveJob.reset();
	}

	// Ensure that all Replicators are removed, because they 
	// point to networkPeer, which is about to be deleted
	this->removeAllChildren();	

	if (networkPeer)
	{
		networkPeer->rawPeer()->DetachPlugin(this);
		networkPeer.reset();
	}

	Super::onServiceProvider(oldProvider, newProvider);
	
	if (newProvider)
	{
		networkPeer.reset(new ConcurrentPeer(boost::polymorphic_downcast<DataModel*>(newProvider)));
		networkPeer->rawPeer()->AttachPlugin(this);
		//networkPeer->rawPeer()->SetMTUSize(1400);
		onCreatePeer();

		receiveJob = shared_ptr<PacketReceiveJob>(new PacketReceiveJob(networkPeer, boost::polymorphic_downcast<DataModel*>(newProvider)));
		TaskScheduler::singleton().add(receiveJob);
	}

	stats = ServiceProvider::find<RBX::Stats::StatsService>(newProvider);
	if (stats)
	{
		RBXASSERT(!shared_from_polymorphic_downcast<Stats::Item>(stats->findFirstChildByName("Network")));
		shared_ptr<Stats::Item> network = Creatable<Instance>::create<PeerStatsItem>(this);
		network->setName("Network");
		network->setParent(stats);
	}
}

}}

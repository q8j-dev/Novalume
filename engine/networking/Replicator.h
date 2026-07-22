#pragma once

#include "network/NetworkTypes.h"

#include "network/PacketBuffer.h"

#include "ClusterUpdateBuffer.h"
#include "ConcurrentPeer.h"
#include "Item.h"
#include "NetworkSettings.h"
#include "PacketIds.h"
#include "ReplicatorStats.h"
#include "Streaming.h"
#include "Network/api.h"
#include "network/NetworkTypes.h"
#include "V8Tree/Instance.h"
#include "Util/ClusterCellIterator.h"
#include "util/StreamRegion.h"
#include "Util/RunStateOwner.h"
#include "Util/Region2.h"
#include "Util/SystemAddress.h"
#include "Util/Memory.h"
#include "Util/IMetric.h"
#include "Util/ProtectedString.h"
#include "V8DataModel/DataModelJob.h"
#include "Voxel/CellChangeListener.h"
#include "voxel2/GridListener.h"
#include "queue"

#include <boost/unordered_set.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/unordered_map.hpp>
#include <boost/pool/pool_alloc.hpp>
#include <boost/optional.hpp>

#define NETWORK_PROTOCOL_VERSION_MIN 24 // Release 0.157
#define NETWORK_PROTOCOL_VERSION 34

#define REPLICATED_FIRST_FINISHED_TAG 12
#define TOP_REPLICATION_CONTAINER_FINISHED_TAG 13

struct ReplicatorTestWrapper;

namespace RBX { 

class PartInstance;
class SimJob;
class Assembly;
class MegaClusterInstance;

namespace Stats {
	class StatsService;
}

namespace Network {

class ConcurrentPeer;
class Players;
class Player;
class PhysicsSender;
class PhysicsReceiver;
class Marker;
class InstancePacketCache;
class ClusterPacketCache;
class OneQuarterClusterPacketCache;
class StrictNetworkFilter;

struct PropValuePair;
class DeserializedChangePropertyItem;
class DeserializedDeleteInstanceItem;
class DeserializedEventInvocationItem;
class DeserializedNewInstanceItem;
class DeserializedJoinDataItem;
class DeserializedPingItem;
class DeserializedMarkerItem;

struct DeserializedPacket
{
	Packet* rawPacket;
	std::vector<shared_ptr<DeserializedItem> > deserializedItems;

	DeserializedPacket() {}
	DeserializedPacket(Packet* pkt) : rawPacket(pkt) {}
};

extern const char* const sReplicator;
class Replicator
	: public RBX::DescribedNonCreatable<Replicator, IdSerializer, sReplicator, Reflection::ClassDescriptor::INTERNAL_LOCAL>
	, public PacketHandler
	, public RBX::IMetric
	, public Voxel::CellChangeListener
	, public Voxel2::GridListener
{
	class ItemSender;

	friend class DeserializedChangePropertyItem;
	friend class DeserializedDeleteInstanceItem;
	friend class DeserializedEventInvocationItem;
	friend class DeserializedNewInstanceItem;
	friend class DeserializedJoinDataItem;
	friend class DeserializedPingItem;
	friend class DeserializedMarkerItem;
	friend class DeserializedTouchItem;

public:
	friend struct ::ReplicatorTestWrapper; //testing

    enum PropertyCacheType
    {
        PropertyCacheType_All,
        PropertyCacheType_NonCacheable,
        PropertyCacheType_Cacheable
    };

	enum DisconnectReason
	{
		DisconnectReason_BadHash,
		DisconnectReason_SecurityKeyMismatch,
		DisconnectReason_ProtocolMismatch,
		DisconnectReason_ReceivePacketError,
		DisconnectReason_ReceivePacketStreamError,
		DisconnectReason_SendPacketError,
		DisconnectReason_IllegalTeleport,
		DisconnectReason_DuplicatePlayer,
		DisconnectReason_DuplicateTicket,
		DisconnectReason_TimeOut,
		DisconnectReason_LuaKick,
		DisconnectReason_OnRemoteSysStats,
        DisconnectReason_HashTimeOut,
		DisconnectReason_CloudEditKick
	};

	// --- ReplicationData and ClusterReplicationData are visible for testing ---
	// The dictionary of objects that are involved in replication
	// The Replicator listens to property changed and child added events
	struct ReplicationData
	{
		shared_ptr<Instance> instance;
		rbx::signals::connection connection;
		bool deleteOnDisconnect;
		bool replicateChildren : 1;
		bool listenToChanges : 1;
	};
	struct ClusterReplicationData
	{
		static const int kUpdateChunkSizeLog2 = 2;
		static const int kUpdateChunkVolume = 1 << (kUpdateChunkSizeLog2 * 3);

		ClusterReplicationData()
		{
			readyToSendChunks = false;
		}

		rbx::signals::connection parentConnection;

		ClusterUpdateBuffer updateBuffer;
		boost::unordered_set<Vector3int32> updateBufferSmooth;

        ClusterChunksIterator initialSendIterator;
		std::vector<SpatialRegion::Id> initialChunkIterator;
		std::vector<StreamRegion::Id> initialSendSmooth;

		bool readyToSendChunks;

		bool hasDataToSend()
		{
			return updateBuffer.size() > 0 || initialSendIterator.size() > 0 || initialChunkIterator.size() > 0 || initialSendSmooth.size() > 0 || updateBufferSmooth.size() > 0;
		}
	};

	boost::shared_ptr<ConcurrentPeer> networkPeer;
	PeerAddress const remotePlayerId;
    Instance* removingInstance; // The currently deleting Instance
    ReplicatorStats replicatorStats;

	// ServerReplicator and Client implement these
	virtual bool checkDistributedReceive(PartInstance* part) = 0;
	virtual bool checkDistributedSend(const PartInstance* part) = 0;
	virtual bool checkDistributedSendFast(const PartInstance* part) = 0;
    virtual void readPlayerSimulationRegion(const PartInstance* playerHead, Region2::WeightedPoint& weightedPoint){RBXASSERT(false);}

	size_t incomingPacketsCount() const;
	double incomingPacketsCountHeadWaitTimeSec(const RBX::Time& timeNow);

	// Call this function to request that the Replicator disconnect.
	// This function will submit a task to set Parent to null.
	// The reason for the delay is that some context are unsafe
	// to set Parent to NULL while the transport is dispatching callbacks
	void requestDisconnect(DisconnectReason reason);
	void requestDisconnectWithSignal(DisconnectReason reason);

	unsigned int getApproximateSizeOfPendingClusterDeltas() const { return approximateSizeOfPendingClusterDeltas; }
	size_t getAdjustedMtuSize() const;
	size_t getPhysicsMtuSize() const;

	// Used for debugging
	void disableProcessPackets();
	void enableProcessPackets();
	virtual void setPropSyncExpiration(double value) = 0;

	Replicator(
		PeerAddress remotePlayerId, 
		boost::shared_ptr<ConcurrentPeer> networkPeer,
		NetworkSettings* networkSettings,
		bool clusterDebounceEnabled);

	~Replicator();

	double getRakOffsetTime() { return rakTimeOffset; }

	// Utility routines
	virtual Player* findTargetPlayer() const = 0;
	virtual Player* getRemotePlayer() const = 0;
	shared_ptr<Instance> getPlayer();
	bool isInstanceAChildOfClientsCharacterModel(const Instance* testInstance) const;

	static Reflection::BoundProp<int> prop_maxDataModelSendBuffer;
	rbx::signal<void(std::string, bool)> disconnectionSignal;

	virtual PluginReceiveResult OnReceive(Packet *packet);

	virtual void OnInternalPacket(InternalPacket *internalPacket, unsigned frameNumber, PeerAddress remoteSystemAddress, NetworkTimeMS time, int isSend);

	virtual const RBX::Name& getClassName() const {
		return Name::getNullName();
	}

	int getPort() const;
	std::string getIpAddress() const;

	shared_ptr<Instance> sendMarker();
	
	bool isSerializePending(const Instance* instance) const;
	bool isPropertyChangedPending(const RBX::Reflection::ConstProperty& property) const;
	bool hasPendingNewInstances() const;

	virtual void requestCharacter() { throw std::runtime_error(""); }
	virtual void requestInstances() { throw std::runtime_error(""); } // overwrite in client replicator
	void closeConnection();

	bool processNextIncomingPacket();
	const std::string& getLastPacketError() const { return lastPacketError; }
	int getLastDisconnectReason() const { return lastDisconnectReason; }
	virtual void postProcessPacket(){} // overwrite in client replicator

    std::string getNetworkStatsString(int verbosityLevel);
	const ReplicatorStats& stats() const { return replicatorStats; }
    ReplicatorStats::PhysicsSenderStats& physicsSenderStats();
	int getBufferCountAvailable(int bufferLimit, PacketPriority priority);

	bool isInitialDataSent();

	std::string getMetric(const std::string& metric) const;
	double getMetricValue(const std::string& metric) const;

	virtual void addTopReplicationContainers(ServiceProvider* newProvider);

	static bool isTopContainer(const Instance* instance);

	inline const NetworkSettings& settings() const { return *networkSettings; }

	virtual bool canUseProtocolVersion(int replicationProtocolVersion) const = 0;

	// Part streaming
	bool isInStreamedRegions(const Extents& ext) const;
	bool isAreaInStreamedRadius(const Vector3& center, float radius) const;
	bool isStreamingEnabled() const { return streamingEnabled; }

    bool isReplicationContainer(const Instance* instance) const;

    virtual bool isProtocolCompatible() const {return true;}

    static void teachDictionaries(const Replicator* rep, RBX::Network::PacketBuffer& bitStream, bool teachSchema, bool toBeCompressed);

    virtual void sendDictionaries();

    virtual bool isServerReplicator() {return false;}

    void sendDisconnectionSignal(std::string peer, bool lostConnection);

	Time getSendQueueHeadTime() const { return pendingItems.head_time(); }

	RBX::Time remoteNetworkTimeToLocalTime(const RemoteTime& time);
	RBX::Time networkTimeToLocalTime(const NetworkTime& time);
	NetworkTime localTimeToNetworkTime(const RBX::Time& time);

protected:
	class ChangePropertyItem;
	class DeleteInstanceItem;
	class EventInvocationItem;
	class JoinDataItem;
	class MarkerItem;
	class TagItem;
	class NewInstanceItem;
	class ReferencePropertyChangedItem;
	class StatsItem;
	class ProcessPacketsJob;
	class PingBackItem;
	class PingItem;
#if defined(RBX_ENABLE_LEGACY_X86_CLIENT_SECURITY)
	class HashItem;
#endif
#if defined(RBX_ENABLE_LEGACY_X86_CLIENT_SECURITY) || defined(RBX_RCC_SECURITY)
	class RockyItem;
#endif
	class PingJob;
	class SendDataJob;
	class StreamJob;
	class Stats;
	class SendStatsJob;

#if (defined(RBX_ENABLE_LEGACY_X86_CLIENT_SECURITY) || defined(RBX_RCC_SECURITY)) && !defined(RBX_STUDIO_BUILD)
	class NetPmcResponseItem;
	class RockyDbgItem;
#endif

#if defined(RBX_RCC_SECURITY)
	class NetPmcChallengeItem;
#endif

	// Item pools
	boost::scoped_ptr<RBX::AutoMemPool> newInstancePool;
	boost::scoped_ptr<RBX::AutoMemPool> deleteInstancePool;
	boost::scoped_ptr<RBX::AutoMemPool> changePropertyPool;
	boost::scoped_ptr<RBX::AutoMemPool> eventInvocationPool;
	boost::scoped_ptr<RBX::AutoMemPool> pingPool;
	boost::scoped_ptr<RBX::AutoMemPool> pingBackPool;
	boost::scoped_ptr<RBX::AutoMemPool> referencePropertyChangedPool;

	DescriptorDictionary<Reflection::ClassDescriptor> classDictionary;
	DescriptorDictionary<Reflection::PropertyDescriptor> propDictionary;
	DescriptorDictionary<Reflection::EventDescriptor> eventDictionary;
	DescriptorDictionary<Reflection::Type> typeDictionary;

	boost::scoped_ptr<StrictNetworkFilter> strictFilter;

	// Replication
	typedef boost::unordered_map<const Instance*, ReplicationData> RepConts;
	RepConts replicationContainers;

    MegaClusterInstance* megaClusterInstance;
	ClusterReplicationData clusterReplicationData;

	const bool clusterDebounceEnabled;
	bool clusterDebounce;

	// HACK FOR SINGLE CLUSTER
	bool isClusterSpotted;
	bool isOkToSendClusters;
	unsigned int approximateSizeOfPendingClusterDeltas;

	shared_ptr<InstancePacketCache> instancePacketCache;
	shared_ptr<OneQuarterClusterPacketCache> oneQuarterClusterPacketCache; // used by streaming
	shared_ptr<ClusterPacketCache> clusterPacketCache;

	// An ordered list of the top-most replication containers
	typedef std::list<Instance*> TopReplConts;
	TopReplConts topReplicationContainers;	// Instances whose children should be replicated
	typedef boost::unordered_map<const Reflection::ClassDescriptor*, TopReplConts::iterator> TopReplContsMap;
	TopReplContsMap topReplicationContainersMap; // fast lookup into top replication containers list

	Players* players;

    boost::shared_ptr<PhysicsReceiver> physicsReceiver;

	// Jobs
	shared_ptr<SendDataJob> sendDataJob;
	shared_ptr<PingJob> pingJob;
	shared_ptr<ProcessPacketsJob> processPacketsJob;
	shared_ptr<Replicator::StreamJob> streamJob;

	class SendClusterJob;
	shared_ptr<SendClusterJob> sendClusterJob;

	// Data-out
	ItemQueue pendingItems;
	ItemQueue highPriorityPendingItems;
	int numItemsLeftFromPrevStep;

	typedef boost::unordered_set<RBX::Reflection::ConstProperty, boost::hash<RBX::Reflection::ConstProperty>, 
		std::equal_to<RBX::Reflection::ConstProperty>, boost::fast_pool_allocator<RBX::Reflection::ConstProperty> > PendingPropertyChanges;
	PendingPropertyChanges pendingChangedPropertyItems;

	typedef boost::unordered_set<const Instance*> PendingInstances;			// note - now watching for concurrency issues here
	mutable RBX::mutex pendingInstancesMutex;
	PendingInstances pendingNewInstances;
	const Instance* serializingInstance;

	// Data-in
	const Reflection::Property* deserializingProperty;	// The currently deserializing Event (if any)
	const Reflection::EventInvocation* deserializingEventInvocation;	// The currently deserializing EventInvocation (if any)

	// Data-in/out
	typedef std::map< const RBX::Name*, shared_ptr<Instance> > DefaultObjects;
	DefaultObjects defaultObjects;  // Used for stripping out default properties (compression)

	// Note: This queue doesn't need a concurrency guard, but we need the timestamping feature
	rbx::timestamped_safe_queue<Packet*> incomingPackets;

	bool processAllPacketsPerStep;	// determine if ProcessPacketsJob should budget each step

	void pushIncomingPacket(Packet* packet);
	void clearIncomingPackets();
	std::queue<shared_ptr<Marker> > incomingMarkers;

	NetworkSettings* networkSettings;

    SharedDictionary<RBX::SystemAddress> systemAddressDictionary;
	SharedDictionary<RBX::ContentId> contentIdDictionary;

	// Part Streaming
    bool streamingEnabled;

    // protocol sync
    bool protocolSyncEnabled;
    bool apiDictionaryCompression;

    bool connected;
    bool enableHashCheckBypass;
    bool enableMccCheckBypass;
	bool canTimeout;

	bool canReplicateInstance(Instance* instance, int replicationProtocolVersion);

    void learnDictionaries(RBX::Network::PacketBuffer& rawBitStream, bool compressed, bool learnSchema);
    
	virtual bool isProtectedStringEnabled() = 0;
    virtual std::string encodeProtectedString(const ProtectedString& value, const Instance* instance, const Reflection::PropertyDescriptor& desc) = 0;
    virtual boost::optional<ProtectedString> decodeProtectedString(const std::string& value, const Instance* instance, const Reflection::PropertyDescriptor& desc) = 0;

	virtual bool isLegalReceiveInstance(Instance* instance, Instance* parent) { return true; }
	virtual bool isLegalDeleteInstance(Instance* instance) { return true; }
	virtual bool isLegalReceiveEvent(Instance* instance, const Reflection::EventDescriptor& desc) { return true; }
	virtual bool isLegalReceiveProperty(Instance* instance, const Reflection::PropertyDescriptor& desc) { return true; }

	virtual bool isLegalSendProperty(Instance* instance, const Reflection::PropertyDescriptor& desc) = 0;
	virtual bool isLegalSendInstance(const Instance* instance) { return true; }
	virtual bool isLegalSendEvent(Instance* instance, const Reflection::EventDescriptor& desc) { return true;}
	virtual bool isCloudEdit() const = 0;
	static bool isCloudEditReplicateProperty(const Reflection::PropertyDescriptor& descriptor);

	virtual bool prepareRemotePlayer(shared_ptr<Instance> instance) { return false; }
    virtual bool checkRemotePlayer() { return true; }

	virtual void rebroadcastEvent(Reflection::EventInvocation& eventInvocation) {}

	virtual void readChangedProperty(RBX::Network::PacketBuffer& bitStream, Reflection::Property prop);
	virtual void readChangedPropertyItem(DeserializedChangePropertyItem* item, Reflection::Property prop);
	virtual void processChangedParentProperty(Guid::Data parentId, Reflection::Property prop);


	virtual void writeChangedProperty(const Instance* instance, const Reflection::PropertyDescriptor& desc, RBX::Network::PacketBuffer& outBitStream);
	virtual void writeChangedRefProperty(const Instance* instance,
		const Reflection::RefPropertyDescriptor& desc, const Guid::Data& newRefGuid,
		RBX::Network::PacketBuffer& outBitStream);

	// Called when a property is received from the remote peer:
	virtual FilterResult filterReceivedChangedProperty(Instance* instance, const Reflection::PropertyDescriptor& desc) { return Accept; }
	virtual FilterResult filterReceivedParent(Instance* instance, Instance* parent) { return Accept; }
	virtual FilterResult filterPhysics(PartInstance* instance);

	// Called when a property changes locally:
	virtual FilterResult filterChangedProperty(Instance* instance, const Reflection::PropertyDescriptor& desc);
	virtual void onPropertyChanged(Instance* instance, const Reflection::PropertyDescriptor* descriptor);

	virtual shared_ptr<Stats> createStatsItem() = 0;

	// These stats are updated asynchronously. Individual stats will be valid
	// but combinations of stats may not be in sync.
	const PeerStatistics* getPeerStatistics() const;
	
	virtual void processPacket(Packet *packet);
	virtual void processDeserializedPacket(const DeserializedPacket& deserializedPacket);
	static void closeReplicationItem(ReplicationData& item);
	bool disconnectReplicationData(shared_ptr<Instance> instance);

	/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

	void onCombinedSignal(ReplicationData* replicationData, Instance::CombinedSignalType type, const ICombinedSignalData* genericData);
	void onChildAdded(shared_ptr<Instance> child, boost::function<void (shared_ptr<Instance>)> replicationMethodFunc);
	void onParentChanged(shared_ptr<Instance> instance);
	void onEventInvocation(Instance* instance, const Reflection::EventDescriptor* descriptor, const Reflection::EventArguments* args, const SystemAddress* target);

	void onTerrainParentChanged(shared_ptr<Instance> instance, shared_ptr<Instance> parent);

	virtual void terrainCellChanged(const Voxel::CellChangeInfo& info);
	virtual void onTerrainRegionChanged(const Voxel2::Region& region);
    void onStatisticsChanged(const ConnectionStats&);

	// Creates a "Default" object which has all default properties. Used to optimize streaming
	const Instance* getDefault(const RBX::Name& className);

	virtual bool wantReplicate(const Instance* source) const;

	virtual void setRefValue(WaitItem& wi, Instance* instance);

	virtual void onSentMarker(long id) {}
	virtual void onSentTag(int tag) {}
	void createPhysicsSender(NetworkSettings::PhysicsSendMethod method);
	void createPhysicsReceiver(NetworkSettings::PhysicsReceiveMethod method, bool isServer);

	void readPropertiesFromValueArray(const std::vector<PropValuePair>& propValueArray, Instance* instance);
    void readProperties(RBX::Network::PacketBuffer& inBitstream, Instance* instance, PropertyCacheType cacheType, bool useDictionary, bool preventBounceBack = true, std::vector<PropValuePair>* valueArray = NULL);
	void readPropertiesInternal(Reflection::Property& property, RBX::Network::PacketBuffer& inBitstream, bool useDictionary, bool preventBounceBack, Reflection::Variant* value);

    virtual void writeProperties(const Instance* instance, RBX::Network::PacketBuffer& outBitstream, PropertyCacheType cacheType, bool useDictionary);
	void writePropertiesInternal(const Instance* instance, const Reflection::ConstProperty& property, RBX::Network::PacketBuffer& outBitstream, PropertyCacheType cacheType, bool useDictionary);

	virtual bool sendItemsPacket();
	int sendItems(ItemSender& sender, ItemQueue& itemQueue);
	virtual bool sendClusterPacket();
	void sendClusterChunk(const StreamRegion::Id &regionId);
	virtual void readItem(RBX::Network::PacketBuffer& inBitstream, Item::ItemType itemType);
	virtual shared_ptr<DeserializedItem> deserializeItem(RBX::Network::PacketBuffer& inBitstream, Item::ItemType itemType);
	virtual void addTopReplicationContainer(Instance* instance, bool replicateProperties, bool replicateChildren,
		boost::function<void (shared_ptr<Instance>)> replicationMethodFunc);
	virtual void dataOutStep();
	virtual void processSendStats(unsigned int sendStats, unsigned int extraStats) {}
    virtual void processHashStats(unsigned int hashStats) {}
    virtual void checkPingItemTime() {}

	// visible for testing
	ReplicationData& addReplicationData(shared_ptr<Instance> instance, bool listenToChanges, bool replicateChildren);

	void sendClusterContent(shared_ptr<RBX::Network::PacketBuffer>& bitStream, ClusterUpdateBuffer& container, unsigned maxBytesSend);
	void sendClusterContent(shared_ptr<RBX::Network::PacketBuffer>& bitStream, ClusterChunksIterator& container, unsigned maxBytesSend);
	void sendClusterContent(shared_ptr<RBX::Network::PacketBuffer>& bitStream, OneQuarterClusterChunkCellIterator &container);

    static void RemoteCheatHelper2(weak_ptr<RBX::DataModel> weakDataModel);

	void serializePropertyValue(const Reflection::ConstProperty& property, RBX::Network::PacketBuffer& outBitStream, bool useDictionary);
	void deserializePropertyValue(RBX::Network::PacketBuffer& inBitStream, Reflection::Property property, bool useDictionary, bool preventBounceBack, Reflection::Variant* value);

    bool deserializeValue(RBX::Network::PacketBuffer& inBitStream, const Reflection::Type& type,	Reflection::Variant& value);

	// visible for testing
	virtual void receiveCluster(RBX::Network::PacketBuffer& inBitstream, Instance* instance, bool usingOneQuarterIterator);

	SharedStringProtectedDictionary& getSharedPropertyProtectedDictionary(const Reflection::PropertyDescriptor& descriptor);
	SharedStringDictionary& getSharedPropertyDictionary(const Reflection::PropertyDescriptor& descriptor);
	SharedStringDictionary& getSharedEventDictionary(const Reflection::EventDescriptor& descriptor);
	SharedBinaryStringDictionary& getSharedPropertyBinaryDictionary(const Reflection::PropertyDescriptor& descriptor);
    
	virtual bool canSendItems() = 0;

    virtual void serializeSFFlags(RBX::Network::PacketBuffer& outBitStream) const{}
    virtual void deserializeSFFlags(RBX::Network::PacketBuffer& inBitStream){}

	unsigned int readJoinData(RBX::Network::PacketBuffer& bitStream);
	unsigned int readJoinDataItem(DeserializedJoinDataItem* item);

    virtual bool isClassRemoved(const Instance* instance) {return false;}
    virtual bool isPropertyRemoved(const Instance* instance, const Name& propertyName) {return false;}
    virtual bool isEventRemoved(const Instance* instance, const Name& eventName) {return false;}

    bool isPropertyCacheable(const Reflection::Type& type);
    bool isPropertyCacheable(const Reflection::Type* type, bool isEnum);

    virtual bool ProcessOutdatedChangedProperty(RBX::Network::PacketBuffer& inBitstream, const RBX::Guid::Data& id, const Instance* instance, const Reflection::PropertyDescriptor* propertyDescriptor, unsigned int propId) {return false;}
    virtual bool ProcessOutdatedProperties(RBX::Network::PacketBuffer& inBitstream, Instance* instance, PropertyCacheType cacheType, bool useDictionary, bool preventBounceBack, std::vector<PropValuePair>* valueArray) {return false;}
    virtual bool ProcessOutdatedInstance(RBX::Network::PacketBuffer& inBitstream, bool isJoinData, const RBX::Guid::Data& id, const Reflection::ClassDescriptor* classDescriptor, unsigned int classId) {return false;}
    virtual bool ProcessOutdatedEventInvocation(RBX::Network::PacketBuffer& inBitstream, const RBX::Guid::Data& id, const Instance* instance, const Reflection::EventDescriptor* eventDescriptor, unsigned int eventId) {return false;}
    virtual bool ProcessOutdatedEnumSerialization(const Reflection::Type& type, const Reflection::Variant& value, RBX::Network::PacketBuffer& outBitStream) {return false;}
    virtual bool ProcessOutdatedEnumDeserialization(RBX::Network::PacketBuffer& inBitStream, const Reflection::Type& type, Reflection::Variant& value) {return false;}
    virtual bool ProcessOutdatedPropertyEnumSerialization(const Reflection::ConstProperty& property, RBX::Network::PacketBuffer& outBitStream) {return false;}
    virtual bool ProcessOutdatedPropertyEnumDeserialization(Reflection::Property& property, RBX::Network::PacketBuffer& inBitStream) {return false;}

    static void compressBitStream(const RBX::Network::PacketBuffer& inUncompressedBitStream, RBX::Network::PacketBuffer& outCompressedBitStream, uint8_t compressRatio);
    static void decompressBitStream(RBX::Network::PacketBuffer& inCompressedBitStream, RBX::Network::PacketBuffer& outUncompressedBitStream);

	void enableDeserializePacketThread();
	// Physics-out
	boost::shared_ptr<PhysicsSender> physicsSender;
	friend class PhysicsReceiver;

private:
	std::string lastPacketError;
	int lastDisconnectReason = -1;

	typedef RBX::DescribedNonCreatable<Replicator, IdSerializer, sReplicator, Reflection::ClassDescriptor::INTERNAL_LOCAL> Super;

	// Physics-in

	shared_ptr<Stats> statsItem;

	// Maintain a string dictionary for each Property type
	typedef std::map<const Reflection::PropertyDescriptor*, shared_ptr<SharedStringDictionary> > PropertyStrings;
	PropertyStrings strings;

	typedef std::map<const Reflection::PropertyDescriptor*, shared_ptr<SharedStringProtectedDictionary> > PropertyProtectedStrings;
	PropertyProtectedStrings protectedStrings;

	typedef std::map<const Reflection::EventDescriptor*, shared_ptr<SharedStringDictionary> > EventStrings;
	EventStrings eventStrings;

	typedef std::map<const Reflection::PropertyDescriptor*, shared_ptr<SharedBinaryStringDictionary> > PropertyBinaryStrings;
	PropertyBinaryStrings binaryStrings;

    float rakTimeOffset;

	bool remoteDeleteOnDisconnect(const Instance* instance) const;

	bool removeFromPendingNewInstances(const Instance* instance);

	void createStatsItems(RBX::Stats::StatsService* stats);
	void updateStatsItem(RBX::Stats::StatsService* stats);
	
	void clusterOutStep();

	bool serializeValue(const Reflection::Type& type, const Reflection::Variant& value, RBX::Network::PacketBuffer& outBitStream);

	void serializeVariant(const Reflection::Variant& value, RBX::Network::PacketBuffer& outBitStream);
	void deserializeVariant(RBX::Network::PacketBuffer& inBitStream, Reflection::Variant& value);

	void serializeEventInvocation(const Reflection::EventInvocation& eventInvocation, RBX::Network::PacketBuffer& outBitStream);
	void deserializeEventInvocation(RBX::Network::PacketBuffer& inBitStream, Reflection::EventInvocation& eventInvocation, bool resolveRefTypes = true);

	void addToPendingItemsList(shared_ptr<Instance> instance);

	rbx::signals::scoped_connection sendFilteredChatMessageConnection;
	void sendFilteredChatMessage(const PeerAddress &systemAddress, const shared_ptr<RBX::Network::PacketBuffer> &data,	const shared_ptr<Instance> sourceInstance, const std::string &blacklist, const std::string &whitelist);

	void writeInstance(shared_ptr<Instance> instance, RBX::Network::PacketBuffer* outBitstream);
	void receiveData(RBX::Network::PacketBuffer& bitStream);
	void receiveData(Packet *packet);

	void readInstanceNew(RBX::Network::PacketBuffer& bitStream, bool isJoinData);
	void readInstanceNewItem(DeserializedNewInstanceItem* item, bool isJoinData);


	void readInstanceDelete(RBX::Network::PacketBuffer& bitStream);
	void readInstanceDeleteItem(DeserializedDeleteInstanceItem* item);
	void deleteInstanceById(Guid::Data id);

	void readChangedProperty(RBX::Network::PacketBuffer& bitStream);
	void readChangedPropertyItem(DeserializedChangePropertyItem* item);


	void readMarker(RBX::Network::PacketBuffer& bitStream);
	void readMarkerItem(DeserializedMarkerItem* item);
	void processMarker(int id);

	void readDataPing(RBX::Network::PacketBuffer& bitStream);
	void readDataPingItem(DeserializedPingItem* item);
	void processDataPing(bool isPingBack, NetworkTime timeStamp);

	void readEventInvocation(RBX::Network::PacketBuffer& bitStream);
	void readEventInvocationItem(DeserializedEventInvocationItem* item);

	void sendDataPing();
	void sendStats(int version);
#if defined(RBX_ENABLE_LEGACY_X86_CLIENT_SECURITY) || defined(RBX_RCC_SECURITY)
	virtual void sendNetPmcChallenge() {}
#endif

	void assignRef(Reflection::Property& property, RBX::Guid::Data id);
	bool shouldStreamingHandleOnAddedForChild(shared_ptr<const Instance> child);
	bool isHighPriorityInstance(const Instance* instance);

	void assignParent(Instance* instance, Instance* parent);
	void assignDefaultPropertyValue(Reflection::Property& property, bool preventBounceBack, Reflection::Variant* var);

	void disconnectClusterReplicationData();

	CEvent packetReceivedEvent;
	boost::mutex receivedPacketsMutex;
	bool deserializePacketsThreadEnabled;
	std::list<Packet*> receivedPackets;
	std::list<Packet*> packetsToDeserailze;
	rbx::timestamped_safe_queue<DeserializedPacket> deserializedPackets;

	boost::scoped_ptr<boost::thread> deserializePacketsThread;
	void deserializePacketsThreadImpl();
	void deserializeData(RBX::Network::PacketBuffer& inBitstream, std::vector<shared_ptr<DeserializedItem> >& items);

	void logPacketError(Packet* packet, const std::string& type, const std::string& message);
};

	class ReplicatorJob : public DataModelJob
	{
	protected:
		boost::shared_ptr<Replicator> replicator;
		ReplicatorJob(const char* name, Replicator& replicator, TaskType taskType);
		static bool canSendPacket(shared_ptr<Replicator>& replicator, PacketPriority packetPriority);

	};
} }

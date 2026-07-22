/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */

#pragma once

#if defined(RBX_MODERN_NETWORKING_RUNTIME) && defined(RBX_RCC_SECURITY)
#error "RBX_RCC_SECURITY is not exposed by the modern Player CMake graph; use the historical server build or add a complete modern RCC feature boundary"
#endif

#include "network/NetworkTypes.h"

#include "network/PacketBuffer.h"

#include "v8tree/Service.h"
#include "util/standardout.h"
#include "network/Player.h"
#include "Peer.h"
#include "Replicator.h"
#include "NetworkFilter.h"
#include "PropertySynchronization.h"
#include "v8datamodel/PartInstance.h"
#include "v8datamodel/Value.h"

#include "network/NetworkTypes.h"
#include "network/NetworkTypes.h"

#include "boost/thread/thread.hpp"
#include <boost/thread/condition.hpp>

#if defined(RBX_ENABLE_LEGACY_X86_CLIENT_SECURITY) || defined(RBX_RCC_SECURITY)
#include "util/ProgramMemoryChecker.h"
#endif
#include "security/FuzzyTokens.h"

#ifdef RBX_RCC_SECURITY
#include "NetPmc.h"
#include "Replicator.RockyItem.h"
#endif

#ifdef RBX_RCC_SECURITY
    DYNAMIC_FASTSTRING(US30605p1)
#endif

namespace RBX { 
	namespace Network {

	class Server;
	class NetworkFilter;
#if defined(RBX_ENABLE_LEGACY_X86_CLIENT_SECURITY) || defined(RBX_RCC_SECURITY)
    struct MccReport;
#endif

    enum PlaceAuthenticationState
    {
        PlaceAuthenticationState_Init,
        PlaceAuthenticationState_Requesting,
        PlaceAuthenticationState_Authenticated,
        PlaceAuthenticationState_Denied,
        PlaceAuthenticationState_DisconnectingClient
    };
	
	extern const char* const sServerReplicator;
	class ServerReplicator
		: public RBX::DescribedNonCreatable<ServerReplicator, Replicator, sServerReplicator, Reflection::ClassDescriptor::INTERNAL_LOCAL>
	{
		typedef RBX::DescribedNonCreatable<ServerReplicator, Replicator, sServerReplicator, Reflection::ClassDescriptor::INTERNAL_LOCAL> Super;
		PropSync::Master propSync;
		class ServerStatsItem;
		friend class ServerStatsItem;
		boost::scoped_ptr<NetworkFilter> basicFilter;
		bool acceptsTerrainChanges;

		DescribedBase* lightingService;
		Reflection::PropertyDescriptor* globalShadowsDescriptor;
		Reflection::PropertyDescriptor* outdoorAmbientDescriptor;
		Reflection::PropertyDescriptor* outlinesDescriptor;

		std::string initialSpawnName;

		Instance* pendingCharaterRequest;
		Time pendingCharacterRequestStartTime;

		friend class Replicator::StreamJob;

		shared_ptr<TaskScheduler::Job> sendStatsJob;


	public: // variables
		int numPartsOwned;
		rbx::signal<void(int, bool, int)> remoteTicketProcessedSignal;
        int remoteProtocolVersion;

	protected:

		rbx::signal<void()> placeAutenticatedSignal;

		// metrics
		Time startTime;
		Analytics::InfluxDb::Points joinAnalytics;

		// TODO: Create a Property for this field:
		shared_ptr<Player> remotePlayer;
		Server* const server;
        boost::scoped_ptr<boost::thread> placeAuthenticationThread;

        PlaceAuthenticationState placeAuthenticationState;

		bool waitingForMarker;
		bool topReplicationContainersSent;
		bool remotePlayerInstalled;
		std::string gameSessionID;

#if defined(RBX_ENABLE_LEGACY_X86_CLIENT_SECURITY) || defined(RBX_RCC_SECURITY)
        typedef std::vector<unsigned int> HashVector;
        PmcHashContainer hashes;
        unsigned long long securityTokens[3];
#endif

		void sendJoinStatsToInflux();

        void PlaceAuthenticationThread(int previousPlaceId, int requestedPlaceId);
        virtual void PlaceAuthenticationThreadImpl(int previousPlaceId, int requestedPlaceId);

        virtual void setAuthenticated(bool authenticated) {}
#if defined(RBX_ENABLE_LEGACY_X86_CLIENT_SECURITY) || defined(RBX_RCC_SECURITY)
        virtual void decodeHashItem(PmcHashContainer& netHashes, unsigned long long* securityTokens) {}
        virtual void processHashValue(const PmcHashContainer& netHashes) {}
        virtual void processHashValuePost(const unsigned long long* const tokens, unsigned int nonce) {}
        virtual void updateHashState(PmcHashContainer& netHashes, unsigned long long* securityTokens) {}
        virtual void processRockyMccReport(const MccReport& report) {}
        virtual void processNetPmcResponseItem(RBX::Network::PacketBuffer& inBitstream) {}
        virtual void processRockyCallInfoItem(RBX::Network::PacketBuffer& inBitstream) {}
#endif

		// Replicator
		/*override*/ bool isProtectedStringEnabled();
        /*override*/ std::string encodeProtectedString(const ProtectedString& value, const Instance* instance, const Reflection::PropertyDescriptor& desc);
        /*override*/ boost::optional<ProtectedString> decodeProtectedString(const std::string& value, const Instance* instance, const Reflection::PropertyDescriptor& desc);

		/*override*/ bool checkDistributedReceive(PartInstance* part);
		/*override*/ bool checkDistributedSend(const PartInstance* part);
		/*override*/ bool checkDistributedSendFast(const PartInstance* part);
		/*override*/ bool isLegalReceiveInstance(Instance* instance, Instance* parent);
		/*override*/ bool isLegalDeleteInstance(Instance* instance);
		/*override*/ bool isLegalReceiveEvent(Instance* instance, const Reflection::EventDescriptor& desc);
		/*override*/ bool isLegalReceiveProperty(Instance* instance, const Reflection::PropertyDescriptor& desc);
		/*override*/ FilterResult filterPhysics(PartInstance* instance);
		/*override*/ bool prepareRemotePlayer(shared_ptr<Instance> instance);
		/*override*/ void onSentMarker(long id);
		/*override*/ void onSentTag(int id);
		/*override*/ bool canSendItems()  { return true; } // The server always can send
		/*override*/ void rebroadcastEvent(Reflection::EventInvocation& eventInvocation);
		/*override*/ bool sendItemsPacket();
		/*override*/ void readItem(RBX::Network::PacketBuffer& inBitstream, RBX::Network::Item::ItemType itemType);
		/*override*/ void addTopReplicationContainers(ServiceProvider* newProvider);
		/*override*/ void addTopReplicationContainer(Instance* instance, bool replicateProperties, bool replicateChildren, boost::function<void (shared_ptr<Instance>)> replicationMethodFunc);
		/*override*/ bool isLegalSendProperty(Instance* instance, const Reflection::PropertyDescriptor& desc);
		/*override*/ FilterResult filterReceivedChangedProperty(Instance* instance, const Reflection::PropertyDescriptor& desc);
		/*override*/ FilterResult filterReceivedParent(Instance* instance, Instance* parent);
		/*override*/ void dataOutStep();
		/*override*/ void onPropertyChanged(Instance* instance, const Reflection::PropertyDescriptor* descriptor);
		/*override*/ void writeChangedProperty(const Instance* instance, const Reflection::PropertyDescriptor& desc, RBX::Network::PacketBuffer& outBitStream);
		/*override*/ void writeChangedRefProperty(const Instance* instance,
			const Reflection::RefPropertyDescriptor& desc, const Guid::Data& newRefGuid,
			RBX::Network::PacketBuffer& outBitStream);
		/*override*/ void receiveCluster(RBX::Network::PacketBuffer& inBitstream, Instance* instance, bool usingOneQuarterIterator);

		/*override*/ void setPropSyncExpiration(double value)
		{
			propSync.setExpiration(RBX::Time::Interval(value));
		}

		/*override*/ shared_ptr<Stats> createStatsItem();
		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);
		/*override*/ bool isCloudEdit() const;

		void sendReplicatedFirstDescendants(shared_ptr<Instance> descendant);


	private:
		void processRequestCharacter(Instance* instance, RBX::Guid::Data id, unsigned int sendStats, std::string preferedSpawnName);
		void readRequestCharacter(RBX::Network::PacketBuffer& bitStream);
		void readClientQuotaUpdate(RBX::Network::PacketBuffer& bitStream);
		void readRegionRemoval(RBX::Network::PacketBuffer& bitStream);
		void readPropAcknowledgement(RBX::Network::PacketBuffer& bitStream);	
		virtual void installRemotePlayer(const std::string& preferedSpawnName);
		static void installRemotePlayerSafe(weak_ptr<ServerReplicator> weakThis, const std::string preferedSpawnName);
        void sendDictionaryFormat();
#if defined(RBX_ENABLE_LEGACY_X86_CLIENT_SECURITY) || defined(RBX_RCC_SECURITY)
        void readHashItem(RBX::Network::PacketBuffer& inBitstream);
        void readRockyItem(RBX::Network::PacketBuffer& inBitstream);
#endif
		static void toggleSendStatsJob(weak_ptr<ServerReplicator> weakServerReplicator, bool required, int version);

	public: // methods
		/*override*/ Player* findTargetPlayer() const 
		{
			return remotePlayer.get();
		}
		/*override*/ Player* getRemotePlayer() const 
		{
			return remotePlayer.get();
		}
		/*override*/ bool canUseProtocolVersion(int protocolVersion) const;
        /*override*/ bool isProtocolCompatible() const;

		void setBasicFilteringEnabled(bool value);
		void preventTerrainChanges();

        virtual void serializeSFFlags(RBX::Network::PacketBuffer& outBitStream) const;

        virtual void sendDictionaries();

		ServerReplicator(PeerAddress systemAddress, Server* server, NetworkSettings* networkSettings);
		~ServerReplicator();

		void sendTop(ConcurrentPeer*peer);

		const PartInstance* readPlayerSimulationRegion(Region2::WeightedPoint& weightedPoint);
        virtual void readPlayerSimulationRegion(const PartInstance* playerHead, Region2::WeightedPoint& weightedPoint);

		boost::function<FilterResult(shared_ptr<Instance>, std::string, Reflection::Variant)> filterProperty;
		boost::function<FilterResult(shared_ptr<Instance>, shared_ptr<Instance> parent)> filterNew;
		boost::function<FilterResult(shared_ptr<Instance>)> filterDelete;
		boost::function<FilterResult(shared_ptr<Instance>, std::string)> filterEvent;

        /*override*/ PluginReceiveResult OnReceive(Packet *packet);

        void writeDescriptorSchema(const Reflection::ClassDescriptor* desc, RBX::Network::PacketBuffer& bitStream) const;

        void teachSchema();
        static RBX::Network::PacketBuffer apiSchemaBitStream;
        static RBX::Network::PacketBuffer apiDictionaryBitStream;
        const RBX::Network::PacketBuffer& getSchemaBitStream() const;
        const RBX::Network::PacketBuffer& getApiDictionaryBitStream() const;
        static void generateSchema(const ServerReplicator* serverRep, bool force);
        static void generateApiDictionary(const ServerReplicator* serverRep, bool force);

        /*override*/ bool isServerReplicator() {return true;};

		void onPlaceAuthenticationComplete(PlaceAuthenticationState placeAuthenticationResult);
	};

#if defined(RBX_RCC_SECURITY)
	class CheatHandlingServerReplicator : public ServerReplicator
	{
		bool isAuthenticated;
		bool isBadTicket;
		std::string ticket;	// the ticket received from the remote client
		int userIdFromTicket;

        unsigned int reportedGoldHash;
        unsigned int hashNonce;
        HashVector lastHashes;
        bool hashInitialized;
        unsigned int ignoreHashFailureMask;
        unsigned int ignoreGoldHashFailureMask;
        unsigned int sendStatsMask;
        unsigned int extraStatsMask;
        unsigned int apiStatsMask;
        unsigned int mccStatsMask;

	public:
		CheatHandlingServerReplicator(PeerAddress systemAddress, Server* server, NetworkSettings* networkSettings);
		/*override*/ PluginReceiveResult OnReceive(Packet *packet);
        /*override*/ void CheatHandlingServerReplicator::sendNetPmcChallenge();

	private:

        struct CallChainSetInfo
        {
            uintptr_t handler[4];
            size_t ret[4];
            CallChainSetInfo()
            {
                for (size_t i = 0; i < 4; ++i)
                {
                    handler[i] = 0;
                    ret[i] = 0;
                }
            }

            friend bool operator==(const CallChainSetInfo& lhs, const CallChainSetInfo& rhs)
            {
                for (size_t i = 0; i < 4; ++i)
                {
                    if ( (lhs.ret[i] != rhs.ret[i]) || (lhs.handler[i] != rhs.handler[i]) )
                    {
                        return false;
                    }
                }
                return true;
            }
        };

        RBX::Security::NetPmcServer netPmc;
        double kickTimeSec;
        std::string kickName;
		bool processedTicket;
		void processTicket(Packet *packet);
		void processSendStats(unsigned int sendStats, unsigned int extraStats);
        void processHashStats(unsigned int hashStats);
        void processGoldHashStats(unsigned int hashStats);
        void processApiStats(unsigned long long apiStats);
		void preauthenticatePlayer(int userId);
		/*override*/ virtual void installRemotePlayer(const std::string& preferedSpawnName);
        void doRemoteSysStats(unsigned int sendStats, unsigned int mask, const char* codeName, const char* details, const std::string& configString = ::DFString::US30605p1);
        void doDelayedSysStats(unsigned int sendStats, unsigned int mask, const char* codeName, const char* details);
        ServerFuzzySecurityToken securityToken;
        ServerFuzzySecurityToken apiToken;
        unsigned long long prevApiToken;
        std::vector<CallChainSetInfo> reportedCallChains;
        size_t numHashItems;
        size_t numMccItems;
        size_t numPingItems;
        bool reportedInvalid;
        bool reportedExploit;
        bool reportedSkip;
        bool reportedApiFail;
        bool reportedApiTamper;
        bool reportedRangeError;
        MccReport lastMccReport;
        MccReport firstMccReport;
        double firstMccReportRxTime;
        double lastMccReportRxTime;
        bool reportedHashItemTime;
        bool reportedMccItemTime;
        bool reportedPingItemTime;
        bool reportedMccError;
        bool reportedNetPmcError;
        bool reportedNetPmcPending;
        bool reportedNetPmcSent;

	protected:
		// Replicator
        /*override*/ bool checkRemotePlayer();
		/*override*/ bool canSendItems()  { return processedTicket; } // The server can only send once we get a ticket from the client
        /*override*/ void setAuthenticated(bool authenticated) {isAuthenticated = authenticated;}
        /*override*/ void PlaceAuthenticationThreadImpl(int previousPlaceId, int requestedPlaceId);
        /*override*/ void decodeHashItem(PmcHashContainer& netHashes, unsigned long long* securityTokens);
        /*override*/ void processHashValue(const PmcHashContainer& netHashes);
        /*override*/ void processHashValuePost(const unsigned long long* const tokens, unsigned int nonce);
        /*override*/ void updateHashState(PmcHashContainer& netHashes, unsigned long long* securityTokens);
        /*override*/ void processRockyMccReport(const MccReport& report);
        /*override*/ void processNetPmcResponseItem(RBX::Network::PacketBuffer& inBitstream);
        /*override*/ void checkPingItemTime();
        /*overrode*/ void processRockyCallInfoItem(RBX::Network::PacketBuffer& inBitstream);

	};
#endif

} }

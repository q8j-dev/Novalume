/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */

#include "Client.h"
#include "ClientReplicator.h"

#include "Util.h"
#include "ConcurrentPeer.h"
#include "network/Players.h"
#include "network/NetworkOwner.h"
#if defined(RBX_ENABLE_LEGACY_X86_CLIENT_SECURITY)
#include "util/ProgramMemoryChecker.h"
#endif
#include "util/standardout.h"
#include "util/ProtectedString.h"
#include "util/RbxStringTable.h"
#include "CPUCount.h"
#include "FastLog.h"
#include "rbx/RbxDbgInfo.h"
#include "v8datamodel/HackDefines.h"
#include "v8datamodel/Workspace.h"
#include "v8datamodel/DataModel.h"
#include "v8datamodel/DebugSettings.h"
#include "v8datamodel/TeleportService.h"

#include "Script/ScriptContext.h"

#include "network/NetworkTypes.h"

LOGGROUP(US14116)
DYNAMIC_FASTFLAG(DebugDisableTimeoutDisconnect)
FASTFLAG(DebugLocalRccServerConnection)

DYNAMIC_LOGGROUP(NetworkJoin)
FASTFLAG(DebugProtocolSynchronization)

#ifndef _WIN32
// For inet_addr() call used below
#include <arpa/inet.h>
#endif

const char* const RBX::Network::sClient = "NetworkClient";

namespace RBX
{
	extern const char *const sHopper;
    class Instance;
}

using namespace RBX;
using namespace RBX::Network;

REFLECTION_BEGIN();
Reflection::BoundProp<std::string> Client::prop_Ticket("Ticket", "Authentication", &Client::ticket);
static Reflection::BoundFuncDesc<Client, shared_ptr<Instance>(int, std::string, int, int, int)> f_connect(&Client::playerConnect, "PlayerConnect", "userId", "server", "serverPort", "clientPort", 0, "threadSleepTime", 30, Security::Plugin);
static Reflection::BoundFuncDesc<Client, void(int)> f_disconnect(&Client::disconnect, "Disconnect", "blockDuration", 3000, Security::LocalUser);
static Reflection::BoundFuncDesc<Client, void(std::string)> func_setGameSessionID(&Client::setGameSessionID, "SetGameSessionID", "gameSessionID", Security::Roblox);
static Reflection::EventDesc<Client, void(std::string, shared_ptr<RBX::Instance>)> event_ConnectionAccepted(&Client::connectionAcceptedSignal, "ConnectionAccepted", "peer", "replicator");
static Reflection::EventDesc<Client, void(std::string)> event_ConnectionRejected(&Client::connectionRejectedSignal, "ConnectionRejected", "peer");
static Reflection::EventDesc<Client, void(std::string, int, std::string)> event_ConnectionFailed(&Client::connectionFailedSignal, "ConnectionFailed", "peer", "code", "reason");
REFLECTION_END();

Client::Client()
	: userId(-1), networkSettings(&NetworkSettings::singleton()), isCloudEditClient(false)
{
	RBX::Security::Context::current().requirePermission(RBX::Security::Plugin, "create a NetworkClient");
	setName(sClient);

	FASTLOG(FLog::Network, "NetworkClient:Create");
}

Client::~Client(void)
{
	FASTLOG(FLog::Network, "NetworkClient:Remove");
}

Client* Client::findClient(const RBX::Instance* context, bool testInDatamodel)
{
	const ServiceProvider* serviceProvider = ServiceProvider::findServiceProvider(context);
	RBXASSERT(!testInDatamodel || serviceProvider!=NULL);
	return ServiceProvider::find<Client>(serviceProvider);
}

bool Client::clientIsPresent(const RBX::Instance* context, bool testInDatamodel)
{
	return findClient(context, testInDatamodel) != NULL;
}

bool Client::physicsOutBandwidthExceeded(const RBX::Instance* context)
{
	if (Client* client = Client::findClient(context))
	{
		if (ClientReplicator* clientRep = client->findFirstChildOfType<ClientReplicator>())
		{
			return clientRep->isLimitedByOutgoingBandwidthLimit();
		}
	}
	return true;
}

double Client::getNetworkBufferHealth(const RBX::Instance* context)
{
	if (Client* client = Client::findClient(context))
	{
		return client->networkPeer->GetBufferHealth();
	}
	return 0.0f;
}

const RBX::SystemAddress Client::findLocalSimulatorAddress(const RBX::Instance* context)
{
	if (Client* client = Client::findClient(context, false)) {
		if (ClientReplicator* clientRep = client->findFirstChildOfType<ClientReplicator>()) {
			return peerAddressToSystemAddress(clientRep->getClientAddress());
		}
	}
	return Network::NetworkOwner::Unassigned();
}

shared_ptr<Instance> Client::playerConnect(int userId, std::string server, int serverPort, int clientPort, int threadSleepTime)
{
	FASTLOG3(FLog::Network, "Client:Connect serverPort(%d) clientPort(%d) threadSleepTime(%d)", serverPort, clientPort, threadSleepTime);

	this->userId = userId;
	Players* players = ServiceProvider::create<Players>(this);
	if(!players)
		throw RBX::runtime_error("Cannot get players");

	shared_ptr<Instance> player = players->createLocalPlayer(userId, TeleportService::getPreviousPlaceId() > 0);

	if (clientPort == 0) {
		clientPort = networkSettings->preferredClientPort;
	}

	SocketDescriptor d(clientPort, "");
	StartupResult startRes = networkPeer->rawPeer()->Startup(1, &d, 1);
	if (startRes != StartupSucceeded)
    {
		if (clientPort==0)
        {
			throw RBX::runtime_error("Failed to start network client");
        }
		else
        {
			throw RBX::runtime_error("Failed to start network client on port %d", clientPort);
        }
    }

	if (server != "localhost" && server != "127.0.0.1" && server != "::1" && !FFlag::DebugLocalRccServerConnection)
		RBX::Security::Context::current().requirePermission(RBX::Security::Roblox, " connect to an external game");
    if (FFlag::DebugLocalRccServerConnection)
    {
        //skip the security check
        Network::versionB = "test";
    }

	ConnectionAttemptResult connectRes = networkPeer->rawPeer()->Connect(server.c_str(), serverPort, Network::versionB.c_str(), Network::versionB.size());
	if (connectRes != ConnectionAttemptStarted)
    {
		throw RBX::runtime_error("Failed to connect to server, id %d", connectRes);
    }
	FASTLOG1F(DFLog::NetworkJoin, "playerConnect connecting to server @ %f s", Time::nowFastSec());

	if(DFFlag::DebugDisableTimeoutDisconnect)
		networkPeer->rawPeer()->SetTimeoutTime(10*60*1000, UnassignedPeerAddress);


	StandardOut::singleton()->printf(MESSAGE_SENSITIVE, "Connecting to %s:%d", server.c_str(), serverPort);

	FASTLOG2(FLog::Network, "Connecting to server, IP(inet_addr): %u Port: %u", inet_addr(server.c_str()), serverPort);

	RBX::RbxDbgInfo::SetServerIP(server.c_str());

	return player;
}



void Client::disconnect(int blockDuration)
{
	FASTLOG(FLog::Network, "Client:Disconnect");

	// The following line will remove the Replicator
	this->visitChildren(boost::bind(&Instance::unlockParent, _1));
	this->removeAllChildren();

	if (networkPeer)
	{
		networkPeer->rawPeer()->CloseConnection(this->serverId, true);
		networkPeer->rawPeer()->Shutdown(blockDuration);
	}
}

void Client::setGameSessionID(std::string value)
{
	if (value != Http::gameSessionID)
	{
		Http::gameSessionID = value;
	}
}

void Client::configureAsCloudEditClient()
{
	isCloudEditClient = true;
}

bool Client::isCloudEdit() const
{
	return isCloudEditClient;
}

void Client::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	if (oldProvider)
	{
		closingConnection.disconnect();

		disconnect(); // We should have disconnected by now (in response to the Closing event)

		Players* players = ServiceProvider::find<Players>(oldProvider);
		players->setConnection(NULL);
	}

	Super::onServiceProvider(oldProvider, newProvider);

	if (newProvider)
	{
		//We're in multiplayer mode, so burn out the studio tools
		if(RBX::DataModel* dataModel = RBX::DataModel::get(this)){
			if(dataModel->lockVerb.get())
				dataModel->lockVerb->doIt(NULL);
		}

		Players* players = ServiceProvider::create<Players>(newProvider);
		players->setConnection(networkPeer.get());

		// Disconnect now before we start getting DescendantRemoving events
		// If we don't disconnect first, then we'll send a shower of delete messages
		// to the Server
		closingConnection = newProvider->closingSignal.connect(boost::bind(&Client::disconnect, this));
	}

}

void Client::sendVersionInfo()
{
    RBX::Network::PacketBuffer bitStream;
    bitStream << (unsigned char) ID_PROTOCOL_SYNC;
    bitStream << protocolVersion;

    networkPeer->rawPeer()->Send(&bitStream, networkSettings->getDataSendPriority(), DATAMODEL_RELIABILITY, DATA_CHANNEL, serverId, false);
}

void Client::sendTicket()
{
	RBX::Network::PacketBuffer bitStream;
	bitStream << (unsigned char) ID_SUBMIT_TICKET;

	bitStream << userId;
    serializeStringCompressed(ticket, bitStream);

	serializeStringCompressed(RBX::DataModel::hash, bitStream);

	bitStream << protocolVersion;

    serializeStringCompressed(securityKey, bitStream);

	// TODO: better way to track protocol changes between versions
	// Network Protocol version 2
	serializeStringCompressed(DebugSettings::singleton().osPlatform(), bitStream);
    serializeStringCompressed(DebugSettings::singleton().getRobloxProductName(), bitStream);

    serializeStringCompressed(Http::gameSessionID, bitStream);

#if defined(RBX_ENABLE_LEGACY_X86_CLIENT_SECURITY)
    const unsigned int reportedGoldHash = RBX::Security::rbxGoldHash;
#else
    const unsigned int reportedGoldHash = 0;
#endif

    bitStream << reportedGoldHash;

	encryptDataPart(bitStream);

	// Send ID_SUBMIT_TICKET
	networkPeer->rawPeer()->Send(&bitStream, networkSettings->getDataSendPriority(), DATAMODEL_RELIABILITY, DATA_CHANNEL, serverId, false);
}

std::string networkErrorToString(int id)
{
	switch (id)
	{
	case ID_INVALID_PASSWORD:
	case ID_HASH_MISMATCH:
		return "ROBLOX version is out of date. Please uninstall and try again.";
	case ID_CONNECTION_ATTEMPT_FAILED:
		return "Connection attempt failed.";
	case ID_SECURITYKEY_MISMATCH:
		return "Version not compatible with server. Please uninstall and try again.";
	default:
		return RBX::format("Network error %d", id);
	}
}

void Client::OnFailedConnectionAttempt(Packet *packet, FailedConnectionReason failedConnectionAttemptReason)
{
	std::string message = networkErrorToString(packet->data[0]);
	StandardOut::singleton()->printf(MESSAGE_SENSITIVE, "Failed to connect to %s. %s\n", peerAddressToString(packet->systemAddress).c_str(), message.c_str());
	connectionFailedSignal(peerAddressToString(packet->systemAddress), (int) packet->data[0], message);
}

// Cheat Engine StealthEdit Plugin helper. Name obscured for security.
#if defined(RBX_ENABLE_LEGACY_X86_CLIENT_SECURITY) && !defined(RBX_STUDIO_BUILD)
static void programMemoryPermissionsHackChecker(weak_ptr<DataModel> weakDataModel) {
	static const unsigned int kSleepBetweenStealthEditChecksMillis = 2 * 1000;
	while (true) {
		shared_ptr<DataModel> dataModel = weakDataModel.lock();
		if (!dataModel) { break; }

		//FASTLOG(FLog::US14116, "Starting stealth check");
		if (ProgramMemoryChecker::areMemoryPagePermissionsSetupForHacking()) {
			//FASTLOG(FLog::US14116, "Caught stealthedit!");
            RBX::Security::setHackFlagVmp<LINE_RAND4>(RBX::Security::hackFlag7, HATE_CATCH_EXECUTABLE_ACCESS_VIOLATION);
		}
		//FASTLOG1(FLog::US14116, "Sleeping stealth for %ums", kSleepBetweenStealthEditChecksMillis);
		boost::this_thread::sleep(boost::posix_time::milliseconds(kSleepBetweenStealthEditChecksMillis));
	}
}
#endif

void Client::sendPreferedSpawnName() const {
	RBX::Network::PacketBuffer bitStream;

	bitStream << (unsigned char) ID_SPAWN_NAME;

    serializeStringCompressed(TeleportService::GetSpawnName(), bitStream);

	FASTLOGS(FLog::Network, "serverId: %s", peerAddressToString(serverId).c_str());

	networkPeer->rawPeer()->Send(&bitStream, networkSettings->getDataSendPriority(), DATAMODEL_RELIABILITY, DATA_CHANNEL, serverId, false);
}

void Client::HandleConnection(Packet *packet)
{
    shared_ptr<Replicator> proxy;
    try
    {
        // send previous placeId
        RBX::Network::PacketBuffer bitStream;
        bitStream << (unsigned char) ID_PLACEID_VERIFICATION;

        bitStream << TeleportService::getPreviousPlaceId();

        networkPeer->rawPeer()->Send(&bitStream, networkSettings->getDataSendPriority(), DATAMODEL_RELIABILITY, DATA_CHANNEL, serverId, false);

        sendTicket();

        sendPreferedSpawnName();

        Workspace* workspace = Workspace::findWorkspace(this);
        workspace->clearTerrain();

        proxy = Creatable<Instance>::create<ClientReplicator>(packet->systemAddress, this, networkPeer->rawPeer()->GetExternalID(serverId), networkSettings);

        proxy->setAndLockParent(this);
        // The legacy plugin chain delivered the accepted-connection packet to
        // a ClientReplicator attached during the same dispatch.  Forward it
        // explicitly with the owned handler list so the client teaches its
        // descriptor dictionaries before it sends its Player join data.
        proxy->OnReceive(packet);

#if defined(RBX_ENABLE_LEGACY_X86_CLIENT_SECURITY) && !defined(RBX_STUDIO_BUILD)
		{
            weak_ptr<DataModel> weakDataModel = weak_from(DataModel::get(this));

            boost::thread t(boost::bind(&programMemoryPermissionsHackChecker, weakDataModel));
            spawnDebugCheckThreads(weakDataModel);
            if (!weakDataModel.lock())
            {
                DataModel::get(this)->addHackFlag(HATE_WEAK_DM_POINTER_BROKEN);
            }
        }
#endif

        connectionAcceptedSignal(peerAddressToString(packet->systemAddress), proxy);
    }
    catch (RBX::base_exception& e)
    {
        RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Error in ID_CONNECTION_REQUEST_ACCEPTED: %s", e.what());
        if (proxy)
        {
            // Disconnect
            proxy->unlockParent();
            proxy->setParent(NULL);
        }
    }
}

PluginReceiveResult Client::OnReceive(Packet *packet)
{
	PluginReceiveResult result = Super::OnReceive(packet);
	if (result!=RR_CONTINUE_PROCESSING)
		return result;

	switch ((unsigned char) packet->data[0])
	{
	case ID_CONNECTION_REQUEST_ACCEPTED:
		{
            StandardOut::singleton()->printf(MESSAGE_SENSITIVE, "Connection accepted from %s\n", peerAddressToString(packet->systemAddress).c_str());

            serverId = packet->systemAddress;

            HandleConnection(packet);
            sendVersionInfo();
        }
        return RR_CONTINUE_PROCESSING;

	case ID_INVALID_PASSWORD:
		StandardOut::singleton()->printf(MESSAGE_SENSITIVE, "Invalid password from %s", peerAddressToString(packet->systemAddress).c_str());
		connectionFailedSignal(peerAddressToString(packet->systemAddress), (int) packet->data[0], networkErrorToString(packet->data[0]));
		connectionRejectedSignal(peerAddressToString(packet->systemAddress));
		return RR_STOP_PROCESSING_AND_DEALLOCATE;
	
	case ID_HASH_MISMATCH:
	case ID_SECURITYKEY_MISMATCH:
		connectionFailedSignal(peerAddressToString(packet->systemAddress), (int) packet->data[0], networkErrorToString(packet->data[0]));
		return RR_STOP_PROCESSING_AND_DEALLOCATE;
	
	case ID_DISCONNECTION_NOTIFICATION:
	case ID_CONNECTION_LOST:
		RBXASSERT(packet->systemAddress==serverId);
		serverId = UnassignedPeerAddress;
		return RR_CONTINUE_PROCESSING;

	default:
		return RR_CONTINUE_PROCESSING;
	}
}

#pragma once

#include "network/GameNetworkingTransport.h"
#include "network/NetworkTypes.h"
#include "network/PacketBuffer.h"

#include "boost/noncopyable.hpp"
#include "boost/shared_ptr.hpp"
#include "V8DataModel/DataModelJob.h"
#include "rbx/threadsafe.h"

#include <deque>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace RBX {

class DataModel;

namespace Network {

struct ConnectionStats {
    int mtuSize = 1200;
    int averagePing = 0;
    int lastPing = 0;
    int lowestPing = 0;
    float maxPacketloss = 0.0f;
    RunningAverage<> bufferHealth{0.1, 1.0};
    int prevBufferSize = 0;
    RunningAverage<double> averageBandwidthExceeded{0.1};
    RunningAverage<double> averageCongestionControlExceeded{0.1};
    RunningAverage<double> kiloBytesSentPerSecond;
    RunningAverage<double> kiloBytesReceivedPerSecond;
    PeerStatistics transportStats;

    void updateBufferHealth(int bufferSize)
    {
        if (bufferSize > prevBufferSize)
            bufferHealth.sample(0.0);
        else if (bufferSize == 0)
            bufferHealth.sample(1.0);
        else
            bufferHealth.sample(0.5);
        prevBufferSize = bufferSize;
    }
};

class ConcurrentPeer : boost::noncopyable {
    class PacketJob;
    class StatsUpdateJob;

    boost::shared_ptr<GameNetworkingTransport> transport;
    boost::shared_ptr<PacketJob> packetJob;
    boost::shared_ptr<StatsUpdateJob> statsUpdateJob;
    DataModel* const dataModel;
    std::vector<PacketHandler*> handlers;
    std::deque<Packet*> receivedPackets;
    mutable std::mutex connectionsMutex;
    std::vector<PeerAddress> connections;
    std::unordered_set<ConnectionId> incomingConnections;
    std::unordered_set<ConnectionId> connectedConnections;
    std::unordered_set<ConnectionId> announcedConnections;
    PeerAddress boundAddress;
    std::uint32_t localNetworkIdentity = 0;
    bool active = false;
    std::uint32_t timeoutMilliseconds = 0;
    std::uint32_t outgoingBitsPerSecondLimit = 0;
    std::string lastStartupError;

    PeerAddress addressFor(ConnectionId connection) const;
    PeerAddress setNetworkIdentity(ConnectionId connection, std::uint32_t identity);
    std::vector<PeerAddress> connectionSnapshot() const;
    void queueEventPacket(unsigned char id, const PeerAddress& address);
    void pollTransport();

public:
    explicit ConcurrentPeer(DataModel* dataModel);
    ~ConcurrentPeer();

    void addStats(PeerAddress address, boost::function<void(const ConnectionStats&)> callback);
    void removeStats(PeerAddress address);

    ConcurrentPeer* rawPeer() { return this; }
    const ConcurrentPeer* rawPeer() const { return this; }
    bool equals(const ConcurrentPeer* other) const { return this == other; }

    void Send(boost::shared_ptr<const PacketBuffer> packet, PacketPriority priority,
        PacketReliability reliability, char orderingChannel, PeerAddress address, bool broadcast);
    bool Send(const PacketBuffer* packet, PacketPriority priority, PacketReliability reliability,
        char orderingChannel, PeerAddress address, bool broadcast);
    bool Send(const char* data, int length, PacketPriority priority, PacketReliability reliability,
        char orderingChannel, PeerAddress address, bool broadcast);

    StartupResult Startup(unsigned int maximumConnections, const SocketDescriptor* descriptors, unsigned int descriptorCount);
    const std::string& GetLastStartupError() const { return lastStartupError; }
    ConnectionAttemptResult Connect(const char* host, unsigned short port, const char* password, int passwordLength);
    void Shutdown(unsigned int blockDuration);
    void CloseConnection(PeerAddress address, bool sendNotification);
    bool IsActive() const { return active; }
    Packet* Receive();
    void DeallocatePacket(Packet* packet) { delete packet; }
    void AttachPlugin(PacketHandler* handler);
    void DetachPlugin(PacketHandler* handler);
    void SetMaximumIncomingConnections(unsigned short) {}
    void SetIncomingPassword(const char*, int) {}
    bool SetCertificate(std::span<const std::byte> certificate, std::string& error);
    void SetTimeoutTime(unsigned int milliseconds, PeerAddress address);
    void SetOccasionalPing(bool) {}
    void SetMTUSize(int) {}
    void SetPerConnectionOutgoingBandwidthLimit(int bitsPerSecond);
    PeerAddress GetMyBoundAddress() const { return boundAddress; }
    PeerAddress GetInternalID(PeerAddress, int) const { return {{}, {"127.0.0.1", boundAddress.endpoint.port}}; }
    PeerAddress GetExternalID(PeerAddress address) const { return addressFor(address.connection); }
    PeerGuid GetGuidFromSystemAddress(PeerAddress) const;

    int GetMTUSize(PeerAddress) const { return 1200; }
    int GetAveragePing(PeerAddress address) const;
    int GetLastPing(PeerAddress address) const { return GetAveragePing(address); }
    int GetLowestPing(PeerAddress address) const { return GetAveragePing(address); }
    const PeerStatistics* GetStatistics(PeerAddress address) const;
    double GetBufferHealth();
    double GetBandwidthExceeded(PeerAddress address);
    double GetCongestionControlExceeded(PeerAddress address);
};

} // namespace Network
} // namespace RBX

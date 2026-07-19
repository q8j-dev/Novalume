#pragma once

#include "network/NetworkTypes.h"

#include "network/Transport.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace RBX::Network {

using NetworkTime = std::uint64_t;
using NetworkTimeMS = std::uint32_t;
using NetworkTimeUS = std::uint64_t;

inline NetworkTimeUS GetTimeUS()
{
    return static_cast<NetworkTimeUS>(std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

inline NetworkTimeMS GetTimeMS()
{
    return static_cast<NetworkTimeMS>(GetTimeUS() / 1000);
}

inline NetworkTime GetTime()
{
    return GetTimeMS();
}

struct PeerAddress {
    ConnectionId connection = InvalidConnectionId;
    Endpoint endpoint;
    std::uint32_t networkIdentity = 0;

    PeerAddress() = default;
    PeerAddress(ConnectionId connection, Endpoint endpoint)
        : connection(connection)
        , endpoint(std::move(endpoint))
    {
    }
    PeerAddress(const char* host, std::uint16_t port)
        : endpoint{host ? host : "", port}
    {
    }

    bool assigned() const { return connection != InvalidConnectionId || !endpoint.host.empty(); }
    std::uint16_t GetPort() const { return networkIdentity == 0 ? endpoint.port : 0; }
    std::uint32_t GetBinaryAddress() const { return networkIdentity; }
    std::string ToString(bool writePort = true, char delimiter = '|') const
    {
        if (!writePort || endpoint.port == 0)
            return endpoint.host;
        return endpoint.host + delimiter + std::to_string(endpoint.port);
    }

    friend bool operator==(const PeerAddress&, const PeerAddress&) = default;
    friend bool operator!=(const PeerAddress&, const PeerAddress&) = default;
    friend bool operator<(const PeerAddress& left, const PeerAddress& right)
    {
        if (left.connection != right.connection)
            return left.connection < right.connection;
        if (left.endpoint.host != right.endpoint.host)
            return left.endpoint.host < right.endpoint.host;
        return left.endpoint.port < right.endpoint.port;
    }
};

inline const PeerAddress UnassignedPeerAddress{};

struct Packet {
    PeerAddress systemAddress;
    std::vector<unsigned char> storage;
    unsigned char* data = nullptr;
    unsigned int length = 0;

    Packet() = default;
    Packet(PeerAddress address, std::vector<std::byte> payload)
        : systemAddress(std::move(address))
        , storage(payload.size())
    {
        for (std::size_t index = 0; index < payload.size(); ++index)
            storage[index] = static_cast<unsigned char>(payload[index]);
        data = storage.data();
        length = static_cast<unsigned int>(storage.size());
    }
};

enum PluginReceiveResult {
    RR_STOP_PROCESSING_AND_DEALLOCATE,
    RR_STOP_PROCESSING,
    RR_CONTINUE_PROCESSING,
};

enum FailedConnectionReason {
    ConnectionAttemptFailed,
};

struct InternalPacket;

class PacketHandler {
public:
    virtual ~PacketHandler() = default;
    virtual PluginReceiveResult OnReceive(Packet*) { return RR_CONTINUE_PROCESSING; }
    virtual void OnFailedConnectionAttempt(Packet*, FailedConnectionReason) {}
    virtual void OnInternalPacket(InternalPacket*, unsigned int, PeerAddress, NetworkTimeMS, int) {}
};

enum PacketPriority {
    IMMEDIATE_PRIORITY,
    HIGH_PRIORITY,
    MEDIUM_PRIORITY,
    LOW_PRIORITY,
    NUMBER_OF_PRIORITIES,
};

enum PacketReliability {
    UNRELIABLE,
    UNRELIABLE_SEQUENCED,
    RELIABLE,
    RELIABLE_ORDERED,
    RELIABLE_SEQUENCED,
};

enum MessageId : unsigned char {
    ID_CONNECTION_REQUEST_ACCEPTED = 16,
    ID_CONNECTION_ATTEMPT_FAILED = 17,
    ID_NEW_INCOMING_CONNECTION = 19,
    ID_DISCONNECTION_NOTIFICATION = 21,
    ID_CONNECTION_LOST = 22,
    ID_INVALID_PASSWORD = 24,
    ID_TIMESTAMP = 27,
    ID_USER_PACKET_ENUM = 129,
};

enum StatisticIndex {
    USER_MESSAGE_BYTES_PUSHED,
    USER_MESSAGE_BYTES_SENT,
    USER_MESSAGE_BYTES_RESENT,
    USER_MESSAGE_BYTES_RECEIVED_PROCESSED,
    USER_MESSAGE_BYTES_RECEIVED_IGNORED,
    ACTUAL_BYTES_SENT,
    ACTUAL_BYTES_RECEIVED,
    StatisticCount,
};

struct PeerStatistics {
    std::array<double, StatisticCount> valueOverLastSecond{};
    std::array<std::uint64_t, StatisticCount> runningTotal{};
    std::array<unsigned int, NUMBER_OF_PRIORITIES> messageInSendBuffer{};
    std::array<unsigned int, NUMBER_OF_PRIORITIES> bytesInSendBuffer{};
    unsigned int messagesInResendBuffer = 0;
    unsigned int bytesInResendBuffer = 0;
    NetworkTimeUS connectionStartTime = GetTimeUS();
    double BPSLimitByOutgoingBandwidthLimit = 0.0;
    bool isLimitedByOutgoingBandwidthLimit = false;
    double BPSLimitByCongestionControl = 0.0;
    bool isLimitedByCongestionControl = false;
    float packetlossLastSecond = 0.0f;
    float packetlossTotal = 0.0f;
};

struct InternalPacket {
    unsigned char* data = nullptr;
    std::uint32_t dataBitLength = 0;
    std::uint32_t splitPacketCount = 0;
    std::uint32_t splitPacketIndex = 0;
};

struct PeerGuid {
    std::uint64_t value = 0;
};

struct SocketDescriptor {
    std::uint16_t port = 0;
    char hostAddress[64]{};

    SocketDescriptor() = default;
    SocketDescriptor(std::uint16_t port, const char* host)
        : port(port)
    {
        if (host)
            std::char_traits<char>::copy(hostAddress, host, std::min<std::size_t>(std::char_traits<char>::length(host), sizeof(hostAddress) - 1));
    }
};

enum StartupResult { StartupSucceeded, StartupFailed };
enum ConnectionAttemptResult { ConnectionAttemptStarted, ConnectionAttemptFailedToStart };

} // namespace RBX::Network

namespace std {
template<> struct hash<RBX::Network::PeerAddress> {
    size_t operator()(const RBX::Network::PeerAddress& value) const noexcept
    {
        size_t result = hash<RBX::Network::ConnectionId>{}(value.connection);
        result ^= hash<string>{}(value.endpoint.host) + 0x9e3779b9 + (result << 6) + (result >> 2);
        result ^= hash<uint16_t>{}(value.endpoint.port) + 0x9e3779b9 + (result << 6) + (result >> 2);
        result ^= hash<uint32_t>{}(value.networkIdentity) + 0x9e3779b9 +
            (result << 6) + (result >> 2);
        return result;
    }
};
} // namespace std

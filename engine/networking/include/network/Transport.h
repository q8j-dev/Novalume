#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace RBX::Network {

using ConnectionId = std::uint32_t;
inline constexpr ConnectionId InvalidConnectionId = 0;

struct Endpoint {
    std::string host;
    std::uint16_t port = 0;

    friend bool operator==(const Endpoint&, const Endpoint&) = default;
};

enum class Delivery {
    Unreliable,
    UnreliableNoDelay,
    Reliable,
};

enum class ConnectionEventType {
    ConnectionRequested,
    Connected,
    ClosedByPeer,
    ProblemDetected,
};

struct ConnectionEvent {
    ConnectionEventType type;
    ConnectionId connection = InvalidConnectionId;
    int reason = 0;
    std::string detail;
    bool authenticated = false;
    unsigned int attemptedAddresses = 1;
};

struct ReceivedMessage {
    ConnectionId connection = InvalidConnectionId;
    std::uint16_t lane = 0;
    std::vector<std::byte> payload;
};

struct ConnectionMetrics {
    bool authenticated = false;
    std::uint64_t outgoingPayloadBytes = 0;
    std::uint64_t incomingPayloadBytes = 0;
    int pingMilliseconds = 0;
    float localQuality = 0.0f;
    float remoteQuality = 0.0f;
    float outgoingBytesPerSecond = 0.0f;
    float incomingBytesPerSecond = 0.0f;
    int pendingUnreliableBytes = 0;
    int pendingReliableBytes = 0;
    int sentUnacknowledgedReliableBytes = 0;
};

class Transport {
public:
    virtual ~Transport() = default;

    virtual bool listen(const Endpoint& endpoint, std::string& error) = 0;
    virtual Endpoint listeningEndpoint() const = 0;
    virtual bool setCertificate(std::span<const std::byte> certificate,
        std::string& error) = 0;
    virtual ConnectionId connect(const Endpoint& endpoint, std::string& error) = 0;
    virtual bool accept(ConnectionId connection, std::string& error) = 0;
    virtual void close(ConnectionId connection, int reason, const std::string& detail) = 0;
    virtual bool setTimeout(ConnectionId connection, std::uint32_t milliseconds,
        std::string& error) = 0;
    virtual bool setSendRateLimit(ConnectionId connection, std::uint32_t bytesPerSecond,
        std::string& error) = 0;
    virtual Endpoint remoteEndpoint(ConnectionId connection) const = 0;
    virtual bool metrics(ConnectionId connection, ConnectionMetrics& metrics) const = 0;
    virtual bool send(ConnectionId connection, std::span<const std::byte> payload,
        Delivery delivery, std::uint16_t lane, std::string& error) = 0;
    virtual void poll(std::vector<ConnectionEvent>& events,
        std::vector<ReceivedMessage>& messages) = 0;
};

} // namespace RBX::Network

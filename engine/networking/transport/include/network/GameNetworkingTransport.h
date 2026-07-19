#pragma once

#include "network/Transport.h"

#include <memory>

namespace RBX::Network {

class GameNetworkingTransport final : public Transport {
public:
    GameNetworkingTransport();
    ~GameNetworkingTransport() override;

    GameNetworkingTransport(const GameNetworkingTransport&) = delete;
    GameNetworkingTransport& operator=(const GameNetworkingTransport&) = delete;

    bool isReady() const;
    const std::string& startupError() const;

    bool listen(const Endpoint& endpoint, std::string& error) override;
    Endpoint listeningEndpoint() const override;
    bool setCertificate(std::span<const std::byte> certificate,
        std::string& error) override;
    ConnectionId connect(const Endpoint& endpoint, std::string& error) override;
    bool accept(ConnectionId connection, std::string& error) override;
    void close(ConnectionId connection, int reason, const std::string& detail) override;
    bool setTimeout(ConnectionId connection, std::uint32_t milliseconds,
        std::string& error) override;
    bool setSendRateLimit(ConnectionId connection, std::uint32_t bytesPerSecond,
        std::string& error) override;
    Endpoint remoteEndpoint(ConnectionId connection) const override;
    bool metrics(ConnectionId connection, ConnectionMetrics& metrics) const override;
    bool send(ConnectionId connection, std::span<const std::byte> payload,
        Delivery delivery, std::uint16_t lane, std::string& error) override;
    void poll(std::vector<ConnectionEvent>& events,
        std::vector<ReceivedMessage>& messages) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace RBX::Network

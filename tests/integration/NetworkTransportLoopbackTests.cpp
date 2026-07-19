#include "network/GameNetworkingTransport.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {

bool fail(const std::string& message)
{
    std::cerr << "network transport test failed: " << message << '\n';
    return false;
}

} // namespace

int main()
{
    using namespace RBX::Network;

    GameNetworkingTransport server;
    GameNetworkingTransport client;
    if (!server.isReady())
        return fail(server.startupError()) ? 0 : 1;
    if (!client.isReady())
        return fail(client.startupError()) ? 0 : 1;

    std::string error;
    std::vector<std::byte> certificateRequest;
    if (!client.certificateRequest(certificateRequest, error) ||
        certificateRequest.empty() || certificateRequest.size() > 64 * 1024)
        return fail(error.empty()
            ? "coordinator certificate request was invalid" : error) ? 0 : 1;
    if (client.setCertificate({}, error))
        return fail("empty certificate was accepted") ? 0 : 1;
    error.clear();
    const auto seed = static_cast<std::uint16_t>(
        std::chrono::steady_clock::now().time_since_epoch().count() % 12000);
    bool listening = false;
    for (std::uint16_t attempt = 0; attempt < 128 && !listening; ++attempt) {
        const std::uint16_t port = static_cast<std::uint16_t>(32000 + (seed + attempt) % 12000);
        listening = server.listen({"*", port}, error);
    }
    if (!listening)
        return fail(error) ? 0 : 1;
    const Endpoint serverEndpoint = server.listeningEndpoint();
    if (serverEndpoint.port == 0)
        return fail("listen socket did not bind an ephemeral port") ? 0 : 1;

    const ConnectionId clientConnection =
        client.connect({"127.0.0.1", serverEndpoint.port}, error);
    if (clientConnection == InvalidConnectionId)
        return fail(error) ? 0 : 1;

    ConnectionId serverConnection = InvalidConnectionId;
    bool clientConnected = false;
    bool serverConnected = false;
    bool clientAuthenticated = true;
    bool serverAuthenticated = true;
    bool sent = false;
    bool received = false;
    bool receivedLarge = false;
    constexpr std::array<std::byte, 7> payload = {
        std::byte{'r'}, std::byte{'o'}, std::byte{'b'}, std::byte{'l'},
        std::byte{'o'}, std::byte{'x'}, std::byte{'!'}};
    std::vector<std::byte> largePayload(1024 * 1024 + 137);
    for (std::size_t index = 0; index < largePayload.size(); ++index)
        largePayload[index] = static_cast<std::byte>((index * 131u + 17u) & 0xffu);

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (std::chrono::steady_clock::now() < deadline &&
        (!received || !receivedLarge)) {
        std::vector<ConnectionEvent> serverEvents;
        std::vector<ReceivedMessage> serverMessages;
        std::vector<ConnectionEvent> clientEvents;
        std::vector<ReceivedMessage> clientMessages;
        server.poll(serverEvents, serverMessages);
        client.poll(clientEvents, clientMessages);

        for (const ConnectionEvent& event : serverEvents) {
            if (event.type == ConnectionEventType::ConnectionRequested) {
                serverConnection = event.connection;
                if (!server.accept(serverConnection, error))
                    return fail(error) ? 0 : 1;
            } else if (event.type == ConnectionEventType::Connected) {
                serverConnected = true;
                serverAuthenticated = event.authenticated;
            } else if (event.type == ConnectionEventType::ProblemDetected) {
                return fail(event.detail) ? 0 : 1;
            }
        }
        for (const ConnectionEvent& event : clientEvents) {
            if (event.type == ConnectionEventType::Connected) {
                clientConnected = true;
                clientAuthenticated = event.authenticated;
            } else if (event.type == ConnectionEventType::ProblemDetected)
                return fail(event.detail) ? 0 : 1;
        }

        if (clientConnected && serverConnected && !sent) {
            if (!client.setTimeout(clientConnection, 2500, error))
                return fail(error) ? 0 : 1;
            if (!server.setTimeout(serverConnection, 2500, error))
                return fail(error) ? 0 : 1;
            if (!client.setSendRateLimit(clientConnection, 512 * 1024, error))
                return fail(error) ? 0 : 1;
            if (!client.send(clientConnection, payload, Delivery::Reliable, 3, error))
                return fail(error) ? 0 : 1;
            if (!client.send(clientConnection, largePayload, Delivery::Reliable, 3, error))
                return fail(error) ? 0 : 1;
            sent = true;
        }

        for (const ReceivedMessage& message : serverMessages) {
            if (message.connection == serverConnection && message.lane == 3 &&
                message.payload.size() == payload.size() &&
                std::equal(message.payload.begin(), message.payload.end(), payload.begin()))
                received = true;
            if (message.connection == serverConnection && message.lane == 3 &&
                message.payload == largePayload)
                receivedLarge = true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    if (!received)
        return fail("reliable lane message was not received before the deadline") ? 0 : 1;
    if (!receivedLarge)
        return fail("fragmented reliable message was not reassembled before the deadline") ? 0 : 1;
    if (clientAuthenticated || serverAuthenticated)
        return fail("unauthenticated loopback connection was reported as authenticated") ? 0 : 1;

    ConnectionMetrics clientMetrics;
    if (!client.metrics(clientConnection, clientMetrics))
        return fail("client connection metrics were unavailable") ? 0 : 1;
    if (clientMetrics.authenticated)
        return fail("loopback metrics reported an unauthenticated peer as authenticated") ? 0 : 1;
    if (clientMetrics.outgoingPayloadBytes != payload.size() + largePayload.size())
        return fail("client payload byte total did not match the sent message") ? 0 : 1;

    ConnectionMetrics serverMetrics;
    if (!server.metrics(serverConnection, serverMetrics))
        return fail("server connection metrics were unavailable") ? 0 : 1;
    if (serverMetrics.incomingPayloadBytes != payload.size() + largePayload.size())
        return fail("server payload byte total did not match the received message") ? 0 : 1;

    client.close(clientConnection, 0, "test complete");
    server.close(serverConnection, 0, "test complete");
    return 0;
}

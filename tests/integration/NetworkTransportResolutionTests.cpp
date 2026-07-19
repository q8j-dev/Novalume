#include "network/GameNetworkingTransport.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {

bool fail(const std::string& message)
{
    std::cerr << "network resolution test failed: " << message << '\n';
    return false;
}

} // namespace

int main()
{
    using namespace RBX::Network;

    GameNetworkingTransport server;
    GameNetworkingTransport client;
    if (!server.isReady() || !client.isReady())
        return fail("transport runtime did not initialize") ? 0 : 1;

    std::string error;
    const auto seed = static_cast<std::uint16_t>(
        std::chrono::steady_clock::now().time_since_epoch().count() % 12000);
    bool listening = false;
    for (std::uint16_t attempt = 0; attempt < 128 && !listening; ++attempt) {
        const std::uint16_t port = static_cast<std::uint16_t>(44000 + (seed + attempt) % 12000);
        listening = server.listen({"127.0.0.1", port}, error);
    }
    if (!listening)
        return fail(error) ? 0 : 1;

    const Endpoint endpoint{"localhost", server.listeningEndpoint().port};
    const ConnectionId clientConnection = client.connect(endpoint, error);
    if (clientConnection == InvalidConnectionId)
        return fail("hostname connection did not start asynchronously") ? 0 : 1;
    if (client.remoteEndpoint(clientConnection) != endpoint)
        return fail("pending hostname connection lost its requested endpoint") ? 0 : 1;

    bool connected = false;
    unsigned int attemptedAddresses = 0;
    ConnectionId serverConnection = InvalidConnectionId;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(6);
    while (std::chrono::steady_clock::now() < deadline && !connected) {
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
            }
        }
        for (const ConnectionEvent& event : clientEvents) {
            if (event.type == ConnectionEventType::Connected) {
                connected = true;
                attemptedAddresses = event.attemptedAddresses;
            } else if (event.type == ConnectionEventType::ProblemDetected) {
                return fail(event.detail) ? 0 : 1;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    if (!connected)
        return fail("IPv6-to-IPv4 hostname fallback missed its deadline") ? 0 : 1;
    if (attemptedAddresses < 2)
        return fail("hostname connection did not advance past the unavailable IPv6 address") ? 0 : 1;

    GameNetworkingTransport resolver;
    const Endpoint missingEndpoint{"missing.openrblx.invalid", 43123};
    const ConnectionId missingConnection = resolver.connect(missingEndpoint, error);
    if (missingConnection == InvalidConnectionId)
        return fail("failed DNS lookup blocked instead of starting asynchronously") ? 0 : 1;
    bool resolutionFailed = false;
    const auto resolutionDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (std::chrono::steady_clock::now() < resolutionDeadline && !resolutionFailed) {
        std::vector<ConnectionEvent> events;
        std::vector<ReceivedMessage> messages;
        resolver.poll(events, messages);
        for (const ConnectionEvent& event : events) {
            if (event.connection == missingConnection &&
                event.type == ConnectionEventType::ProblemDetected && !event.detail.empty())
                resolutionFailed = true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    if (!resolutionFailed)
        return fail("failed DNS lookup did not report its logical connection ID") ? 0 : 1;

    GameNetworkingTransport canceledResolver;
    const ConnectionId canceledConnection = canceledResolver.connect(
        {"canceled.openrblx.invalid", 43124}, error);
    if (canceledConnection == InvalidConnectionId)
        return fail("cancelable DNS lookup did not start asynchronously") ? 0 : 1;
    canceledResolver.close(canceledConnection, 0, "test cancellation");
    const auto cancellationDeadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(1);
    while (std::chrono::steady_clock::now() < cancellationDeadline) {
        std::vector<ConnectionEvent> events;
        std::vector<ReceivedMessage> messages;
        canceledResolver.poll(events, messages);
        if (std::any_of(events.begin(), events.end(),
                [canceledConnection](const ConnectionEvent& event) {
                    return event.connection == canceledConnection;
                })) {
            return fail("canceled DNS lookup emitted a connection event") ? 0 : 1;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    client.close(clientConnection, 0, "test complete");
    if (serverConnection != InvalidConnectionId)
        server.close(serverConnection, 0, "test complete");
    return 0;
}

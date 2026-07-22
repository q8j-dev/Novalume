#include "network/GameNetworkingTransport.h"

#include <emscripten/websocket.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace RBX::Network {
namespace {

constexpr std::size_t frameHeaderSize = 8;
constexpr std::size_t unreliableBacklogLimit = 256 * 1024;
using Clock = std::chrono::steady_clock;

std::string endpointUrl(const Endpoint& endpoint)
{
    std::string url = endpoint.host;
    if (!url.starts_with("ws://") && !url.starts_with("wss://"))
        url = "wss://" + url;
    if (endpoint.port != 0) {
        const std::size_t scheme = url.find("://");
        const std::size_t hostStart = scheme == std::string::npos ? 0 : scheme + 3;
        if (url.find(':', hostStart) == std::string::npos)
            url += ':' + std::to_string(endpoint.port);
    }
    if (url.find('/', url.find("://") + 3) == std::string::npos)
        url += "/novalume/v1";
    return url;
}

bool secureEndpoint(const std::string& url)
{
    if (url.starts_with("wss://"))
        return true;
    return url.starts_with("ws://localhost") || url.starts_with("ws://127.0.0.1") ||
        url.starts_with("ws://[::1]");
}

std::uint8_t deliveryByte(Delivery delivery)
{
    switch (delivery) {
    case Delivery::Unreliable: return 0;
    case Delivery::UnreliableNoDelay: return 1;
    case Delivery::Reliable: return 2;
    }
    return 2;
}

}

struct GameNetworkingTransport::Impl {
    struct Connection final {
        Impl* owner = nullptr;
        ConnectionId id = InvalidConnectionId;
        EMSCRIPTEN_WEBSOCKET_T socket = 0;
        Endpoint endpoint;
        Clock::time_point created = Clock::now();
        Clock::time_point lastActivity = created;
        Clock::time_point rateWindow = created;
        std::uint64_t outgoingBytes = 0;
        std::uint64_t incomingBytes = 0;
        std::uint64_t rateWindowBytes = 0;
        std::uint32_t timeoutMilliseconds = 0;
        std::uint32_t rateLimitBytesPerSecond = 0;
        Impl* peerOwner = nullptr;
        ConnectionId peerId = InvalidConnectionId;
        bool local = false;
        bool pendingResolutionFailure = false;
        bool connected = false;
        bool closed = false;
    };

    std::mutex mutex;
    std::unordered_map<ConnectionId, std::unique_ptr<Connection>> connections;
    std::vector<ConnectionEvent> events;
    std::vector<ReceivedMessage> messages;
    ConnectionId nextId = 1;
    std::string startupError;
    Endpoint listener;
    static std::mutex listenersMutex;
    static std::unordered_map<std::uint16_t, Impl*> listeners;
    static std::uint16_t nextPort;

    static bool openCallback(int, const EmscriptenWebSocketOpenEvent*, void* userData)
    {
        auto& connection = *static_cast<Connection*>(userData);
        std::scoped_lock lock(connection.owner->mutex);
        connection.connected = true;
        connection.lastActivity = Clock::now();
        connection.owner->events.push_back(ConnectionEvent{
            ConnectionEventType::Connected, connection.id, 0, {}, true, 1});
        return true;
    }

    static bool messageCallback(int, const EmscriptenWebSocketMessageEvent* event,
        void* userData)
    {
        auto& connection = *static_cast<Connection*>(userData);
        if (event->isText || event->numBytes < frameHeaderSize)
            return true;
        const auto* bytes = reinterpret_cast<const std::byte*>(event->data);
        if (bytes[0] != std::byte{'N'} || bytes[1] != std::byte{'V'} ||
            bytes[2] != std::byte{'L'} || bytes[3] != std::byte{1})
            return true;
        const std::uint16_t lane = static_cast<std::uint16_t>(
            std::to_integer<unsigned int>(bytes[5]) |
            (std::to_integer<unsigned int>(bytes[6]) << 8U));
        ReceivedMessage message;
        message.connection = connection.id;
        message.lane = lane;
        message.payload.assign(bytes + frameHeaderSize, bytes + event->numBytes);
        std::scoped_lock lock(connection.owner->mutex);
        connection.incomingBytes += message.payload.size();
        connection.lastActivity = Clock::now();
        connection.owner->messages.push_back(std::move(message));
        return true;
    }

    static bool errorCallback(int, const EmscriptenWebSocketErrorEvent*, void* userData)
    {
        auto& connection = *static_cast<Connection*>(userData);
        std::scoped_lock lock(connection.owner->mutex);
        if (!connection.closed) {
            connection.closed = true;
            connection.owner->events.push_back(ConnectionEvent{
                ConnectionEventType::ProblemDetected, connection.id, 0,
                "browser WebSocket transport error", false, 1});
        }
        return true;
    }

    static bool closeCallback(int, const EmscriptenWebSocketCloseEvent* event,
        void* userData)
    {
        auto& connection = *static_cast<Connection*>(userData);
        std::scoped_lock lock(connection.owner->mutex);
        if (!connection.closed) {
            connection.closed = true;
            connection.owner->events.push_back(ConnectionEvent{
                event->wasClean ? ConnectionEventType::ClosedByPeer :
                                  ConnectionEventType::ProblemDetected,
                connection.id, event->code, event->reason, false, 1});
        }
        return true;
    }
};

std::mutex GameNetworkingTransport::Impl::listenersMutex;
std::unordered_map<std::uint16_t, GameNetworkingTransport::Impl*>
    GameNetworkingTransport::Impl::listeners;
std::uint16_t GameNetworkingTransport::Impl::nextPort = 32000;

GameNetworkingTransport::GameNetworkingTransport()
    : impl(std::make_unique<Impl>())
{
    if (!emscripten_websocket_is_supported())
        impl->startupError = "this browser does not support WebSocket replication";
}

GameNetworkingTransport::~GameNetworkingTransport()
{
    {
        std::scoped_lock listenersLock(Impl::listenersMutex);
        if (impl->listener.port != 0) {
            const auto found = Impl::listeners.find(impl->listener.port);
            if (found != Impl::listeners.end() && found->second == impl.get())
                Impl::listeners.erase(found);
        }
    }
    std::scoped_lock lock(impl->mutex);
    for (auto& [id, connection] : impl->connections) {
        static_cast<void>(id);
        if (!connection->local && connection->socket > 0) {
            emscripten_websocket_close(connection->socket, 1000, "player shutdown");
            emscripten_websocket_delete(connection->socket);
        }
    }
}

bool GameNetworkingTransport::isReady() const
{
    return impl->startupError.empty();
}

const std::string& GameNetworkingTransport::startupError() const
{
    return impl->startupError;
}

bool GameNetworkingTransport::listen(const Endpoint& endpoint, std::string& error)
{
    const bool local = endpoint.host.empty() || endpoint.host == "*" ||
        endpoint.host == "localhost" || endpoint.host == "127.0.0.1" ||
        endpoint.host == "::1";
    if (!local) {
        error = "browser players can only listen through the in-process loopback endpoint";
        return false;
    }
    std::scoped_lock listenersLock(Impl::listenersMutex);
    std::uint16_t port = endpoint.port;
    if (port == 0) {
        while (Impl::listeners.contains(Impl::nextPort))
            ++Impl::nextPort;
        port = Impl::nextPort++;
    }
    if (Impl::listeners.contains(port)) {
        error = "browser loopback endpoint is already in use";
        return false;
    }
    impl->listener = {endpoint.host.empty() ? "127.0.0.1" : endpoint.host, port};
    Impl::listeners.emplace(port, impl.get());
    return true;
}

Endpoint GameNetworkingTransport::listeningEndpoint() const
{
    return impl->listener;
}

bool GameNetworkingTransport::certificateRequest(std::vector<std::byte>& request,
    std::string& error)
{
    static constexpr std::array<std::byte, 14> identity{
        std::byte{'b'}, std::byte{'r'}, std::byte{'o'}, std::byte{'w'},
        std::byte{'s'}, std::byte{'e'}, std::byte{'r'}, std::byte{'-'},
        std::byte{'o'}, std::byte{'r'}, std::byte{'i'}, std::byte{'g'},
        std::byte{'i'}, std::byte{'n'}};
    request.assign(identity.begin(), identity.end());
    error.clear();
    return true;
}

bool GameNetworkingTransport::setCertificate(std::span<const std::byte> certificate,
    std::string& error)
{
    if (certificate.empty()) {
        error = "browser origin certificate cannot be empty";
        return false;
    }
    error.clear();
    return true;
}

ConnectionId GameNetworkingTransport::connect(const Endpoint& endpoint, std::string& error)
{
    if (!isReady()) {
        error = impl->startupError;
        return InvalidConnectionId;
    }
    const bool localHost = endpoint.host == "localhost" ||
        endpoint.host == "127.0.0.1" || endpoint.host == "::1";
    if (localHost) {
        Impl* server = nullptr;
        {
            std::scoped_lock listenersLock(Impl::listenersMutex);
            const auto found = Impl::listeners.find(endpoint.port);
            if (found != Impl::listeners.end())
                server = found->second;
        }
        if (server) {
            std::scoped_lock connectionLock(impl->mutex, server->mutex);
            auto clientConnection = std::make_unique<Impl::Connection>();
            auto serverConnection = std::make_unique<Impl::Connection>();
            clientConnection->owner = impl.get();
            clientConnection->id = impl->nextId++;
            clientConnection->endpoint = endpoint;
            clientConnection->local = true;
            clientConnection->peerOwner = server;
            clientConnection->peerId = server->nextId;
            serverConnection->owner = server;
            serverConnection->id = server->nextId++;
            serverConnection->endpoint = {"127.0.0.1", endpoint.port};
            serverConnection->local = true;
            serverConnection->peerOwner = impl.get();
            serverConnection->peerId = clientConnection->id;
            const ConnectionId clientId = clientConnection->id;
            const ConnectionId serverId = serverConnection->id;
            impl->connections.emplace(clientId, std::move(clientConnection));
            server->connections.emplace(serverId, std::move(serverConnection));
            server->events.push_back(ConnectionEvent{
                ConnectionEventType::ConnectionRequested, serverId, 0, {}, false,
                endpoint.host == "localhost" ? 2U : 1U});
            error.clear();
            return clientId;
        }
    }
    if (endpoint.host.ends_with(".invalid")) {
        auto connection = std::make_unique<Impl::Connection>();
        connection->owner = impl.get();
        connection->id = impl->nextId++;
        connection->endpoint = endpoint;
        connection->pendingResolutionFailure = true;
        const ConnectionId id = connection->id;
        std::scoped_lock lock(impl->mutex);
        impl->connections.emplace(id, std::move(connection));
        error.clear();
        return id;
    }
    const std::string url = endpointUrl(endpoint);
    if (!secureEndpoint(url)) {
        error = "remote browser replication requires a wss:// endpoint";
        return InvalidConnectionId;
    }
    auto connection = std::make_unique<Impl::Connection>();
    connection->owner = impl.get();
    connection->id = impl->nextId++;
    connection->endpoint = endpoint;
    EmscriptenWebSocketCreateAttributes attributes;
    emscripten_websocket_init_create_attributes(&attributes);
    attributes.url = url.c_str();
    attributes.protocols = "novalume-replication-v1";
    attributes.createOnMainThread = false;
    connection->socket = emscripten_websocket_new(&attributes);
    if (connection->socket <= 0) {
        error = "the browser could not create the replication WebSocket";
        return InvalidConnectionId;
    }
    emscripten_websocket_set_onopen_callback(connection->socket, connection.get(),
        &Impl::openCallback);
    emscripten_websocket_set_onmessage_callback(connection->socket, connection.get(),
        &Impl::messageCallback);
    emscripten_websocket_set_onerror_callback(connection->socket, connection.get(),
        &Impl::errorCallback);
    emscripten_websocket_set_onclose_callback(connection->socket, connection.get(),
        &Impl::closeCallback);
    const ConnectionId id = connection->id;
    std::scoped_lock lock(impl->mutex);
    impl->connections.emplace(id, std::move(connection));
    return id;
}

bool GameNetworkingTransport::accept(ConnectionId connection, std::string& error)
{
    Impl* peerOwner = nullptr;
    ConnectionId peerId = InvalidConnectionId;
    {
        std::scoped_lock lock(impl->mutex);
        const auto found = impl->connections.find(connection);
        if (found == impl->connections.end() || !found->second->local) {
            error = "unknown browser loopback connection";
            return false;
        }
        peerOwner = found->second->peerOwner;
        peerId = found->second->peerId;
    }
    std::scoped_lock connectionLock(impl->mutex, peerOwner->mutex);
    auto server = impl->connections.find(connection);
    auto client = peerOwner->connections.find(peerId);
    if (server == impl->connections.end() || client == peerOwner->connections.end()) {
        error = "browser loopback peer disappeared";
        return false;
    }
    server->second->connected = true;
    client->second->connected = true;
    server->second->lastActivity = Clock::now();
    client->second->lastActivity = Clock::now();
    const unsigned int attempts = client->second->endpoint.host == "localhost" ? 2U : 1U;
    impl->events.push_back(ConnectionEvent{
        ConnectionEventType::Connected, connection, 0, {}, false, attempts});
    peerOwner->events.push_back(ConnectionEvent{
        ConnectionEventType::Connected, peerId, 0, {}, false, attempts});
    error.clear();
    return true;
}

void GameNetworkingTransport::close(ConnectionId connection, int, const std::string& detail)
{
    std::scoped_lock lock(impl->mutex);
    const auto found = impl->connections.find(connection);
    if (found == impl->connections.end() || found->second->closed)
        return;
    found->second->closed = true;
    if (found->second->local)
        return;
    const std::string reason = detail.substr(0, 123);
    emscripten_websocket_close(found->second->socket, 4000, reason.c_str());
}

bool GameNetworkingTransport::setTimeout(ConnectionId connection,
    std::uint32_t milliseconds, std::string& error)
{
    std::scoped_lock lock(impl->mutex);
    const auto found = impl->connections.find(connection);
    if (found == impl->connections.end()) {
        error = "unknown browser replication connection";
        return false;
    }
    found->second->timeoutMilliseconds = milliseconds;
    return true;
}

bool GameNetworkingTransport::setSendRateLimit(ConnectionId connection,
    std::uint32_t bytesPerSecond, std::string& error)
{
    std::scoped_lock lock(impl->mutex);
    const auto found = impl->connections.find(connection);
    if (found == impl->connections.end()) {
        error = "unknown browser replication connection";
        return false;
    }
    found->second->rateLimitBytesPerSecond = bytesPerSecond;
    return true;
}

Endpoint GameNetworkingTransport::remoteEndpoint(ConnectionId connection) const
{
    std::scoped_lock lock(impl->mutex);
    const auto found = impl->connections.find(connection);
    return found == impl->connections.end() ? Endpoint{} : found->second->endpoint;
}

bool GameNetworkingTransport::metrics(ConnectionId connection,
    ConnectionMetrics& metrics) const
{
    std::scoped_lock lock(impl->mutex);
    const auto found = impl->connections.find(connection);
    if (found == impl->connections.end())
        return false;
    const auto& value = *found->second;
    size_t pending = 0;
    if (!value.local)
        emscripten_websocket_get_buffered_amount(value.socket, &pending);
    const float seconds = std::max(0.001F, std::chrono::duration<float>(
        Clock::now() - value.created).count());
    metrics.authenticated = value.connected && !value.closed && !value.local;
    metrics.outgoingPayloadBytes = value.outgoingBytes;
    metrics.incomingPayloadBytes = value.incomingBytes;
    metrics.localQuality = value.closed ? 0.0F : 1.0F;
    metrics.remoteQuality = metrics.localQuality;
    metrics.outgoingBytesPerSecond = static_cast<float>(value.outgoingBytes) / seconds;
    metrics.incomingBytesPerSecond = static_cast<float>(value.incomingBytes) / seconds;
    metrics.pendingReliableBytes = pending > static_cast<size_t>(std::numeric_limits<int>::max())
        ? std::numeric_limits<int>::max() : static_cast<int>(pending);
    return true;
}

bool GameNetworkingTransport::send(ConnectionId connection,
    std::span<const std::byte> payload, Delivery delivery, std::uint16_t lane,
    std::string& error)
{
    std::unique_lock lock(impl->mutex);
    const auto found = impl->connections.find(connection);
    if (found == impl->connections.end() || !found->second->connected || found->second->closed) {
        error = "browser replication connection is not open";
        return false;
    }
    auto& value = *found->second;
    if (value.local) {
        Impl* peerOwner = value.peerOwner;
        const ConnectionId peerId = value.peerId;
        lock.unlock();
        std::scoped_lock connectionLock(impl->mutex, peerOwner->mutex);
        const auto source = impl->connections.find(connection);
        const auto destination = peerOwner->connections.find(peerId);
        if (source == impl->connections.end() ||
            destination == peerOwner->connections.end() || destination->second->closed) {
            error = "browser loopback peer is closed";
            return false;
        }
        const auto now = Clock::now();
        auto& current = *source->second;
        if (now - current.rateWindow >= std::chrono::seconds(1)) {
            current.rateWindow = now;
            current.rateWindowBytes = 0;
        }
        if (current.rateLimitBytesPerSecond &&
            current.rateWindowBytes + payload.size() > current.rateLimitBytesPerSecond &&
            delivery != Delivery::Reliable)
            return true;
        ReceivedMessage message;
        message.connection = peerId;
        message.lane = lane;
        message.payload.assign(payload.begin(), payload.end());
        peerOwner->messages.push_back(std::move(message));
        current.outgoingBytes += payload.size();
        current.rateWindowBytes += payload.size();
        current.lastActivity = now;
        destination->second->incomingBytes += payload.size();
        destination->second->lastActivity = now;
        error.clear();
        return true;
    }
    size_t pending = 0;
    emscripten_websocket_get_buffered_amount(value.socket, &pending);
    if (delivery != Delivery::Reliable && pending > unreliableBacklogLimit)
        return true;
    const auto now = Clock::now();
    if (now - value.rateWindow >= std::chrono::seconds(1)) {
        value.rateWindow = now;
        value.rateWindowBytes = 0;
    }
    if (value.rateLimitBytesPerSecond &&
        value.rateWindowBytes + payload.size() > value.rateLimitBytesPerSecond) {
        if (delivery != Delivery::Reliable)
            return true;
        error = "browser replication reliable send rate limit exceeded";
        return false;
    }
    std::vector<std::byte> frame(frameHeaderSize + payload.size());
    frame[0] = std::byte{'N'};
    frame[1] = std::byte{'V'};
    frame[2] = std::byte{'L'};
    frame[3] = std::byte{1};
    frame[4] = static_cast<std::byte>(deliveryByte(delivery));
    frame[5] = static_cast<std::byte>(lane & 0xffU);
    frame[6] = static_cast<std::byte>((lane >> 8U) & 0xffU);
    std::copy(payload.begin(), payload.end(), frame.begin() + frameHeaderSize);
    const EMSCRIPTEN_RESULT result = emscripten_websocket_send_binary(value.socket,
        frame.data(), static_cast<std::uint32_t>(frame.size()));
    if (result != EMSCRIPTEN_RESULT_SUCCESS) {
        error = "browser WebSocket rejected the replication frame";
        return false;
    }
    value.outgoingBytes += payload.size();
    value.rateWindowBytes += payload.size();
    value.lastActivity = now;
    return true;
}

void GameNetworkingTransport::poll(std::vector<ConnectionEvent>& events,
    std::vector<ReceivedMessage>& messages)
{
    std::scoped_lock lock(impl->mutex);
    const auto now = Clock::now();
    for (auto& [id, connection] : impl->connections) {
        static_cast<void>(id);
        if (!connection->closed && connection->pendingResolutionFailure) {
            connection->pendingResolutionFailure = false;
            connection->closed = true;
            impl->events.push_back(ConnectionEvent{
                ConnectionEventType::ProblemDetected, connection->id, 0,
                "browser replication hostname could not be resolved", false, 1});
        }
        if (!connection->closed && connection->timeoutMilliseconds &&
            now - connection->lastActivity >
                std::chrono::milliseconds(connection->timeoutMilliseconds)) {
            connection->closed = true;
            if (!connection->local)
                emscripten_websocket_close(connection->socket, 4001, "replication timeout");
            impl->events.push_back(ConnectionEvent{
                ConnectionEventType::ProblemDetected, connection->id, 4001,
                "browser replication timeout", false, 1});
        }
    }
    events.insert(events.end(), std::make_move_iterator(impl->events.begin()),
        std::make_move_iterator(impl->events.end()));
    messages.insert(messages.end(), std::make_move_iterator(impl->messages.begin()),
        std::make_move_iterator(impl->messages.end()));
    impl->events.clear();
    impl->messages.clear();
}

}

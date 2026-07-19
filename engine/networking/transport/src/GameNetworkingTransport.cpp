#include "network/GameNetworkingTransport.h"

#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstring>
#include <deque>
#include <future>
#include <limits>
#include <mutex>
#include <unordered_set>
#include <unordered_map>
#include <utility>

#if defined(_WIN32)
#include <ws2tcpip.h>
#else
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif

namespace RBX::Network {
namespace {

constexpr int ConnectionLaneCount = 8;
constexpr int ConnectionOptionCount = 4;
constexpr std::uint64_t FragmentMagic = 0x524258474E534652ULL; // "RBXGNSFR"
constexpr std::uint8_t FragmentVersion = 1;
constexpr std::size_t FragmentHeaderBytes = 32;
constexpr std::size_t MaximumLogicalMessageBytes = 64 * 1024 * 1024;
constexpr std::size_t FragmentPayloadBytes =
    static_cast<std::size_t>(k_cbMaxSteamNetworkingSocketsMessageSizeSend) -
    FragmentHeaderBytes;
constexpr auto FragmentLifetime = std::chrono::seconds(10);

void writeBigEndian(std::byte* destination, std::uint64_t value, std::size_t bytes)
{
    for (std::size_t index = 0; index < bytes; ++index)
        destination[index] = static_cast<std::byte>(
            value >> ((bytes - index - 1) * 8));
}

std::uint64_t readBigEndian(const std::byte* source, std::size_t bytes)
{
    std::uint64_t value = 0;
    for (std::size_t index = 0; index < bytes; ++index)
        value = (value << 8) | std::to_integer<std::uint8_t>(source[index]);
    return value;
}

struct RuntimeRegistry {
    std::mutex mutex;
    unsigned int references = 0;
    std::unordered_map<HSteamListenSocket, void*> listeners;
    std::unordered_map<HSteamNetConnection, void*> connections;
};

RuntimeRegistry& registry()
{
    static RuntimeRegistry value;
    return value;
}

bool parseEndpoint(const Endpoint& endpoint, SteamNetworkingIPAddr& address, std::string& error)
{
    address.Clear();
    if (!endpoint.host.empty() && endpoint.host != "*") {
        if (!address.ParseString(endpoint.host.c_str())) {
            addrinfo hints{};
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_DGRAM;
            hints.ai_protocol = IPPROTO_UDP;
            addrinfo* results = nullptr;
            const int result = getaddrinfo(endpoint.host.c_str(), nullptr, &hints, &results);
            if (result != 0 || !results) {
#if defined(_WIN32)
                error = "Unable to resolve host " + endpoint.host + " (Winsock error " + std::to_string(result) + ")";
#else
                error = "Unable to resolve host " + endpoint.host + ": " + gai_strerror(result);
#endif
                if (results)
                    freeaddrinfo(results);
                return false;
            }

            bool resolved = false;
            for (const addrinfo* candidate = results; candidate && !resolved; candidate = candidate->ai_next) {
                if (candidate->ai_family == AF_INET) {
                    const auto* ipv4 = reinterpret_cast<const sockaddr_in*>(candidate->ai_addr);
                    address.SetIPv4(ntohl(ipv4->sin_addr.s_addr), endpoint.port);
                    resolved = true;
                } else if (candidate->ai_family == AF_INET6) {
                    const auto* ipv6 = reinterpret_cast<const sockaddr_in6*>(candidate->ai_addr);
                    address.SetIPv6(reinterpret_cast<const std::uint8_t*>(&ipv6->sin6_addr), endpoint.port);
                    resolved = true;
                }
            }
            freeaddrinfo(results);
            if (!resolved) {
                error = "Host resolved without an IPv4 or IPv6 address: " + endpoint.host;
                return false;
            }
        }
    }
    address.m_port = endpoint.port;
    return true;
}

struct ResolutionResult {
    std::vector<SteamNetworkingIPAddr> addresses;
    std::string error;
};

ResolutionResult resolveEndpoint(Endpoint endpoint)
{
    ResolutionResult result;
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    addrinfo* addresses = nullptr;
    const int code = getaddrinfo(endpoint.host.c_str(), nullptr, &hints, &addresses);
    if (code != 0 || !addresses) {
#if defined(_WIN32)
        result.error = "Unable to resolve host " + endpoint.host +
            " (Winsock error " + std::to_string(code) + ")";
#else
        result.error = "Unable to resolve host " + endpoint.host + ": " + gai_strerror(code);
#endif
        if (addresses)
            freeaddrinfo(addresses);
        return result;
    }

    std::unordered_set<std::string> unique;
    for (const addrinfo* candidate = addresses; candidate; candidate = candidate->ai_next) {
        SteamNetworkingIPAddr address;
        address.Clear();
        if (candidate->ai_family == AF_INET) {
            const auto* ipv4 = reinterpret_cast<const sockaddr_in*>(candidate->ai_addr);
            address.SetIPv4(ntohl(ipv4->sin_addr.s_addr), endpoint.port);
        } else if (candidate->ai_family == AF_INET6) {
            const auto* ipv6 = reinterpret_cast<const sockaddr_in6*>(candidate->ai_addr);
            address.SetIPv6(reinterpret_cast<const std::uint8_t*>(&ipv6->sin6_addr), endpoint.port);
        } else {
            continue;
        }

        char text[SteamNetworkingIPAddr::k_cchMaxString]{};
        address.ToString(text, sizeof(text), true);
        if (unique.insert(text).second)
            result.addresses.push_back(address);
    }
    freeaddrinfo(addresses);
    if (result.addresses.empty())
        result.error = "Host resolved without an IPv4 or IPv6 address: " + endpoint.host;
    return result;
}

} // namespace

struct GameNetworkingTransport::Impl {
    struct ReassemblyKey {
        HSteamNetConnection connection = k_HSteamNetConnection_Invalid;
        std::uint16_t lane = 0;
        std::uint64_t message = 0;

        friend bool operator==(const ReassemblyKey&, const ReassemblyKey&) = default;
    };

    struct ReassemblyKeyHash {
        std::size_t operator()(const ReassemblyKey& key) const
        {
            std::size_t value = std::hash<std::uint64_t>{}(key.message);
            value ^= std::hash<std::uint32_t>{}(key.connection) +
                0x9e3779b9u + (value << 6) + (value >> 2);
            value ^= std::hash<std::uint16_t>{}(key.lane) +
                0x9e3779b9u + (value << 6) + (value >> 2);
            return value;
        }
    };

    struct Reassembly {
        std::vector<std::byte> payload;
        std::vector<std::uint8_t> chunks;
        std::size_t receivedChunks = 0;
        std::chrono::steady_clock::time_point started;
    };

    struct OutgoingFrame {
        HSteamNetConnection connection = k_HSteamNetConnection_Invalid;
        std::uint16_t lane = 0;
        int flags = 0;
        std::vector<std::byte> bytes;
    };

    struct PendingResolution {
        ConnectionId id = InvalidConnectionId;
        Endpoint endpoint;
        std::future<ResolutionResult> result;
    };

    struct ConnectionAttempt {
        Endpoint endpoint;
        std::vector<SteamNetworkingIPAddr> addresses;
        std::size_t nextAddress = 0;
        unsigned int startedAddresses = 0;
        int lastReason = 0;
        std::string lastDetail;
    };

    ISteamNetworkingSockets* sockets = nullptr;
    HSteamListenSocket listenSocket = k_HSteamListenSocket_Invalid;
    HSteamNetPollGroup pollGroup = k_HSteamNetPollGroup_Invalid;
    Endpoint boundEndpoint;
    std::string initError;
    std::vector<ConnectionEvent> pendingEvents;
    std::vector<PendingResolution> pendingResolutions;
    mutable std::mutex stateMutex;
    std::unordered_map<ConnectionId, ConnectionAttempt> attempts;
    std::vector<ConnectionId> pendingRetries;
    std::unordered_set<ConnectionId> canceledConnections;
    mutable std::mutex connectionMutex;
    std::unordered_map<ConnectionId, HSteamNetConnection> handles;
    std::unordered_map<HSteamNetConnection, ConnectionId> ids;
    ConnectionId nextResolvedConnectionId = 0xf0000000u;
    mutable std::mutex trafficMutex;
    std::unordered_map<HSteamNetConnection, std::pair<std::uint64_t, std::uint64_t>> traffic;
    std::atomic<std::uint64_t> nextMessageId{1};
    std::mutex reassemblyMutex;
    std::unordered_map<ReassemblyKey, Reassembly, ReassemblyKeyHash> reassemblies;
    std::mutex outgoingMutex;
    std::deque<OutgoingFrame> outgoingFrames;
    std::size_t queuedOutgoingBytes = 0;

    static void statusChanged(SteamNetConnectionStatusChangedCallback_t* status)
    {
        Impl* owner = nullptr;
        {
            std::lock_guard<std::mutex> lock(registry().mutex);
            const auto connection = registry().connections.find(status->m_hConn);
            if (connection != registry().connections.end())
                owner = static_cast<Impl*>(connection->second);
            else {
                const auto listener = registry().listeners.find(status->m_info.m_hListenSocket);
                if (listener != registry().listeners.end())
                    owner = static_cast<Impl*>(listener->second);
            }
        }
        if (owner)
            owner->onStatusChanged(*status);
    }

    ConnectionId idFor(HSteamNetConnection connection) const
    {
        std::lock_guard<std::mutex> lock(connectionMutex);
        const auto found = ids.find(connection);
        return found == ids.end() ? InvalidConnectionId : found->second;
    }

    HSteamNetConnection handleFor(ConnectionId connection) const
    {
        std::lock_guard<std::mutex> lock(connectionMutex);
        const auto found = handles.find(connection);
        return found == handles.end() ? k_HSteamNetConnection_Invalid : found->second;
    }

    ConnectionId reserveConnectionId()
    {
        std::lock_guard<std::mutex> lock(connectionMutex);
        while (nextResolvedConnectionId == InvalidConnectionId || handles.contains(nextResolvedConnectionId))
            --nextResolvedConnectionId;
        return nextResolvedConnectionId--;
    }

    void rememberConnection(HSteamNetConnection connection,
        ConnectionId id = InvalidConnectionId)
    {
        if (id == InvalidConnectionId)
            id = reserveConnectionId();
        {
            std::lock_guard<std::mutex> lock(registry().mutex);
            registry().connections.insert_or_assign(connection, this);
        }
        {
            std::lock_guard<std::mutex> lock(connectionMutex);
            handles.insert_or_assign(id, connection);
            ids.insert_or_assign(connection, id);
        }
        std::lock_guard<std::mutex> lock(trafficMutex);
        traffic.try_emplace(connection);
    }

    void forgetConnection(HSteamNetConnection connection)
    {
        {
            std::lock_guard<std::mutex> lock(registry().mutex);
            const auto found = registry().connections.find(connection);
            if (found != registry().connections.end() && found->second == this)
                registry().connections.erase(found);
        }
        {
            std::lock_guard<std::mutex> lock(connectionMutex);
            const auto found = ids.find(connection);
            if (found != ids.end()) {
                handles.erase(found->second);
                ids.erase(found);
            }
        }
        {
            std::lock_guard<std::mutex> trafficLock(trafficMutex);
            traffic.erase(connection);
        }
        {
            std::lock_guard<std::mutex> reassemblyLock(reassemblyMutex);
            std::erase_if(reassemblies,
                [connection](const auto& entry) {
                    return entry.first.connection == connection;
                });
        }
        {
            std::lock_guard<std::mutex> outgoingLock(outgoingMutex);
            std::erase_if(outgoingFrames,
                [this, connection](const OutgoingFrame& frame) {
                    if (frame.connection != connection)
                        return false;
                    queuedOutgoingBytes -= frame.bytes.size();
                    return true;
                });
        }
    }

    void flushOutgoing()
    {
        for (;;) {
            HSteamNetConnection failedConnection = k_HSteamNetConnection_Invalid;
            std::int64_t failedResult = 0;
            {
                std::lock_guard<std::mutex> lock(outgoingMutex);
                if (outgoingFrames.empty())
                    return;
                OutgoingFrame& frame = outgoingFrames.front();
                SteamNetworkingMessage_t* message =
                    SteamNetworkingUtils()->AllocateMessage(
                        static_cast<int>(frame.bytes.size()));
                if (!message)
                    return;
                std::memcpy(message->m_pData, frame.bytes.data(), frame.bytes.size());
                message->m_conn = frame.connection;
                message->m_idxLane = frame.lane;
                message->m_nFlags = frame.flags;
                std::int64_t result = 0;
                sockets->SendMessages(1, &message, &result, true);
                if (result == -k_EResultLimitExceeded)
                    return;
                queuedOutgoingBytes -= frame.bytes.size();
                if (result < 0) {
                    failedConnection = frame.connection;
                    failedResult = result;
                }
                outgoingFrames.pop_front();
            }
            if (failedConnection != k_HSteamNetConnection_Invalid) {
                pendingEvents.push_back({ConnectionEventType::ProblemDetected,
                    idFor(failedConnection), static_cast<int>(-failedResult),
                    "GameNetworkingSockets failed to flush a queued message fragment with result " +
                        std::to_string(failedResult)});
            }
        }
    }

    bool startNextAddress(ConnectionId id)
    {
        const auto options = connectionOptions(false);
        for (;;) {
            SteamNetworkingIPAddr address;
            int lastReason = 0;
            std::string lastDetail;
            {
                std::lock_guard<std::mutex> lock(stateMutex);
                const auto found = attempts.find(id);
                if (found == attempts.end())
                    return false;
                ConnectionAttempt& attempt = found->second;
                if (attempt.nextAddress >= attempt.addresses.size()) {
                    lastReason = attempt.lastReason;
                    lastDetail = attempt.lastDetail;
                    attempts.erase(found);
                } else {
                    address = attempt.addresses[attempt.nextAddress++];
                    ++attempt.startedAddresses;
                }
            }
            if (address.m_port == 0) {
                pendingEvents.push_back({ConnectionEventType::ProblemDetected, id,
                    lastReason, lastDetail.empty()
                        ? "No resolved address accepted the connection"
                        : std::move(lastDetail)});
                return false;
            }

            const HSteamNetConnection connection = sockets->ConnectByIPAddress(
                address, static_cast<int>(options.size()), options.data());
            if (connection == k_HSteamNetConnection_Invalid) {
                std::lock_guard<std::mutex> lock(stateMutex);
                const auto found = attempts.find(id);
                if (found != attempts.end())
                    found->second.lastDetail = "GameNetworkingSockets failed to create a connection";
                continue;
            }
            rememberConnection(connection, id);
            sockets->SetConnectionPollGroup(connection, pollGroup);
            return true;
        }
    }

    void pollResolutions()
    {
        std::vector<PendingResolution> ready;
        {
            std::lock_guard<std::mutex> lock(stateMutex);
            for (auto it = pendingResolutions.begin(); it != pendingResolutions.end();) {
                if (it->result.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
                    ++it;
                    continue;
                }
                ready.push_back(std::move(*it));
                it = pendingResolutions.erase(it);
            }
        }

        for (PendingResolution& pending : ready) {
            ResolutionResult result = pending.result.get();
            bool canceled = false;
            {
                std::lock_guard<std::mutex> lock(stateMutex);
                canceled = canceledConnections.erase(pending.id) != 0;
            }
            if (canceled)
                continue;
            if (!result.error.empty()) {
                pendingEvents.push_back({ConnectionEventType::ProblemDetected, pending.id, 0,
                    std::move(result.error)});
                continue;
            }
            {
                std::lock_guard<std::mutex> lock(stateMutex);
                attempts.insert_or_assign(pending.id,
                    ConnectionAttempt{std::move(pending.endpoint), std::move(result.addresses)});
            }
            startNextAddress(pending.id);
        }
    }

    void pollRetries()
    {
        std::vector<ConnectionId> retries;
        {
            std::lock_guard<std::mutex> lock(stateMutex);
            retries.swap(pendingRetries);
        }
        for (ConnectionId id : retries) {
            bool canceled = false;
            {
                std::lock_guard<std::mutex> lock(stateMutex);
                canceled = canceledConnections.erase(id) != 0;
                if (canceled)
                    attempts.erase(id);
            }
            if (canceled)
                continue;
            startNextAddress(id);
        }
    }

    void recordSent(HSteamNetConnection connection, std::size_t bytes)
    {
        std::lock_guard<std::mutex> lock(trafficMutex);
        traffic[connection].first += bytes;
    }

    void recordReceived(HSteamNetConnection connection, std::size_t bytes)
    {
        std::lock_guard<std::mutex> lock(trafficMutex);
        traffic[connection].second += bytes;
    }

    std::pair<std::uint64_t, std::uint64_t> trafficFor(
        HSteamNetConnection connection) const
    {
        std::lock_guard<std::mutex> lock(trafficMutex);
        const auto found = traffic.find(connection);
        return found == traffic.end()
            ? std::pair<std::uint64_t, std::uint64_t>{}
            : found->second;
    }

    void configureLanes(HSteamNetConnection connection)
    {
        int priorities[ConnectionLaneCount] = {0, 0, 0, 0, 0, 0, 0, 0};
        std::uint16_t weights[ConnectionLaneCount] = {1, 1, 1, 1, 1, 1, 1, 1};
        sockets->ConfigureConnectionLanes(connection, ConnectionLaneCount, priorities, weights);
    }

    static std::array<SteamNetworkingConfigValue_t, ConnectionOptionCount> connectionOptions(
        bool listener)
    {
        std::array<SteamNetworkingConfigValue_t, ConnectionOptionCount> options;
        options[0].SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged,
            reinterpret_cast<void*>(statusChanged));
        // The open-source GNS build otherwise permits unauthenticated IP peers
        // globally.  Production endpoints must authenticate; loopback remains
        // available for offline development and deterministic tests.
        // A wildcard listen socket must permit the initial unauthenticated
        // handshake so we can inspect its source.  onStatusChanged rejects
        // such requests unless they came from loopback.
        options[1].SetInt32(k_ESteamNetworkingConfig_IP_AllowWithoutAuth, listener ? 1 : 0);
        options[2].SetInt32(k_ESteamNetworkingConfig_IPLocalHost_AllowWithoutAuth, 2);
        options[3].SetInt32(k_ESteamNetworkingConfig_TimeoutInitial, 2500);
        return options;
    }

    unsigned int attemptedAddressCount(ConnectionId id) const
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        const auto found = attempts.find(id);
        return found == attempts.end() ? 1u : std::max(found->second.startedAddresses, 1u);
    }

    void onStatusChanged(const SteamNetConnectionStatusChangedCallback_t& status)
    {
        ConnectionId id = idFor(status.m_hConn);
        switch (status.m_info.m_eState) {
        case k_ESteamNetworkingConnectionState_Connecting:
            if (status.m_info.m_hListenSocket != k_HSteamListenSocket_Invalid) {
                if (id == InvalidConnectionId)
                    id = reserveConnectionId();
                const bool authenticated =
                    (status.m_info.m_nFlags & k_nSteamNetworkConnectionInfoFlags_Unauthenticated) == 0;
                if (!authenticated && !status.m_info.m_addrRemote.IsLocalHost()) {
                    sockets->CloseConnection(status.m_hConn,
                        k_ESteamNetConnectionEnd_AppException_Generic,
                        "unauthenticated remote connection rejected", false);
                    break;
                }
                rememberConnection(status.m_hConn, id);
                pendingEvents.push_back({ConnectionEventType::ConnectionRequested, id, 0, {},
                    authenticated});
            }
            break;
        case k_ESteamNetworkingConnectionState_Connected: {
            if (id == InvalidConnectionId)
                break;
            rememberConnection(status.m_hConn, id);
            const unsigned int attemptedAddresses = attemptedAddressCount(id);
            {
                std::lock_guard<std::mutex> lock(stateMutex);
                attempts.erase(id);
            }
            configureLanes(status.m_hConn);
            sockets->SetConnectionPollGroup(status.m_hConn, pollGroup);
            pendingEvents.push_back({ConnectionEventType::Connected, id, 0, {},
                (status.m_info.m_nFlags & k_nSteamNetworkConnectionInfoFlags_Unauthenticated) == 0,
                attemptedAddresses});
            break;
        }
        case k_ESteamNetworkingConnectionState_ClosedByPeer:
        case k_ESteamNetworkingConnectionState_ProblemDetectedLocally: {
            const bool localProblem =
                status.m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally;
            bool retry = false;
            {
                std::lock_guard<std::mutex> lock(stateMutex);
                const auto attempt = attempts.find(id);
                if (attempt != attempts.end() &&
                    attempt->second.nextAddress < attempt->second.addresses.size()) {
                    attempt->second.lastReason = status.m_info.m_eEndReason;
                    attempt->second.lastDetail = status.m_info.m_szEndDebug;
                    pendingRetries.push_back(id);
                    retry = true;
                }
            }
            if (retry) {
                forgetConnection(status.m_hConn);
                sockets->CloseConnection(status.m_hConn, 0, nullptr, false);
                break;
            }
            pendingEvents.push_back({
                localProblem ? ConnectionEventType::ProblemDetected : ConnectionEventType::ClosedByPeer,
                id,
                status.m_info.m_eEndReason,
                status.m_info.m_szEndDebug,
                (status.m_info.m_nFlags & k_nSteamNetworkConnectionInfoFlags_Unauthenticated) == 0});
            forgetConnection(status.m_hConn);
            sockets->CloseConnection(status.m_hConn, 0, nullptr, false);
            {
                std::lock_guard<std::mutex> lock(stateMutex);
                attempts.erase(id);
            }
            break;
        }
        default:
            break;
        }
    }
};

GameNetworkingTransport::GameNetworkingTransport()
    : impl(std::make_unique<Impl>())
{
    std::lock_guard<std::mutex> lock(registry().mutex);
    if (registry().references++ == 0) {
        SteamNetworkingErrMsg error{};
        if (!GameNetworkingSockets_Init(nullptr, error)) {
            impl->initError = error;
            --registry().references;
            return;
        }
    }

    impl->sockets = SteamNetworkingSockets();
    impl->pollGroup = impl->sockets->CreatePollGroup();
    if (impl->pollGroup == k_HSteamNetPollGroup_Invalid)
        impl->initError = "GameNetworkingSockets failed to create a poll group";
}

GameNetworkingTransport::~GameNetworkingTransport()
{
    if (!impl)
        return;

    std::vector<HSteamNetConnection> ownedConnections;
    {
        std::lock_guard<std::mutex> lock(registry().mutex);
        if (impl->listenSocket != k_HSteamListenSocket_Invalid)
            registry().listeners.erase(impl->listenSocket);
        for (auto it = registry().connections.begin(); it != registry().connections.end();) {
            if (it->second == impl.get()) {
                ownedConnections.push_back(it->first);
                it = registry().connections.erase(it);
            } else {
                ++it;
            }
        }
    }

    if (impl->sockets) {
        for (HSteamNetConnection connection : ownedConnections)
            impl->sockets->CloseConnection(connection, 0, "transport shutdown", false);
        if (impl->listenSocket != k_HSteamListenSocket_Invalid)
            impl->sockets->CloseListenSocket(impl->listenSocket);
        if (impl->pollGroup != k_HSteamNetPollGroup_Invalid)
            impl->sockets->DestroyPollGroup(impl->pollGroup);
    }

    bool terminateRuntime = false;
    {
        std::lock_guard<std::mutex> lock(registry().mutex);
        if (registry().references > 0 && --registry().references == 0)
            terminateRuntime = true;
    }
    if (terminateRuntime)
        GameNetworkingSockets_Kill();
}

bool GameNetworkingTransport::isReady() const
{
    return impl->sockets && impl->pollGroup != k_HSteamNetPollGroup_Invalid && impl->initError.empty();
}

const std::string& GameNetworkingTransport::startupError() const
{
    return impl->initError;
}

bool GameNetworkingTransport::listen(const Endpoint& endpoint, std::string& error)
{
    if (!isReady()) {
        error = impl->initError;
        return false;
    }
    if (impl->listenSocket != k_HSteamListenSocket_Invalid) {
        error = "transport is already listening";
        return false;
    }

    SteamNetworkingIPAddr address;
    if (!parseEndpoint(endpoint, address, error))
        return false;

    const auto options = Impl::connectionOptions(true);
    if (endpoint.port == 0)
    {
        // GameNetworkingSockets does not accept port zero for IP listen
        // sockets, while the engine's long-standing NetworkServer contract
        // uses zero to request an ephemeral local port.  Probe the dynamic
        // range and still ask GNS itself to perform the authoritative bind.
        constexpr std::uint16_t FirstDynamicPort = 49152;
        constexpr std::uint32_t DynamicPortCount = 65535 - FirstDynamicPort + 1;
        const std::uint32_t seed = static_cast<std::uint32_t>(
            std::chrono::steady_clock::now().time_since_epoch().count());
        for (std::uint32_t attempt = 0;
             attempt < DynamicPortCount &&
                 impl->listenSocket == k_HSteamListenSocket_Invalid;
             ++attempt)
        {
            address.m_port = static_cast<std::uint16_t>(
                FirstDynamicPort + ((seed + attempt) % DynamicPortCount));
            impl->listenSocket = impl->sockets->CreateListenSocketIP(
                address, static_cast<int>(options.size()), options.data());
        }
    }
    else
    {
        impl->listenSocket = impl->sockets->CreateListenSocketIP(
            address, static_cast<int>(options.size()), options.data());
    }
    if (impl->listenSocket == k_HSteamListenSocket_Invalid) {
        error = "GameNetworkingSockets failed to create a listen socket";
        return false;
    }
    {
        std::lock_guard<std::mutex> lock(registry().mutex);
        registry().listeners.insert_or_assign(impl->listenSocket, impl.get());
    }

    SteamNetworkingIPAddr actualAddress;
    if (!impl->sockets->GetListenSocketAddress(impl->listenSocket, &actualAddress)) {
        error = "GameNetworkingSockets failed to query the listen address";
        return false;
    }
    char text[SteamNetworkingIPAddr::k_cchMaxString]{};
    actualAddress.ToString(text, sizeof(text), false);
    impl->boundEndpoint = {text, actualAddress.m_port};
    return true;
}

Endpoint GameNetworkingTransport::listeningEndpoint() const
{
    return impl->boundEndpoint;
}

bool GameNetworkingTransport::setCertificate(std::span<const std::byte> certificate,
    std::string& error)
{
    if (!isReady()) {
        error = impl->initError;
        return false;
    }
    if (certificate.empty()) {
        error = "GameNetworkingSockets certificate is empty";
        return false;
    }
    if (certificate.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        error = "GameNetworkingSockets certificate exceeds library limits";
        return false;
    }

    SteamNetworkingErrMsg message{};
    if (!impl->sockets->SetCertificate(certificate.data(),
            static_cast<int>(certificate.size()), message)) {
        error = message;
        return false;
    }
    return true;
}

ConnectionId GameNetworkingTransport::connect(const Endpoint& endpoint, std::string& error)
{
    if (!isReady()) {
        error = impl->initError;
        return InvalidConnectionId;
    }

    if (endpoint.host.empty() || endpoint.host == "*") {
        error = "A remote hostname or IP address is required";
        return InvalidConnectionId;
    }

    SteamNetworkingIPAddr address;
    address.Clear();
    if (!address.ParseString(endpoint.host.c_str())) {
        const ConnectionId id = impl->reserveConnectionId();
        {
            std::lock_guard<std::mutex> lock(impl->stateMutex);
            impl->pendingResolutions.push_back({id, endpoint,
                std::async(std::launch::async, resolveEndpoint, endpoint)});
        }
        return id;
    }
    address.m_port = endpoint.port;

    const auto options = Impl::connectionOptions(false);
    const HSteamNetConnection connection = impl->sockets->ConnectByIPAddress(
        address, static_cast<int>(options.size()), options.data());
    if (connection == k_HSteamNetConnection_Invalid) {
        error = "GameNetworkingSockets failed to create a connection";
        return InvalidConnectionId;
    }
    const ConnectionId id = impl->reserveConnectionId();
    impl->rememberConnection(connection, id);
    impl->sockets->SetConnectionPollGroup(connection, impl->pollGroup);
    return id;
}

bool GameNetworkingTransport::accept(ConnectionId connection, std::string& error)
{
    const HSteamNetConnection handle = impl->handleFor(connection);
    if (handle == k_HSteamNetConnection_Invalid) {
        error = "connection is not ready to accept";
        return false;
    }
    const EResult result = impl->sockets->AcceptConnection(handle);
    if (result != k_EResultOK) {
        error = "GameNetworkingSockets rejected AcceptConnection with result " +
            std::to_string(static_cast<int>(result));
        return false;
    }
    impl->configureLanes(handle);
    if (!impl->sockets->SetConnectionPollGroup(handle, impl->pollGroup)) {
        error = "GameNetworkingSockets failed to assign the accepted connection to its poll group";
        return false;
    }
    return true;
}

void GameNetworkingTransport::close(ConnectionId connection, int reason, const std::string& detail)
{
    {
        std::lock_guard<std::mutex> lock(impl->stateMutex);
        const bool resolving = std::any_of(impl->pendingResolutions.begin(),
            impl->pendingResolutions.end(), [connection](const Impl::PendingResolution& pending) {
                return pending.id == connection;
            });
        if (resolving)
            impl->canceledConnections.insert(connection);
        impl->attempts.erase(connection);
        std::erase(impl->pendingRetries, connection);
    }

    const HSteamNetConnection handle = impl->handleFor(connection);
    if (handle == k_HSteamNetConnection_Invalid)
        return;
    impl->forgetConnection(handle);
    impl->sockets->CloseConnection(handle, reason, detail.c_str(), false);
}

bool GameNetworkingTransport::setTimeout(ConnectionId connection, std::uint32_t milliseconds,
    std::string& error)
{
    if (milliseconds > static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max())) {
        error = "connection timeout exceeds GameNetworkingSockets limits";
        return false;
    }

    ISteamNetworkingUtils* utils = SteamNetworkingUtils();
    const std::int32_t value = static_cast<std::int32_t>(milliseconds);
    const HSteamNetConnection handle = impl->handleFor(connection);
    if (handle == k_HSteamNetConnection_Invalid) {
        error = "connection is not ready for timeout configuration";
        return false;
    }
    const bool initial = utils->SetConnectionConfigValueInt32(
        handle, k_ESteamNetworkingConfig_TimeoutInitial, value);
    const bool connected = utils->SetConnectionConfigValueInt32(
        handle, k_ESteamNetworkingConfig_TimeoutConnected, value);
    if (!initial || !connected)
        error = "GameNetworkingSockets rejected the connection timeout";
    return initial && connected;
}

bool GameNetworkingTransport::setSendRateLimit(ConnectionId connection,
    std::uint32_t bytesPerSecond, std::string& error)
{
    // GameNetworkingSockets clamps configured rates to 100 MiB/s.  Use that
    // library maximum for the historical zero-means-unlimited setting.
    constexpr std::uint32_t LibraryMaximum = 100u * 1024u * 1024u;
    const std::uint32_t requested = bytesPerSecond == 0 ? LibraryMaximum : bytesPerSecond;
    if (requested > static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max())) {
        error = "send rate exceeds GameNetworkingSockets limits";
        return false;
    }

    ISteamNetworkingUtils* utils = SteamNetworkingUtils();
    const std::int32_t value = static_cast<std::int32_t>(requested);
    const HSteamNetConnection handle = impl->handleFor(connection);
    if (handle == k_HSteamNetConnection_Invalid) {
        error = "connection is not ready for send-rate configuration";
        return false;
    }
    const bool minimum = utils->SetConnectionConfigValueInt32(
        handle, k_ESteamNetworkingConfig_SendRateMin, value);
    const bool maximum = utils->SetConnectionConfigValueInt32(
        handle, k_ESteamNetworkingConfig_SendRateMax, value);
    if (!minimum || !maximum)
        error = "GameNetworkingSockets rejected the send rate";
    return minimum && maximum;
}

Endpoint GameNetworkingTransport::remoteEndpoint(ConnectionId connection) const
{
    const HSteamNetConnection handle = impl->handleFor(connection);
    if (handle == k_HSteamNetConnection_Invalid) {
        std::lock_guard<std::mutex> lock(impl->stateMutex);
        const auto pending = impl->attempts.find(connection);
        if (pending != impl->attempts.end())
            return pending->second.endpoint;
        const auto resolving = std::find_if(impl->pendingResolutions.begin(),
            impl->pendingResolutions.end(), [connection](const Impl::PendingResolution& item) {
                return item.id == connection;
            });
        return resolving == impl->pendingResolutions.end() ? Endpoint{} : resolving->endpoint;
    }
    SteamNetConnectionInfo_t info{};
    if (!impl->sockets || !impl->sockets->GetConnectionInfo(handle, &info))
        return {};

    char host[SteamNetworkingIPAddr::k_cchMaxString]{};
    info.m_addrRemote.ToString(host, sizeof(host), false);
    return {host, info.m_addrRemote.m_port};
}

bool GameNetworkingTransport::metrics(ConnectionId connection, ConnectionMetrics& metrics) const
{
    if (!impl->sockets)
        return false;

    const HSteamNetConnection handle = impl->handleFor(connection);
    if (handle == k_HSteamNetConnection_Invalid)
        return false;

    SteamNetConnectionRealTimeStatus_t status{};
    if (impl->sockets->GetConnectionRealTimeStatus(handle, &status, 0, nullptr) != k_EResultOK)
        return false;

    SteamNetConnectionInfo_t info{};
    if (!impl->sockets->GetConnectionInfo(handle, &info))
        return false;

    metrics.authenticated =
        (info.m_nFlags & k_nSteamNetworkConnectionInfoFlags_Unauthenticated) == 0;
    const auto [outgoingPayloadBytes, incomingPayloadBytes] =
        impl->trafficFor(handle);
    metrics.outgoingPayloadBytes = outgoingPayloadBytes;
    metrics.incomingPayloadBytes = incomingPayloadBytes;
    metrics.pingMilliseconds = status.m_nPing;
    metrics.localQuality = status.m_flConnectionQualityLocal;
    metrics.remoteQuality = status.m_flConnectionQualityRemote;
    metrics.outgoingBytesPerSecond = status.m_flOutBytesPerSec;
    metrics.incomingBytesPerSecond = status.m_flInBytesPerSec;
    metrics.pendingUnreliableBytes = status.m_cbPendingUnreliable;
    metrics.pendingReliableBytes = status.m_cbPendingReliable;
    metrics.sentUnacknowledgedReliableBytes = status.m_cbSentUnackedReliable;
    return true;
}

bool GameNetworkingTransport::send(ConnectionId connection, std::span<const std::byte> payload,
    Delivery delivery, std::uint16_t lane, std::string& error)
{
    if (lane >= ConnectionLaneCount) {
        error = "message lane exceeds the configured transport lane count";
        return false;
    }
    if (payload.size() > MaximumLogicalMessageBytes) {
        error = "message exceeds the configured 64 MiB logical-message limit";
        return false;
    }

    const HSteamNetConnection handle = impl->handleFor(connection);
    if (handle == k_HSteamNetConnection_Invalid) {
        error = "connection is not ready to send";
        return false;
    }

    int flags = 0;
    switch (delivery) {
    case Delivery::Reliable:
        flags = k_nSteamNetworkingSend_Reliable;
        break;
    case Delivery::UnreliableNoDelay:
        flags = k_nSteamNetworkingSend_UnreliableNoDelay;
        break;
    case Delivery::Unreliable:
        flags = k_nSteamNetworkingSend_Unreliable;
        break;
    }

    const std::uint64_t messageId = impl->nextMessageId.fetch_add(1);
    const std::size_t chunkCount = std::max<std::size_t>(
        1, (payload.size() + FragmentPayloadBytes - 1) / FragmentPayloadBytes);
    std::vector<Impl::OutgoingFrame> frames;
    frames.reserve(chunkCount);
    for (std::size_t chunk = 0; chunk < chunkCount; ++chunk) {
        const std::size_t offset = chunk * FragmentPayloadBytes;
        const std::size_t chunkBytes = std::min(
            FragmentPayloadBytes, payload.size() - std::min(offset, payload.size()));
        Impl::OutgoingFrame& outgoing = frames.emplace_back();
        outgoing.connection = handle;
        outgoing.lane = lane;
        outgoing.flags = flags;
        outgoing.bytes.resize(FragmentHeaderBytes + chunkBytes);
        std::byte* frame = outgoing.bytes.data();
        writeBigEndian(frame + 0, FragmentMagic, 8);
        frame[8] = static_cast<std::byte>(FragmentVersion);
        frame[9] = static_cast<std::byte>(delivery);
        writeBigEndian(frame + 10, FragmentHeaderBytes, 2);
        writeBigEndian(frame + 12, messageId, 8);
        writeBigEndian(frame + 20, payload.size(), 4);
        writeBigEndian(frame + 24, chunk, 2);
        writeBigEndian(frame + 26, chunkCount, 2);
        writeBigEndian(frame + 28, chunkBytes, 4);
        if (chunkBytes != 0)
            std::memcpy(frame + FragmentHeaderBytes, payload.data() + offset, chunkBytes);
    }
    {
        std::lock_guard<std::mutex> lock(impl->outgoingMutex);
        constexpr std::size_t MaximumQueuedOutgoingBytes = 128 * 1024 * 1024;
        const std::size_t framedTotal = payload.size() +
            chunkCount * FragmentHeaderBytes;
        if (framedTotal > MaximumQueuedOutgoingBytes -
                std::min(impl->queuedOutgoingBytes, MaximumQueuedOutgoingBytes)) {
            error = "logical-message queue exceeds the configured 128 MiB limit";
            return false;
        }
        for (Impl::OutgoingFrame& frame : frames) {
            impl->queuedOutgoingBytes += frame.bytes.size();
            impl->outgoingFrames.push_back(std::move(frame));
        }
    }
    impl->recordSent(handle, payload.size());
    // Outgoing progress must not depend on Receive()/poll().  After the join
    // stream goes idle there may be no inbound packet to wake the receive job,
    // while replication updates and marker requests still need to leave
    // immediately.  flushOutgoing is serialized by outgoingMutex.
    impl->flushOutgoing();
    return true;
}

void GameNetworkingTransport::poll(std::vector<ConnectionEvent>& events,
    std::vector<ReceivedMessage>& messages)
{
    if (!isReady())
        return;

    impl->pollResolutions();
    impl->sockets->RunCallbacks();
    impl->pollRetries();
    impl->flushOutgoing();
    events.insert(events.end(),
        std::make_move_iterator(impl->pendingEvents.begin()),
        std::make_move_iterator(impl->pendingEvents.end()));
    impl->pendingEvents.clear();

    {
        const auto now = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lock(impl->reassemblyMutex);
        std::erase_if(impl->reassemblies,
            [now](const auto& entry) {
                return now - entry.second.started > FragmentLifetime;
            });
    }

    SteamNetworkingMessage_t* incoming[32]{};
    for (;;) {
        const int count = impl->sockets->ReceiveMessagesOnPollGroup(
            impl->pollGroup, incoming, static_cast<int>(std::size(incoming)));
        if (count <= 0)
            break;
        for (int index = 0; index < count; ++index) {
            SteamNetworkingMessage_t* message = incoming[index];
            const auto* first = static_cast<const std::byte*>(message->m_pData);
            const std::size_t frameBytes = static_cast<std::size_t>(message->m_cbSize);
            if (frameBytes >= FragmentHeaderBytes &&
                readBigEndian(first + 0, 8) == FragmentMagic &&
                std::to_integer<std::uint8_t>(first[8]) == FragmentVersion &&
                readBigEndian(first + 10, 2) == FragmentHeaderBytes) {
                const std::uint64_t messageId = readBigEndian(first + 12, 8);
                const std::size_t totalBytes = readBigEndian(first + 20, 4);
                const std::size_t chunkIndex = readBigEndian(first + 24, 2);
                const std::size_t chunkCount = readBigEndian(first + 26, 2);
                const std::size_t chunkBytes = readBigEndian(first + 28, 4);
                const std::size_t expectedChunks = std::max<std::size_t>(
                    1, (totalBytes + FragmentPayloadBytes - 1) / FragmentPayloadBytes);
                const std::size_t offset = chunkIndex * FragmentPayloadBytes;
                const std::size_t expectedChunkBytes = chunkIndex < expectedChunks
                    ? std::min(FragmentPayloadBytes, totalBytes - std::min(offset, totalBytes))
                    : 0;
                if (totalBytes <= MaximumLogicalMessageBytes &&
                    chunkCount == expectedChunks && chunkIndex < chunkCount &&
                    chunkBytes == expectedChunkBytes &&
                    frameBytes == FragmentHeaderBytes + chunkBytes) {
                    const Impl::ReassemblyKey key{
                        message->m_conn, message->m_idxLane, messageId};
                    std::lock_guard<std::mutex> lock(impl->reassemblyMutex);
                    Impl::Reassembly& assembly = impl->reassemblies[key];
                    if (assembly.payload.empty() && totalBytes != 0) {
                        assembly.payload.resize(totalBytes);
                        assembly.chunks.resize(chunkCount);
                        assembly.started = std::chrono::steady_clock::now();
                    } else if (totalBytes == 0 && assembly.chunks.empty()) {
                        assembly.chunks.resize(1);
                        assembly.started = std::chrono::steady_clock::now();
                    }
                    if (assembly.payload.size() == totalBytes &&
                        assembly.chunks.size() == chunkCount &&
                        assembly.chunks[chunkIndex] == 0) {
                        if (chunkBytes != 0)
                            std::memcpy(assembly.payload.data() + offset,
                                first + FragmentHeaderBytes, chunkBytes);
                        assembly.chunks[chunkIndex] = 1;
                        ++assembly.receivedChunks;
                        if (assembly.receivedChunks == chunkCount) {
                            ReceivedMessage received;
                            received.connection = impl->idFor(message->m_conn);
                            received.lane = message->m_idxLane;
                            received.payload = std::move(assembly.payload);
                            impl->recordReceived(message->m_conn, totalBytes);
                            messages.push_back(std::move(received));
                            impl->reassemblies.erase(key);
                        }
                    }
                }
            }
            message->Release();
        }
    }
}

} // namespace RBX::Network

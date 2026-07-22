#include "ConcurrentPeer.h"

#include "NetworkSettings.h"
#include "util/standardout.h"
#include "v8datamodel/DataModel.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <random>

LOGGROUP(NetworkStatsReport)

using namespace RBX;
using namespace RBX::Network;

namespace {

constexpr std::uint16_t IdentityLane = 7;
constexpr std::array<std::byte, 8> IdentityMagic = {
    std::byte{'R'}, std::byte{'B'}, std::byte{'X'}, std::byte{'I'},
    std::byte{'D'}, std::byte{'0'}, std::byte{'1'}, std::byte{0}};
constexpr std::size_t IdentityMessageBytes = 16;
constexpr std::uint8_t IdentityHello = 1;
constexpr std::uint8_t IdentityAcknowledgement = 2;

std::uint32_t generateNetworkIdentity()
{
    std::random_device source;
    std::uint32_t value = (static_cast<std::uint32_t>(source()) << 16) ^
        static_cast<std::uint32_t>(source());
    return value == 0 ? 1 : value;
}

std::array<std::byte, IdentityMessageBytes> identityMessage(
    std::uint8_t type, std::uint32_t identity)
{
    std::array<std::byte, IdentityMessageBytes> message{};
    std::copy(IdentityMagic.begin(), IdentityMagic.end(), message.begin());
    message[8] = static_cast<std::byte>(type);
    message[12] = static_cast<std::byte>(identity >> 24);
    message[13] = static_cast<std::byte>(identity >> 16);
    message[14] = static_cast<std::byte>(identity >> 8);
    message[15] = static_cast<std::byte>(identity);
    return message;
}

bool decodeIdentityMessage(const ReceivedMessage& message,
    std::uint8_t& type, std::uint32_t& identity)
{
    if (message.lane != IdentityLane || message.payload.size() != IdentityMessageBytes ||
        !std::equal(IdentityMagic.begin(), IdentityMagic.end(), message.payload.begin()))
        return false;
    type = std::to_integer<std::uint8_t>(message.payload[8]);
    identity = (static_cast<std::uint32_t>(
                    std::to_integer<std::uint8_t>(message.payload[12])) << 24) |
        (static_cast<std::uint32_t>(
             std::to_integer<std::uint8_t>(message.payload[13])) << 16) |
        (static_cast<std::uint32_t>(
             std::to_integer<std::uint8_t>(message.payload[14])) << 8) |
        static_cast<std::uint32_t>(
            std::to_integer<std::uint8_t>(message.payload[15]));
    return identity != 0 &&
        (type == IdentityHello || type == IdentityAcknowledgement);
}

Delivery toDelivery(PacketReliability reliability, PacketPriority priority)
{
    switch (reliability) {
    case RELIABLE:
    case RELIABLE_ORDERED:
    case RELIABLE_SEQUENCED:
        return Delivery::Reliable;
    case UNRELIABLE_SEQUENCED:
        return Delivery::Unreliable;
    case UNRELIABLE:
        return priority == IMMEDIATE_PRIORITY ? Delivery::UnreliableNoDelay : Delivery::Unreliable;
    }
    return Delivery::Reliable;
}

} // namespace

class ConcurrentPeer::PacketJob : public DataModelJob {
public:
    struct SendData {
        boost::shared_ptr<const PacketBuffer> packet;
        PacketPriority priority;
        PacketReliability reliability;
        char lane;
        PeerAddress address;
        bool broadcast;
    };

    rbx::timestamped_safe_queue<SendData> sendQueue;
    boost::weak_ptr<GameNetworkingTransport> transport;
    ConcurrentPeer* owner;

    PacketJob(boost::shared_ptr<GameNetworkingTransport> transport, ConcurrentPeer* owner, DataModel* dataModel)
        : DataModelJob("Network Send", DataModelJob::NetworkPeer, false, shared_from(dataModel), Time::Interval(0))
        , transport(transport)
        , owner(owner)
    {
        cyclicExecutive = true;
        cyclicPriority = CyclicExecutiveJobPriority_Network_ReceiveIncoming;
    }

private:
    Time::Interval sleepTime(const Stats&) override
    {
        // reschedule() can race with the end of a scheduler step: a producer
        // may enqueue while this job is still considered runnable, after
        // which the job observes an empty queue and sleeps indefinitely.  A
        // bounded idle poll closes that lost-wakeup window while keeping the
        // explicit reschedule fast path for normal sends.
        return sendQueue.empty() ? Time::Interval(1.0 / 60.0) : Time::Interval::zero();
    }

    Error error(const Stats& stats) override
    {
        if (!sendQueue.empty()) {
            Error result;
            result.error = 1.0;
            return result;
        }
        return computeStandardErrorCyclicExecutiveSleeping(stats, 60.0);
    }

    TaskScheduler::StepResult stepDataModelJob(const Stats&) override
    {
        boost::shared_ptr<GameNetworkingTransport> safeTransport = transport.lock();
        if (!safeTransport)
            return TaskScheduler::Done;

        SendData data;
        while (sendQueue.pop_if_present(data)) {
            const std::span<const std::byte> payload(
                reinterpret_cast<const std::byte*>(data.packet->GetData()),
                data.packet->GetNumberOfBytesUsed());
            std::string error;
            if (data.broadcast) {
                for (const PeerAddress& address : owner->connectionSnapshot()) {
                    error.clear();
                    if (!safeTransport->send(address.connection, payload,
                            toDelivery(data.reliability, data.priority), data.lane, error)) {
                        StandardOut::singleton()->printf(MESSAGE_WARNING,
                            "Network send failed for connection %llu: %s",
                            static_cast<unsigned long long>(address.connection),
                            error.empty() ? "unspecified transport error" : error.c_str());
                    }
                }
            } else {
                if (!safeTransport->send(data.address.connection, payload,
                        toDelivery(data.reliability, data.priority), data.lane, error)) {
                    StandardOut::singleton()->printf(MESSAGE_WARNING,
                        "Network send failed for connection %llu: %s",
                        static_cast<unsigned long long>(data.address.connection),
                        error.empty() ? "unspecified transport error" : error.c_str());
                }
            }
        }
        return TaskScheduler::Stepped;
    }
};

class ConcurrentPeer::StatsUpdateJob : public DataModelJob {
public:
    struct Entry {
        ConnectionStats stats;
        boost::function<void(const ConnectionStats&)> callback;
    };

    boost::mutex mutex;
    std::unordered_map<PeerAddress, Entry> entries;
    RunningAverage<> bufferHealth{0.1, 1.0};
    boost::weak_ptr<GameNetworkingTransport> transport;

    StatsUpdateJob(boost::shared_ptr<GameNetworkingTransport> transport, DataModel* dataModel)
        : DataModelJob("Network Stats", DataModelJob::NetworkPeer, false, shared_from(dataModel), Time::Interval(0))
        , transport(transport)
    {
        cyclicExecutive = true;
        cyclicPriority = CyclicExecutiveJobPriority_Network_ProcessIncoming;
    }

    void update(PeerAddress address, Entry& entry)
    {
        boost::shared_ptr<GameNetworkingTransport> safeTransport = transport.lock();
        if (!safeTransport)
            return;
        ConnectionMetrics metrics;
        if (!safeTransport->metrics(address.connection, metrics))
            return;

        entry.stats.averagePing = metrics.pingMilliseconds;
        entry.stats.lastPing = metrics.pingMilliseconds;
        if (entry.stats.lowestPing == 0 || metrics.pingMilliseconds < entry.stats.lowestPing)
            entry.stats.lowestPing = metrics.pingMilliseconds;
        entry.stats.maxPacketloss = std::max(entry.stats.maxPacketloss, 1.0f - metrics.localQuality);
        entry.stats.kiloBytesSentPerSecond.sample(metrics.outgoingBytesPerSecond / 1000.0f);
        entry.stats.kiloBytesReceivedPerSecond.sample(metrics.incomingBytesPerSecond / 1000.0f);
        const int pending = metrics.pendingReliableBytes + metrics.pendingUnreliableBytes;
        entry.stats.transportStats.messageInSendBuffer[MEDIUM_PRIORITY] = pending > 0 ? 1 : 0;
        entry.stats.transportStats.bytesInSendBuffer[MEDIUM_PRIORITY] = pending;
        entry.stats.transportStats.valueOverLastSecond[USER_MESSAGE_BYTES_PUSHED] = metrics.outgoingBytesPerSecond;
        entry.stats.transportStats.valueOverLastSecond[USER_MESSAGE_BYTES_RECEIVED_PROCESSED] = metrics.incomingBytesPerSecond;
        entry.stats.transportStats.valueOverLastSecond[ACTUAL_BYTES_SENT] = metrics.outgoingBytesPerSecond;
        entry.stats.transportStats.valueOverLastSecond[ACTUAL_BYTES_RECEIVED] = metrics.incomingBytesPerSecond;
        entry.stats.transportStats.runningTotal[USER_MESSAGE_BYTES_PUSHED] = metrics.outgoingPayloadBytes;
        entry.stats.transportStats.runningTotal[USER_MESSAGE_BYTES_SENT] = metrics.outgoingPayloadBytes;
        entry.stats.transportStats.runningTotal[USER_MESSAGE_BYTES_RECEIVED_PROCESSED] = metrics.incomingPayloadBytes;
        entry.stats.transportStats.runningTotal[ACTUAL_BYTES_SENT] = metrics.outgoingPayloadBytes;
        entry.stats.transportStats.runningTotal[ACTUAL_BYTES_RECEIVED] = metrics.incomingPayloadBytes;
        entry.stats.transportStats.packetlossLastSecond = 1.0f - metrics.localQuality;
        entry.stats.updateBufferHealth(pending);
    }

private:
    Time::Interval sleepTime(const Stats& stats) override { return computeStandardSleepTime(stats, 30); }
    Error error(const Stats& stats) override { return computeStandardErrorCyclicExecutiveSleeping(stats, 30); }

    TaskScheduler::StepResult stepDataModelJob(const Stats&) override
    {
        boost::mutex::scoped_lock lock(mutex);
        int pending = 0;
        for (auto& [address, entry] : entries) {
            update(address, entry);
            for (unsigned int value : entry.stats.transportStats.bytesInSendBuffer)
                pending += value;
            if (entry.callback)
                entry.callback(entry.stats);
        }
        bufferHealth.sample(pending == 0 ? 1.0 : 0.5);
        return TaskScheduler::Stepped;
    }
};

ConcurrentPeer::ConcurrentPeer(DataModel* dataModel)
    : transport(new GameNetworkingTransport())
    , dataModel(dataModel)
    , localNetworkIdentity(generateNetworkIdentity())
{
    if (!transport->isReady())
        throw std::runtime_error(transport->startupError());
    packetJob.reset(new PacketJob(transport, this, dataModel));
    statsUpdateJob.reset(new StatsUpdateJob(transport, dataModel));
    TaskScheduler::singleton().add(packetJob);
    TaskScheduler::singleton().add(statsUpdateJob);
}

ConcurrentPeer::~ConcurrentPeer()
{
    // Both jobs retain a raw owner pointer.  A non-blocking remove only takes
    // them out of the scheduler queue; an already-running step can otherwise
    // return into a destroyed ConcurrentPeer (and into its connection and
    // callback containers).  Join the jobs before releasing any owner state.
    TaskScheduler::singleton().removeBlocking(packetJob);
    TaskScheduler::singleton().removeBlocking(statsUpdateJob);
    packetJob.reset();
    statsUpdateJob.reset();
    while (!receivedPackets.empty()) {
        delete receivedPackets.front();
        receivedPackets.pop_front();
    }
}

void ConcurrentPeer::addStats(PeerAddress address, boost::function<void(const ConnectionStats&)> callback)
{
    boost::mutex::scoped_lock lock(statsUpdateJob->mutex);
    auto [found, inserted] = statsUpdateJob->entries.try_emplace(address);
    found->second.callback = callback;
    statsUpdateJob->update(address, found->second);
    callback(found->second.stats);
}

void ConcurrentPeer::removeStats(PeerAddress address)
{
    boost::mutex::scoped_lock lock(statsUpdateJob->mutex);
    statsUpdateJob->entries.erase(address);
}

void ConcurrentPeer::Send(boost::shared_ptr<const PacketBuffer> packet, PacketPriority priority,
    PacketReliability reliability, char orderingChannel, PeerAddress address, bool broadcast)
{
    packetJob->sendQueue.push({packet, priority, reliability, orderingChannel, address, broadcast});
    TaskScheduler::singleton().reschedule(packetJob);
}

bool ConcurrentPeer::Send(const PacketBuffer* packet, PacketPriority priority,
    PacketReliability reliability, char orderingChannel, PeerAddress address, bool broadcast)
{
    if (!packet)
        return false;
    boost::shared_ptr<PacketBuffer> copy(new PacketBuffer(packet->GetNumberOfBytesUsed()));
    copy->Write(reinterpret_cast<const char*>(packet->GetData()), packet->GetNumberOfBytesUsed());
    Send(copy, priority, reliability, orderingChannel, address, broadcast);
    return true;
}

bool ConcurrentPeer::Send(const char* data, int length, PacketPriority priority,
    PacketReliability reliability, char orderingChannel, PeerAddress address, bool broadcast)
{
    if (!data || length < 0)
        return false;
    boost::shared_ptr<PacketBuffer> packet(new PacketBuffer(static_cast<std::size_t>(length)));
    packet->Write(data, static_cast<std::size_t>(length));
    Send(packet, priority, reliability, orderingChannel, address, broadcast);
    return true;
}

StartupResult ConcurrentPeer::Startup(unsigned int maximumConnections, const SocketDescriptor* descriptors, unsigned int descriptorCount)
{
    lastStartupError.clear();
    if (maximumConnections > 1) {
        const SocketDescriptor descriptor = descriptorCount > 0 ? descriptors[0] : SocketDescriptor{};
        std::string error;
        if (!transport->listen({descriptor.hostAddress[0] ? descriptor.hostAddress : "*", descriptor.port}, error))
        {
            lastStartupError = error.empty() ? "unspecified transport error" : error;
            StandardOut::singleton()->printf(MESSAGE_ERROR,
                "Network transport listen failed: %s",
                lastStartupError.c_str());
            return StartupFailed;
        }
        boundAddress = {InvalidConnectionId, transport->listeningEndpoint()};
    } else {
        const std::uint16_t port = descriptorCount > 0 ? descriptors[0].port : std::uint16_t{0};
        boundAddress = {InvalidConnectionId, {"0.0.0.0", port}};
    }
    active = true;
    return StartupSucceeded;
}

ConnectionAttemptResult ConcurrentPeer::Connect(const char* host, unsigned short port, const char*, int)
{
    std::string error;
    const ConnectionId connection = transport->connect({host ? host : "", port}, error);
    if (connection == InvalidConnectionId)
        return ConnectionAttemptFailedToStart;
    {
        std::lock_guard<std::mutex> lock(connectionsMutex);
        connections.push_back({connection, {host ? host : "", port}});
    }
    active = true;
    return ConnectionAttemptStarted;
}

void ConcurrentPeer::Shutdown(unsigned int)
{
    std::vector<PeerAddress> closing;
    {
        std::lock_guard<std::mutex> lock(connectionsMutex);
        closing.swap(connections);
    }
    for (const PeerAddress& address : closing)
        transport->close(address.connection, 0, "network shutdown");
    active = false;
}

void ConcurrentPeer::CloseConnection(PeerAddress address, bool)
{
    transport->close(address.connection, 0, "connection closed");
    std::lock_guard<std::mutex> lock(connectionsMutex);
    std::erase_if(connections, [address](const PeerAddress& item) { return item.connection == address.connection; });
}

void ConcurrentPeer::AttachPlugin(PacketHandler* handler)
{
    if (handler && std::find(handlers.begin(), handlers.end(), handler) == handlers.end())
        handlers.push_back(handler);
}

void ConcurrentPeer::DetachPlugin(PacketHandler* handler)
{
    std::erase(handlers, handler);
}

PeerAddress ConcurrentPeer::addressFor(ConnectionId connection) const
{
    std::lock_guard<std::mutex> lock(connectionsMutex);
    const auto found = std::find_if(connections.begin(), connections.end(),
        [connection](const PeerAddress& address) { return address.connection == connection; });
    if (found != connections.end())
        return *found;
    return {connection, transport->remoteEndpoint(connection)};
}

PeerAddress ConcurrentPeer::setNetworkIdentity(
    ConnectionId connection, std::uint32_t identity)
{
    std::lock_guard<std::mutex> lock(connectionsMutex);
    const auto found = std::find_if(connections.begin(), connections.end(),
        [connection](const PeerAddress& address) {
            return address.connection == connection;
        });
    if (found == connections.end())
        return {connection, transport->remoteEndpoint(connection)};
    found->networkIdentity = identity;
    return *found;
}

std::vector<PeerAddress> ConcurrentPeer::connectionSnapshot() const
{
    std::lock_guard<std::mutex> lock(connectionsMutex);
    return connections;
}

void ConcurrentPeer::queueEventPacket(unsigned char id, const PeerAddress& address)
{
    std::vector<std::byte> payload{static_cast<std::byte>(id)};
    receivedPackets.push_back(new Packet(address, std::move(payload)));
}

void ConcurrentPeer::pollTransport()
{
    std::vector<ConnectionEvent> events;
    std::vector<ReceivedMessage> messages;
    transport->poll(events, messages);

    for (const ConnectionEvent& event : events) {
        PeerAddress address = addressFor(event.connection);
        switch (event.type) {
        case ConnectionEventType::ConnectionRequested: {
            std::string error;
            if (transport->accept(event.connection, error)) {
                {
                    std::lock_guard<std::mutex> lock(connectionsMutex);
                    connections.push_back(address);
                    incomingConnections.insert(event.connection);
                }
                if (timeoutMilliseconds != 0)
                    transport->setTimeout(event.connection, timeoutMilliseconds, error);
                transport->setSendRateLimit(event.connection, outgoingBitsPerSecondLimit / 8, error);
            }
            break;
        }
        case ConnectionEventType::Connected: {
            bool incoming = false;
            {
                std::lock_guard<std::mutex> lock(connectionsMutex);
                if (std::none_of(connections.begin(), connections.end(), [event](const PeerAddress& item) { return item.connection == event.connection; }))
                    connections.push_back(address);
                incoming = incomingConnections.find(event.connection) !=
                    incomingConnections.end();
                connectedConnections.insert(event.connection);
            }
            if (timeoutMilliseconds != 0) {
                std::string error;
                transport->setTimeout(event.connection, timeoutMilliseconds, error);
            }
            {
                std::string error;
                transport->setSendRateLimit(event.connection, outgoingBitsPerSecondLimit / 8, error);
            }
            // Replication uses a stable client identity for distributed
            // physics ownership. GNS connection handles and UDP ports are
            // intentionally local details, so exchange an owned identity
            // before exposing the RakNet-compatible connected event.
            if (!incoming) {
                const auto hello = identityMessage(
                    IdentityHello, localNetworkIdentity);
                std::string error;
                if (!transport->send(event.connection, hello,
                        Delivery::Reliable, IdentityLane, error))
                    queueEventPacket(ID_CONNECTION_LOST, address);
            }
            break;
        }
        case ConnectionEventType::ClosedByPeer:
            {
                std::lock_guard<std::mutex> lock(connectionsMutex);
                incomingConnections.erase(event.connection);
                connectedConnections.erase(event.connection);
                announcedConnections.erase(event.connection);
            }
            queueEventPacket(ID_DISCONNECTION_NOTIFICATION, address);
            break;
        case ConnectionEventType::ProblemDetected:
            {
                std::lock_guard<std::mutex> lock(connectionsMutex);
                incomingConnections.erase(event.connection);
                connectedConnections.erase(event.connection);
                announcedConnections.erase(event.connection);
            }
            queueEventPacket(ID_CONNECTION_LOST, address);
            break;
        }
    }
    for (ReceivedMessage& message : messages) {
        std::uint8_t identityType = 0;
        std::uint32_t identity = 0;
        if (decodeIdentityMessage(message, identityType, identity)) {
            bool incoming = false;
            bool connected = false;
            {
                std::lock_guard<std::mutex> lock(connectionsMutex);
                incoming = incomingConnections.contains(message.connection);
                connected = connectedConnections.contains(message.connection);
            }
            if (connected && incoming && identityType == IdentityHello) {
                PeerAddress address = setNetworkIdentity(message.connection, identity);
                const auto acknowledgement = identityMessage(
                    IdentityAcknowledgement, identity);
                std::string error;
                if (transport->send(message.connection, acknowledgement,
                        Delivery::Reliable, IdentityLane, error)) {
                    bool announce = false;
                    {
                        std::lock_guard<std::mutex> lock(connectionsMutex);
                        announce = announcedConnections.insert(
                            message.connection).second;
                    }
                    if (announce)
                        queueEventPacket(ID_NEW_INCOMING_CONNECTION, address);
                }
                else
                    queueEventPacket(ID_CONNECTION_LOST, address);
            } else if (connected && !incoming &&
                       identityType == IdentityAcknowledgement &&
                       identity == localNetworkIdentity) {
                PeerAddress address = setNetworkIdentity(message.connection, identity);
                bool announce = false;
                {
                    std::lock_guard<std::mutex> lock(connectionsMutex);
                    announce = announcedConnections.insert(
                        message.connection).second;
                }
                if (announce)
                    queueEventPacket(ID_CONNECTION_REQUEST_ACCEPTED, address);
            }
            continue;
        }
        receivedPackets.push_back(new Packet(addressFor(message.connection), std::move(message.payload)));
    }
}

Packet* ConcurrentPeer::Receive()
{
    for (;;) {
        if (receivedPackets.empty())
            pollTransport();
        if (receivedPackets.empty())
            return nullptr;

        Packet* packet = receivedPackets.front();
        receivedPackets.pop_front();
        bool consumed = false;
        for (PacketHandler* handler : handlers) {
            const PluginReceiveResult result = handler->OnReceive(packet);
            if (result == RR_STOP_PROCESSING) {
                consumed = true;
                break;
            }
            if (result == RR_STOP_PROCESSING_AND_DEALLOCATE) {
                delete packet;
                consumed = true;
                break;
            }
        }
        if (!consumed)
            return packet;
    }
}

void ConcurrentPeer::SetTimeoutTime(unsigned int milliseconds, PeerAddress address)
{
    if (address == UnassignedPeerAddress)
        timeoutMilliseconds = milliseconds;

    std::string error;
    if (address != UnassignedPeerAddress) {
        transport->setTimeout(address.connection, milliseconds, error);
        return;
    }
    for (const PeerAddress& connection : connectionSnapshot())
        transport->setTimeout(connection.connection, milliseconds, error);
}

bool ConcurrentPeer::SetCertificate(std::span<const std::byte> certificate, std::string& error)
{
    return transport->setCertificate(certificate, error);
}

bool ConcurrentPeer::GetCertificateRequest(std::vector<std::byte>& request,
    std::string& error)
{
    return transport->certificateRequest(request, error);
}

void ConcurrentPeer::SetPerConnectionOutgoingBandwidthLimit(int bitsPerSecond)
{
    outgoingBitsPerSecondLimit = static_cast<std::uint32_t>(std::max(bitsPerSecond, 0));
    std::string error;
    for (const PeerAddress& connection : connectionSnapshot())
        transport->setSendRateLimit(connection.connection, outgoingBitsPerSecondLimit / 8, error);
}

PeerGuid ConcurrentPeer::GetGuidFromSystemAddress(PeerAddress) const
{
    return {static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(this))};
}

int ConcurrentPeer::GetAveragePing(PeerAddress address) const
{
    ConnectionMetrics metrics;
    return transport->metrics(address.connection, metrics) ? metrics.pingMilliseconds : 0;
}

const PeerStatistics* ConcurrentPeer::GetStatistics(PeerAddress address) const
{
    boost::mutex::scoped_lock lock(statsUpdateJob->mutex);
    const auto found = statsUpdateJob->entries.find(address);
    return found == statsUpdateJob->entries.end() ? nullptr : &found->second.stats.transportStats;
}

double ConcurrentPeer::GetBufferHealth()
{
    boost::mutex::scoped_lock lock(statsUpdateJob->mutex);
    return statsUpdateJob->bufferHealth.value();
}

double ConcurrentPeer::GetBandwidthExceeded(PeerAddress address)
{
    boost::mutex::scoped_lock lock(statsUpdateJob->mutex);
    return statsUpdateJob->entries[address].stats.averageBandwidthExceeded.value();
}

double ConcurrentPeer::GetCongestionControlExceeded(PeerAddress address)
{
    boost::mutex::scoped_lock lock(statsUpdateJob->mutex);
    return statsUpdateJob->entries[address].stats.averageCongestionControlExceeded.value();
}

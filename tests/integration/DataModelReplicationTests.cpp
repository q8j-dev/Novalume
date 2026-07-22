#include "network/api.h"
#include "Client.h"
#include "ClientReplicator.h"
#include "Marker.h"
#include "network/Players.h"
#include "Server.h"
#include "Script/LuaSettings.h"
#include "v8datamodel/CommonVerbs.h"
#include "util/Http.h"
#include "util/Profiling.h"
#include "v8datamodel/BasicPartInstance.h"
#include "v8datamodel/DataModel.h"
#include "v8datamodel/DebugSettings.h"
#include "v8datamodel/factoryregistration.h"
#include "v8datamodel/GameBasicSettings.h"
#include "v8datamodel/GameSettings.h"
#include "v8datamodel/PhysicsSettings.h"
#include "util/RunStateOwner.h"
#include "v8datamodel/Workspace.h"
#include "security/SecurityContext.h"

#include <atomic>
#include <chrono>
#include <exception>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <typeinfo>
#include <utility>

namespace {

bool waitUntil(const std::function<bool()>& predicate,
               std::chrono::steady_clock::duration timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (predicate())
            return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return predicate();
}

} // namespace

int main() {
    boost::shared_ptr<RBX::DataModel> serverDataModel;
    boost::shared_ptr<RBX::DataModel> clientDataModel;
    RBX::Network::Server* server = nullptr;
    RBX::Network::Client* client = nullptr;
    boost::shared_ptr<RBX::BasicPartInstance> serverPart;

    try {
        RBX::Profiling::init(false);
        RBX::FactoryRegistrator factoryObjects;
        RBX::Http::init(RBX::Http::WinHttp,
                        RBX::Http::CookieSharingSingleProcessMultipleThreads);
        RBX::GameSettings::singleton();
        RBX::LuaSettings::singleton();
        RBX::DebugSettings::singleton();
        RBX::PhysicsSettings::singleton();
        RBX::GameBasicSettings::singleton();
        RBX::Network::initWithoutSecurity();
        FLog::SetValue("DataModelJobs", "0", FASTVARTYPE_STATIC);
        FLog::SetValue("GfxClusters", "0", FASTVARTYPE_STATIC);

        RBX::Security::Impersonator permission(RBX::Security::LocalGUI_);
        serverDataModel = RBX::DataModel::createDataModel(
            true, new RBX::NullVerb(nullptr, ""), false);
        clientDataModel = RBX::DataModel::createDataModel(
            true, new RBX::NullVerb(nullptr, ""), false);

        {
            RBX::DataModel::LegacyLock lock(
                serverDataModel.get(), RBX::DataModelJob::Write);
            serverPart =
                RBX::Creatable<RBX::Instance>::create<RBX::BasicPartInstance>();
            serverPart->setName("GameNetworkingSocketsReplicationProbe");
            serverPart->setAnchored(true);
            serverPart->setParent(serverDataModel->getWorkspace());

            RBX::ServiceProvider::create<RBX::Network::Players>(
                serverDataModel.get())->setCharacterAutoSpawnProperty(false);

            server = RBX::ServiceProvider::create<RBX::Network::Server>(
                serverDataModel.get());
            server->start(0, 0);
            RBX::ServiceProvider::create<RBX::RunService>(
                serverDataModel.get())->run();
        }

        std::atomic<bool> accepted{false};
        std::atomic<bool> gameLoaded{false};
        std::atomic<bool> failed{false};
        std::string failureReason;
        rbx::signals::scoped_connection acceptedConnection;
        rbx::signals::scoped_connection failedConnection;
        rbx::signals::scoped_connection rejectedConnection;
        rbx::signals::scoped_connection gameLoadedConnection;
        boost::shared_ptr<RBX::Network::ClientReplicator> clientReplicator;
        {
            RBX::DataModel::LegacyLock lock(
                clientDataModel.get(), RBX::DataModelJob::Write);
            client = RBX::ServiceProvider::create<RBX::Network::Client>(
                clientDataModel.get());
            acceptedConnection = client->connectionAcceptedSignal.connect(
                [&accepted, &gameLoaded, &gameLoadedConnection,
                    &clientReplicator](
                    std::string, boost::shared_ptr<RBX::Instance> instance) {
                    clientReplicator =
                        RBX::Instance::fastSharedDynamicCast<
                            RBX::Network::ClientReplicator>(instance);
                    if (!clientReplicator)
                        throw std::runtime_error(
                            "accepted connection did not create a ClientReplicator");
                    gameLoadedConnection = clientReplicator->gameLoadedSignal.connect(
                        [&gameLoaded] { gameLoaded = true; });
                    accepted = true;
                });
            failedConnection = client->connectionFailedSignal.connect(
                [&failed, &failureReason](std::string, int, std::string reason) {
                    failureReason = std::move(reason);
                    failed = true;
                });
            rejectedConnection = client->connectionRejectedSignal.connect(
                [&failed, &failureReason](std::string) {
                    failureReason = "connection rejected";
                    failed = true;
                });
            RBX::ServiceProvider::create<RBX::RunService>(
                clientDataModel.get())->run();
            client->playerConnect(1, "127.0.0.1", server->getPort(), 0, 0);
        }

        if (!waitUntil([&] { return accepted.load() || failed.load(); },
                       std::chrono::seconds(20)))
            throw std::runtime_error("timed out waiting for DataModel connection acceptance");
        if (failed)
            throw std::runtime_error("DataModel connection failed: " + failureReason);

        if (!waitUntil([&] { return gameLoaded.load() || failed.load(); },
                       std::chrono::seconds(20)))
            throw std::runtime_error("timed out waiting for complete join data");
        if (failed)
            throw std::runtime_error("DataModel join failed: " + failureReason);

        const auto waitForRoundTripMarker = [&] {
            std::atomic<bool> returned{false};
            boost::shared_ptr<RBX::Instance> markerInstance;
            rbx::signals::scoped_connection markerConnection;
            {
                RBX::DataModel::LegacyLock lock(
                    clientDataModel.get(), RBX::DataModelJob::Write);
                markerInstance = clientReplicator->sendMarker();
                RBX::Network::Marker* marker =
                    RBX::Instance::fastDynamicCast<RBX::Network::Marker>(
                        markerInstance.get());
                if (!marker)
                    throw std::runtime_error(
                        "ClientReplicator did not create a network marker");
                markerConnection = marker->receivedSignal.connect(
                    [&returned] { returned = true; });
                if (marker->hasReturned())
                    returned = true;
            }
            if (!waitUntil([&] { return returned.load() || failed.load(); },
                           std::chrono::seconds(20)))
                throw std::runtime_error(
                    "timed out waiting for a replication queue round trip");
            if (failed)
                throw std::runtime_error(
                    "DataModel marker round trip failed: " + failureReason);
        };

        waitForRoundTripMarker();

        const bool replicated = waitUntil(
            [&] {
                RBX::DataModel::LegacyLock lock(
                    clientDataModel.get(), RBX::DataModelJob::Read);
                return clientDataModel->getWorkspace()->findFirstChildByName(
                           "GameNetworkingSocketsReplicationProbe") != nullptr;
            },
            std::chrono::seconds(20));
        if (!replicated)
            throw std::runtime_error("connected client did not receive the server instance");

        {
            RBX::DataModel::LegacyLock lock(
                serverDataModel.get(), RBX::DataModelJob::Write);
            serverPart->setName("GameNetworkingSocketsReplicationProbeUpdated");
            serverPart->setCoordinateFrame(
                RBX::CoordinateFrame(RBX::Vector3(12.0f, 5.0f, -7.0f)));
            serverPart->setColor(RBX::BrickColor::brickRed());
        }
        waitForRoundTripMarker();

        const bool propertyUpdateReplicated = waitUntil(
            [&] {
                RBX::DataModel::LegacyLock lock(
                    clientDataModel.get(), RBX::DataModelJob::Read);
                RBX::PartInstance* part = RBX::Instance::fastDynamicCast<RBX::PartInstance>(
                    clientDataModel->getWorkspace()->findFirstChildByName(
                        "GameNetworkingSocketsReplicationProbeUpdated"));
                return part &&
                    (part->getCoordinateFrame().translation -
                        RBX::Vector3(12.0f, 5.0f, -7.0f)).magnitude() < 0.001f &&
                    part->getColor() == RBX::BrickColor::brickRed();
            },
            std::chrono::seconds(20));
        if (!propertyUpdateReplicated)
        {
            RBX::DataModel::LegacyLock lock(
                clientDataModel.get(), RBX::DataModelJob::Read);
            RBX::Instance* observed = clientDataModel->getWorkspace()->findFirstChildByName(
                "GameNetworkingSocketsReplicationProbeUpdated");
            if (!observed)
                observed = clientDataModel->getWorkspace()->findFirstChildByName(
                    "GameNetworkingSocketsReplicationProbe");
            if (RBX::PartInstance* part =
                    RBX::Instance::fastDynamicCast<RBX::PartInstance>(observed))
                std::cerr << "observed live part name=" << part->getName()
                          << " position=" << part->getCoordinateFrame().translation
                          << " brickColor=" << part->getColor().number << '\n';
            throw std::runtime_error(
                "connected client did not receive live server property updates");
        }

        {
            RBX::DataModel::LegacyLock lock(
                serverDataModel.get(), RBX::DataModelJob::Write);
            serverPart->setParent(nullptr);
        }
        waitForRoundTripMarker();
        const bool removalReplicated = waitUntil(
            [&] {
                RBX::DataModel::LegacyLock lock(
                    clientDataModel.get(), RBX::DataModelJob::Read);
                return clientDataModel->getWorkspace()->findFirstChildByName(
                           "GameNetworkingSocketsReplicationProbeUpdated") == nullptr;
            },
            std::chrono::seconds(20));
        if (!removalReplicated)
            throw std::runtime_error(
                "connected client did not receive the server instance removal");

        std::cout << "DataModel create/update/remove replication passed on port "
                  << server->getPort() << '\n';

        {
            RBX::DataModel::LegacyLock lock(
                clientDataModel.get(), RBX::DataModelJob::Write);
            client->disconnect(100);
        }
        {
            RBX::DataModel::LegacyLock lock(
                serverDataModel.get(), RBX::DataModelJob::Write);
            server->stop(100);
        }
        RBX::DataModel::closeDataModel(clientDataModel);
        RBX::DataModel::closeDataModel(serverDataModel);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "DataModelReplicationTests [" << typeid(error).name()
                  << "]: " << error.what() << '\n';
        if (clientDataModel)
            RBX::DataModel::closeDataModel(clientDataModel);
        if (serverDataModel)
            RBX::DataModel::closeDataModel(serverDataModel);
        return 1;
    }
}

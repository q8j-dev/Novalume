add_library(rbx-networking-runtime STATIC EXCLUDE_FROM_ALL
    ../API.cpp
    ../ChatFilter.cpp
    ../BoostAppend.cpp
    ../Client.cpp
    ../ClientReplicator.cpp
    ../ClusterUpdateBuffer.cpp
    ../Compressor.cpp
    ../ConcurrentPeer.cpp
    ../Dictionary.cpp
    ../DirectPhysicsReceiver.cpp
    ../ErrorCompPhysicsSender.cpp
    ../ErrorCompPhysicsSender2.cpp
    ../GameConfigurer.cpp
    ../GamePerfMonitor.cpp
    ../GuidRegistryService.cpp
    ../InterpolatingPhysicsReceiver.cpp
    ../Item.cpp
    ../Marker.cpp
    ../MechanismItem.cpp
    ../MovementHistoryJob.cpp
    ../NetworkClusterPacketCache.cpp
    ../NetworkPacketCache.cpp
    ../NetworkSettings.cpp
    ../NetworkOwnerJob.cpp
    ../NetworkProfiler.cpp
    ../NetworkFilter.cpp
    ../Peer.cpp
    ../PersistentDataStore.cpp
    ../Player.cpp
    ../Players.cpp
    ../PhysicsReceiver.cpp
    ../PhysicsSender.cpp
    ../Replicator.ChangePropertyItem.cpp
    ../Replicator.DeleteInstanceItem.cpp
    ../Replicator.EventInvocationItem.cpp
    ../Replicator.GCJob.cpp
    ../Replicator.ItemSender.cpp
    ../Replicator.JoinDataItem.cpp
    ../Replicator.MarkerItem.cpp
    ../Replicator.NewInstanceItem.cpp
    ../Replicator.PingBackItem.cpp
    ../Replicator.PingItem.cpp
    ../Replicator.ReferencePropertyChangedItem.cpp
    ../Replicator.StatsItem.cpp
    ../Replicator.StreamJob.cpp
    ../Replicator.TagItem.cpp
    ../Replicator.cpp
    ../ReplicatorStats.cpp
    ../RoundRobinPhysicsSender.cpp
    ../Server.cpp
    ../ServerReplicator.cpp
    ../Streaming.cpp
    ../TopNErrorsPhysicsSender.cpp
    ../Util.cpp
    ../WebChatFilter.cpp)
add_library(Roblox::NetworkingRuntime ALIAS rbx-networking-runtime)

if(RBX_ENABLE_LEGACY_X86_CLIENT_SECURITY)
    target_sources(rbx-networking-runtime PRIVATE
        ../NetPmc.cpp
        ../Replicator.HashItem.cpp
        ../Replicator.RockyItem.cpp
        "${CMAKE_SOURCE_DIR}/engine/security/src/platform/windows/CheatEngine.cpp"
        "${CMAKE_SOURCE_DIR}/engine/security/src/platform/windows/ProgramMemoryChecker.cpp")
    target_link_libraries(rbx-networking-runtime PRIVATE
        Roblox::ProtectionMarkers psapi gdi32 user32)
endif()

target_include_directories(rbx-networking-runtime
    PUBLIC "${CMAKE_CURRENT_LIST_DIR}/../include"
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/../include/network"
        "${CMAKE_CURRENT_LIST_DIR}/.."
        "${CMAKE_SOURCE_DIR}/App/include"
        "${CMAKE_SOURCE_DIR}/App"
        "${CMAKE_SOURCE_DIR}/engine/core/include"
        "${CMAKE_SOURCE_DIR}/engine/core/math/include"
        "${CMAKE_SOURCE_DIR}/engine/security/include"
        "${CMAKE_SOURCE_DIR}/engine/reflection/include"
        "${CMAKE_SOURCE_DIR}/engine/serialization/include"
        "${CMAKE_SOURCE_DIR}/engine/datamodel/tree/include"
        "${CMAKE_SOURCE_DIR}/engine/datamodel/runtime/include"
        "${CMAKE_SOURCE_DIR}/engine/simulation/include"
        "${CMAKE_SOURCE_DIR}/engine/animation/include"
        "${CMAKE_SOURCE_DIR}/engine/avatar/humanoid/include"
        "${CMAKE_SOURCE_DIR}/engine/interaction/tools/include"
        "${CMAKE_SOURCE_DIR}/engine/ui/runtime/include"
        "${CMAKE_SOURCE_DIR}/engine/scripting/include"
        "${CMAKE_SOURCE_DIR}/engine/scripting/lua/src"
        "${CMAKE_SOURCE_DIR}/engine/rendering/foundation/include"
        "${CMAKE_SOURCE_DIR}/engine/rendering/adorn/include"
        "${CMAKE_SOURCE_DIR}/engine/terrain/include"
        "${CMAKE_SOURCE_DIR}/engine/audio/include"
        "${CMAKE_SOURCE_DIR}/apps/player/shared/client"
        "${CMAKE_SOURCE_DIR}/third_party/bullet_historical"
        "${CMAKE_SOURCE_DIR}/Library/boost/include"
        "${CMAKE_SOURCE_DIR}/Library/SDL2/include")
target_link_libraries(rbx-networking-runtime PUBLIC
    Roblox::Core Roblox::ValueTypes Roblox::DataModelRuntime Roblox::RuntimeSupport
    Roblox::AvatarHumanoid Roblox::Simulation Roblox::AudioContracts
    Roblox::NetworkTransport)
target_compile_features(rbx-networking-runtime PUBLIC cxx_std_20)
set_source_files_properties(../NetworkProfiler.cpp PROPERTIES
    COMPILE_DEFINITIONS NETWORK_PROFILER=1)
target_compile_definitions(rbx-networking-runtime PRIVATE
    RBX_MODERN_NETWORKING_RUNTIME=1
    BOOST_NO_CXX98_FUNCTION_BASE=1
    _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR=1
    _LIBCPP_ENABLE_CXX17_REMOVED_UNARY_BINARY_FUNCTION=1)
target_compile_options(rbx-networking-runtime PRIVATE
    $<$<COMPILE_LANG_AND_ID:CXX,AppleClang>:-Wno-enum-constexpr-conversion>)

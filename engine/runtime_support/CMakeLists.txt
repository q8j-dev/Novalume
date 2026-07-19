add_library(rbx-runtime-support STATIC EXCLUDE_FROM_ALL
    "${CMAKE_SOURCE_DIR}/App/util/Action.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/Analytics.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/AsyncHttpQueue.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/CameraSubject.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/ContentFilter.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/ContentProviderJob.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/HeartbeatInstance.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/Handle.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/HitTest.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/HttpAsync.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/Shared/Http.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/Shared/HttpCacheEntry.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/Shared/HttpPlatformImpl.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/IndexedMesh.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/IndexedTree.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/KeywordFilter.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/LegacyContentTable.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/MD5Hasher.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/LuaWebService.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/MovementHistory.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/Name.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/PathInterpolatedCFrame.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/UintSet.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/Profiling.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/RbxStringTable.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/RobloxGoogleAnalytics.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/RunStateOwner.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/ScriptInformationProvider.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/SpanningEdge.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/SpanningNode.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/SpanningTree.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/SpatialRegion.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/SystemAddress.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/Statistics.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/SteppedInstance.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/StreamRegion.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/ThreadPool.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/URL.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/UserInputBase.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/Utilities.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/standardout.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/md5.c"
    "${CMAKE_SOURCE_DIR}/App/util/gpc.c"
    "${CMAKE_SOURCE_DIR}/App/util/xxhash.c")
add_library(Roblox::RuntimeSupport ALIAS rbx-runtime-support)
set_source_files_properties("${CMAKE_SOURCE_DIR}/App/util/gpc.c" PROPERTIES LANGUAGE CXX)

target_include_directories(rbx-runtime-support
    PUBLIC "${CMAKE_SOURCE_DIR}/App/include"
    PRIVATE
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
        "${CMAKE_SOURCE_DIR}/engine/networking/include"
        "${CMAKE_SOURCE_DIR}/engine/rendering/foundation/include"
        "${CMAKE_SOURCE_DIR}/engine/rendering/adorn/include"
        "${CMAKE_SOURCE_DIR}/engine/terrain/include"
        "${CMAKE_SOURCE_DIR}/apps/player/shared/client"
        "${CMAKE_SOURCE_DIR}/third_party/bullet_historical"
        "${CMAKE_SOURCE_DIR}/Library/boost/include"
        "${CMAKE_SOURCE_DIR}/Library/SDL2/include")
target_link_libraries(rbx-runtime-support PUBLIC
    Roblox::Core Roblox::Math Roblox::Security Roblox::Reflection Roblox::ValueTypes
    Roblox::DataModelRuntime Roblox::Simulation Roblox::BoostRuntime CURL::libcurl)

if(APPLE)
    target_sources(rbx-runtime-support PRIVATE
        "${CMAKE_SOURCE_DIR}/App/util/Unix/ProgramMemoryChecker.cpp"
        "${CMAKE_SOURCE_DIR}/App/util/Unix/MachOBaseAddr.mm")
endif()
target_compile_features(rbx-runtime-support PUBLIC cxx_std_20)
target_compile_definitions(rbx-runtime-support PRIVATE
    BOOST_NO_CXX98_FUNCTION_BASE=1
    _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR=1)
target_compile_options(rbx-runtime-support PRIVATE
    $<$<COMPILE_LANG_AND_ID:CXX,AppleClang>:-Wno-enum-constexpr-conversion>)

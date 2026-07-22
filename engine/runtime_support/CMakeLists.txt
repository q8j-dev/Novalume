set(RBX_HTTP_PLATFORM_SOURCE src/Shared/HttpPlatformImpl.cpp)
if(EMSCRIPTEN)
    set(RBX_HTTP_PLATFORM_SOURCE src/Shared/HttpPlatformImplWeb.cpp)
endif()

add_library(rbx-runtime-support STATIC EXCLUDE_FROM_ALL
    src/Action.cpp
    src/Analytics.cpp
    src/AsyncHttpQueue.cpp
    src/CameraSubject.cpp
    src/ContentFilter.cpp
    src/ContentProviderJob.cpp
    src/HeartbeatInstance.cpp
    src/Handle.cpp
    src/HitTest.cpp
    src/HttpAsync.cpp
    src/Shared/Http.cpp
    src/Shared/HttpCacheEntry.cpp
    ${RBX_HTTP_PLATFORM_SOURCE}
    src/IndexedMesh.cpp
    src/IndexedTree.cpp
    src/KeywordFilter.cpp
    src/LegacyContentTable.cpp
    src/MD5Hasher.cpp
    src/LuaWebService.cpp
    src/MovementHistory.cpp
    src/Name.cpp
    src/PathInterpolatedCFrame.cpp
    src/UintSet.cpp
    src/Profiling.cpp
    src/RbxStringTable.cpp
    src/RobloxGoogleAnalytics.cpp
    src/RunStateOwner.cpp
    src/ScriptInformationProvider.cpp
    src/SpanningEdge.cpp
    src/SpanningNode.cpp
    src/SpanningTree.cpp
    src/SpatialRegion.cpp
    src/SystemAddress.cpp
    src/Statistics.cpp
    src/SteppedInstance.cpp
    src/StreamRegion.cpp
    src/ThreadPool.cpp
    src/URL.cpp
    src/UserInputBase.cpp
    src/Utilities.cpp
    src/standardout.cpp
    src/md5.c
    src/gpc.c
    src/xxhash.c)
add_library(Roblox::RuntimeSupport ALIAS rbx-runtime-support)
set_source_files_properties(src/gpc.c PROPERTIES LANGUAGE CXX)

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
    Roblox::DataModelRuntime Roblox::Simulation Roblox::BoostRuntime)
if(EMSCRIPTEN)
    target_compile_options(rbx-runtime-support PRIVATE -pthread)
    target_link_options(rbx-runtime-support PUBLIC "SHELL:-sFETCH=1" -pthread)
else()
    target_link_libraries(rbx-runtime-support PUBLIC CURL::libcurl)
endif()

target_compile_features(rbx-runtime-support PUBLIC cxx_std_20)
target_compile_definitions(rbx-runtime-support PRIVATE
    BOOST_NO_CXX98_FUNCTION_BASE=1
    _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR=1)
target_compile_options(rbx-runtime-support PRIVATE
    $<$<COMPILE_LANG_AND_ID:CXX,AppleClang>:-Wno-enum-constexpr-conversion>)

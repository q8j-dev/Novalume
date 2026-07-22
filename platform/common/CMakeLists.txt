add_library(rbx-platform-contract INTERFACE)
add_library(Roblox::PlatformContract ALIAS rbx-platform-contract)
target_include_directories(rbx-platform-contract INTERFACE include)
target_compile_features(rbx-platform-contract INTERFACE cxx_std_20)

add_library(rbx-platform-recent-documents STATIC src/RecentDocuments.cpp)
add_library(Roblox::PlatformRecentDocuments ALIAS rbx-platform-recent-documents)
target_link_libraries(rbx-platform-recent-documents PUBLIC Roblox::PlatformContract)
target_compile_features(rbx-platform-recent-documents PUBLIC cxx_std_20)
rbx_enable_warnings(rbx-platform-recent-documents)

if(NOT APPLE AND NOT WIN32 AND NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_library(rbx-platform-clipboard STATIC src/ClipboardStub.cpp)
    add_library(Roblox::PlatformClipboard ALIAS rbx-platform-clipboard)
    target_link_libraries(rbx-platform-clipboard PUBLIC Roblox::PlatformContract)
    target_compile_features(rbx-platform-clipboard PUBLIC cxx_std_20)
endif()

function(rbx_add_portable_platform_runtime target)
    set(_rbx_platform_runtime_sources
        "${CMAKE_SOURCE_DIR}/platform/common/src/FileSystemIndependent.cpp"
        "${CMAKE_SOURCE_DIR}/platform/common/src/MemoryStatsCommon.cpp")
    list(APPEND _rbx_platform_runtime_sources ${ARGN})
    add_library(${target} STATIC EXCLUDE_FROM_ALL
        ${_rbx_platform_runtime_sources})
    target_include_directories(${target} PRIVATE
        "${CMAKE_SOURCE_DIR}/App/include"
        "${CMAKE_SOURCE_DIR}/App"
        "${CMAKE_SOURCE_DIR}/engine/core/include"
        "${CMAKE_SOURCE_DIR}/engine/core/math/include"
        "${CMAKE_SOURCE_DIR}/engine/security/include"
        "${CMAKE_SOURCE_DIR}/engine/reflection/include"
        "${CMAKE_SOURCE_DIR}/engine/datamodel/runtime/include"
        "${CMAKE_SOURCE_DIR}/engine/ui/runtime/include"
        "${CMAKE_SOURCE_DIR}/engine/datamodel/tree/include"
        "${CMAKE_SOURCE_DIR}/engine/simulation/include"
        "${CMAKE_SOURCE_DIR}/engine/animation/include"
        "${CMAKE_SOURCE_DIR}/engine/scripting/include"
        "${CMAKE_SOURCE_DIR}/engine/scripting/lua/src"
        "${CMAKE_SOURCE_DIR}/engine/networking/include"
        "${CMAKE_SOURCE_DIR}/engine/rendering/foundation/include"
        "${CMAKE_SOURCE_DIR}/engine/rendering/adorn/include"
        "${CMAKE_SOURCE_DIR}/engine/interaction/tools/include"
        "${CMAKE_SOURCE_DIR}/apps/player/shared/client"
        "${CMAKE_SOURCE_DIR}/third_party/bullet_historical"
        "${CMAKE_SOURCE_DIR}/Library/boost/include"
        "${CMAKE_SOURCE_DIR}/Library/SDL2/include")
    target_link_libraries(${target} PUBLIC
        Roblox::Core Roblox::ValueTypes Roblox::Security)
    target_compile_features(${target} PUBLIC cxx_std_20)
    target_compile_definitions(${target} PRIVATE
        BOOST_NO_CXX98_FUNCTION_BASE=1)
    rbx_enable_warnings(${target})
endfunction()

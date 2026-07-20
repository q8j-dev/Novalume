cmake_minimum_required(VERSION 3.28)

project(Novalume VERSION 0.1.0 LANGUAGES C CXX)

option(RBX_BUILD_PLAYER "Build the shared RobloxPlayer runtime" ON)
option(RBX_BUILD_TESTS "Build modernization tests" ON)
option(RBX_FETCH_DEPENDENCIES "Fetch pinned open-source dependencies into the build tree" ON)
option(RBX_ENABLE_LEGACY_BUILD "Configure the historical monolithic build (migration only)" OFF)
option(RBX_ENABLE_LEGACY_RENDERERS "Compile historical D3D/OpenGL renderers" OFF)
set(RBX_PLAYER_REFERENCE_ROOT "" CACHE PATH
    "Build-time path to the supplied Windows Player snapshot for verified resource import")
set(RBX_STUDIO_REFERENCE_ROOT "" CACHE PATH
    "Build-time path to the supplied Windows Studio snapshot for verified avatar import")
set(RBX_ARCHIVED_AVATAR_ASSET_ROOT "" CACHE PATH
    "Build-time path to the authorized, hash-pinned avatar asset archive")
set(RBX_APK_REFERENCE_PATH "" CACHE FILEPATH
    "Build-time path to the supplied Android APK for static mobile inventory verification")

if(RBX_ENABLE_LEGACY_BUILD)
    include(cmake/LegacyRoot.cmake)
    return()
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(RbxCompilerWarnings)

if(APPLE)
    # bgfx's Metal implementation contains Objective-C++ translation units, so
    # enable the language before its targets are created.
    enable_language(OBJCXX)
endif()

include(RbxDependencies)

add_subdirectory(third_party/boost_runtime)
add_subdirectory(shaders)
add_subdirectory(engine/core)
add_subdirectory(engine/security)
add_subdirectory(engine/reflection)
add_subdirectory(engine/geometry)
add_subdirectory(engine/value_types)
add_subdirectory(engine/audio)
add_subdirectory(engine/media)
add_subdirectory(third_party/lz4_historical)
add_subdirectory(engine/serialization)
add_subdirectory(engine/assets)
add_subdirectory(engine/datamodel/tree)
add_subdirectory(engine/physics)
add_subdirectory(engine/terrain)
add_subdirectory(engine/simulation)
add_subdirectory(engine/animation)
add_subdirectory(platform/common)
if(APPLE AND CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    add_subdirectory(platform/macos)
elseif(WIN32)
    add_subdirectory(platform/windows)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_subdirectory(platform/linux)
endif()
add_subdirectory(engine/datamodel/runtime)
add_subdirectory(engine/runtime_support)
add_subdirectory(engine/terrain/runtime)
add_subdirectory(engine/avatar/humanoid)
add_subdirectory(engine/networking/transport)
add_subdirectory(engine/networking/runtime)
add_subdirectory(engine/geometry/csg)
add_subdirectory(engine/rendering/core)
add_subdirectory(engine/rendering/foundation)
add_subdirectory(engine/rendering/adorn)
add_subdirectory(engine/interaction/tools)
add_subdirectory(engine/ui/layout)
add_subdirectory(engine/ui/input)
add_subdirectory(engine/ui/runtime)
add_subdirectory(engine/scripting)
add_subdirectory(engine/rendering/materials)
add_subdirectory(engine/rendering/text)
add_subdirectory(engine/rendering/ui)
add_subdirectory(engine/rendering/scene)

if(RBX_BUILD_PLAYER)
    add_subdirectory(apps/player)
endif()

if(RBX_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

add_custom_target(check-dependency-boundaries
    COMMAND "${CMAKE_COMMAND}"
        -DSOURCE_ROOT="${CMAKE_CURRENT_SOURCE_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tools/dependency_boundary/check.cmake"
    COMMENT "Checking engine/platform/application dependency boundaries"
    VERBATIM)

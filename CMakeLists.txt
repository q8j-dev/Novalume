cmake_minimum_required(VERSION 3.28)

project(Novalume VERSION 0.1.0 LANGUAGES C CXX)

if(WIN32)
    enable_language(RC)
endif()

option(RBX_BUILD_PLAYER "Build the shared RobloxPlayer runtime" ON)
option(RBX_BUILD_TESTS "Build modernization tests" ON)
option(RBX_FETCH_DEPENDENCIES "Fetch pinned open-source dependencies into the build tree" ON)
option(RBX_ENABLE_LEGACY_BUILD "Configure the historical monolithic build (migration only)" OFF)
option(RBX_ENABLE_LEGACY_RENDERERS "Compile historical D3D/OpenGL renderers" OFF)
option(RBX_ENABLE_VMPROTECT
    "Compile proprietary VMProtect marker calls (requires a separately supplied SDK)" OFF)
set(RBX_VMPROTECT_INCLUDE_DIR "" CACHE PATH
    "Directory containing VMProtect/VMProtectSDK.h when RBX_ENABLE_VMPROTECT is ON")
set(RBX_VMPROTECT_LIBRARY "" CACHE FILEPATH
    "Target-architecture VMProtect SDK import library when RBX_ENABLE_VMPROTECT is ON")
option(RBX_ENABLE_LEGACY_X86_CLIENT_SECURITY
    "Compile the historical Win32/x86 client anti-tamper jobs (requires the real VMProtect SDK)" OFF)
set(RBX_PLAYER_REFERENCE_ROOT "" CACHE PATH
    "Build-time path to the supplied Windows Player snapshot for verified resource import")
set(RBX_STUDIO_REFERENCE_ROOT "" CACHE PATH
    "Build-time path to the supplied Windows Studio snapshot for verified avatar import")
set(RBX_ARCHIVED_AVATAR_ASSET_ROOT "" CACHE PATH
    "Build-time path to the authorized, hash-pinned avatar asset archive")
set(RBX_APK_REFERENCE_PATH "" CACHE FILEPATH
    "Build-time path to the supplied Android APK for static mobile inventory verification")
set(RBX_PREBUILT_RUNTIME_RESOURCE_BUNDLE "" CACHE FILEPATH
    "Build-time path to a private, hash-verified generated Player runtime resource bundle")
set(RBX_PREBUILT_RUNTIME_RESOURCE_BUNDLE_SHA256 "" CACHE STRING
    "Required SHA-256 pin for RBX_PREBUILT_RUNTIME_RESOURCE_BUNDLE")

if(RBX_ENABLE_LEGACY_BUILD)
    include(cmake/LegacyRoot.cmake)
    return()
endif()

if(RBX_ENABLE_LEGACY_X86_CLIENT_SECURITY)
    if(NOT WIN32)
        message(FATAL_ERROR
            "RBX_ENABLE_LEGACY_X86_CLIENT_SECURITY is only supported on Win32/x86")
    endif()
    if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
        message(FATAL_ERROR
            "RBX_ENABLE_LEGACY_X86_CLIENT_SECURITY requires a 32-bit x86 target")
    endif()
    if(NOT MSVC)
        message(FATAL_ERROR
            "RBX_ENABLE_LEGACY_X86_CLIENT_SECURITY requires the MSVC Win32 toolchain")
    endif()
    if(NOT RBX_ENABLE_VMPROTECT)
        message(FATAL_ERROR
            "RBX_ENABLE_LEGACY_X86_CLIENT_SECURITY requires RBX_ENABLE_VMPROTECT "
            "and a genuine vendor SDK/import library")
    endif()
    add_compile_definitions(RBX_ENABLE_LEGACY_X86_CLIENT_SECURITY=1)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)

if(CMAKE_C_COMPILER_ID MATCHES "AppleClang|Clang")
    foreach(prefix_map IN ITEMS
        "${CMAKE_SOURCE_DIR}=."
        "${CMAKE_BINARY_DIR}=./build")
        add_compile_options(
            "$<$<CONFIG:Release>:-ffile-prefix-map=${prefix_map}>"
            "$<$<CONFIG:Release>:-fdebug-prefix-map=${prefix_map}>"
            "$<$<CONFIG:Release>:-fmacro-prefix-map=${prefix_map}>")
    endforeach()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(RbxCompilerWarnings)

if(APPLE)
    # bgfx's Metal implementation contains Objective-C++ translation units, so
    # enable the language before its targets are created.
    enable_language(OBJCXX)
endif()

include(RbxDependencies)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(RBX_LEGACY_CASE_COMPAT_DIR
        "${CMAKE_BINARY_DIR}/generated/case-sensitive-include")
    function(rbx_add_case_compat alias source)
        get_filename_component(alias_directory "${alias}" DIRECTORY)
        file(MAKE_DIRECTORY
            "${RBX_LEGACY_CASE_COMPAT_DIR}/${alias_directory}")
        if(NOT EXISTS "${RBX_LEGACY_CASE_COMPAT_DIR}/${alias}")
            file(CREATE_LINK "${CMAKE_SOURCE_DIR}/${source}"
                "${RBX_LEGACY_CASE_COMPAT_DIR}/${alias}" SYMBOLIC)
        endif()
    endfunction()
    rbx_add_case_compat("rbx/boost.hpp" "engine/core/include/rbx/Boost.hpp")
    rbx_add_case_compat("V8DataModel/PartInstance.h" "engine/datamodel/runtime/include/v8datamodel/PartInstance.h")
    rbx_add_case_compat("V8DataModel/Workspace.h" "engine/datamodel/runtime/include/v8datamodel/Workspace.h")
    rbx_add_case_compat("V8DataModel/MouseCommand.h" "engine/datamodel/runtime/include/v8datamodel/MouseCommand.h")
    rbx_add_case_compat("V8DataModel/Filters.h" "engine/datamodel/runtime/include/v8datamodel/Filters.h")
    rbx_add_case_compat("V8DataModel/JointInstance.h" "engine/datamodel/runtime/include/v8datamodel/JointInstance.h")
    rbx_add_case_compat("V8World/Primitive.h" "engine/simulation/include/v8world/Primitive.h")
    rbx_add_case_compat("V8World/Contact.h" "engine/simulation/include/v8world/Contact.h")
    rbx_add_case_compat("V8World/World.h" "engine/simulation/include/v8world/World.h")
    rbx_add_case_compat("V8World/ContactManager.h" "engine/simulation/include/v8world/ContactManager.h")
    rbx_add_case_compat("V8World/WeldJoint.h" "engine/simulation/include/v8world/WeldJoint.h")
    rbx_add_case_compat("V8World/MultiJoint.h" "engine/simulation/include/v8world/MultiJoint.h")
    rbx_add_case_compat("V8World/Tolerance.h" "engine/simulation/include/v8world/Tolerance.h")
    rbx_add_case_compat("V8World/Joint.h" "engine/simulation/include/v8world/Joint.h")
    rbx_add_case_compat("V8World/Geometry.h" "engine/simulation/include/v8world/Geometry.h")
    rbx_add_case_compat("V8World/MegaClusterPoly.h" "engine/simulation/include/v8world/MegaClusterPoly.h")
    rbx_add_case_compat("V8World/IWorldStage.h" "engine/simulation/include/v8world/IWorldStage.h")
    rbx_add_case_compat("V8World/Assembly.h" "engine/simulation/include/v8world/Assembly.h")
    rbx_add_case_compat("V8kernel/Body.h" "engine/simulation/include/v8kernel/Body.h")
    rbx_add_case_compat("Util/Units.h" "App/include/util/Units.h")
    rbx_add_case_compat("Util/Math.h" "App/include/util/Math.h")
    rbx_add_case_compat("Util/SimSendFilter.h" "App/include/util/SimSendFilter.h")
    include_directories(BEFORE SYSTEM "${RBX_LEGACY_CASE_COMPAT_DIR}")
endif()

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

if(RBX_BUILD_PLAYER AND
   (CMAKE_SYSTEM_NAME STREQUAL "Linux" OR WIN32))
    set(CPACK_PACKAGE_NAME "Novalume-RobloxPlayer")
    set(CPACK_PACKAGE_VENDOR "q8j-dev")
    set(CPACK_PACKAGE_VERSION "0.1.0")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
        "Cross-platform Roblox Player with the authentic Durango launcher")
    if(WIN32)
        set(CPACK_GENERATOR "ZIP")
    else()
        set(CPACK_GENERATOR "TGZ")
    endif()
    include(CPack)
endif()

add_custom_target(check-dependency-boundaries
    COMMAND "${CMAKE_COMMAND}"
        -DSOURCE_ROOT="${CMAKE_CURRENT_SOURCE_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tools/dependency_boundary/check.cmake"
    COMMENT "Checking engine/platform/application dependency boundaries"
    VERBATIM)

if(NOT TARGET Roblox::PlatformHost)
    message(FATAL_ERROR "RobloxPlayer needs a host adapter for this platform")
endif()

add_subdirectory(shared/client)

add_executable(RobloxPlayer MACOSX_BUNDLE
    LauncherResourceIntegrity.cpp
    LauncherResourceIntegrity.h
    main.cpp
    PlayerMain.h
    LocalPlayerThumbnailProvider.cpp
    LocalPlayerThumbnailProvider.h
    PlayerRuntime.cpp
    PlayerRuntime.h
    shared/client_base/RenderSettingsItem.cpp
    shared/client_base/RenderSettingsItem.h)
target_link_libraries(RobloxPlayer PRIVATE
    Roblox::RenderScene Roblox::LuauScripting Roblox::RuntimeUI Roblox::InteractionTools
    Roblox::AudioDataModel Roblox::DataModelRuntime Roblox::RuntimeSupport
    Roblox::Core
    Roblox::AvatarHumanoid Roblox::NetworkingRuntime Roblox::TerrainRuntime
    Roblox::PlayerShared Roblox::Assets Roblox::PlatformRuntime Roblox::PlatformHost)
target_compile_features(RobloxPlayer PRIVATE cxx_std_20)
target_include_directories(RobloxPlayer PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/shared/client_base"
    "${CMAKE_SOURCE_DIR}/engine/networking"
    "${CMAKE_SOURCE_DIR}/engine/rendering/materials/include/GfxRender"
    "${CMAKE_SOURCE_DIR}/engine/rendering/scene/include/GfxRender"
    "${CMAKE_SOURCE_DIR}/Library/SDL2/include")
target_compile_definitions(RobloxPlayer PRIVATE
    _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR=1)
target_compile_options(RobloxPlayer PRIVATE
    $<$<COMPILE_LANG_AND_ID:CXX,AppleClang>:-Wno-enum-constexpr-conversion>)
rbx_enable_warnings(RobloxPlayer)

add_executable(RobloxPlayerLegacy MACOSX_BUNDLE EXCLUDE_FROM_ALL
    LauncherResourceIntegrity.cpp
    LauncherResourceIntegrity.h
    main.cpp
    PlayerMain.h
    LocalPlayerThumbnailProvider.cpp
    LocalPlayerThumbnailProvider.h
    PlayerRuntime.cpp
    PlayerRuntime.h
    shared/client_base/RenderSettingsItem.cpp
    shared/client_base/RenderSettingsItem.h)
target_link_libraries(RobloxPlayerLegacy PRIVATE
    Roblox::RenderScene Roblox::LuauScripting Roblox::RuntimeUI Roblox::InteractionTools
    Roblox::AudioDataModel Roblox::DataModelRuntime Roblox::RuntimeSupport
    Roblox::Core
    Roblox::AvatarHumanoid Roblox::NetworkingRuntime Roblox::TerrainRuntime
    Roblox::PlayerShared Roblox::Assets Roblox::PlatformRuntime Roblox::PlatformHost)
target_compile_features(RobloxPlayerLegacy PRIVATE cxx_std_20)
target_include_directories(RobloxPlayerLegacy PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/shared/client_base"
    "${CMAKE_SOURCE_DIR}/engine/networking"
    "${CMAKE_SOURCE_DIR}/engine/rendering/materials/include/GfxRender"
    "${CMAKE_SOURCE_DIR}/engine/rendering/scene/include/GfxRender"
    "${CMAKE_SOURCE_DIR}/Library/SDL2/include")
target_compile_definitions(RobloxPlayerLegacy PRIVATE
    RBX_LEGACY_UI_TEST=1
    _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR=1)
target_compile_options(RobloxPlayerLegacy PRIVATE
    $<$<COMPILE_LANG_AND_ID:CXX,AppleClang>:-Wno-enum-constexpr-conversion>)
rbx_enable_warnings(RobloxPlayerLegacy)

if(WIN32)
    set(RBX_PLAYER_ICON_FILE
        "${CMAKE_CURRENT_SOURCE_DIR}/windows/source/Roblox.ico")
    set(RBX_PLAYER_MANIFEST_FILE
        "${CMAKE_CURRENT_SOURCE_DIR}/windows/RobloxPlayer.manifest")
    configure_file(
        windows/RobloxPlayerResources.h.in
        "${CMAKE_CURRENT_BINARY_DIR}/windows/RobloxPlayerResources.h"
        @ONLY)
    set(RBX_PLAYER_RC_DEPENDENCIES
        "${CMAKE_CURRENT_BINARY_DIR}/windows/RobloxPlayerResources.h"
        "${RBX_PLAYER_ICON_FILE}"
        "${RBX_PLAYER_MANIFEST_FILE}")
    set_source_files_properties(windows/RobloxPlayer.rc PROPERTIES
        OBJECT_DEPENDS "${RBX_PLAYER_RC_DEPENDENCIES}")
    target_sources(RobloxPlayer PRIVATE
        windows/PlayerEntrypoint.cpp
        windows/RobloxPlayer.rc)
    target_sources(RobloxPlayerLegacy PRIVATE
        windows/PlayerEntrypoint.cpp
        windows/RobloxPlayer.rc)
    target_include_directories(RobloxPlayer PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}/windows")
    target_include_directories(RobloxPlayerLegacy PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}/windows")
    target_compile_definitions(RobloxPlayer PRIVATE
        NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE)
    target_compile_definitions(RobloxPlayerLegacy PRIVATE
        NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE)
    set_target_properties(RobloxPlayer RobloxPlayerLegacy PROPERTIES
        WIN32_EXECUTABLE TRUE)
    target_link_options(RobloxPlayer PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/MANIFEST:NO>)
    target_link_options(RobloxPlayerLegacy PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/MANIFEST:NO>)
    target_link_libraries(RobloxPlayer PRIVATE shell32)
    target_link_libraries(RobloxPlayerLegacy PRIVATE shell32)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    target_sources(RobloxPlayer PRIVATE ios/PlayerEntrypoint.mm)
    target_sources(RobloxPlayerLegacy PRIVATE ios/PlayerEntrypoint.mm)
    target_link_libraries(RobloxPlayer PRIVATE "-framework UIKit")
    target_link_libraries(RobloxPlayerLegacy PRIVATE "-framework UIKit")
    set(RBX_PLAYER_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info-iOS.plist.in")
else()
    set(RBX_PLAYER_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in")
endif()

set_target_properties(RobloxPlayer PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST "${RBX_PLAYER_INFO_PLIST}"
    MACOSX_BUNDLE_BUNDLE_NAME "Roblox"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.roblox.offline-player"
    MACOSX_BUNDLE_INFO_STRING "Shared cross-platform Roblox player"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_BUNDLE_VERSION "1")

set_target_properties(RobloxPlayerLegacy PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST "${RBX_PLAYER_INFO_PLIST}"
    MACOSX_BUNDLE_BUNDLE_NAME "Roblox Legacy UI Test"
    MACOSX_BUNDLE_GUI_IDENTIFIER "dev.q8j.novalume.legacy-ui-test"
    MACOSX_BUNDLE_INFO_STRING "Novalume legacy UI and avatar test player"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_BUNDLE_VERSION "1")

if(APPLE)
    set(RBX_PLAYER_RESOURCE_DIR
        "$<TARGET_BUNDLE_CONTENT_DIR:RobloxPlayer>/Resources")
    set(RBX_LEGACY_PLAYER_RESOURCE_DIR
        "$<TARGET_BUNDLE_CONTENT_DIR:RobloxPlayerLegacy>/Resources")
else()
    set(RBX_PLAYER_RESOURCE_DIR "$<TARGET_FILE_DIR:RobloxPlayer>/Resources")
    set(RBX_LEGACY_PLAYER_RESOURCE_DIR
        "$<TARGET_FILE_DIR:RobloxPlayerLegacy>/Resources")
endif()

set(RBX_RESOURCE_MANIFEST "${CMAKE_BINARY_DIR}/generated/resources/manifest.json")
add_custom_command(
    OUTPUT "${RBX_RESOURCE_MANIFEST}"
    COMMAND "${CMAKE_COMMAND}"
        "-DINPUT_ROOT=${CMAKE_SOURCE_DIR}/content"
        "-DOUTPUT_FILE=${RBX_RESOURCE_MANIFEST}"
        -P "${CMAKE_SOURCE_DIR}/cmake/GenerateResourceManifest.cmake"
    DEPENDS "${CMAKE_SOURCE_DIR}/cmake/GenerateResourceManifest.cmake"
    COMMENT "Hashing preserved Roblox content into the resource manifest"
    VERBATIM)
add_custom_target(rbx-resource-manifest DEPENDS "${RBX_RESOURCE_MANIFEST}")

set(RBX_UI_OVERLAY_ROOT "")
set(RBX_AVATAR_RUNTIME_ROOT "")
set(RBX_STUDIO_RUNTIME_ROOT "")
set(RBX_CURRENT_PLAYER_SCRIPTS_MODEL "")
set(RBX_CURRENT_PLAYER_SCRIPTS_MANIFEST "")
set(RBX_IN_EXPERIENCE_MODEL_ROOT "")
set(RBX_PLAYER_CJK_FONT "")
set(RBX_PREBUILT_RUNTIME_RESOURCE_ROOT "")
if(RBX_PREBUILT_RUNTIME_RESOURCE_BUNDLE)
    if(NOT EXISTS "${RBX_PREBUILT_RUNTIME_RESOURCE_BUNDLE}" OR
       IS_DIRECTORY "${RBX_PREBUILT_RUNTIME_RESOURCE_BUNDLE}")
        message(FATAL_ERROR
            "RBX_PREBUILT_RUNTIME_RESOURCE_BUNDLE is not a private runtime bundle file")
    endif()
    string(LENGTH "${RBX_PREBUILT_RUNTIME_RESOURCE_BUNDLE_SHA256}"
        RBX_PREBUILT_RUNTIME_RESOURCE_BUNDLE_SHA256_LENGTH)
    if(NOT RBX_PREBUILT_RUNTIME_RESOURCE_BUNDLE_SHA256 MATCHES "^[0-9a-fA-F]+$" OR
       NOT RBX_PREBUILT_RUNTIME_RESOURCE_BUNDLE_SHA256_LENGTH EQUAL 64)
        message(FATAL_ERROR
            "RBX_PREBUILT_RUNTIME_RESOURCE_BUNDLE_SHA256 must pin the private runtime bundle")
    endif()
    file(SHA256 "${RBX_PREBUILT_RUNTIME_RESOURCE_BUNDLE}"
        RBX_PREBUILT_RUNTIME_RESOURCE_BUNDLE_ACTUAL_SHA256)
    string(TOLOWER "${RBX_PREBUILT_RUNTIME_RESOURCE_BUNDLE_SHA256}"
        RBX_PREBUILT_RUNTIME_RESOURCE_BUNDLE_EXPECTED_SHA256)
    if(NOT RBX_PREBUILT_RUNTIME_RESOURCE_BUNDLE_ACTUAL_SHA256 STREQUAL
       RBX_PREBUILT_RUNTIME_RESOURCE_BUNDLE_EXPECTED_SHA256)
        message(FATAL_ERROR "Private runtime resource bundle SHA-256 does not match its pin")
    endif()
    find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
    set(RBX_PREBUILT_RUNTIME_RESOURCE_ROOT
        "${CMAKE_BINARY_DIR}/generated/resources/prebuilt-runtime")
    execute_process(
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_SOURCE_DIR}/tools/resource_importer/runtime_resource_bundle.py"
            verify
            --bundle "${RBX_PREBUILT_RUNTIME_RESOURCE_BUNDLE}"
            --extract "${RBX_PREBUILT_RUNTIME_RESOURCE_ROOT}"
        RESULT_VARIABLE RBX_RUNTIME_BUNDLE_RESULT
        OUTPUT_VARIABLE RBX_RUNTIME_BUNDLE_OUTPUT
        ERROR_VARIABLE RBX_RUNTIME_BUNDLE_ERROR)
    if(NOT RBX_RUNTIME_BUNDLE_RESULT EQUAL 0)
        message(FATAL_ERROR
            "Private runtime resource bundle verification failed: ${RBX_RUNTIME_BUNDLE_ERROR}")
    endif()
    message(STATUS "Verified private Player runtime resources: ${RBX_RUNTIME_BUNDLE_OUTPUT}")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${RBX_PREBUILT_RUNTIME_RESOURCE_BUNDLE}"
        "${CMAKE_SOURCE_DIR}/tools/resource_importer/runtime_resource_bundle.py")
    set(RBX_UI_OVERLAY_ROOT
        "${RBX_PREBUILT_RUNTIME_RESOURCE_ROOT}/overlays/player-2026")
    set(RBX_AVATAR_RUNTIME_ROOT
        "${RBX_PREBUILT_RUNTIME_RESOURCE_ROOT}/overlays/avatar-runtime")
    set(RBX_STUDIO_RUNTIME_ROOT
        "${RBX_PREBUILT_RUNTIME_RESOURCE_ROOT}/overlays/studio-runtime")
    set(RBX_CURRENT_PLAYER_SCRIPTS_MODEL
        "${RBX_PREBUILT_RUNTIME_RESOURCE_ROOT}/content/scripts/PlayerScripts.rbxmx")
    set(RBX_CURRENT_PLAYER_SCRIPTS_MANIFEST
        "${RBX_PREBUILT_RUNTIME_RESOURCE_ROOT}/content/scripts/PlayerScripts.manifest.json")
    set(RBX_IN_EXPERIENCE_MODEL_ROOT
        "${RBX_PREBUILT_RUNTIME_RESOURCE_ROOT}/models/InExperience")
    set(RBX_PLAYER_CJK_FONT
        "${RBX_PREBUILT_RUNTIME_RESOURCE_ROOT}/PlatformContent/pc/fonts/NotoSansCJKjp-Regular.otf")
    set_property(TARGET RobloxPlayer APPEND PROPERTY LINK_DEPENDS
        "${RBX_PREBUILT_RUNTIME_RESOURCE_BUNDLE}")
    set_property(TARGET RobloxPlayerLegacy APPEND PROPERTY LINK_DEPENDS
        "${RBX_PREBUILT_RUNTIME_RESOURCE_BUNDLE}")
endif()

if(RBX_PREBUILT_RUNTIME_RESOURCE_ROOT)
    # All generated roots were extracted and verified above.  The generic
    # packaging rules below copy the exact same layout as a local reference build.
elseif(RBX_STUDIO_REFERENCE_ROOT)
    if(NOT IS_DIRECTORY "${RBX_STUDIO_REFERENCE_ROOT}")
        message(FATAL_ERROR "RBX_STUDIO_REFERENCE_ROOT is not the supplied Studio directory")
    endif()
    find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
    if(NOT IS_DIRECTORY "${RBX_ARCHIVED_AVATAR_ASSET_ROOT}")
        message(FATAL_ERROR
            "RBX_ARCHIVED_AVATAR_ASSET_ROOT is required for authentic Rthro meshes")
    endif()
    set(RBX_AVATAR_RUNTIME_ROOT
        "${CMAKE_BINARY_DIR}/generated/resources/avatar-runtime-overlay")
    set(RBX_AVATAR_RUNTIME_MANIFEST
        "${RBX_AVATAR_RUNTIME_ROOT}/avatar-runtime-manifest.json")
    file(GLOB_RECURSE RBX_STUDIO_AVATAR_REFERENCE_FILES CONFIGURE_DEPENDS
        "${RBX_STUDIO_REFERENCE_ROOT}/content/avatar/*")
    file(GLOB_RECURSE RBX_ARCHIVED_AVATAR_ASSET_FILES CONFIGURE_DEPENDS
        "${RBX_ARCHIVED_AVATAR_ASSET_ROOT}/assets/*")
    add_custom_command(
        OUTPUT "${RBX_AVATAR_RUNTIME_MANIFEST}"
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_SOURCE_DIR}/tools/resource_importer/import_avatar_runtime.py"
            --manifest
                "${CMAKE_SOURCE_DIR}/tools/resource_importer/avatar-runtime-assets.json"
            --studio-root "${RBX_STUDIO_REFERENCE_ROOT}"
            --asset-archive-root "${RBX_ARCHIVED_AVATAR_ASSET_ROOT}"
            --output "${RBX_AVATAR_RUNTIME_ROOT}"
        DEPENDS
            "${CMAKE_SOURCE_DIR}/tools/resource_importer/import_avatar_runtime.py"
            "${CMAKE_SOURCE_DIR}/tools/resource_importer/avatar-runtime-assets.json"
            "${RBX_STUDIO_REFERENCE_ROOT}/content/models/Thumbnails/Mannequins/R15-plus.rbxm"
            "${RBX_STUDIO_REFERENCE_ROOT}/StudioContent/models/RigBuilder/AnthroRigs.rbxm"
            ${RBX_STUDIO_AVATAR_REFERENCE_FILES}
            ${RBX_ARCHIVED_AVATAR_ASSET_FILES}
            "${RBX_ARCHIVED_AVATAR_ASSET_ROOT}/manifest.json"
        COMMENT "Importing hash-pinned R15 runtime assets from the supplied Studio build"
        VERBATIM)
    add_custom_target(rbx-avatar-runtime-overlay
        DEPENDS "${RBX_AVATAR_RUNTIME_MANIFEST}")
    add_dependencies(RobloxPlayer rbx-avatar-runtime-overlay)
    add_dependencies(RobloxPlayerLegacy rbx-avatar-runtime-overlay)
    set_property(TARGET RobloxPlayer APPEND PROPERTY LINK_DEPENDS
        "${RBX_AVATAR_RUNTIME_MANIFEST}")
    set_property(TARGET RobloxPlayerLegacy APPEND PROPERTY LINK_DEPENDS
        "${RBX_AVATAR_RUNTIME_MANIFEST}")

    set(RBX_STUDIO_RUNTIME_ROOT
        "${CMAKE_BINARY_DIR}/generated/resources/studio-runtime-overlay")
    set(RBX_STUDIO_RUNTIME_MANIFEST
        "${RBX_STUDIO_RUNTIME_ROOT}/studio-runtime-manifest.json")
    add_custom_command(
        OUTPUT "${RBX_STUDIO_RUNTIME_MANIFEST}"
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_SOURCE_DIR}/tools/resource_importer/import_studio_runtime.py"
            --manifest
                "${CMAKE_SOURCE_DIR}/tools/resource_importer/studio-runtime-assets.json"
            --studio-root "${RBX_STUDIO_REFERENCE_ROOT}"
            --output "${RBX_STUDIO_RUNTIME_ROOT}"
        DEPENDS
            "${CMAKE_SOURCE_DIR}/tools/resource_importer/import_studio_runtime.py"
            "${CMAKE_SOURCE_DIR}/tools/resource_importer/studio-runtime-assets.json"
            "${RBX_STUDIO_REFERENCE_ROOT}/ExtraContent/models/ServerCoreScripts.rbxm"
        COMMENT "Importing hash-pinned Studio server runtime assets"
        VERBATIM)
    add_custom_target(rbx-studio-runtime-overlay
        DEPENDS "${RBX_STUDIO_RUNTIME_MANIFEST}")
    add_dependencies(RobloxPlayer rbx-studio-runtime-overlay)
    add_dependencies(RobloxPlayerLegacy rbx-studio-runtime-overlay)

    set(RBX_CURRENT_PLAYER_SCRIPTS_ROOT
        "${CMAKE_BINARY_DIR}/generated/resources/current-player-scripts")
    set(RBX_CURRENT_PLAYER_SCRIPTS_MODEL
        "${RBX_CURRENT_PLAYER_SCRIPTS_ROOT}/PlayerScripts.rbxmx")
    set(RBX_CURRENT_PLAYER_SCRIPTS_MANIFEST
        "${RBX_CURRENT_PLAYER_SCRIPTS_ROOT}/PlayerScripts.manifest.json")
    file(GLOB_RECURSE RBX_CURRENT_PLAYER_SCRIPT_SOURCES CONFIGURE_DEPENDS
        "${RBX_STUDIO_REFERENCE_ROOT}/ExtraContent/scripts/PlayerScripts/StarterPlayerScripts/*.lua"
        "${RBX_STUDIO_REFERENCE_ROOT}/ExtraContent/scripts/PlayerScripts/StarterPlayerScripts/PlayerModule.module/*.lua")
    add_custom_command(
        OUTPUT "${RBX_CURRENT_PLAYER_SCRIPTS_MODEL}"
            "${RBX_CURRENT_PLAYER_SCRIPTS_MANIFEST}"
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_SOURCE_DIR}/tools/resource_importer/build_current_player_scripts.py"
            --studio-root "${RBX_STUDIO_REFERENCE_ROOT}"
            --output "${RBX_CURRENT_PLAYER_SCRIPTS_MODEL}"
            --manifest "${RBX_CURRENT_PLAYER_SCRIPTS_MANIFEST}"
        DEPENDS
            "${CMAKE_SOURCE_DIR}/tools/resource_importer/build_current_player_scripts.py"
            "${RBX_STUDIO_REFERENCE_ROOT}/ExtraContent/scripts/PlayerScripts/default.rbxp"
            ${RBX_CURRENT_PLAYER_SCRIPT_SOURCES}
        COMMENT "Building the exact supplied Studio PlayerModule into a runtime model"
        VERBATIM)
    add_custom_target(rbx-current-player-scripts
        DEPENDS "${RBX_CURRENT_PLAYER_SCRIPTS_MODEL}"
            "${RBX_CURRENT_PLAYER_SCRIPTS_MANIFEST}")
    add_dependencies(RobloxPlayer rbx-current-player-scripts)
    add_dependencies(RobloxPlayerLegacy rbx-current-player-scripts)
    set_property(TARGET RobloxPlayer APPEND PROPERTY LINK_DEPENDS
        "${RBX_CURRENT_PLAYER_SCRIPTS_MODEL}"
        "${RBX_CURRENT_PLAYER_SCRIPTS_MANIFEST}")
    set_property(TARGET RobloxPlayerLegacy APPEND PROPERTY LINK_DEPENDS
        "${RBX_CURRENT_PLAYER_SCRIPTS_MODEL}"
        "${RBX_CURRENT_PLAYER_SCRIPTS_MANIFEST}")
else()
    message(WARNING
        "Neither a private runtime bundle nor a supplied Studio path is configured; authentic R15 assets will not be packaged")
endif()

if(RBX_PREBUILT_RUNTIME_RESOURCE_ROOT)
    # The UI overlay, InExperience patch, and CJK font were verified together
    # by the private bundle importer above.
elseif(RBX_PLAYER_REFERENCE_ROOT AND RBX_APK_REFERENCE_PATH)
    if(NOT IS_DIRECTORY "${RBX_PLAYER_REFERENCE_ROOT}")
        message(FATAL_ERROR "RBX_PLAYER_REFERENCE_ROOT is not the supplied Player directory")
    endif()
    if(NOT EXISTS "${RBX_APK_REFERENCE_PATH}")
        message(FATAL_ERROR "RBX_APK_REFERENCE_PATH does not exist")
    endif()
    find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
    set(RBX_UI_INVENTORY "${CMAKE_BINARY_DIR}/generated/resources/in-game-ui-inventory.json")
    set(RBX_UI_OVERLAY_ROOT "${CMAKE_BINARY_DIR}/generated/resources/in-game-ui-overlay")
    set(RBX_UI_OVERLAY_MANIFEST "${RBX_UI_OVERLAY_ROOT}/overlay-manifest.json")
    set(RBX_PLAYER_CJK_FONT
        "${RBX_PLAYER_REFERENCE_ROOT}/PlatformContent/pc/fonts/NotoSansCJKjp-Regular.otf")
    set(RBX_PLAYER_CJK_FONT_SHA256
        "f18253affd968c8ea592d8df6d84b1d4c5d3a46d267e53a1f6749e6c4f76a23d")
    if(NOT EXISTS "${RBX_PLAYER_CJK_FONT}")
        message(FATAL_ERROR "Supplied Player directory has no current PC CJK fallback font")
    endif()
    file(SHA256 "${RBX_PLAYER_CJK_FONT}" RBX_PLAYER_CJK_FONT_ACTUAL_SHA256)
    if(NOT RBX_PLAYER_CJK_FONT_ACTUAL_SHA256 STREQUAL RBX_PLAYER_CJK_FONT_SHA256)
        message(FATAL_ERROR "Supplied Player CJK fallback font hash does not match the selected 2026 reference")
    endif()

    file(GLOB_RECURSE RBX_PLAYER_UI_REFERENCE_FILES CONFIGURE_DEPENDS
        "${RBX_PLAYER_REFERENCE_ROOT}/content/textures/ui/*"
        "${RBX_PLAYER_REFERENCE_ROOT}/content/textures/Cursors/KeyboardMouse/*"
        "${RBX_PLAYER_REFERENCE_ROOT}/content/fonts/*"
        "${RBX_PLAYER_REFERENCE_ROOT}/ExtraContent/textures/ui/AvatarExperience/*"
        "${RBX_PLAYER_REFERENCE_ROOT}/ExtraContent/textures/ui/Controls/*"
        "${RBX_PLAYER_REFERENCE_ROOT}/ExtraContent/textures/ui/Gamepad/*"
        "${RBX_PLAYER_REFERENCE_ROOT}/ExtraContent/textures/ui/ImageSet/InGameMenu/*"
        "${RBX_PLAYER_REFERENCE_ROOT}/ExtraContent/textures/ui/InGameChat/*"
        "${RBX_PLAYER_REFERENCE_ROOT}/ExtraContent/textures/ui/InGameMenu/*"
        "${RBX_PLAYER_REFERENCE_ROOT}/ExtraContent/textures/ui/Settings/*"
        "${RBX_PLAYER_REFERENCE_ROOT}/ExtraContent/LuaPackages/Packages/_Index/BuilderIcons/BuilderIcons/BuilderIcons.json"
        "${RBX_PLAYER_REFERENCE_ROOT}/ExtraContent/LuaPackages/Packages/_Index/BuilderIcons/BuilderIcons/Font/*"
        "${RBX_PLAYER_REFERENCE_ROOT}/ExtraContent/LuaPackages/Packages/_Index/FoundationImages/FoundationImages/*"
        "${RBX_PLAYER_REFERENCE_ROOT}/ExtraContent/models/AvatarContextMenu/AvatarContextArrow.rbxm"
        "${RBX_PLAYER_REFERENCE_ROOT}/content/configs/DateTimeLocaleConfigs/*.json"
        "${RBX_PLAYER_REFERENCE_ROOT}/content/fonts/BuilderSans-*.otf"
        "${RBX_PLAYER_REFERENCE_ROOT}/content/fonts/families/BuilderSans.json")
    file(GLOB_RECURSE RBX_LEGACY_UI_REFERENCE_FILES CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/content/textures/ui/*"
        "${CMAKE_SOURCE_DIR}/content/fonts/*")

    add_custom_command(
        OUTPUT "${RBX_UI_INVENTORY}"
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_SOURCE_DIR}/tools/resource_importer/reference_inventory.py"
            --spec "${CMAKE_SOURCE_DIR}/tools/resource_importer/ui-reference-spec.json"
            --corpus "legacy=${CMAKE_SOURCE_DIR}"
            --corpus "player=${RBX_PLAYER_REFERENCE_ROOT}"
            --corpus "apk=${RBX_APK_REFERENCE_PATH}"
            --output "${RBX_UI_INVENTORY}"
        DEPENDS
            "${CMAKE_SOURCE_DIR}/tools/resource_importer/reference_inventory.py"
            "${CMAKE_SOURCE_DIR}/tools/resource_importer/ui-reference-spec.json"
            "${RBX_APK_REFERENCE_PATH}"
            ${RBX_PLAYER_UI_REFERENCE_FILES}
            ${RBX_LEGACY_UI_REFERENCE_FILES}
        COMMENT "Hashing and comparing scoped 2016/2026 in-game Player UI resources"
        VERBATIM)
    add_custom_command(
        OUTPUT "${RBX_UI_OVERLAY_MANIFEST}"
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_SOURCE_DIR}/tools/resource_importer/import_in_game_ui.py"
            --inventory "${RBX_UI_INVENTORY}"
            --player-root "${RBX_PLAYER_REFERENCE_ROOT}"
            --output "${RBX_UI_OVERLAY_ROOT}"
        DEPENDS
            "${RBX_UI_INVENTORY}"
            "${CMAKE_SOURCE_DIR}/tools/resource_importer/import_in_game_ui.py"
        COMMENT "Importing hash-verified in-game Player UI into the generated overlay"
        VERBATIM)
    add_custom_target(rbx-in-game-ui-overlay DEPENDS "${RBX_UI_OVERLAY_MANIFEST}")
    add_dependencies(RobloxPlayer rbx-in-game-ui-overlay)

    set(RBX_IN_EXPERIENCE_MODEL_ROOT
        "${RBX_PLAYER_REFERENCE_ROOT}/ExtraContent/models/InExperience")
    if(NOT EXISTS "${RBX_IN_EXPERIENCE_MODEL_ROOT}/InExperience.rbxm" OR
       NOT EXISTS "${RBX_IN_EXPERIENCE_MODEL_ROOT}/InExperience_checksum")
        message(FATAL_ERROR "Supplied Player directory has no authoritative InExperience patch")
    endif()
else()
    message(WARNING
        "Neither a private runtime bundle nor supplied Player/APK paths are configured; authentic in-game UI overlay will not be packaged")
endif()

add_dependencies(RobloxPlayer rbx-player-shaders rbx-resource-manifest)
add_dependencies(RobloxPlayerLegacy rbx-player-shaders rbx-resource-manifest)
set(RBX_PLAYER_SHADER_LINK_DEPENDS
    "${CMAKE_BINARY_DIR}/generated/shaders/shaders_bgfx-glsl.pack"
    "${CMAKE_BINARY_DIR}/generated/shaders/shaders_bgfx-spirv.pack"
    "${CMAKE_BINARY_DIR}/generated/shaders/shaders.json")
if(APPLE AND CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    list(APPEND RBX_PLAYER_SHADER_LINK_DEPENDS
        "${CMAKE_BINARY_DIR}/generated/shaders/shaders_bgfx-metal.pack")
elseif(WIN32)
    list(APPEND RBX_PLAYER_SHADER_LINK_DEPENDS
        "${CMAKE_BINARY_DIR}/generated/shaders/shaders_bgfx-d3d11.pack")
elseif(EMSCRIPTEN)
    list(APPEND RBX_PLAYER_SHADER_LINK_DEPENDS
        "${CMAKE_BINARY_DIR}/generated/shaders/shaders_bgfx-wgsl.pack")
endif()
set_property(TARGET RobloxPlayer APPEND PROPERTY LINK_DEPENDS
    ${RBX_PLAYER_SHADER_LINK_DEPENDS})
set_property(TARGET RobloxPlayerLegacy APPEND PROPERTY LINK_DEPENDS
    ${RBX_PLAYER_SHADER_LINK_DEPENDS})

set(RBX_DURANGO_LAUNCHER_SOURCE_ROOT
    "${CMAKE_SOURCE_DIR}/PlatformContent/durango")
file(GLOB_RECURSE RBX_DURANGO_LAUNCHER_FILES LIST_DIRECTORIES false
    CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/PlatformContent/durango/scripts/ui/*"
    "${CMAKE_SOURCE_DIR}/PlatformContent/durango/textures/*"
    "${CMAKE_SOURCE_DIR}/PlatformContent/durango/terrain/*"
    "${CMAKE_SOURCE_DIR}/PlatformContent/durango/sounds/ui/*")
list(FILTER RBX_DURANGO_LAUNCHER_FILES EXCLUDE REGEX "(^|/)\\.DS_Store$")
list(APPEND RBX_DURANGO_LAUNCHER_FILES
    "${CMAKE_SOURCE_DIR}/PlatformContent/durango/ScaledWorldv4.7.rbxl")

set(RBX_DURANGO_LAUNCHER_GENERATED_ROOT
    "${CMAKE_BINARY_DIR}/generated/launcher")
set(RBX_DURANGO_LAUNCHER_PACKAGE_ROOT
    "${RBX_DURANGO_LAUNCHER_GENERATED_ROOT}/package")
set(RBX_DURANGO_LAUNCHER_MANIFEST
    "${RBX_DURANGO_LAUNCHER_PACKAGE_ROOT}/launcher-manifest.v1")
set(RBX_DURANGO_LAUNCHER_GENERATED_HEADER
    "${RBX_DURANGO_LAUNCHER_GENERATED_ROOT}/include/LauncherResourceManifest.generated.h")
set(RBX_DURANGO_LAUNCHER_STAGED_FILES)
foreach(source_file IN LISTS RBX_DURANGO_LAUNCHER_FILES)
    file(RELATIVE_PATH source_relative
        "${RBX_DURANGO_LAUNCHER_SOURCE_ROOT}" "${source_file}")
    if(source_relative MATCHES "^scripts/ui/")
        string(REGEX REPLACE "^scripts/ui/" "scripts/"
            logical_path "${source_relative}")
    else()
        set(logical_path "${source_relative}")
    endif()
    list(APPEND RBX_DURANGO_LAUNCHER_STAGED_FILES
        "${RBX_DURANGO_LAUNCHER_PACKAGE_ROOT}/content/${logical_path}")
endforeach()

add_custom_command(
    OUTPUT
        "${RBX_DURANGO_LAUNCHER_MANIFEST}"
        "${RBX_DURANGO_LAUNCHER_GENERATED_HEADER}"
        ${RBX_DURANGO_LAUNCHER_STAGED_FILES}
    COMMAND "${CMAKE_COMMAND}"
        "-DINPUT_ROOT=${RBX_DURANGO_LAUNCHER_SOURCE_ROOT}"
        "-DOUTPUT_GUARD_ROOT=${RBX_DURANGO_LAUNCHER_GENERATED_ROOT}"
        "-DOUTPUT_ROOT=${RBX_DURANGO_LAUNCHER_PACKAGE_ROOT}"
        "-DOUTPUT_HEADER=${RBX_DURANGO_LAUNCHER_GENERATED_HEADER}"
        -P "${CMAKE_SOURCE_DIR}/cmake/GenerateDurangoLauncherPayload.cmake"
    DEPENDS
        "${CMAKE_SOURCE_DIR}/cmake/GenerateDurangoLauncherPayload.cmake"
        ${RBX_DURANGO_LAUNCHER_FILES}
    COMMENT "Staging and hashing the authentic Durango launcher payload"
    VERBATIM)
add_custom_target(rbx-durango-launcher-payload DEPENDS
    "${RBX_DURANGO_LAUNCHER_MANIFEST}"
    "${RBX_DURANGO_LAUNCHER_GENERATED_HEADER}"
    ${RBX_DURANGO_LAUNCHER_STAGED_FILES})
add_dependencies(RobloxPlayer rbx-durango-launcher-payload)
add_dependencies(RobloxPlayerLegacy rbx-durango-launcher-payload)
target_include_directories(RobloxPlayer PRIVATE
    "${RBX_DURANGO_LAUNCHER_GENERATED_ROOT}/include")
target_include_directories(RobloxPlayerLegacy PRIVATE
    "${RBX_DURANGO_LAUNCHER_GENERATED_ROOT}/include")
set_property(TARGET RobloxPlayer APPEND PROPERTY LINK_DEPENDS
    ${RBX_DURANGO_LAUNCHER_FILES})
set_property(TARGET RobloxPlayerLegacy APPEND PROPERTY LINK_DEPENDS
    ${RBX_DURANGO_LAUNCHER_FILES})

add_custom_command(TARGET RobloxPlayer POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${CMAKE_SOURCE_DIR}/content"
        "${RBX_PLAYER_RESOURCE_DIR}/content"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${CMAKE_SOURCE_DIR}/PlatformContent/pc"
        "${RBX_PLAYER_RESOURCE_DIR}/PlatformContent/pc"
    COMMAND "${CMAKE_COMMAND}" -E rm -rf
        "${RBX_PLAYER_RESOURCE_DIR}/launcher"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${RBX_DURANGO_LAUNCHER_PACKAGE_ROOT}"
        "${RBX_PLAYER_RESOURCE_DIR}/launcher"
    COMMAND "${CMAKE_COMMAND}" -E make_directory
        "${RBX_PLAYER_RESOURCE_DIR}/places"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_SOURCE_DIR}/apps/studio/source/content/FirstPlaceBasePlate.rbxl"
        "${RBX_PLAYER_RESOURCE_DIR}/places/Baseplate.rbxl"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${CMAKE_BINARY_DIR}/generated/shaders"
        "${RBX_PLAYER_RESOURCE_DIR}/shaders"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${CMAKE_SOURCE_DIR}/shaders/manifests"
        "${RBX_PLAYER_RESOURCE_DIR}/shader-manifests"
    COMMAND "${CMAKE_COMMAND}" -E copy
        "${RBX_RESOURCE_MANIFEST}"
        "${RBX_PLAYER_RESOURCE_DIR}/resource-manifest.json"
    COMMAND "${CMAKE_COMMAND}" -E copy
        "${CMAKE_SOURCE_DIR}/THIRD_PARTY_NOTICES.md"
        "${RBX_PLAYER_RESOURCE_DIR}/THIRD_PARTY_NOTICES.md"
    COMMENT "Packaging owned shaders and source-tree Roblox resources")

add_custom_command(TARGET RobloxPlayerLegacy POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${CMAKE_SOURCE_DIR}/content"
        "${RBX_LEGACY_PLAYER_RESOURCE_DIR}/content"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${CMAKE_SOURCE_DIR}/PlatformContent/pc"
        "${RBX_LEGACY_PLAYER_RESOURCE_DIR}/PlatformContent/pc"
    COMMAND "${CMAKE_COMMAND}" -E rm -rf
        "${RBX_LEGACY_PLAYER_RESOURCE_DIR}/launcher"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${RBX_DURANGO_LAUNCHER_PACKAGE_ROOT}"
        "${RBX_LEGACY_PLAYER_RESOURCE_DIR}/launcher"
    COMMAND "${CMAKE_COMMAND}" -E make_directory
        "${RBX_LEGACY_PLAYER_RESOURCE_DIR}/places"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_SOURCE_DIR}/apps/studio/source/content/FirstPlaceBasePlate.rbxl"
        "${RBX_LEGACY_PLAYER_RESOURCE_DIR}/places/Baseplate.rbxl"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${CMAKE_BINARY_DIR}/generated/shaders"
        "${RBX_LEGACY_PLAYER_RESOURCE_DIR}/shaders"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${CMAKE_SOURCE_DIR}/shaders/manifests"
        "${RBX_LEGACY_PLAYER_RESOURCE_DIR}/shader-manifests"
    COMMAND "${CMAKE_COMMAND}" -E copy
        "${RBX_RESOURCE_MANIFEST}"
        "${RBX_LEGACY_PLAYER_RESOURCE_DIR}/resource-manifest.json"
    COMMAND "${CMAKE_COMMAND}" -E copy
        "${CMAKE_SOURCE_DIR}/THIRD_PARTY_NOTICES.md"
        "${RBX_LEGACY_PLAYER_RESOURCE_DIR}/THIRD_PARTY_NOTICES.md"
    COMMENT "Packaging the isolated legacy UI and avatar test player")

if(RBX_PLAYER_CJK_FONT)
    add_custom_command(TARGET RobloxPlayer POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${RBX_PLAYER_RESOURCE_DIR}/PlatformContent/pc/fonts"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${RBX_PLAYER_CJK_FONT}"
            "${RBX_PLAYER_RESOURCE_DIR}/PlatformContent/pc/fonts/NotoSansCJKjp-Regular.otf"
        COMMENT "Packaging the hash-verified current Player CJK fallback font")
    add_custom_command(TARGET RobloxPlayerLegacy POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${RBX_LEGACY_PLAYER_RESOURCE_DIR}/PlatformContent/pc/fonts"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${RBX_PLAYER_CJK_FONT}"
            "${RBX_LEGACY_PLAYER_RESOURCE_DIR}/PlatformContent/pc/fonts/NotoSansCJKjp-Regular.otf"
        COMMENT "Packaging the hash-verified current Player CJK fallback font in the legacy test Player")
endif()

if(RBX_UI_OVERLAY_ROOT)
    add_custom_command(TARGET RobloxPlayer POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E rm -rf
            "${RBX_PLAYER_RESOURCE_DIR}/overlays/player-2026"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${RBX_UI_OVERLAY_ROOT}"
            "${RBX_PLAYER_RESOURCE_DIR}/overlays/player-2026"
        COMMENT "Packaging only the scoped, hash-verified in-game Player UI overlay")

    add_custom_command(TARGET RobloxPlayer POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${RBX_PLAYER_RESOURCE_DIR}/models/InExperience"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${RBX_IN_EXPERIENCE_MODEL_ROOT}/InExperience.rbxm"
            "${RBX_PLAYER_RESOURCE_DIR}/models/InExperience/InExperience.rbxm"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${RBX_IN_EXPERIENCE_MODEL_ROOT}/InExperience_checksum"
            "${RBX_PLAYER_RESOURCE_DIR}/models/InExperience/InExperience_checksum"
        COMMENT "Packaging the checksum-verified Player InExperience data-model patch")
endif()

if(RBX_AVATAR_RUNTIME_ROOT)
    add_custom_command(TARGET RobloxPlayer POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E rm -rf
            "${RBX_PLAYER_RESOURCE_DIR}/overlays/avatar-runtime"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${RBX_AVATAR_RUNTIME_ROOT}"
            "${RBX_PLAYER_RESOURCE_DIR}/overlays/avatar-runtime"
        COMMENT "Packaging hash-pinned Studio R15 runtime assets")
    add_custom_command(TARGET RobloxPlayerLegacy POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E rm -rf
            "${RBX_LEGACY_PLAYER_RESOURCE_DIR}/overlays/avatar-runtime"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${RBX_AVATAR_RUNTIME_ROOT}"
            "${RBX_LEGACY_PLAYER_RESOURCE_DIR}/overlays/avatar-runtime"
        COMMENT "Packaging hash-pinned Studio R15 runtime assets in the legacy test Player")
endif()

if(RBX_STUDIO_RUNTIME_ROOT)
    add_custom_command(TARGET RobloxPlayer POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E rm -rf
            "${RBX_PLAYER_RESOURCE_DIR}/overlays/studio-runtime"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${RBX_STUDIO_RUNTIME_ROOT}"
            "${RBX_PLAYER_RESOURCE_DIR}/overlays/studio-runtime"
        COMMENT "Packaging hash-pinned Studio server runtime assets")
    add_custom_command(TARGET RobloxPlayerLegacy POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E rm -rf
            "${RBX_LEGACY_PLAYER_RESOURCE_DIR}/overlays/studio-runtime"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${RBX_STUDIO_RUNTIME_ROOT}"
            "${RBX_LEGACY_PLAYER_RESOURCE_DIR}/overlays/studio-runtime"
        COMMENT "Packaging Studio server runtime assets in the legacy test Player")
endif()

if(RBX_CURRENT_PLAYER_SCRIPTS_MODEL)
    add_custom_command(TARGET RobloxPlayer POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${RBX_PLAYER_RESOURCE_DIR}/content/scripts"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${RBX_CURRENT_PLAYER_SCRIPTS_MODEL}"
            "${RBX_PLAYER_RESOURCE_DIR}/content/scripts/PlayerScripts.rbxmx"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${RBX_CURRENT_PLAYER_SCRIPTS_MANIFEST}"
            "${RBX_PLAYER_RESOURCE_DIR}/content/scripts/PlayerScripts.manifest.json"
        COMMENT "Packaging the hash-recorded current Studio PlayerModule")
    add_custom_command(TARGET RobloxPlayerLegacy POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${RBX_LEGACY_PLAYER_RESOURCE_DIR}/content/scripts"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${RBX_CURRENT_PLAYER_SCRIPTS_MODEL}"
            "${RBX_LEGACY_PLAYER_RESOURCE_DIR}/content/scripts/PlayerScripts.rbxmx"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${RBX_CURRENT_PLAYER_SCRIPTS_MANIFEST}"
            "${RBX_LEGACY_PLAYER_RESOURCE_DIR}/content/scripts/PlayerScripts.manifest.json"
        COMMENT "Packaging current Studio PlayerModule in the legacy UI test Player")
endif()

if(APPLE)
    add_custom_command(TARGET RobloxPlayer POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy
            "${CMAKE_SOURCE_DIR}/docs/modernization/player.spdx.json"
            "${RBX_PLAYER_RESOURCE_DIR}/player.spdx.json"
        COMMAND "${CMAKE_COMMAND}" -E rm -rf
            "${RBX_PLAYER_RESOURCE_DIR}/licenses"
        VERBATIM)
    add_custom_command(TARGET RobloxPlayerLegacy POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy
            "${CMAKE_SOURCE_DIR}/docs/modernization/player.spdx.json"
            "${RBX_LEGACY_PLAYER_RESOURCE_DIR}/player.spdx.json"
        COMMAND "${CMAKE_COMMAND}" -E rm -rf
            "${RBX_LEGACY_PLAYER_RESOURCE_DIR}/licenses"
        VERBATIM)

    function(rbx_package_apple_license component license_file)
        if(NOT EXISTS "${license_file}")
            message(FATAL_ERROR "Missing ${component} license file: ${license_file}")
        endif()
        add_custom_command(TARGET RobloxPlayer POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E make_directory
                "${RBX_PLAYER_RESOURCE_DIR}/licenses/${component}"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${license_file}"
                "${RBX_PLAYER_RESOURCE_DIR}/licenses/${component}/"
            VERBATIM)
        add_custom_command(TARGET RobloxPlayerLegacy POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E make_directory
                "${RBX_LEGACY_PLAYER_RESOURCE_DIR}/licenses/${component}"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${license_file}"
                "${RBX_LEGACY_PLAYER_RESOURCE_DIR}/licenses/${component}/"
            VERBATIM)
    endfunction()

    rbx_package_apple_license(bgfx "${rbx_bgfx_SOURCE_DIR}/LICENSE")
    rbx_package_apple_license(bx "${rbx_bx_SOURCE_DIR}/LICENSE")
    rbx_package_apple_license(bimg "${rbx_bimg_SOURCE_DIR}/LICENSE")
    rbx_package_apple_license(bgfx.cmake "${rbx_bgfx_cmake_SOURCE_DIR}/LICENSE")
    rbx_package_apple_license(miniaudio "${rbx_miniaudio_SOURCE_DIR}/LICENSE")
    rbx_package_apple_license(FFmpeg "${RBX_FFMPEG_ROOT}/share/licenses/ffmpeg/LICENSE.md")
    rbx_package_apple_license(FFmpeg "${RBX_FFMPEG_ROOT}/share/licenses/ffmpeg/COPYING.LGPLv2.1")
    rbx_package_apple_license(GameNetworkingSockets "${rbx_gns_SOURCE_DIR}/LICENSE")
    rbx_package_apple_license(protobuf "${rbx_protobuf_SOURCE_DIR}/LICENSE")
    rbx_package_apple_license(Abseil "${rbx_abseil_SOURCE_DIR}/LICENSE")
    rbx_package_apple_license(OpenSSL "${RBX_OPENSSL_ROOT}/share/licenses/openssl/LICENSE.txt")
    rbx_package_apple_license(FreeType "${rbx_freetype_SOURCE_DIR}/LICENSE.TXT")
    rbx_package_apple_license(FreeType "${rbx_freetype_SOURCE_DIR}/docs/FTL.TXT")
    rbx_package_apple_license(FreeType "${rbx_freetype_SOURCE_DIR}/docs/GPLv2.TXT")
    rbx_package_apple_license(HarfBuzz "${rbx_harfbuzz_SOURCE_DIR}/COPYING")
    rbx_package_apple_license(utf8proc "${rbx_utf8proc_SOURCE_DIR}/LICENSE.md")
    rbx_package_apple_license(SheenBidi "${rbx_sheenbidi_SOURCE_DIR}/LICENSE")
    rbx_package_apple_license(curl "${rbx_curl_SOURCE_DIR}/COPYING")
    rbx_package_apple_license(zstd "${rbx_zstd_SOURCE_DIR}/LICENSE")
    rbx_package_apple_license(Luau "${rbx_luau_SOURCE_DIR}/LICENSE.txt")
    rbx_package_apple_license(Draco "${rbx_draco_SOURCE_DIR}/LICENSE")
    rbx_package_apple_license(Boost "${CMAKE_SOURCE_DIR}/Library/boost/LICENSE_1_0.txt")
    rbx_package_apple_license(LZ4 "${CMAKE_SOURCE_DIR}/third_party/lz4_historical/LICENSE")

    add_custom_command(TARGET RobloxPlayer POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E rm -rf
            "${RBX_PLAYER_RESOURCE_DIR}/sources/FFmpeg"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${RBX_FFMPEG_ROOT}/share/sources/ffmpeg"
            "${RBX_PLAYER_RESOURCE_DIR}/sources/FFmpeg"
        VERBATIM)
    add_custom_command(TARGET RobloxPlayerLegacy POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E rm -rf
            "${RBX_LEGACY_PLAYER_RESOURCE_DIR}/sources/FFmpeg"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${RBX_FFMPEG_ROOT}/share/sources/ffmpeg"
            "${RBX_LEGACY_PLAYER_RESOURCE_DIR}/sources/FFmpeg"
        VERBATIM)
    if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set_property(TARGET RobloxPlayer APPEND PROPERTY LINK_DEPENDS
        "${CMAKE_SOURCE_DIR}/cmake/BundleMacOSRuntimeDependencies.cmake")
    set_property(TARGET RobloxPlayerLegacy APPEND PROPERTY LINK_DEPENDS
        "${CMAKE_SOURCE_DIR}/cmake/BundleMacOSRuntimeDependencies.cmake")
    add_custom_command(TARGET RobloxPlayer POST_BUILD
        COMMAND "${CMAKE_COMMAND}"
            "-DAPP_BUNDLE=$<TARGET_BUNDLE_DIR:RobloxPlayer>"
            "-DSEARCH_DIRECTORIES=${RBX_FFMPEG_ROOT}/lib"
            -P "${CMAKE_SOURCE_DIR}/cmake/BundleMacOSRuntimeDependencies.cmake"
        VERBATIM)

    add_custom_command(TARGET RobloxPlayerLegacy POST_BUILD
        COMMAND "${CMAKE_COMMAND}"
            "-DAPP_BUNDLE=$<TARGET_BUNDLE_DIR:RobloxPlayerLegacy>"
            "-DSEARCH_DIRECTORIES=${RBX_FFMPEG_ROOT}/lib"
            -P "${CMAKE_SOURCE_DIR}/cmake/BundleMacOSRuntimeDependencies.cmake"
        VERBATIM)

    add_custom_command(TARGET RobloxPlayer POST_BUILD
        COMMAND codesign --force --deep --sign -
            "$<TARGET_BUNDLE_DIR:RobloxPlayer>"
        COMMENT "Ad-hoc signing the completed local RobloxPlayer bundle")

    add_custom_command(TARGET RobloxPlayerLegacy POST_BUILD
        COMMAND codesign --force --deep --sign -
            "$<TARGET_BUNDLE_DIR:RobloxPlayerLegacy>"
        COMMENT "Ad-hoc signing the legacy UI and avatar test bundle")

    set(RBX_LOCAL_PLAYER_DIR "$ENV{HOME}/Applications/Novalume")
    add_custom_target(RobloxPlayerLocalInstall
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${RBX_LOCAL_PLAYER_DIR}"
        COMMAND "${CMAKE_COMMAND}" -E rm -rf
            "${RBX_LOCAL_PLAYER_DIR}/RobloxPlayer.app"
        COMMAND /usr/bin/ditto
            "$<TARGET_BUNDLE_DIR:RobloxPlayer>"
            "${RBX_LOCAL_PLAYER_DIR}/RobloxPlayer.app"
        COMMAND "${CMAKE_COMMAND}"
            "-DAPP_BUNDLE=${RBX_LOCAL_PLAYER_DIR}/RobloxPlayer.app"
            "-DMAX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}"
            "-DSOURCE_ROOT=${CMAKE_SOURCE_DIR}"
            "-DBINARY_ROOT=${CMAKE_BINARY_DIR}"
            "-DREJECT_LOCAL_PATHS=$<IF:$<CONFIG:Release>,ON,OFF>"
            -P "${CMAKE_SOURCE_DIR}/cmake/VerifyMacOSBundle.cmake"
        DEPENDS RobloxPlayer
        COMMENT "Installing the self-contained Player outside protected Desktop storage"
        VERBATIM)

    add_custom_target(RobloxPlayerLegacyLocalInstall
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${RBX_LOCAL_PLAYER_DIR}"
        COMMAND "${CMAKE_COMMAND}" -E rm -rf
            "${RBX_LOCAL_PLAYER_DIR}/RobloxPlayerLegacy.app"
        COMMAND /usr/bin/ditto
            "$<TARGET_BUNDLE_DIR:RobloxPlayerLegacy>"
            "${RBX_LOCAL_PLAYER_DIR}/RobloxPlayerLegacy.app"
        COMMAND "${CMAKE_COMMAND}"
            "-DAPP_BUNDLE=${RBX_LOCAL_PLAYER_DIR}/RobloxPlayerLegacy.app"
            "-DMAX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}"
            "-DSOURCE_ROOT=${CMAKE_SOURCE_DIR}"
            "-DBINARY_ROOT=${CMAKE_BINARY_DIR}"
            "-DREJECT_LOCAL_PATHS=$<IF:$<CONFIG:Release>,ON,OFF>"
            -P "${CMAKE_SOURCE_DIR}/cmake/VerifyMacOSBundle.cmake"
        DEPENDS RobloxPlayerLegacy
        COMMENT "Installing the legacy UI test Player outside protected Desktop storage"
        VERBATIM)
    endif()
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    include(GNUInstallDirs)
    install(TARGETS RobloxPlayer
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
    install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Resources/"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/novalume")
    install(FILES linux/roblox-player.desktop
        DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")
    install(FILES linux/roblox-player.xml
        DESTINATION "${CMAKE_INSTALL_DATADIR}/mime/packages")
    install(FILES "${CMAKE_SOURCE_DIR}/content/textures/loading/robloxlogo.png"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps"
        RENAME roblox-player.png)
endif()

if(WIN32)
    if(NOT VCPKG_INSTALLED_DIR OR NOT VCPKG_TARGET_TRIPLET)
        message(FATAL_ERROR
            "Windows Player packaging requires the configured vcpkg install tree and triplet")
    endif()
    set(RBX_PLAYER_RUNTIME_SEARCH_DIRECTORY
        "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin")
    install(TARGETS RobloxPlayer
        RUNTIME_DEPENDENCIES
            DIRECTORIES "${RBX_PLAYER_RUNTIME_SEARCH_DIRECTORY}"
            PRE_EXCLUDE_REGEXES
                "api-ms-.*"
                "ext-ms-.*"
            POST_EXCLUDE_REGEXES
                ".*[Ww]indows[/\\\\][Ss]ystem32[/\\\\].*"
        RUNTIME DESTINATION ".")
    install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/Resources/"
        DESTINATION "Resources")

    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ".")
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_COMPONENT Player)
    include(InstallRequiredSystemLibraries)
endif()

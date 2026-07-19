if(NOT TARGET Roblox::PlatformHost)
    message(FATAL_ERROR "RobloxPlayer needs a host adapter for this platform")
endif()

add_subdirectory(shared/client)

add_executable(RobloxPlayer MACOSX_BUNDLE
    main.cpp
    LocalPlayerThumbnailProvider.cpp
    LocalPlayerThumbnailProvider.h
    PlayerRuntime.cpp
    PlayerRuntime.h
    shared/client_base/RenderSettingsItem.cpp
    shared/client_base/RenderSettingsItem.h)
target_link_libraries(RobloxPlayer PRIVATE
    Roblox::RenderScene Roblox::LuauScripting Roblox::RuntimeUI Roblox::InteractionTools
    Roblox::AudioDataModel Roblox::DataModelRuntime Roblox::RuntimeSupport
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

add_executable(RobloxPlayerLegacy MACOSX_BUNDLE
    main.cpp
    LocalPlayerThumbnailProvider.cpp
    LocalPlayerThumbnailProvider.h
    PlayerRuntime.cpp
    PlayerRuntime.h
    shared/client_base/RenderSettingsItem.cpp
    shared/client_base/RenderSettingsItem.h)
target_link_libraries(RobloxPlayerLegacy PRIVATE
    Roblox::RenderScene Roblox::LuauScripting Roblox::RuntimeUI Roblox::InteractionTools
    Roblox::AudioDataModel Roblox::DataModelRuntime Roblox::RuntimeSupport
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

set_target_properties(RobloxPlayer PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
    MACOSX_BUNDLE_BUNDLE_NAME "Roblox"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.roblox.offline-player"
    MACOSX_BUNDLE_INFO_STRING "Shared cross-platform Roblox player"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1.0"
    MACOSX_BUNDLE_BUNDLE_VERSION "1")

set_target_properties(RobloxPlayerLegacy PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
    MACOSX_BUNDLE_BUNDLE_NAME "Roblox Legacy UI Test"
    MACOSX_BUNDLE_GUI_IDENTIFIER "dev.q8j.novalume.legacy-ui-test"
    MACOSX_BUNDLE_INFO_STRING "Novalume legacy UI and avatar test player"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1.0"
    MACOSX_BUNDLE_BUNDLE_VERSION "1")

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
if(RBX_STUDIO_REFERENCE_ROOT)
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
        "Supplied Studio path is not configured; authentic R15 assets will not be packaged")
endif()

if(RBX_PLAYER_REFERENCE_ROOT AND RBX_APK_REFERENCE_PATH)
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
        "Supplied Player/APK paths are not configured; authentic in-game UI overlay will not be packaged")
endif()

add_dependencies(RobloxPlayer rbx-player-shaders rbx-resource-manifest)
add_dependencies(RobloxPlayerLegacy rbx-player-shaders rbx-resource-manifest)
set_property(TARGET RobloxPlayer APPEND PROPERTY LINK_DEPENDS
    "${CMAKE_BINARY_DIR}/generated/shaders/shaders_bgfx-glsl.pack"
    "${CMAKE_BINARY_DIR}/generated/shaders/shaders_bgfx-metal.pack"
    "${CMAKE_BINARY_DIR}/generated/shaders/shaders.json")
set_property(TARGET RobloxPlayerLegacy APPEND PROPERTY LINK_DEPENDS
    "${CMAKE_BINARY_DIR}/generated/shaders/shaders_bgfx-glsl.pack"
    "${CMAKE_BINARY_DIR}/generated/shaders/shaders_bgfx-metal.pack"
    "${CMAKE_BINARY_DIR}/generated/shaders/shaders.json")

add_custom_command(TARGET RobloxPlayer POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${CMAKE_SOURCE_DIR}/content"
        "$<TARGET_BUNDLE_CONTENT_DIR:RobloxPlayer>/Resources/content"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${CMAKE_SOURCE_DIR}/PlatformContent/pc"
        "$<TARGET_BUNDLE_CONTENT_DIR:RobloxPlayer>/Resources/PlatformContent/pc"
    COMMAND "${CMAKE_COMMAND}" -E make_directory
        "$<TARGET_BUNDLE_CONTENT_DIR:RobloxPlayer>/Resources/places"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_SOURCE_DIR}/apps/studio/source/content/FirstPlaceBasePlate.rbxl"
        "$<TARGET_BUNDLE_CONTENT_DIR:RobloxPlayer>/Resources/places/Baseplate.rbxl"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${CMAKE_BINARY_DIR}/generated/shaders"
        "$<TARGET_BUNDLE_CONTENT_DIR:RobloxPlayer>/Resources/shaders"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${CMAKE_SOURCE_DIR}/shaders/manifests"
        "$<TARGET_BUNDLE_CONTENT_DIR:RobloxPlayer>/Resources/shader-manifests"
    COMMAND "${CMAKE_COMMAND}" -E copy
        "${RBX_RESOURCE_MANIFEST}"
        "$<TARGET_BUNDLE_CONTENT_DIR:RobloxPlayer>/Resources/resource-manifest.json"
    COMMAND "${CMAKE_COMMAND}" -E copy
        "${CMAKE_SOURCE_DIR}/THIRD_PARTY_NOTICES.md"
        "$<TARGET_BUNDLE_CONTENT_DIR:RobloxPlayer>/Resources/THIRD_PARTY_NOTICES.md"
    COMMAND "${CMAKE_COMMAND}" -E copy
        "${CMAKE_SOURCE_DIR}/docs/modernization/player.spdx.json"
        "$<TARGET_BUNDLE_CONTENT_DIR:RobloxPlayer>/Resources/player.spdx.json"
    COMMENT "Packaging owned shaders and source-tree Roblox resources")

add_custom_command(TARGET RobloxPlayerLegacy POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${CMAKE_SOURCE_DIR}/content"
        "$<TARGET_BUNDLE_CONTENT_DIR:RobloxPlayerLegacy>/Resources/content"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${CMAKE_SOURCE_DIR}/PlatformContent/pc"
        "$<TARGET_BUNDLE_CONTENT_DIR:RobloxPlayerLegacy>/Resources/PlatformContent/pc"
    COMMAND "${CMAKE_COMMAND}" -E make_directory
        "$<TARGET_BUNDLE_CONTENT_DIR:RobloxPlayerLegacy>/Resources/places"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_SOURCE_DIR}/apps/studio/source/content/FirstPlaceBasePlate.rbxl"
        "$<TARGET_BUNDLE_CONTENT_DIR:RobloxPlayerLegacy>/Resources/places/Baseplate.rbxl"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${CMAKE_BINARY_DIR}/generated/shaders"
        "$<TARGET_BUNDLE_CONTENT_DIR:RobloxPlayerLegacy>/Resources/shaders"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${CMAKE_SOURCE_DIR}/shaders/manifests"
        "$<TARGET_BUNDLE_CONTENT_DIR:RobloxPlayerLegacy>/Resources/shader-manifests"
    COMMAND "${CMAKE_COMMAND}" -E copy
        "${RBX_RESOURCE_MANIFEST}"
        "$<TARGET_BUNDLE_CONTENT_DIR:RobloxPlayerLegacy>/Resources/resource-manifest.json"
    COMMAND "${CMAKE_COMMAND}" -E copy
        "${CMAKE_SOURCE_DIR}/THIRD_PARTY_NOTICES.md"
        "$<TARGET_BUNDLE_CONTENT_DIR:RobloxPlayerLegacy>/Resources/THIRD_PARTY_NOTICES.md"
    COMMAND "${CMAKE_COMMAND}" -E copy
        "${CMAKE_SOURCE_DIR}/docs/modernization/player.spdx.json"
        "$<TARGET_BUNDLE_CONTENT_DIR:RobloxPlayerLegacy>/Resources/player.spdx.json"
    COMMENT "Packaging the isolated legacy UI and avatar test player")

if(RBX_PLAYER_CJK_FONT)
    add_custom_command(TARGET RobloxPlayer POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "$<TARGET_BUNDLE_CONTENT_DIR:RobloxPlayer>/Resources/PlatformContent/pc/fonts"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${RBX_PLAYER_CJK_FONT}"
            "$<TARGET_BUNDLE_CONTENT_DIR:RobloxPlayer>/Resources/PlatformContent/pc/fonts/NotoSansCJKjp-Regular.otf"
        COMMENT "Packaging the hash-verified current Player CJK fallback font")
    add_custom_command(TARGET RobloxPlayerLegacy POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "$<TARGET_BUNDLE_CONTENT_DIR:RobloxPlayerLegacy>/Resources/PlatformContent/pc/fonts"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${RBX_PLAYER_CJK_FONT}"
            "$<TARGET_BUNDLE_CONTENT_DIR:RobloxPlayerLegacy>/Resources/PlatformContent/pc/fonts/NotoSansCJKjp-Regular.otf"
        COMMENT "Packaging the hash-verified current Player CJK fallback font in the legacy test Player")
endif()

if(RBX_UI_OVERLAY_ROOT)
    add_custom_command(TARGET RobloxPlayer POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E rm -rf
            "$<TARGET_BUNDLE_CONTENT_DIR:RobloxPlayer>/Resources/overlays/player-2026"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${RBX_UI_OVERLAY_ROOT}"
            "$<TARGET_BUNDLE_CONTENT_DIR:RobloxPlayer>/Resources/overlays/player-2026"
        COMMENT "Packaging only the scoped, hash-verified in-game Player UI overlay")

    add_custom_command(TARGET RobloxPlayer POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "$<TARGET_BUNDLE_CONTENT_DIR:RobloxPlayer>/Resources/models/InExperience"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${RBX_IN_EXPERIENCE_MODEL_ROOT}/InExperience.rbxm"
            "$<TARGET_BUNDLE_CONTENT_DIR:RobloxPlayer>/Resources/models/InExperience/InExperience.rbxm"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${RBX_IN_EXPERIENCE_MODEL_ROOT}/InExperience_checksum"
            "$<TARGET_BUNDLE_CONTENT_DIR:RobloxPlayer>/Resources/models/InExperience/InExperience_checksum"
        COMMENT "Packaging the checksum-verified Player InExperience data-model patch")
endif()

if(RBX_AVATAR_RUNTIME_ROOT)
    add_custom_command(TARGET RobloxPlayer POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E rm -rf
            "$<TARGET_BUNDLE_CONTENT_DIR:RobloxPlayer>/Resources/overlays/avatar-runtime"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${RBX_AVATAR_RUNTIME_ROOT}"
            "$<TARGET_BUNDLE_CONTENT_DIR:RobloxPlayer>/Resources/overlays/avatar-runtime"
        COMMENT "Packaging hash-pinned Studio R15 runtime assets")
    add_custom_command(TARGET RobloxPlayerLegacy POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E rm -rf
            "$<TARGET_BUNDLE_CONTENT_DIR:RobloxPlayerLegacy>/Resources/overlays/avatar-runtime"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${RBX_AVATAR_RUNTIME_ROOT}"
            "$<TARGET_BUNDLE_CONTENT_DIR:RobloxPlayerLegacy>/Resources/overlays/avatar-runtime"
        COMMENT "Packaging hash-pinned Studio R15 runtime assets in the legacy test Player")
endif()

if(RBX_STUDIO_RUNTIME_ROOT)
    add_custom_command(TARGET RobloxPlayer POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E rm -rf
            "$<TARGET_BUNDLE_CONTENT_DIR:RobloxPlayer>/Resources/overlays/studio-runtime"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${RBX_STUDIO_RUNTIME_ROOT}"
            "$<TARGET_BUNDLE_CONTENT_DIR:RobloxPlayer>/Resources/overlays/studio-runtime"
        COMMENT "Packaging hash-pinned Studio server runtime assets")
    add_custom_command(TARGET RobloxPlayerLegacy POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E rm -rf
            "$<TARGET_BUNDLE_CONTENT_DIR:RobloxPlayerLegacy>/Resources/overlays/studio-runtime"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${RBX_STUDIO_RUNTIME_ROOT}"
            "$<TARGET_BUNDLE_CONTENT_DIR:RobloxPlayerLegacy>/Resources/overlays/studio-runtime"
        COMMENT "Packaging Studio server runtime assets in the legacy test Player")
endif()

if(RBX_CURRENT_PLAYER_SCRIPTS_MODEL)
    add_custom_command(TARGET RobloxPlayer POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "$<TARGET_BUNDLE_CONTENT_DIR:RobloxPlayer>/Resources/content/scripts"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${RBX_CURRENT_PLAYER_SCRIPTS_MODEL}"
            "$<TARGET_BUNDLE_CONTENT_DIR:RobloxPlayer>/Resources/content/scripts/PlayerScripts.rbxmx"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${RBX_CURRENT_PLAYER_SCRIPTS_MANIFEST}"
            "$<TARGET_BUNDLE_CONTENT_DIR:RobloxPlayer>/Resources/content/scripts/PlayerScripts.manifest.json"
        COMMENT "Packaging the hash-recorded current Studio PlayerModule")
    add_custom_command(TARGET RobloxPlayerLegacy POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "$<TARGET_BUNDLE_CONTENT_DIR:RobloxPlayerLegacy>/Resources/content/scripts"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${RBX_CURRENT_PLAYER_SCRIPTS_MODEL}"
            "$<TARGET_BUNDLE_CONTENT_DIR:RobloxPlayerLegacy>/Resources/content/scripts/PlayerScripts.rbxmx"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${RBX_CURRENT_PLAYER_SCRIPTS_MANIFEST}"
            "$<TARGET_BUNDLE_CONTENT_DIR:RobloxPlayerLegacy>/Resources/content/scripts/PlayerScripts.manifest.json"
        COMMENT "Packaging current Studio PlayerModule in the legacy UI test Player")
endif()

if(APPLE)
    add_custom_command(TARGET RobloxPlayer POST_BUILD
        COMMAND codesign --force --deep --sign -
            "$<TARGET_BUNDLE_DIR:RobloxPlayer>"
        COMMENT "Ad-hoc signing the completed local RobloxPlayer bundle")

    add_custom_command(TARGET RobloxPlayerLegacy POST_BUILD
        COMMAND codesign --force --deep --sign -
            "$<TARGET_BUNDLE_DIR:RobloxPlayerLegacy>"
        COMMENT "Ad-hoc signing the legacy UI and avatar test bundle")

    # Interactive builds live under a Desktop workspace, which macOS protects
    # with a per-signature privacy prompt. Keep the build artifact intact and
    # provide a runnable copy whose packaged resources do not require Desktop
    # access. CI and headless verification continue to use the build artifact.
    set(RBX_LOCAL_PLAYER_DIR "$ENV{HOME}/Applications/Novalume")
    add_custom_target(RobloxPlayerLocalInstall
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${RBX_LOCAL_PLAYER_DIR}"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "$<TARGET_BUNDLE_DIR:RobloxPlayer>"
            "${RBX_LOCAL_PLAYER_DIR}/RobloxPlayer.app"
        DEPENDS RobloxPlayer
        COMMENT "Installing the self-contained Player outside protected Desktop storage"
        VERBATIM)

    add_custom_target(RobloxPlayerLegacyLocalInstall
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${RBX_LOCAL_PLAYER_DIR}"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "$<TARGET_BUNDLE_DIR:RobloxPlayerLegacy>"
            "${RBX_LOCAL_PLAYER_DIR}/RobloxPlayerLegacy.app"
        DEPENDS RobloxPlayerLegacy
        COMMENT "Installing the legacy UI test Player outside protected Desktop storage"
        VERBATIM)
endif()

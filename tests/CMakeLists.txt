add_executable(rbx-gfxcore-contract-tests unit/GfxCoreContractTests.cpp)
target_link_libraries(rbx-gfxcore-contract-tests PRIVATE Roblox::GfxCoreContract)
rbx_enable_warnings(rbx-gfxcore-contract-tests)
add_test(NAME gfxcore-contract COMMAND rbx-gfxcore-contract-tests)

add_executable(rbx-thumbnail-texture-provider-tests
    unit/ThumbnailTextureProviderTests.cpp)
target_link_libraries(rbx-thumbnail-texture-provider-tests PRIVATE
    Roblox::RenderFoundation)
rbx_enable_warnings(rbx-thumbnail-texture-provider-tests)
target_compile_options(rbx-thumbnail-texture-provider-tests PRIVATE
    $<$<COMPILE_LANG_AND_ID:CXX,AppleClang>:-Wno-enum-constexpr-conversion>)
add_test(NAME thumbnail-texture-provider-contract
    COMMAND rbx-thumbnail-texture-provider-tests)

add_executable(rbx-text-shaping-tests unit/TextShapingTests.cpp)
target_include_directories(rbx-text-shaping-tests PRIVATE
    "${CMAKE_SOURCE_DIR}/engine/rendering/materials/include/GfxRender")
target_link_libraries(rbx-text-shaping-tests PRIVATE
    Roblox::RenderText Roblox::RenderMaterials
    Roblox::LuauScripting Roblox::RuntimeUI Roblox::InteractionTools
    Roblox::AudioDataModel Roblox::DataModelRuntime Roblox::RuntimeSupport
    Roblox::AvatarHumanoid Roblox::NetworkingRuntime Roblox::TerrainRuntime
    Roblox::PlayerShared Roblox::PlatformRuntime Roblox::PlatformHost
    Roblox::RenderScene)
rbx_enable_warnings(rbx-text-shaping-tests)
target_compile_options(rbx-text-shaping-tests PRIVATE
    $<$<COMPILE_LANG_AND_ID:CXX,AppleClang>:-Wno-enum-constexpr-conversion>)
set(RBX_TEXT_SHAPING_FALLBACK_ARGS)
if(RBX_PLAYER_REFERENCE_ROOT AND EXISTS
    "${RBX_PLAYER_REFERENCE_ROOT}/content/fonts/NotoNaskhArabicUI-Regular.ttf" AND EXISTS
    "${RBX_PLAYER_REFERENCE_ROOT}/content/fonts/NotoSansDevanagariUI-Regular.ttf")
    list(APPEND RBX_TEXT_SHAPING_FALLBACK_ARGS
        "${RBX_PLAYER_REFERENCE_ROOT}/content/fonts/NotoNaskhArabicUI-Regular.ttf"
        "${RBX_PLAYER_REFERENCE_ROOT}/content/fonts/NotoSansDevanagariUI-Regular.ttf")
endif()
add_test(NAME text-shaping-contract
    COMMAND rbx-text-shaping-tests
        "${CMAKE_SOURCE_DIR}/content/fonts/SourceSansPro-Regular.ttf"
        ${RBX_TEXT_SHAPING_FALLBACK_ARGS})
set_tests_properties(text-shaping-contract PROPERTIES LABELS "rendering;text;unicode")

add_executable(rbx-file-mesh-data-tests unit/FileMeshDataTests.cpp)
target_include_directories(rbx-file-mesh-data-tests PRIVATE
    "${CMAKE_SOURCE_DIR}/engine/datamodel/runtime/include")
target_link_libraries(rbx-file-mesh-data-tests PRIVATE Roblox::RenderFoundation)
rbx_enable_warnings(rbx-file-mesh-data-tests)
target_compile_options(rbx-file-mesh-data-tests PRIVATE
    $<$<COMPILE_LANG_AND_ID:CXX,AppleClang>:-Wno-enum-constexpr-conversion>)
add_test(NAME file-mesh-data-contract COMMAND rbx-file-mesh-data-tests)
set_tests_properties(file-mesh-data-contract PROPERTIES
    LABELS "rendering;avatar;r15;assets")

add_executable(rbx-fastlog-contract-tests unit/FastLogTests.cpp)
target_link_libraries(rbx-fastlog-contract-tests PRIVATE Roblox::Core)
rbx_enable_warnings(rbx-fastlog-contract-tests)
add_test(NAME fastlog-contract COMMAND rbx-fastlog-contract-tests)

add_executable(rbx-asset-mount-tests unit/AssetMountTableTests.cpp)
target_link_libraries(rbx-asset-mount-tests PRIVATE Roblox::Assets)
rbx_enable_warnings(rbx-asset-mount-tests)
add_test(NAME asset-mount-contract COMMAND rbx-asset-mount-tests)

add_executable(rbx-place-package-tests unit/PlacePackageTests.cpp)
target_link_libraries(rbx-place-package-tests PRIVATE Roblox::Assets)
rbx_enable_warnings(rbx-place-package-tests)
add_test(NAME place-package-contract COMMAND rbx-place-package-tests)

add_executable(rbx-screen-layout-tests unit/ScreenLayoutTests.cpp)
target_link_libraries(rbx-screen-layout-tests PRIVATE Roblox::UILayout)
rbx_enable_warnings(rbx-screen-layout-tests)
add_test(NAME screen-layout-contract COMMAND rbx-screen-layout-tests)

add_executable(rbx-preferred-input-tests unit/PreferredInputTests.cpp)
target_link_libraries(rbx-preferred-input-tests PRIVATE Roblox::UIInput)
rbx_enable_warnings(rbx-preferred-input-tests)
add_test(NAME preferred-input-contract COMMAND rbx-preferred-input-tests)

add_executable(rbx-luau-runtime-tests unit/LuauRuntimeTests.cpp)
target_link_libraries(rbx-luau-runtime-tests PRIVATE Roblox::LuauRuntime)
rbx_enable_warnings(rbx-luau-runtime-tests)
add_test(NAME luau-runtime-contract COMMAND rbx-luau-runtime-tests)
set_tests_properties(luau-runtime-contract PROPERTIES LABELS "scripting;luau;security")

add_executable(rbx-ui-component-tests unit/UIComponentTests.cpp)
target_include_directories(rbx-ui-component-tests PRIVATE
    "${CMAKE_SOURCE_DIR}/Library/SDL2/include")
target_link_libraries(rbx-ui-component-tests PRIVATE
    Roblox::LuauScripting Roblox::RuntimeUI Roblox::InteractionTools
    Roblox::AudioDataModel Roblox::DataModelRuntime Roblox::RuntimeSupport
    Roblox::AvatarHumanoid Roblox::NetworkingRuntime Roblox::TerrainRuntime
    Roblox::PlayerShared Roblox::PlatformRuntime Roblox::PlatformHost
    Roblox::RenderScene)
target_compile_definitions(rbx-ui-component-tests PRIVATE
    _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR=1)
target_compile_options(rbx-ui-component-tests PRIVATE
    $<$<COMPILE_LANG_AND_ID:CXX,AppleClang>:-Wno-enum-constexpr-conversion>)
add_test(NAME ui-component-contract COMMAND rbx-ui-component-tests)
set_tests_properties(ui-component-contract PROPERTIES RUN_SERIAL TRUE)

add_executable(rbx-animation-pose-tests unit/AnimationPoseTests.cpp)
target_include_directories(rbx-animation-pose-tests PRIVATE
    "${CMAKE_SOURCE_DIR}/Library/SDL2/include"
    "${CMAKE_SOURCE_DIR}/App/include"
    "${CMAKE_SOURCE_DIR}/engine/datamodel/runtime/include")
target_link_libraries(rbx-animation-pose-tests PRIVATE
    Roblox::Animation Roblox::LuauScripting Roblox::RuntimeUI
    Roblox::InteractionTools Roblox::AudioDataModel Roblox::DataModelRuntime
    Roblox::RuntimeSupport Roblox::AvatarHumanoid Roblox::NetworkingRuntime
    Roblox::TerrainRuntime Roblox::PlayerShared Roblox::Assets
    Roblox::PlatformRuntime Roblox::PlatformHost)
target_compile_definitions(rbx-animation-pose-tests PRIVATE
    _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR=1
    _LIBCPP_ENABLE_CXX17_REMOVED_UNARY_BINARY_FUNCTION=1)
target_compile_options(rbx-animation-pose-tests PRIVATE
    $<$<COMPILE_LANG_AND_ID:CXX,AppleClang>:-Wno-enum-constexpr-conversion>)
add_test(NAME animation-pose-contract COMMAND rbx-animation-pose-tests)
set_tests_properties(animation-pose-contract PROPERTIES
    RUN_SERIAL TRUE
    LABELS "animation;avatar;r6;r15")

if(RBX_PLAYER_REFERENCE_ROOT AND EXISTS
    "${RBX_PLAYER_REFERENCE_ROOT}/ExtraContent/models/InExperience/InExperience.rbxm")
    find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
    set(RBX_IN_EXPERIENCE_MODEL
        "${RBX_PLAYER_REFERENCE_ROOT}/ExtraContent/models/InExperience/InExperience.rbxm")
    add_executable(rbx-in-experience-model-load-tests
        integration/InExperienceModelLoadTests.cpp)
    target_include_directories(rbx-in-experience-model-load-tests PRIVATE
        "${CMAKE_SOURCE_DIR}/Library/SDL2/include")
    target_link_libraries(rbx-in-experience-model-load-tests PRIVATE
        Roblox::LuauScripting Roblox::RuntimeUI Roblox::InteractionTools
        Roblox::AudioDataModel Roblox::DataModelRuntime Roblox::RuntimeSupport
        Roblox::AvatarHumanoid Roblox::NetworkingRuntime Roblox::TerrainRuntime
        Roblox::PlayerShared Roblox::PlatformRuntime Roblox::PlatformHost)
    target_compile_definitions(rbx-in-experience-model-load-tests PRIVATE
        _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR=1)
    target_compile_options(rbx-in-experience-model-load-tests PRIVATE
        $<$<COMPILE_LANG_AND_ID:CXX,AppleClang>:-Wno-enum-constexpr-conversion>)
    add_test(NAME in-experience-model-load
        COMMAND rbx-in-experience-model-load-tests
            "${RBX_PLAYER_REFERENCE_ROOT}/ExtraContent/models/InExperience/InExperience.rbxm")
    set_tests_properties(in-experience-model-load PROPERTIES
        RUN_SERIAL TRUE
        LABELS "ui;reverse-engineering;serialization")

    add_executable(rbx-data-model-patch-tests
        integration/DataModelPatchTests.cpp)
    target_include_directories(rbx-data-model-patch-tests PRIVATE
        "${CMAKE_SOURCE_DIR}/Library/SDL2/include")
    target_link_libraries(rbx-data-model-patch-tests PRIVATE
        Roblox::LuauScripting Roblox::RuntimeUI Roblox::InteractionTools
        Roblox::AudioDataModel Roblox::DataModelRuntime Roblox::RuntimeSupport
        Roblox::AvatarHumanoid Roblox::NetworkingRuntime Roblox::TerrainRuntime
        Roblox::PlayerShared Roblox::PlatformRuntime Roblox::PlatformHost)
    target_compile_definitions(rbx-data-model-patch-tests PRIVATE
        _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR=1)
    target_compile_options(rbx-data-model-patch-tests PRIVATE
        $<$<COMPILE_LANG_AND_ID:CXX,AppleClang>:-Wno-enum-constexpr-conversion>)
    add_test(NAME data-model-patch-contract
        COMMAND rbx-data-model-patch-tests
            "${RBX_IN_EXPERIENCE_MODEL}"
            "${RBX_PLAYER_REFERENCE_ROOT}/ExtraContent/models/InExperience/InExperience_checksum")
    set_tests_properties(data-model-patch-contract PROPERTIES
        RUN_SERIAL TRUE
        LABELS "ui;reverse-engineering;serialization;security")

    set(RBX_LUAU_FIXTURE_DIR "${CMAKE_BINARY_DIR}/test-output/in-experience-luau")
    set(RBX_LUAU_SIGNED_FIXTURE "${RBX_LUAU_FIXTURE_DIR}/Constants.signed")
    set(RBX_LUAU_DECODED_FIXTURE "${RBX_LUAU_FIXTURE_DIR}/Constants.lua")
    set(RBX_LUAU_MANIFEST "${RBX_LUAU_FIXTURE_DIR}/manifest.json")
    set(RBX_PLAYERLIST_LUAU_FIXTURE_DIR
        "${CMAKE_BINARY_DIR}/test-output/in-experience-playerlist-luau")
    set(RBX_PLAYERLIST_LUAU_SIGNED_FIXTURE
        "${RBX_PLAYERLIST_LUAU_FIXTURE_DIR}/Constants.signed")
    set(RBX_PLAYERLIST_LUAU_DECODED_FIXTURE
        "${RBX_PLAYERLIST_LUAU_FIXTURE_DIR}/Constants.lua")
    set(RBX_PLAYERLIST_LUAU_MANIFEST
        "${RBX_PLAYERLIST_LUAU_FIXTURE_DIR}/manifest.json")
    add_custom_command(
        OUTPUT "${RBX_LUAU_SIGNED_FIXTURE}" "${RBX_LUAU_DECODED_FIXTURE}"
            "${RBX_LUAU_MANIFEST}"
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_SOURCE_DIR}/tools/fixture_inspector/extract_in_experience_bytecode.py"
            --model "${RBX_IN_EXPERIENCE_MODEL}"
            --prefix "PatchRoot/DataModelInstances/CoreGui/RobloxGui/Modules/Chrome/ChromeShared/Unibar"
            --module "PatchRoot/DataModelInstances/CoreGui/RobloxGui/Modules/Chrome/ChromeShared/Unibar/Constants"
            --include-signed
            --output "${RBX_LUAU_FIXTURE_DIR}"
        DEPENDS "${RBX_IN_EXPERIENCE_MODEL}"
            "${CMAKE_SOURCE_DIR}/tools/fixture_inspector/extract_in_experience_bytecode.py"
            "${CMAKE_SOURCE_DIR}/tools/fixture_inspector/in_experience_contracts.py"
        VERBATIM)
    add_custom_target(rbx-in-experience-luau-fixture DEPENDS
        "${RBX_LUAU_SIGNED_FIXTURE}" "${RBX_LUAU_DECODED_FIXTURE}"
        "${RBX_LUAU_MANIFEST}")
    add_custom_command(
        OUTPUT "${RBX_PLAYERLIST_LUAU_SIGNED_FIXTURE}"
            "${RBX_PLAYERLIST_LUAU_DECODED_FIXTURE}"
            "${RBX_PLAYERLIST_LUAU_MANIFEST}"
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_SOURCE_DIR}/tools/fixture_inspector/extract_in_experience_bytecode.py"
            --model "${RBX_IN_EXPERIENCE_MODEL}"
            --prefix "PatchRoot/DataModelInstances/CorePackages/Workspace/Packages/_Workspace/PlayerList/PlayerList/Common"
            --module "PatchRoot/DataModelInstances/CorePackages/Workspace/Packages/_Workspace/PlayerList/PlayerList/Common/Constants"
            --include-signed
            --output "${RBX_PLAYERLIST_LUAU_FIXTURE_DIR}"
        DEPENDS "${RBX_IN_EXPERIENCE_MODEL}"
            "${CMAKE_SOURCE_DIR}/tools/fixture_inspector/extract_in_experience_bytecode.py"
            "${CMAKE_SOURCE_DIR}/tools/fixture_inspector/in_experience_contracts.py"
        VERBATIM)
    add_custom_target(rbx-in-experience-playerlist-luau-fixture DEPENDS
        "${RBX_PLAYERLIST_LUAU_SIGNED_FIXTURE}"
        "${RBX_PLAYERLIST_LUAU_DECODED_FIXTURE}"
        "${RBX_PLAYERLIST_LUAU_MANIFEST}")
    add_executable(rbx-in-experience-luau-bytecode-tests
        integration/InExperienceLuauBytecodeTests.cpp)
    target_link_libraries(rbx-in-experience-luau-bytecode-tests PRIVATE
        Roblox::LuauRuntime)
    add_dependencies(rbx-in-experience-luau-bytecode-tests
        rbx-in-experience-luau-fixture
        rbx-in-experience-playerlist-luau-fixture)
    rbx_enable_warnings(rbx-in-experience-luau-bytecode-tests)
    add_test(NAME in-experience-luau-bytecode
        COMMAND rbx-in-experience-luau-bytecode-tests
            "${RBX_LUAU_SIGNED_FIXTURE}" "${RBX_LUAU_DECODED_FIXTURE}"
            "${RBX_PLAYERLIST_LUAU_SIGNED_FIXTURE}"
            "${RBX_PLAYERLIST_LUAU_DECODED_FIXTURE}")
    set_tests_properties(in-experience-luau-bytecode PROPERTIES
        LABELS "ui;reverse-engineering;scripting;luau")
endif()

if(RBX_STUDIO_REFERENCE_ROOT AND EXISTS
    "${RBX_STUDIO_REFERENCE_ROOT}/content/avatar/characterR15.rbxm")
    add_executable(rbx-avatar-runtime-model-tests
        integration/AvatarRuntimeModelTests.cpp)
    target_include_directories(rbx-avatar-runtime-model-tests PRIVATE
        "${CMAKE_SOURCE_DIR}/Library/SDL2/include")
    target_link_libraries(rbx-avatar-runtime-model-tests PRIVATE
        Roblox::LuauScripting Roblox::RuntimeUI Roblox::InteractionTools
        Roblox::AudioDataModel Roblox::DataModelRuntime Roblox::RuntimeSupport
        Roblox::AvatarHumanoid Roblox::NetworkingRuntime Roblox::TerrainRuntime
        Roblox::PlayerShared Roblox::PlatformRuntime Roblox::PlatformHost)
    target_compile_definitions(rbx-avatar-runtime-model-tests PRIVATE
        _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR=1)
    target_compile_options(rbx-avatar-runtime-model-tests PRIVATE
        $<$<COMPILE_LANG_AND_ID:CXX,AppleClang>:-Wno-enum-constexpr-conversion>)
    add_test(NAME avatar-runtime-model-contract
        COMMAND rbx-avatar-runtime-model-tests "${RBX_STUDIO_REFERENCE_ROOT}")
    set_tests_properties(avatar-runtime-model-contract PROPERTIES
        RUN_SERIAL TRUE
        LABELS "avatar;r15;animation;serialization;reverse-engineering")
endif()

set(RBX_SELECTED_MODERN_PLACE_ROOT "${CMAKE_SOURCE_DIR}/../2026-place-files")
set(RBX_SELECTED_MODERN_PLACE_NAME "Backrooms Game - Fully Scripted - FREE")
if(EXISTS "${RBX_SELECTED_MODERN_PLACE_ROOT}/${RBX_SELECTED_MODERN_PLACE_NAME}.rbxl" AND
   EXISTS "${RBX_SELECTED_MODERN_PLACE_ROOT}/${RBX_SELECTED_MODERN_PLACE_NAME}.rbxlx")
    add_executable(rbx-modern-place-serialization-tests
        integration/ModernPlaceSerializationTests.cpp)
    target_include_directories(rbx-modern-place-serialization-tests PRIVATE
        "${CMAKE_SOURCE_DIR}/Library/SDL2/include")
    target_link_libraries(rbx-modern-place-serialization-tests PRIVATE
        Roblox::LuauScripting Roblox::RuntimeUI Roblox::InteractionTools
        Roblox::AudioDataModel Roblox::DataModelRuntime Roblox::RuntimeSupport
        Roblox::AvatarHumanoid Roblox::NetworkingRuntime Roblox::TerrainRuntime
        Roblox::PlayerShared Roblox::PlatformRuntime Roblox::PlatformHost)
    target_compile_definitions(rbx-modern-place-serialization-tests PRIVATE
        _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR=1)
    target_compile_options(rbx-modern-place-serialization-tests PRIVATE
        $<$<COMPILE_LANG_AND_ID:CXX,AppleClang>:-Wno-enum-constexpr-conversion>)
    add_test(NAME modern-place-serialization-contract
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_SOURCE_DIR}/tests/integration/test_modern_place_pair.py"
            "$<TARGET_FILE:rbx-modern-place-serialization-tests>"
            "${RBX_SELECTED_MODERN_PLACE_ROOT}/${RBX_SELECTED_MODERN_PLACE_NAME}.rbxl"
            "${RBX_SELECTED_MODERN_PLACE_ROOT}/${RBX_SELECTED_MODERN_PLACE_NAME}.rbxlx")
    set_tests_properties(modern-place-serialization-contract PROPERTIES
        RUN_SERIAL TRUE
        LABELS "serialization;compatibility;2026-place")
endif()

add_executable(rbx-audio-runtime-tests unit/AudioEngineTests.cpp)
target_link_libraries(rbx-audio-runtime-tests PRIVATE Roblox::AudioRuntime)
rbx_enable_warnings(rbx-audio-runtime-tests)
add_test(NAME audio-runtime-contract COMMAND rbx-audio-runtime-tests)

add_executable(rbx-network-transport-tests integration/NetworkTransportLoopbackTests.cpp)
target_link_libraries(rbx-network-transport-tests PRIVATE Roblox::NetworkTransport)
rbx_enable_warnings(rbx-network-transport-tests)
add_test(NAME network-transport-loopback COMMAND rbx-network-transport-tests)

add_executable(rbx-network-resolution-tests integration/NetworkTransportResolutionTests.cpp)
target_link_libraries(rbx-network-resolution-tests PRIVATE Roblox::NetworkTransport)
rbx_enable_warnings(rbx-network-resolution-tests)
add_test(NAME network-transport-resolution COMMAND rbx-network-resolution-tests)

add_executable(rbx-packet-buffer-tests unit/PacketBufferTests.cpp)
target_link_libraries(rbx-packet-buffer-tests PRIVATE Roblox::NetworkTransport)
rbx_enable_warnings(rbx-packet-buffer-tests)
add_test(NAME packet-buffer-contract COMMAND rbx-packet-buffer-tests)

add_executable(rbx-datamodel-replication-tests
    integration/DataModelReplicationTests.cpp)
target_include_directories(rbx-datamodel-replication-tests PRIVATE
    "${CMAKE_SOURCE_DIR}/engine/networking"
    "${CMAKE_SOURCE_DIR}/Library/SDL2/include")
target_link_libraries(rbx-datamodel-replication-tests PRIVATE
    Roblox::LuauScripting Roblox::RuntimeUI Roblox::InteractionTools
    Roblox::AudioDataModel Roblox::DataModelRuntime Roblox::RuntimeSupport
    Roblox::AvatarHumanoid Roblox::NetworkingRuntime Roblox::TerrainRuntime
    Roblox::PlayerShared Roblox::PlatformRuntime Roblox::PlatformHost)
target_compile_definitions(rbx-datamodel-replication-tests PRIVATE
    _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR=1)
target_compile_options(rbx-datamodel-replication-tests PRIVATE
    $<$<COMPILE_LANG_AND_ID:CXX,AppleClang>:-Wno-enum-constexpr-conversion>)
add_test(NAME network-datamodel-replication
    COMMAND rbx-datamodel-replication-tests)
set_tests_properties(network-datamodel-replication PROPERTIES
    TIMEOUT 60
    RUN_SERIAL TRUE
    LABELS "networking;replication;datamodel;gns")

if(APPLE)
    find_program(RBX_FFMPEG_EXECUTABLE ffmpeg REQUIRED)
    set(RBX_MEDIA_FIXTURE "${CMAKE_BINARY_DIR}/test-output/media-decoder.mp4")
    add_custom_command(
        OUTPUT "${RBX_MEDIA_FIXTURE}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/test-output"
        COMMAND "${RBX_FFMPEG_EXECUTABLE}" -y -loglevel error
            -f lavfi -i "testsrc2=size=96x64:rate=30:duration=1"
            -f lavfi -i "sine=frequency=440:sample_rate=48000:duration=1"
            -c:v mpeg4 -q:v 4 -c:a aac -shortest "${RBX_MEDIA_FIXTURE}"
        VERBATIM)
    add_custom_target(rbx-media-decoder-fixture DEPENDS "${RBX_MEDIA_FIXTURE}")
    add_executable(rbx-media-decoder-tests integration/MediaDecoderTests.cpp)
    target_link_libraries(rbx-media-decoder-tests PRIVATE Roblox::MediaRuntime)
    add_dependencies(rbx-media-decoder-tests rbx-media-decoder-fixture)
    rbx_enable_warnings(rbx-media-decoder-tests)
    add_test(NAME media-decoder COMMAND rbx-media-decoder-tests "${RBX_MEDIA_FIXTURE}")
    set_tests_properties(media-decoder PROPERTIES LABELS "media;video;audio")

    add_executable(rbx-audio-output-device-smoke EXCLUDE_FROM_ALL
        integration/AudioOutputDeviceSmoke.cpp)
    target_link_libraries(rbx-audio-output-device-smoke PRIVATE Roblox::AudioRuntime)
    rbx_enable_warnings(rbx-audio-output-device-smoke)

    add_executable(rbx-gfxcore-bgfx-tests integration/GfxCoreBgfxTests.cpp)
    target_link_libraries(rbx-gfxcore-bgfx-tests PRIVATE
        Roblox::GfxCoreContract Roblox::PlatformHost)
    target_compile_definitions(rbx-gfxcore-bgfx-tests PRIVATE
        "RBX_TEST_SHADER_DIR=\"${CMAKE_BINARY_DIR}/generated/shaders/metal\"")
    add_dependencies(rbx-gfxcore-bgfx-tests rbx-player-shaders)
    add_test(NAME gfxcore-bgfx-metal COMMAND rbx-gfxcore-bgfx-tests)

    if(TARGET RobloxPlayer)
        set(RBX_BASEPLATE_RENDER_PROOF
            "${CMAKE_BINARY_DIR}/test-output/baseplate-render.ppm")
        add_test(NAME player-baseplate-metal-render
            COMMAND "$<TARGET_FILE:RobloxPlayer>"
                --headless-verify
                --render-proof "${RBX_BASEPLATE_RENDER_PROOF}")
        set_tests_properties(player-baseplate-metal-render PROPERTIES
            TIMEOUT 30
            RUN_SERIAL TRUE
            LABELS "player;rendering;metal")

        set(RBX_SHADOW_MAP_RENDER_PROOF
            "${CMAKE_BINARY_DIR}/test-output/shadow-map-render.ppm")
        add_test(NAME player-shadow-map-metal-render
            COMMAND "$<TARGET_FILE:RobloxPlayer>"
                --headless-verify
                --verify-shadow-map
                --frame-limit 120
                --render-proof "${RBX_SHADOW_MAP_RENDER_PROOF}")
        set_tests_properties(player-shadow-map-metal-render PROPERTIES
            TIMEOUT 30
            RUN_SERIAL TRUE
            LABELS "player;rendering;metal;lighting;shadow-map")

        set(RBX_SKYBOX_RENDER_PROOF
            "${CMAKE_BINARY_DIR}/test-output/skybox-render.ppm")
        add_test(NAME player-skybox-metal-render
            COMMAND "$<TARGET_FILE:RobloxPlayer>"
                --headless-verify
                --verify-skybox
                --frame-limit 120
                --render-proof "${RBX_SKYBOX_RENDER_PROOF}")
        set_tests_properties(player-skybox-metal-render PROPERTIES
            TIMEOUT 30
            RUN_SERIAL TRUE
            LABELS "player;rendering;metal;skybox;assets")

        add_test(NAME player-audio-runtime
            COMMAND "$<TARGET_FILE:RobloxPlayer>"
                --headless-verify
                --verify-audio
                --frame-limit 120)
        set_tests_properties(player-audio-runtime PROPERTIES
            TIMEOUT 30
            RUN_SERIAL TRUE
            LABELS "player;audio;runtime")

        add_test(NAME player-r15-runtime
            COMMAND "$<TARGET_FILE:RobloxPlayer>"
                --headless-verify
                --r15
                --frame-limit 300)
        set_tests_properties(player-r15-runtime PROPERTIES
            TIMEOUT 30
            RUN_SERIAL TRUE
            LABELS "player;avatar;r15;animation;rendering")

        add_test(NAME player-r15-plus-runtime
            COMMAND "$<TARGET_FILE:RobloxPlayer>"
                --headless-verify
                --r15-plus
                --frame-limit 300)
        set_tests_properties(player-r15-plus-runtime PROPERTIES
            TIMEOUT 30
            RUN_SERIAL TRUE
            LABELS "player;avatar;r15-plus;animation;skinning;rendering")

        add_test(NAME player-rthro-normal-runtime
            COMMAND "$<TARGET_FILE:RobloxPlayer>"
                --headless-verify
                --rthro-normal
                --frame-limit 300)
        set_tests_properties(player-rthro-normal-runtime PROPERTIES
            TIMEOUT 30
            RUN_SERIAL TRUE
            LABELS "player;avatar;rthro;animation;rendering")

        add_test(NAME player-rthro-slender-runtime
            COMMAND "$<TARGET_FILE:RobloxPlayer>"
                --headless-verify
                --rthro-slender
                --frame-limit 300)
        set_tests_properties(player-rthro-slender-runtime PROPERTIES
            TIMEOUT 30
            RUN_SERIAL TRUE
            LABELS "player;avatar;rthro;animation;rendering")

        add_test(NAME player-offline-asset-cache-metal
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_SOURCE_DIR}/tests/integration/test_player_offline_asset_cache.py"
                "$<TARGET_FILE:RobloxPlayer>"
                "$ENV{HOME}/Downloads/Baseplate.rbxlx"
                "${CMAKE_BINARY_DIR}/test-output/offline-assets")
        set_tests_properties(player-offline-asset-cache-metal PROPERTIES
            TIMEOUT 90
            RUN_SERIAL TRUE
            LABELS "player;rendering;metal;assets;offline")

        set(RBX_VIEWPORT_RENDER_PROOF
            "${CMAKE_BINARY_DIR}/test-output/viewport-frame-render.ppm")
        add_test(NAME player-viewport-frame-metal-render
            COMMAND "$<TARGET_FILE:RobloxPlayer>"
                --headless-verify
                --verify-viewport-rendering
                --render-proof "${RBX_VIEWPORT_RENDER_PROOF}")
        set_tests_properties(player-viewport-frame-metal-render PROPERTIES
            TIMEOUT 30
            RUN_SERIAL TRUE
            LABELS "player;rendering;metal;ui")

        add_dependencies(RobloxPlayer rbx-media-decoder-fixture)
        set(RBX_VIDEO_RENDER_PROOF
            "${CMAKE_BINARY_DIR}/test-output/video-frame-render.ppm")
        add_test(NAME player-video-frame-metal-render
            COMMAND "$<TARGET_FILE:RobloxPlayer>"
                --headless-verify
                --verify-video-rendering "${RBX_MEDIA_FIXTURE}"
                --render-proof "${RBX_VIDEO_RENDER_PROOF}")
        set_tests_properties(player-video-frame-metal-render PROPERTIES
            TIMEOUT 30
            RUN_SERIAL TRUE
            LABELS "player;rendering;metal;ui;video;audio")
    endif()

    add_executable(rbx-visual-engine-link-probe EXCLUDE_FROM_ALL
        integration/VisualEngineLinkProbe.cpp)
    set(RBX_VISUAL_ENGINE_RESOURCE_ROOT
        "${CMAKE_BINARY_DIR}/generated/visual-engine-resources")
    add_custom_command(
        OUTPUT "${RBX_VISUAL_ENGINE_RESOURCE_ROOT}/prepared.stamp"
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${RBX_VISUAL_ENGINE_RESOURCE_ROOT}"
        COMMAND "${CMAKE_COMMAND}" -E rm -rf
            "${RBX_VISUAL_ENGINE_RESOURCE_ROOT}/content"
            "${RBX_VISUAL_ENGINE_RESOURCE_ROOT}/shaders"
            "${RBX_VISUAL_ENGINE_RESOURCE_ROOT}/PlatformContent"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${CMAKE_SOURCE_DIR}/content"
            "${RBX_VISUAL_ENGINE_RESOURCE_ROOT}/content"
        COMMAND "${CMAKE_COMMAND}" -E create_symlink
            "${CMAKE_BINARY_DIR}/generated/shaders"
            "${RBX_VISUAL_ENGINE_RESOURCE_ROOT}/shaders"
        COMMAND "${CMAKE_COMMAND}" -E create_symlink
            "${CMAKE_SOURCE_DIR}/PlatformContent"
            "${RBX_VISUAL_ENGINE_RESOURCE_ROOT}/PlatformContent"
        COMMAND "${CMAKE_COMMAND}" -E touch
            "${RBX_VISUAL_ENGINE_RESOURCE_ROOT}/prepared.stamp"
        DEPENDS rbx-player-shaders
        COMMENT "Preparing VisualEngine runtime resources"
        VERBATIM)
    add_custom_target(rbx-visual-engine-resources DEPENDS
        "${RBX_VISUAL_ENGINE_RESOURCE_ROOT}/prepared.stamp")
    add_dependencies(rbx-visual-engine-link-probe rbx-visual-engine-resources)
    target_compile_definitions(rbx-visual-engine-link-probe PRIVATE
        "RBX_TEST_VISUAL_ENGINE_RESOURCES=\"${RBX_VISUAL_ENGINE_RESOURCE_ROOT}\""
        _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR=1)
    target_compile_options(rbx-visual-engine-link-probe PRIVATE
        -Wno-enum-constexpr-conversion)
    target_link_libraries(rbx-visual-engine-link-probe PRIVATE
        Roblox::RenderScene Roblox::Scripting Roblox::RuntimeUI Roblox::InteractionTools
        Roblox::AudioDataModel Roblox::DataModelRuntime Roblox::RuntimeSupport
        Roblox::AvatarHumanoid Roblox::NetworkingRuntime Roblox::TerrainRuntime Roblox::PlayerShared
        Roblox::PlatformRuntime Roblox::PlatformHost)
endif()

add_test(NAME dependency-boundaries
    COMMAND "${CMAKE_COMMAND}"
        -DSOURCE_ROOT="${CMAKE_SOURCE_DIR}"
        -P "${CMAKE_SOURCE_DIR}/tools/dependency_boundary/check.cmake")

find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
add_test(NAME reference-resource-inventory
    COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/tests/unit/test_reference_inventory.py")
add_test(NAME in-game-ui-import
    COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/tests/unit/test_in_game_ui_import.py")
add_test(NAME player-ui-binary-contracts
    COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/tests/unit/test_player_ui_contracts.py")
add_test(NAME root-layout-classification
    COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/tests/unit/test_root_layout.py")

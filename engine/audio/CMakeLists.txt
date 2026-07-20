add_library(rbx-audio-contracts INTERFACE)
add_library(Roblox::AudioContracts ALIAS rbx-audio-contracts)

target_include_directories(rbx-audio-contracts INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_subdirectory(runtime)

# The authentic DataModel family remains EXCLUDE_FROM_ALL for consumers that
# do not need gameplay audio, but RobloxPlayer links it directly.  Sound,
# SoundChannel and SoundService therefore use this implementation in the
# packaged product rather than a parallel verification-only path.
add_library(rbx-audio-datamodel STATIC EXCLUDE_FROM_ALL
    src/AudioGraph.cpp
    src/AudioPlayer.cpp
    src/Sound.cpp
    src/SoundChannel.cpp
    src/SoundGroups.cpp
    src/SoundService.cpp
    src/SoundWorld.cpp
    include/audio/AudioGraph.h
    include/audio/Sound.h
    include/audio/SoundChannel.h
    include/audio/SoundGroups.h
    include/audio/SoundService.h
    include/audio/SoundWorld.h)
add_library(Roblox::AudioDataModel ALIAS rbx-audio-datamodel)

target_include_directories(rbx-audio-datamodel
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE
        "${CMAKE_SOURCE_DIR}/App/include"
        "${CMAKE_SOURCE_DIR}/App"
        "${CMAKE_SOURCE_DIR}/engine/core/include"
        "${CMAKE_SOURCE_DIR}/engine/core/math/include"
        "${CMAKE_SOURCE_DIR}/engine/reflection/include"
        "${CMAKE_SOURCE_DIR}/engine/serialization/include"
        "${CMAKE_SOURCE_DIR}/engine/datamodel/tree/include"
        "${CMAKE_SOURCE_DIR}/engine/datamodel/runtime/include"
        "${CMAKE_SOURCE_DIR}/engine/simulation/include"
        "${CMAKE_SOURCE_DIR}/engine/animation/include"
        "${CMAKE_SOURCE_DIR}/engine/interaction/tools/include"
        "${CMAKE_SOURCE_DIR}/engine/ui/runtime/include"
        "${CMAKE_SOURCE_DIR}/engine/scripting/include"
        "${CMAKE_SOURCE_DIR}/engine/networking/include"
        "${CMAKE_SOURCE_DIR}/engine/rendering/foundation/include"
        "${CMAKE_SOURCE_DIR}/engine/rendering/adorn/include"
        "${CMAKE_SOURCE_DIR}/apps/player/shared/client"
        "${CMAKE_SOURCE_DIR}/third_party/bullet_historical"
        "${CMAKE_SOURCE_DIR}/Library/boost/include"
        "${CMAKE_SOURCE_DIR}/Library/SDL2/include")
target_link_libraries(rbx-audio-datamodel PUBLIC
    Roblox::AudioContracts Roblox::AudioRuntime Roblox::DataModelRuntime
    Roblox::Simulation Roblox::LuauVM)
target_compile_features(rbx-audio-datamodel PUBLIC cxx_std_20)
target_compile_definitions(rbx-audio-datamodel PRIVATE
    BOOST_NO_CXX98_FUNCTION_BASE=1
    RBX_LUAU_VM=1
    _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR=1)
target_compile_options(rbx-audio-datamodel PRIVATE
    $<$<COMPILE_LANG_AND_ID:CXX,AppleClang>:-Wno-enum-constexpr-conversion>)

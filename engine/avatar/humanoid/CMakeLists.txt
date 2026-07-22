file(GLOB RBX_HUMANOID_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB RBX_HUMANOID_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/humanoid/*.h")

add_library(rbx-avatar-humanoid STATIC EXCLUDE_FROM_ALL
    ${RBX_HUMANOID_SOURCES}
    ${RBX_HUMANOID_HEADERS})
add_library(Roblox::AvatarHumanoid ALIAS rbx-avatar-humanoid)

target_include_directories(rbx-avatar-humanoid
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE
        "${CMAKE_SOURCE_DIR}/App/include"
        "${CMAKE_SOURCE_DIR}/App"
        "${CMAKE_SOURCE_DIR}/engine/datamodel/tree/include"
        "${CMAKE_SOURCE_DIR}/engine/simulation/include"
        "${CMAKE_SOURCE_DIR}/engine/animation/include"
        "${CMAKE_SOURCE_DIR}/engine/interaction/tools/include"
        "${CMAKE_SOURCE_DIR}/engine/ui/runtime/include"
        "${CMAKE_SOURCE_DIR}/engine/reflection/include"
        "${CMAKE_SOURCE_DIR}/engine/serialization/include"
        "${CMAKE_SOURCE_DIR}/engine/core/include"
        "${CMAKE_SOURCE_DIR}/engine/scripting/include"
        "${CMAKE_SOURCE_DIR}/engine/scripting/lua/src"
        "${CMAKE_SOURCE_DIR}/engine/security/include"
        "${CMAKE_SOURCE_DIR}/apps/player/shared/client"
        "${CMAKE_SOURCE_DIR}/engine/networking/include"
        "${CMAKE_SOURCE_DIR}/engine/rendering/foundation/include"
        "${CMAKE_SOURCE_DIR}/engine/rendering/adorn/include"
        "${CMAKE_SOURCE_DIR}/Library/boost/include"
        "${CMAKE_SOURCE_DIR}/Library/SDL2/include"
        "${CMAKE_SOURCE_DIR}/third_party/bullet_historical")
target_link_libraries(rbx-avatar-humanoid
    PRIVATE
        Roblox::DataModelRuntime
        Roblox::Simulation
        Roblox::Physics
        Roblox::Animation
        Roblox::AudioContracts
        Roblox::ProtectionMarkers)
target_compile_features(rbx-avatar-humanoid PUBLIC cxx_std_20)
target_compile_definitions(rbx-avatar-humanoid PRIVATE
    BOOST_NO_CXX98_FUNCTION_BASE=1
    _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR=1
    _LIBCPP_ENABLE_CXX17_REMOVED_UNARY_BINARY_FUNCTION=1)
target_compile_options(rbx-avatar-humanoid PRIVATE
    $<$<COMPILE_LANG_AND_ID:CXX,AppleClang>:-Wno-enum-constexpr-conversion>)
rbx_enable_warnings(rbx-avatar-humanoid)

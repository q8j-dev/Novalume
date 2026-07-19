file(GLOB RBX_SIMULATION_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/v8world/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/v8kernel/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/solver/*.h")
file(GLOB RBX_SIMULATION_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/world/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/kernel/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/solver/*.cpp")

add_library(rbx-simulation STATIC EXCLUDE_FROM_ALL
    ${RBX_SIMULATION_HEADERS} ${RBX_SIMULATION_SOURCES})
add_library(Roblox::Simulation ALIAS rbx-simulation)

target_include_directories(rbx-simulation
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE
        "${CMAKE_SOURCE_DIR}/engine/datamodel/runtime/include"
        "${CMAKE_SOURCE_DIR}/engine/animation/include"
        "${CMAKE_SOURCE_DIR}/engine/datamodel/tree/include"
        "${CMAKE_SOURCE_DIR}/engine/reflection/include"
        "${CMAKE_SOURCE_DIR}/engine/serialization/include"
        "${CMAKE_SOURCE_DIR}/engine/rendering/foundation/include"
        "${CMAKE_SOURCE_DIR}/engine/core/include"
        "${CMAKE_SOURCE_DIR}/engine/core/include"
        "${CMAKE_SOURCE_DIR}/App/include"
        "${CMAKE_SOURCE_DIR}/engine/avatar/humanoid/include"
        "${CMAKE_SOURCE_DIR}/engine/terrain/include"
        "${CMAKE_SOURCE_DIR}/App"
        "${CMAKE_SOURCE_DIR}/third_party/bullet_historical"
        "${CMAKE_SOURCE_DIR}/apps/player/shared/client"
        "${CMAKE_SOURCE_DIR}/engine/networking/include"
        "${CMAKE_SOURCE_DIR}/engine/rendering/adorn/include"
        "${CMAKE_SOURCE_DIR}/engine/interaction/tools/include"
        "${CMAKE_SOURCE_DIR}/engine/ui/runtime/include"
        "${CMAKE_SOURCE_DIR}/engine/scripting/include"
        "${CMAKE_SOURCE_DIR}/engine/scripting/lua/src"
        "${CMAKE_SOURCE_DIR}/Rendering/RbxG3D/include"
        "${CMAKE_SOURCE_DIR}/Rendering/g3d/include"
        "${CMAKE_SOURCE_DIR}/engine/core/math/include"
        "${CMAKE_SOURCE_DIR}/Library/boost/include"
        "${CMAKE_SOURCE_DIR}/Library/SDL2/include")
target_link_libraries(rbx-simulation PUBLIC Roblox::DataModelTree Roblox::Physics Roblox::Math Roblox::Terrain)
target_compile_features(rbx-simulation PUBLIC cxx_std_20)
target_compile_definitions(rbx-simulation PRIVATE
    BOOST_NO_CXX98_FUNCTION_BASE=1
    _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR=1)
target_compile_options(rbx-simulation PRIVATE
    $<$<COMPILE_LANG_AND_ID:CXX,AppleClang>:-Wno-enum-constexpr-conversion>)

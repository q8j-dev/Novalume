file(GLOB RBX_INTERACTION_TOOL_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/Tool/*.h")
file(GLOB RBX_INTERACTION_TOOL_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
add_library(rbx-interaction-tools STATIC EXCLUDE_FROM_ALL
    ${RBX_INTERACTION_TOOL_HEADERS} ${RBX_INTERACTION_TOOL_SOURCES})
add_library(Roblox::InteractionTools ALIAS rbx-interaction-tools)

target_include_directories(rbx-interaction-tools
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE
        "${CMAKE_SOURCE_DIR}/engine/datamodel/runtime/include"
        "${CMAKE_SOURCE_DIR}/engine/datamodel/tree/include"
        "${CMAKE_SOURCE_DIR}/engine/animation/include"
        "${CMAKE_SOURCE_DIR}/engine/simulation/include"
        "${CMAKE_SOURCE_DIR}/engine/rendering/adorn/include"
        "${CMAKE_SOURCE_DIR}/engine/rendering/foundation/include"
        "${CMAKE_SOURCE_DIR}/engine/ui/runtime/include"
        "${CMAKE_SOURCE_DIR}/engine/scripting/include"
        "${CMAKE_SOURCE_DIR}/engine/scripting/lua/src"
        "${CMAKE_SOURCE_DIR}/engine/reflection/include"
        "${CMAKE_SOURCE_DIR}/engine/serialization/include"
        "${CMAKE_SOURCE_DIR}/engine/core/math/include"
        "${CMAKE_SOURCE_DIR}/engine/core/include"
        "${CMAKE_SOURCE_DIR}/engine/core/include"
        "${CMAKE_SOURCE_DIR}/App/include"
        "${CMAKE_SOURCE_DIR}/App"
        "${CMAKE_SOURCE_DIR}/third_party/bullet_historical"
        "${CMAKE_SOURCE_DIR}/apps/player/shared/client"
        "${CMAKE_SOURCE_DIR}/engine/networking/include"
        "${CMAKE_SOURCE_DIR}/Library/SDL2/include"
        "${CMAKE_SOURCE_DIR}/Library/boost/include")
target_link_libraries(rbx-interaction-tools PUBLIC
    Roblox::DataModelRuntime Roblox::RenderAdorn Roblox::RenderFoundation
    Roblox::Simulation Roblox::Animation Roblox::Math Roblox::AudioContracts)
target_compile_features(rbx-interaction-tools PUBLIC cxx_std_20)
target_compile_definitions(rbx-interaction-tools PRIVATE
    BOOST_NO_CXX98_FUNCTION_BASE=1
    _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR=1)
target_compile_options(rbx-interaction-tools PRIVATE
    $<$<COMPILE_LANG_AND_ID:CXX,AppleClang>:-Wno-enum-constexpr-conversion>)

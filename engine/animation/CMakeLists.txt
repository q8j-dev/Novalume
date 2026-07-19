file(GLOB RBX_ANIMATION_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/v8datamodel/*.h")
file(GLOB RBX_ANIMATION_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
add_library(rbx-animation STATIC EXCLUDE_FROM_ALL
    ${RBX_ANIMATION_HEADERS} ${RBX_ANIMATION_SOURCES})
add_library(Roblox::Animation ALIAS rbx-animation)

target_include_directories(rbx-animation
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE
        "${CMAKE_SOURCE_DIR}/engine/datamodel/runtime/include"
        "${CMAKE_SOURCE_DIR}/engine/datamodel/tree/include"
        "${CMAKE_SOURCE_DIR}/engine/reflection/include"
        "${CMAKE_SOURCE_DIR}/engine/serialization/include"
        "${CMAKE_SOURCE_DIR}/engine/simulation/include"
        "${CMAKE_SOURCE_DIR}/engine/rendering/foundation/include"
        "${CMAKE_SOURCE_DIR}/engine/core/math/include"
        "${CMAKE_SOURCE_DIR}/engine/core/include"
        "${CMAKE_SOURCE_DIR}/engine/core/include"
        "${CMAKE_SOURCE_DIR}/App/include"
        "${CMAKE_SOURCE_DIR}/engine/avatar/humanoid/include"
        "${CMAKE_SOURCE_DIR}/App"
        "${CMAKE_SOURCE_DIR}/apps/player/shared/client"
        "${CMAKE_SOURCE_DIR}/engine/networking/include"
        "${CMAKE_SOURCE_DIR}/engine/rendering/adorn/include"
        "${CMAKE_SOURCE_DIR}/engine/interaction/tools/include"
        "${CMAKE_SOURCE_DIR}/engine/ui/runtime/include"
        "${CMAKE_SOURCE_DIR}/engine/scripting/include"
        "${CMAKE_SOURCE_DIR}/engine/scripting/lua/src"
        "${CMAKE_SOURCE_DIR}/Library/boost/include"
        "${CMAKE_SOURCE_DIR}/Library/SDL2/include")
target_link_libraries(rbx-animation PUBLIC
    Roblox::DataModelTree Roblox::Serialization Roblox::Reflection
    Roblox::Simulation Roblox::Math)
target_compile_features(rbx-animation PUBLIC cxx_std_20)
target_compile_definitions(rbx-animation PRIVATE
    BOOST_NO_CXX98_FUNCTION_BASE=1
    _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR=1)
target_compile_options(rbx-animation PRIVATE
    $<$<COMPILE_LANG_AND_ID:CXX,AppleClang>:-Wno-enum-constexpr-conversion>)

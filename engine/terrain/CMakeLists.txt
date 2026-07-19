file(GLOB RBX_TERRAIN_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/voxel/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/voxel/*.inl"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/voxel2/*.h")
file(GLOB RBX_TERRAIN_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/voxel/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/smooth/*.cpp")

add_library(rbx-terrain STATIC EXCLUDE_FROM_ALL
    ${RBX_TERRAIN_HEADERS} ${RBX_TERRAIN_SOURCES})
add_library(Roblox::Terrain ALIAS rbx-terrain)

target_include_directories(rbx-terrain
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE
        "${CMAKE_SOURCE_DIR}/engine/core/include"
        "${CMAKE_SOURCE_DIR}/engine/core/math/include"
        "${CMAKE_SOURCE_DIR}/apps/player/shared/client"
        "${CMAKE_SOURCE_DIR}/App/include"
        "${CMAKE_SOURCE_DIR}/Library/boost/include"
        "${CMAKE_SOURCE_DIR}/Library/rapidjson/include")
target_link_libraries(rbx-terrain
    PUBLIC Roblox::Core Roblox::Math)
target_compile_features(rbx-terrain PUBLIC cxx_std_20)
target_compile_definitions(rbx-terrain PRIVATE
    BOOST_NO_CXX98_FUNCTION_BASE=1
    _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR=1)
target_compile_options(rbx-terrain PRIVATE
    $<$<COMPILE_LANG_AND_ID:CXX,AppleClang>:-Wno-enum-constexpr-conversion>)

add_library(rbx-terrain-runtime STATIC EXCLUDE_FROM_ALL
    "${CMAKE_CURRENT_LIST_DIR}/../src/runtime/Serializer.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../src/runtime/Voxelizer.cpp")
add_library(Roblox::TerrainRuntime ALIAS rbx-terrain-runtime)

target_include_directories(rbx-terrain-runtime
    PUBLIC "${CMAKE_CURRENT_LIST_DIR}/../include"
    PRIVATE
        "${CMAKE_SOURCE_DIR}/engine/core/include"
        "${CMAKE_SOURCE_DIR}/engine/core/math/include"
        "${CMAKE_SOURCE_DIR}/engine/datamodel/runtime/include"
        "${CMAKE_SOURCE_DIR}/engine/datamodel/tree/include"
        "${CMAKE_SOURCE_DIR}/engine/reflection/include"
        "${CMAKE_SOURCE_DIR}/engine/simulation/include"
        "${CMAKE_SOURCE_DIR}/engine/networking/include"
        "${CMAKE_SOURCE_DIR}/engine/rendering/foundation/include"
        "${CMAKE_SOURCE_DIR}/engine/avatar/humanoid/include"
        "${CMAKE_SOURCE_DIR}/engine/ui/runtime/include"
        "${CMAKE_SOURCE_DIR}/third_party/bullet_historical"
        "${CMAKE_SOURCE_DIR}/App/include"
        "${CMAKE_SOURCE_DIR}/Library/boost/include")
target_include_directories(rbx-terrain-runtime PRIVATE
    "${CMAKE_SOURCE_DIR}/Library/SDL2/include")
target_link_libraries(rbx-terrain-runtime
    PUBLIC Roblox::Terrain
    PRIVATE Roblox::DataModelRuntime Roblox::Simulation)
target_compile_features(rbx-terrain-runtime PUBLIC cxx_std_20)
target_compile_definitions(rbx-terrain-runtime PRIVATE
    BOOST_NO_CXX98_FUNCTION_BASE=1
    _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR=1)
target_compile_options(rbx-terrain-runtime PRIVATE
    $<$<COMPILE_LANG_AND_ID:CXX,AppleClang>:-Wno-enum-constexpr-conversion>)

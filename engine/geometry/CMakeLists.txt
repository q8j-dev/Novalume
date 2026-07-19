add_library(rbx-geometry-core STATIC EXCLUDE_FROM_ALL
    src/quadedge.cpp)
add_library(Roblox::GeometryCore ALIAS rbx-geometry-core)

target_include_directories(rbx-geometry-core PRIVATE
    "${CMAKE_SOURCE_DIR}/App/include"
    "${CMAKE_SOURCE_DIR}/engine/core/include")
target_link_libraries(rbx-geometry-core PUBLIC Roblox::Core Roblox::Math)
target_compile_features(rbx-geometry-core PUBLIC cxx_std_20)
target_compile_definitions(rbx-geometry-core PRIVATE
    BOOST_NO_CXX98_FUNCTION_BASE=1)
rbx_enable_warnings(rbx-geometry-core)

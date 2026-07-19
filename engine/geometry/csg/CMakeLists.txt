add_library(rbx-geometry-csg STATIC EXCLUDE_FROM_ALL
    CSGKernel.cpp
    CSGKernel.h)
add_library(Roblox::GeometryCSG ALIAS rbx-geometry-csg)

target_include_directories(rbx-geometry-csg
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/sgCore"
        "${CMAKE_SOURCE_DIR}/engine/datamodel/runtime/include"
        "${CMAKE_SOURCE_DIR}/App/include"
        "${CMAKE_SOURCE_DIR}/Library/boost/include")
target_link_libraries(rbx-geometry-csg
    PUBLIC Roblox::Math
    PRIVATE Roblox::DataModelRuntime)
target_compile_features(rbx-geometry-csg PUBLIC cxx_std_20)
target_compile_definitions(rbx-geometry-csg PRIVATE
    BOOST_NO_CXX98_FUNCTION_BASE=1
    _LIBCPP_ENABLE_CXX17_REMOVED_UNARY_BINARY_FUNCTION=1)
target_compile_options(rbx-geometry-csg PRIVATE
    $<$<COMPILE_LANG_AND_ID:CXX,AppleClang>:-Wno-enum-constexpr-conversion>)
rbx_enable_warnings(rbx-geometry-csg)

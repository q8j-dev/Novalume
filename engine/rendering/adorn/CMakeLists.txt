add_library(rbx-render-adorn STATIC EXCLUDE_FROM_ALL
    src/Draw.cpp
    src/DrawAdorn.cpp
    src/HitTest.cpp)
add_library(Roblox::RenderAdorn ALIAS rbx-render-adorn)

target_include_directories(rbx-render-adorn
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE
        "${CMAKE_SOURCE_DIR}/engine/rendering/foundation/include"
        "${CMAKE_SOURCE_DIR}/engine/datamodel/runtime/include"
        "${CMAKE_SOURCE_DIR}/engine/simulation/include"
        "${CMAKE_SOURCE_DIR}/engine/interaction/tools/include"
        "${CMAKE_SOURCE_DIR}/engine/ui/runtime/include"
        "${CMAKE_SOURCE_DIR}/engine/scripting/include"
        "${CMAKE_SOURCE_DIR}/engine/core/math/include"
        "${CMAKE_SOURCE_DIR}/engine/core/include"
        "${CMAKE_SOURCE_DIR}/engine/core/include"
        "${CMAKE_SOURCE_DIR}/App/include"
        "${CMAKE_SOURCE_DIR}/third_party/bullet_historical"
        "${CMAKE_SOURCE_DIR}/Library/SDL2/include"
        "${CMAKE_SOURCE_DIR}/Library/boost/include")
target_link_libraries(rbx-render-adorn PUBLIC
    Roblox::RenderFoundation Roblox::DataModelRuntime Roblox::Simulation Roblox::Math)
target_compile_features(rbx-render-adorn PUBLIC cxx_std_20)
target_compile_definitions(rbx-render-adorn PRIVATE
    BOOST_NO_CXX98_FUNCTION_BASE=1
    _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR=1)
target_compile_options(rbx-render-adorn PRIVATE
    $<$<COMPILE_LANG_AND_ID:CXX,AppleClang>:-Wno-enum-constexpr-conversion>)

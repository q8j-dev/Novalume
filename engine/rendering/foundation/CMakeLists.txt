add_library(rbx-render-foundation STATIC EXCLUDE_FROM_ALL
    src/Adorn.cpp
    src/AdornBillboarder.cpp
    src/AdornBillboarder2D.cpp
    src/AdornSurface.cpp
    src/FileMeshData.cpp
    src/FrameRateManager.cpp
    src/GfxPart.cpp
    src/IAdornableCollector.cpp
    src/PartIdentifier.cpp
    src/RenderCaps.cpp
    src/RenderSettings.cpp
    src/RenderStats.cpp
    src/ThumbnailTextureProvider.cpp
    src/ViewBase.cpp
    src/ViewportBillboarder.cpp)
add_library(Roblox::RenderFoundation ALIAS rbx-render-foundation)

target_include_directories(rbx-render-foundation
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_SOURCE_DIR}/App/include"
        "${CMAKE_SOURCE_DIR}/engine/avatar/humanoid/include"
        "${CMAKE_SOURCE_DIR}/Rendering/RbxG3D/include"
        "${CMAKE_SOURCE_DIR}/Rendering/g3d/include"
        "${CMAKE_SOURCE_DIR}/engine/core/math/include"
        "${CMAKE_SOURCE_DIR}/Library/boost/include"
    PRIVATE
        "${CMAKE_SOURCE_DIR}/engine/datamodel/runtime/include"
        "${CMAKE_SOURCE_DIR}/engine/animation/include"
        "${CMAKE_SOURCE_DIR}/engine/datamodel/tree/include"
        "${CMAKE_SOURCE_DIR}/engine/reflection/include"
        "${CMAKE_SOURCE_DIR}/engine/serialization/include"
        "${CMAKE_SOURCE_DIR}/engine/core/include"
        "${CMAKE_SOURCE_DIR}/engine/core/include"
        "${CMAKE_SOURCE_DIR}/App"
        "${CMAKE_SOURCE_DIR}/third_party/bullet_historical"
        "${CMAKE_SOURCE_DIR}/apps/player/shared/client"
        "${CMAKE_SOURCE_DIR}/engine/rendering/adorn/include"
        "${CMAKE_SOURCE_DIR}/engine/interaction/tools/include"
        "${CMAKE_SOURCE_DIR}/engine/ui/runtime/include"
        "${CMAKE_SOURCE_DIR}/engine/scripting/include"
        "${CMAKE_SOURCE_DIR}/Library/SDL2/include"
        $<$<BOOL:${rbx_draco_SOURCE_DIR}>:${rbx_draco_SOURCE_DIR}/src>
        $<$<BOOL:${rbx_draco_SOURCE_DIR}>:${CMAKE_BINARY_DIR}>)
target_link_libraries(rbx-render-foundation PUBLIC
    Roblox::GfxCoreContract Roblox::Simulation Roblox::Math draco::draco)
target_compile_features(rbx-render-foundation PUBLIC cxx_std_20)
target_compile_definitions(rbx-render-foundation
    PUBLIC BOOST_NO_CXX98_FUNCTION_BASE=1
    PRIVATE
    _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR=1)
target_compile_options(rbx-render-foundation PRIVATE
    $<$<COMPILE_LANG_AND_ID:CXX,AppleClang>:-Wno-enum-constexpr-conversion>)

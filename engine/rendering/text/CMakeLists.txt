add_library(rbx-render-text STATIC EXCLUDE_FROM_ALL
    src/TypesetterBitmap.cpp
    src/TypesetterDynamic.cpp
    include/GfxRender/TypesetterBitmap.h
    include/GfxRender/TypesetterDynamic.h
)
add_library(Roblox::RenderText ALIAS rbx-render-text)

target_include_directories(rbx-render-text
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE
        "${CMAKE_SOURCE_DIR}/engine/rendering/scene/include/GfxRender"
        "${CMAKE_SOURCE_DIR}/engine/rendering/materials/include/GfxRender"
        "${CMAKE_SOURCE_DIR}/engine/rendering/text/include/GfxRender"
        "${CMAKE_SOURCE_DIR}/engine/rendering/ui/include/GfxRender"
        "${CMAKE_SOURCE_DIR}/engine/datamodel/runtime/include"
        "${CMAKE_SOURCE_DIR}/engine/animation/include"
        "${CMAKE_SOURCE_DIR}/engine/simulation/include"
        "${CMAKE_SOURCE_DIR}/engine/ui/runtime/include"
        "${CMAKE_SOURCE_DIR}/engine/scripting/include"
        "${CMAKE_SOURCE_DIR}/engine/scripting/lua/src"
        "${CMAKE_SOURCE_DIR}/engine/interaction/tools/include"
        "${CMAKE_SOURCE_DIR}/engine/rendering/adorn/include"
        "${CMAKE_SOURCE_DIR}/engine/rendering/foundation/include"
        "${CMAKE_SOURCE_DIR}/Rendering/RbxG3D/include"
        "${CMAKE_SOURCE_DIR}/Rendering/g3d/include"
        "${CMAKE_SOURCE_DIR}/engine/core/include"
        "${CMAKE_SOURCE_DIR}/engine/core/include"
        "${CMAKE_SOURCE_DIR}/Log/include"
        "${CMAKE_SOURCE_DIR}/App/include"
        "${CMAKE_SOURCE_DIR}/third_party/bullet_historical"
        "${CMAKE_SOURCE_DIR}/apps/player/shared/client"
        "${CMAKE_SOURCE_DIR}/Library/boost/include"
        "${CMAKE_SOURCE_DIR}/Library/SDL2/include"
        "${CMAKE_SOURCE_DIR}/Rendering/GfxRender"
)
target_link_libraries(rbx-render-text PUBLIC
    Roblox::GfxCoreContract Roblox::RenderFoundation Roblox::Freetype
    Roblox::HarfBuzz Roblox::Utf8proc Roblox::SheenBidi)
target_compile_features(rbx-render-text PUBLIC cxx_std_20)
target_compile_definitions(rbx-render-text PRIVATE
    BOOST_NO_CXX98_FUNCTION_BASE=1
    _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR=1)
target_compile_options(rbx-render-text PRIVATE
    $<$<COMPILE_LANG_AND_ID:CXX,AppleClang>:-Wno-enum-constexpr-conversion>)

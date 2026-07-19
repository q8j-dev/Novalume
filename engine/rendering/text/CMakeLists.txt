set(RBX_FREETYPE_ROOT "${CMAKE_SOURCE_DIR}/Rendering/GfxRender/freetype")
set(RBX_FREETYPE_SOURCES
    ${RBX_FREETYPE_ROOT}/src/autofit/autofit.c
    ${RBX_FREETYPE_ROOT}/src/base/ftbase.c
    ${RBX_FREETYPE_ROOT}/src/base/ftbbox.c
    ${RBX_FREETYPE_ROOT}/src/base/ftbdf.c
    ${RBX_FREETYPE_ROOT}/src/base/ftbitmap.c
    ${RBX_FREETYPE_ROOT}/src/base/ftcid.c
    ${RBX_FREETYPE_ROOT}/src/base/ftfntfmt.c
    ${RBX_FREETYPE_ROOT}/src/base/ftfstype.c
    ${RBX_FREETYPE_ROOT}/src/base/ftgasp.c
    ${RBX_FREETYPE_ROOT}/src/base/ftglyph.c
    ${RBX_FREETYPE_ROOT}/src/base/ftgxval.c
    ${RBX_FREETYPE_ROOT}/src/base/ftinit.c
    ${RBX_FREETYPE_ROOT}/src/base/ftlcdfil.c
    ${RBX_FREETYPE_ROOT}/src/base/ftmm.c
    ${RBX_FREETYPE_ROOT}/src/base/ftotval.c
    ${RBX_FREETYPE_ROOT}/src/base/ftpatent.c
    ${RBX_FREETYPE_ROOT}/src/base/ftpfr.c
    ${RBX_FREETYPE_ROOT}/src/base/ftstroke.c
    ${RBX_FREETYPE_ROOT}/src/base/ftsynth.c
    ${RBX_FREETYPE_ROOT}/src/base/ftsystem.c
    ${RBX_FREETYPE_ROOT}/src/base/fttype1.c
    ${RBX_FREETYPE_ROOT}/src/base/ftwinfnt.c
    ${RBX_FREETYPE_ROOT}/src/bdf/bdf.c
    ${RBX_FREETYPE_ROOT}/src/bzip2/ftbzip2.c
    ${RBX_FREETYPE_ROOT}/src/cache/ftcache.c
    ${RBX_FREETYPE_ROOT}/src/cff/cff.c
    ${RBX_FREETYPE_ROOT}/src/cid/type1cid.c
    ${RBX_FREETYPE_ROOT}/src/gzip/ftgzip.c
    ${RBX_FREETYPE_ROOT}/src/lzw/ftlzw.c
    ${RBX_FREETYPE_ROOT}/src/pcf/pcf.c
    ${RBX_FREETYPE_ROOT}/src/pfr/pfr.c
    ${RBX_FREETYPE_ROOT}/src/psaux/psaux.c
    ${RBX_FREETYPE_ROOT}/src/pshinter/pshinter.c
    ${RBX_FREETYPE_ROOT}/src/psnames/psnames.c
    ${RBX_FREETYPE_ROOT}/src/raster/raster.c
    ${RBX_FREETYPE_ROOT}/src/sfnt/sfnt.c
    ${RBX_FREETYPE_ROOT}/src/smooth/smooth.c
    ${RBX_FREETYPE_ROOT}/src/truetype/truetype.c
    ${RBX_FREETYPE_ROOT}/src/type1/type1.c
    ${RBX_FREETYPE_ROOT}/src/type42/type42.c
    ${RBX_FREETYPE_ROOT}/src/winfonts/winfnt.c)
add_library(rbx-text-freetype STATIC EXCLUDE_FROM_ALL ${RBX_FREETYPE_SOURCES})
target_include_directories(rbx-text-freetype PUBLIC ${RBX_FREETYPE_ROOT}/include)
target_compile_definitions(rbx-text-freetype PRIVATE FT2_BUILD_LIBRARY)

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
        "${CMAKE_SOURCE_DIR}/Rendering/GfxRender/freetype/include"
)
target_link_libraries(rbx-render-text PUBLIC
    Roblox::GfxCoreContract Roblox::RenderFoundation rbx-text-freetype)
target_compile_features(rbx-render-text PUBLIC cxx_std_20)
target_compile_definitions(rbx-render-text PRIVATE
    BOOST_NO_CXX98_FUNCTION_BASE=1
    _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR=1)
target_compile_options(rbx-render-text PRIVATE
    $<$<COMPILE_LANG_AND_ID:CXX,AppleClang>:-Wno-enum-constexpr-conversion>)

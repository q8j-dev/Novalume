include(Rendering)

include_directories(../GfxAdapters)
include_directories(../../ClientShared)
include_directories(../../engine/rendering/scene/include/GfxRender)
include_directories(../../engine/rendering/materials/include/GfxRender)
include_directories(../../engine/rendering/text/include/GfxRender)
include_directories(../../engine/rendering/ui/include/GfxRender)

#add_subdirectory(freetype)


add_definitions(-DFT2_BUILD_LIBRARY)

include_directories(freetype/include)
include_directories(freetype/src)

file(GLOB PUBLIC_HEADERS "freetype/include/ft2build.h" "include/freetype/*.h")
file(GLOB PUBLIC_CONFIG_HEADERS "freetype/include/freetype/config/*.h")
file(GLOB PRIVATE_HEADERS "freetype/include/freetype/internal/*.h")

set(BASE_SRCS
  freetype/src/autofit/autofit.c
  freetype/src/base/ftbase.c
  freetype/src/base/ftbbox.c
  freetype/src/base/ftbdf.c
  freetype/src/base/ftbitmap.c
  freetype/src/base/ftcid.c
  freetype/src/base/ftfntfmt.c
  freetype/src/base/ftfstype.c
  freetype/src/base/ftgasp.c
  freetype/src/base/ftglyph.c
  freetype/src/base/ftgxval.c
  freetype/src/base/ftinit.c
  freetype/src/base/ftlcdfil.c
  freetype/src/base/ftmm.c
  freetype/src/base/ftotval.c
  freetype/src/base/ftpatent.c
  freetype/src/base/ftpfr.c
  freetype/src/base/ftstroke.c
  freetype/src/base/ftsynth.c
  freetype/src/base/ftsystem.c
  freetype/src/base/fttype1.c
  freetype/src/base/ftwinfnt.c
  freetype/src/bdf/bdf.c
  freetype/src/bzip2/ftbzip2.c
  freetype/src/cache/ftcache.c
  freetype/src/cff/cff.c
  freetype/src/cid/type1cid.c
  freetype/src/gzip/ftgzip.c
  freetype/src/lzw/ftlzw.c
  freetype/src/pcf/pcf.c
  freetype/src/pfr/pfr.c
  freetype/src/psaux/psaux.c
  freetype/src/pshinter/pshinter.c
  freetype/src/psnames/psnames.c
  freetype/src/raster/raster.c
  freetype/src/sfnt/sfnt.c
  freetype/src/smooth/smooth.c
  freetype/src/truetype/truetype.c
  freetype/src/type1/type1.c
  freetype/src/type42/type42.c
  freetype/src/winfonts/winfnt.c
)

list(APPEND HEADERS LightObject.h)
list(APPEND HEADERS VertexStreamer.h)
list(APPEND HEADERS ShaderManager.h)
list(APPEND HEADERS ParticleEmitter.h)
list(APPEND HEADERS Emitter.h)
list(APPEND HEADERS TextureRef.h)
list(APPEND HEADERS SceneManager.h)
list(APPEND HEADERS RenderNode.h)
list(APPEND HEADERS SSAO.h)
list(APPEND HEADERS MegaCluster.h)
list(APPEND HEADERS MaterialGenerator.h)
list(APPEND HEADERS TextureManager.h)
list(APPEND HEADERS RenderQueue.h)
list(APPEND HEADERS SpatialGrid.h)
list(APPEND HEADERS GlobalShaderData.h)
list(APPEND HEADERS TextureCompositor.h)
list(APPEND HEADERS RenderView.h)
list(APPEND HEADERS Sky.h)
list(APPEND HEADERS Util.h)
list(APPEND HEADERS Material.h)
list(APPEND HEADERS LightGrid.h)
list(APPEND HEADERS TypesetterBitmap.h)
list(APPEND HEADERS SpatialHashedScene.h)
list(APPEND HEADERS RenderCamera.h)
list(APPEND HEADERS Water.h)
list(APPEND HEADERS EmitterShared.h)
list(APPEND HEADERS GeometryGenerator.h)
list(APPEND HEADERS VisualEngine.h)
list(APPEND HEADERS SceneUpdater.h)
list(APPEND HEADERS ScreenSpaceEffect.h)
list(APPEND HEADERS Image.h)
list(APPEND HEADERS FastCluster.h)
list(APPEND HEADERS CullableSceneNode.h)
list(APPEND HEADERS AdornRender.h)
list(APPEND HEADERS SuperCluster.h)
list(APPEND HEADERS ObjectExporter.h)
list(APPEND HEADERS EnvMap.h)
list(APPEND HEADERS ExplosionEmitter.h)
list(APPEND HEADERS CustomEmitter.h)
list(APPEND HEADERS SmoothCluster.h)
list(APPEND HEADERS TextureAtlas.h)
list(APPEND HEADERS TypesetterDynamic.h)

list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/engine/rendering/scene/src/RenderQueue.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/engine/rendering/scene/src/RenderView.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/engine/rendering/scene/src/SpatialHashedScene.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/engine/rendering/scene/src/ParticleEmitter.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/engine/rendering/materials/src/TextureRef.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/engine/rendering/materials/src/GlobalShaderData.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/engine/rendering/ui/src/SSAO.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/engine/rendering/scene/src/RenderNode.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/engine/rendering/materials/src/Material.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/engine/rendering/materials/src/Image.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/engine/rendering/scene/src/CullableSceneNode.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/engine/rendering/scene/src/FastCluster.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/engine/rendering/scene/src/RenderCamera.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/engine/rendering/materials/src/TextureManager.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/engine/rendering/scene/src/LightGrid.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/engine/rendering/materials/src/MaterialGenerator.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/engine/rendering/scene/src/MegaCluster.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/engine/rendering/materials/src/TextureCompositor.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/engine/rendering/scene/src/GeometryGenerator.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/engine/rendering/scene/src/Sky.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/engine/rendering/scene/src/VertexStreamer.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/engine/rendering/ui/src/AdornRender.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/engine/rendering/scene/src/Water.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/engine/rendering/scene/src/Emitter.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/engine/rendering/scene/src/stdafx.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/engine/rendering/scene/src/LightObject.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/engine/rendering/text/src/TypesetterBitmap.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/engine/rendering/scene/src/SceneManager.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/engine/rendering/ui/src/ScreenSpaceEffect.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/engine/rendering/scene/src/VisualEngine.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/engine/rendering/materials/src/ShaderManager.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/engine/rendering/scene/src/SceneUpdater.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/engine/rendering/scene/src/SuperCluster.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/engine/rendering/scene/src/ObjectExporter.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/engine/rendering/materials/src/EnvMap.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/engine/rendering/scene/src/ExplosionEmitter.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/engine/rendering/scene/src/CustomEmitter.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/engine/rendering/scene/src/SmoothCluster.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/engine/rendering/materials/src/TextureAtlas.cpp")
list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/engine/rendering/text/src/TypesetterDynamic.cpp")

add_library(GfxRender OBJECT ${PUBLIC_HEADERS} ${PUBLIC_CONFIG_HEADERS} ${PRIVATE_HEADERS} ${BASE_SRCS} ${SOURCES} ${HEADERS})

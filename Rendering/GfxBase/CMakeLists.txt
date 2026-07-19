include(App)

set(RBX_GFXBASE_ROOT "${CMAKE_SOURCE_DIR}/engine/rendering/foundation")
include_directories("${RBX_GFXBASE_ROOT}/include")
include_directories(../g3d/include)
include_directories(../RbxG3d/include)
include_directories(../AppDraw/include)

set(HEADERS
    ${RBX_GFXBASE_ROOT}/include/GfxBase/Adorn.h
    ${RBX_GFXBASE_ROOT}/include/GfxBase/AdornBillboarder.h
    ${RBX_GFXBASE_ROOT}/include/GfxBase/AdornBillboarder2D.h
    ${RBX_GFXBASE_ROOT}/include/GfxBase/AdornSurface.h
    ${RBX_GFXBASE_ROOT}/include/GfxBase/AsyncResult.h
    ${RBX_GFXBASE_ROOT}/include/GfxBase/FileMeshData.h
    ${RBX_GFXBASE_ROOT}/include/GfxBase/FrameRateManager.h
    ${RBX_GFXBASE_ROOT}/include/GfxBase/GfxPart.h
    ${RBX_GFXBASE_ROOT}/include/GfxBase/IAdornable.h
    ${RBX_GFXBASE_ROOT}/include/GfxBase/IAdornableCollector.h
    ${RBX_GFXBASE_ROOT}/include/GfxBase/Image.h
    ${RBX_GFXBASE_ROOT}/include/GfxBase/MeshFileStructs.h
    ${RBX_GFXBASE_ROOT}/include/GfxBase/MeshGen.h
    ${RBX_GFXBASE_ROOT}/include/GfxBase/Part.h
    ${RBX_GFXBASE_ROOT}/include/GfxBase/PartIdentifier.h
    ${RBX_GFXBASE_ROOT}/include/GfxBase/RenderCaps.h
    ${RBX_GFXBASE_ROOT}/include/GfxBase/RenderSettings.h
    ${RBX_GFXBASE_ROOT}/include/GfxBase/RenderStats.h
    ${RBX_GFXBASE_ROOT}/include/GfxBase/TextureProxyBase.h
    ${RBX_GFXBASE_ROOT}/include/GfxBase/Type.h
    ${RBX_GFXBASE_ROOT}/include/GfxBase/Typesetter.h
    ${RBX_GFXBASE_ROOT}/include/GfxBase/ViewBase.h
    ${RBX_GFXBASE_ROOT}/include/GfxBase/ViewportBillboarder.h)

set(SOURCES
    ${RBX_GFXBASE_ROOT}/src/Adorn.cpp
    ${RBX_GFXBASE_ROOT}/src/AdornBillboarder.cpp
    ${RBX_GFXBASE_ROOT}/src/AdornBillboarder2D.cpp
    ${RBX_GFXBASE_ROOT}/src/AdornSurface.cpp
    ${RBX_GFXBASE_ROOT}/src/FileMeshData.cpp
    ${RBX_GFXBASE_ROOT}/src/FrameRateManager.cpp
    ${RBX_GFXBASE_ROOT}/src/GfxPart.cpp
    ${RBX_GFXBASE_ROOT}/src/IAdornableCollector.cpp
    ${RBX_GFXBASE_ROOT}/src/PartIdentifier.cpp
    ${RBX_GFXBASE_ROOT}/src/RenderCaps.cpp
    ${RBX_GFXBASE_ROOT}/src/RenderSettings.cpp
    ${RBX_GFXBASE_ROOT}/src/RenderStats.cpp
    ${RBX_GFXBASE_ROOT}/src/ViewBase.cpp
    ${RBX_GFXBASE_ROOT}/src/ViewportBillboarder.cpp)

add_library(GfxBase OBJECT ${SOURCES} ${HEADERS})

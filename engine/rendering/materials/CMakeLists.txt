add_library(rbx-render-materials STATIC EXCLUDE_FROM_ALL
    src/EnvMap.cpp
    src/GlobalShaderData.cpp
    src/Image.cpp
    src/Material.cpp
    src/MaterialGenerator.cpp
    src/ShaderManager.cpp
    src/TextureAtlas.cpp
    src/TextureCompositor.cpp
    src/TextureManager.cpp
    src/TextureRef.cpp
    include/GfxRender/EnvMap.h
    include/GfxRender/GlobalShaderData.h
    include/GfxRender/Image.h
    include/GfxRender/ImageInfo.h
    include/GfxRender/Material.h
    include/GfxRender/MaterialGenerator.h
    include/GfxRender/ShaderManager.h
    include/GfxRender/TextureAtlas.h
    include/GfxRender/TextureCompositor.h
    include/GfxRender/TextureManager.h
    include/GfxRender/TextureRef.h
)
add_library(Roblox::RenderMaterials ALIAS rbx-render-materials)

target_include_directories(rbx-render-materials
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
)
target_link_libraries(rbx-render-materials PUBLIC Roblox::GfxCoreContract Roblox::RenderFoundation)
target_compile_features(rbx-render-materials PUBLIC cxx_std_20)
target_compile_definitions(rbx-render-materials PRIVATE
    BOOST_NO_CXX98_FUNCTION_BASE=1
    _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR=1)
target_compile_options(rbx-render-materials PRIVATE
    $<$<COMPILE_LANG_AND_ID:CXX,AppleClang>:-Wno-enum-constexpr-conversion>)

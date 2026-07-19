set(RBX_SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/generated/shaders")
set(RBX_VARYING_DEF "${CMAKE_CURRENT_SOURCE_DIR}/varying/player_ui.def.sc")
set(RBX_SCENE_SHADER_PACK "${RBX_SHADER_OUTPUT_DIR}/shaders_bgfx-glsl.pack")
set(RBX_METAL_SCENE_SHADER_PACK "${RBX_SHADER_OUTPUT_DIR}/shaders_bgfx-metal.pack")

file(GLOB RBX_SCENE_SHADER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.hlsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.h")

find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)

add_custom_command(
    OUTPUT "${RBX_SCENE_SHADER_PACK}" "${RBX_SHADER_OUTPUT_DIR}/shaders.json"
    COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/tools/shader_pipeline/compile_scene_pack.py"
        --source-pack "${CMAKE_CURRENT_SOURCE_DIR}/shaders_glsl3.pack"
        --shader-db "${CMAKE_CURRENT_SOURCE_DIR}/shaders.json"
        --shaderc "$<TARGET_FILE:shaderc>"
        --translator "${CMAKE_SOURCE_DIR}/Rendering/ShaderCompiler/ShaderCompiler"
        --source-root "${CMAKE_CURRENT_SOURCE_DIR}/source"
        --output "${RBX_SCENE_SHADER_PACK}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders.json"
        "${RBX_SHADER_OUTPUT_DIR}/shaders.json"
    DEPENDS
        shaderc
        "${CMAKE_SOURCE_DIR}/tools/shader_pipeline/compile_scene_pack.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders_glsl3.pack"
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders.json"
        "${CMAKE_SOURCE_DIR}/Rendering/ShaderCompiler/ShaderCompiler"
        ${RBX_SCENE_SHADER_SOURCES}
    COMMENT "Compiling the complete scene shader pack for bgfx OpenGL"
    VERBATIM)

if(APPLE AND CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    add_custom_command(
        OUTPUT "${RBX_METAL_SCENE_SHADER_PACK}"
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_SOURCE_DIR}/tools/shader_pipeline/compile_metal_scene_pack.py"
            --translator "${CMAKE_SOURCE_DIR}/Rendering/ShaderCompiler/ShaderCompiler"
            --source-pack "${CMAKE_CURRENT_SOURCE_DIR}/shaders_glsl3.pack"
            --shader-db "${CMAKE_CURRENT_SOURCE_DIR}/shaders.json"
            --source-root "${CMAKE_CURRENT_SOURCE_DIR}/source"
            --output "${RBX_METAL_SCENE_SHADER_PACK}"
        DEPENDS
            "${CMAKE_SOURCE_DIR}/tools/shader_pipeline/compile_metal_scene_pack.py"
            "${CMAKE_SOURCE_DIR}/Rendering/ShaderCompiler/ShaderCompiler"
            "${CMAKE_CURRENT_SOURCE_DIR}/shaders_glsl3.pack"
            "${CMAKE_CURRENT_SOURCE_DIR}/shaders.json"
            ${RBX_SCENE_SHADER_SOURCES}
        COMMENT "Translating the complete scene shader pack for bgfx Metal"
        VERBATIM)
endif()

bgfx_compile_shaders(
    TYPE VERTEX
    SHADERS "${CMAKE_CURRENT_SOURCE_DIR}/source/vs_player_ui.sc"
    VARYING_DEF "${RBX_VARYING_DEF}"
    OUTPUT_DIR "${RBX_SHADER_OUTPUT_DIR}"
    OUT_FILES_VAR RBX_PLAYER_UI_VERTEX_OUTPUTS
    INCLUDE_DIRS "${BGFX_DIR}/src" "${CMAKE_CURRENT_SOURCE_DIR}/include")
bgfx_compile_shaders(
    TYPE FRAGMENT
    SHADERS "${CMAKE_CURRENT_SOURCE_DIR}/source/fs_player_ui.sc"
    VARYING_DEF "${RBX_VARYING_DEF}"
    OUTPUT_DIR "${RBX_SHADER_OUTPUT_DIR}"
    OUT_FILES_VAR RBX_PLAYER_UI_FRAGMENT_OUTPUTS
    INCLUDE_DIRS "${BGFX_DIR}/src" "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_custom_target(rbx-player-shaders ALL
    DEPENDS
        ${RBX_PLAYER_UI_VERTEX_OUTPUTS}
        ${RBX_PLAYER_UI_FRAGMENT_OUTPUTS}
        "${RBX_SCENE_SHADER_PACK}"
        "${RBX_SHADER_OUTPUT_DIR}/shaders.json")

if(APPLE AND CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    add_custom_target(rbx-metal-scene-shaders DEPENDS "${RBX_METAL_SCENE_SHADER_PACK}")
    add_dependencies(rbx-player-shaders rbx-metal-scene-shaders)
endif()

set(RBX_SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/generated/shaders")
set(RBX_VARYING_DEF "${CMAKE_CURRENT_SOURCE_DIR}/varying/player_ui.def.sc")
set(RBX_SCENE_VARYING_DEF
    "${CMAKE_CURRENT_SOURCE_DIR}/varying/scene_shadow.def.sc")
set(RBX_SCENE_SHADER_PACK "${RBX_SHADER_OUTPUT_DIR}/shaders_bgfx-glsl.pack")
set(RBX_METAL_SCENE_SHADER_PACK "${RBX_SHADER_OUTPUT_DIR}/shaders_bgfx-metal.pack")
set(RBX_SPIRV_SCENE_SHADER_PACK "${RBX_SHADER_OUTPUT_DIR}/shaders_bgfx-spirv.pack")
set(RBX_D3D11_SCENE_SHADER_PACK "${RBX_SHADER_OUTPUT_DIR}/shaders_bgfx-d3d11.pack")
set(RBX_WGSL_SCENE_SHADER_PACK "${RBX_SHADER_OUTPUT_DIR}/shaders_bgfx-wgsl.pack")

file(GLOB RBX_SCENE_SHADER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.hlsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.sc")

if(EMSCRIPTEN)
    set(RBX_SCENE_GLSL_PLATFORM "asm.js")
elseif(WIN32)
    set(RBX_SCENE_GLSL_PLATFORM "windows")
elseif(UNIX AND NOT APPLE)
    set(RBX_SCENE_GLSL_PLATFORM "linux")
else()
    set(RBX_SCENE_GLSL_PLATFORM "osx")
endif()

find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)

if(CMAKE_CROSSCOMPILING)
    set(RBX_SHADERC_EXECUTABLE "${RBX_HOST_SHADERC}")
    set(RBX_SHADERC_DEPENDENCY "${RBX_HOST_SHADERC}")
else()
    set(RBX_SHADERC_EXECUTABLE "$<TARGET_FILE:shaderc>")
    set(RBX_SHADERC_DEPENDENCY shaderc)
endif()

add_custom_command(
    OUTPUT "${RBX_SCENE_SHADER_PACK}" "${RBX_SHADER_OUTPUT_DIR}/shaders.json"
    COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/tools/shader_pipeline/compile_scene_pack.py"
        --source-pack "${CMAKE_CURRENT_SOURCE_DIR}/shaders_glsl3.pack"
        --shader-db "${CMAKE_CURRENT_SOURCE_DIR}/shaders.json"
        --shaderc "${RBX_SHADERC_EXECUTABLE}"
        --source-root "${CMAKE_CURRENT_SOURCE_DIR}/source"
        --varying-def "${RBX_SCENE_VARYING_DEF}"
        --bgfx-include "${BGFX_DIR}/src"
        --platform "${RBX_SCENE_GLSL_PLATFORM}"
        --output "${RBX_SCENE_SHADER_PACK}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders.json"
        "${RBX_SHADER_OUTPUT_DIR}/shaders.json"
    DEPENDS
        "${RBX_SHADERC_DEPENDENCY}"
        "${CMAKE_SOURCE_DIR}/tools/shader_pipeline/compile_scene_pack.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders_glsl3.pack"
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders.json"
        "${RBX_SCENE_VARYING_DEF}"
        ${RBX_SCENE_SHADER_SOURCES}
    COMMENT "Compiling the complete scene shader pack for bgfx OpenGL"
    VERBATIM)

if(WIN32)
    add_custom_command(
        OUTPUT "${RBX_D3D11_SCENE_SHADER_PACK}"
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_SOURCE_DIR}/tools/shader_pipeline/compile_scene_pack.py"
            --source-pack "${CMAKE_CURRENT_SOURCE_DIR}/shaders_glsl3.pack"
            --shader-db "${CMAKE_CURRENT_SOURCE_DIR}/shaders.json"
            --shaderc "${RBX_SHADERC_EXECUTABLE}"
            --source-root "${CMAKE_CURRENT_SOURCE_DIR}/source"
            --platform windows
            --profile s_5_0
            --preprocessor "${CMAKE_C_COMPILER}"
            --output "${RBX_D3D11_SCENE_SHADER_PACK}"
        DEPENDS
            "${RBX_SHADERC_DEPENDENCY}"
            "${CMAKE_SOURCE_DIR}/tools/shader_pipeline/compile_scene_pack.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/shaders_glsl3.pack"
            "${CMAKE_CURRENT_SOURCE_DIR}/shaders.json"
            ${RBX_SCENE_SHADER_SOURCES}
        COMMENT "Compiling the complete scene shader pack for bgfx Direct3D 11"
        VERBATIM)
endif()

if(EMSCRIPTEN)
    add_custom_command(
        OUTPUT "${RBX_WGSL_SCENE_SHADER_PACK}"
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_SOURCE_DIR}/tools/shader_pipeline/compile_scene_pack.py"
            --source-pack "${CMAKE_CURRENT_SOURCE_DIR}/shaders_glsl3.pack"
            --shader-db "${CMAKE_CURRENT_SOURCE_DIR}/shaders.json"
            --shaderc "${RBX_SHADERC_EXECUTABLE}"
            --source-root "${CMAKE_CURRENT_SOURCE_DIR}/source"
            --platform asm.js
            --profile wgsl
            --preprocessor "${CMAKE_C_COMPILER}"
            --output "${RBX_WGSL_SCENE_SHADER_PACK}"
        DEPENDS
            "${RBX_SHADERC_DEPENDENCY}"
            "${CMAKE_SOURCE_DIR}/tools/shader_pipeline/compile_scene_pack.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/shaders_glsl3.pack"
            "${CMAKE_CURRENT_SOURCE_DIR}/shaders.json"
            ${RBX_SCENE_SHADER_SOURCES}
        COMMENT "Compiling the complete scene shader pack for bgfx WebGPU"
        VERBATIM)
endif()

add_custom_command(
    OUTPUT "${RBX_SPIRV_SCENE_SHADER_PACK}"
    COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_SOURCE_DIR}/tools/shader_pipeline/compile_scene_pack.py"
        --source-pack "${CMAKE_CURRENT_SOURCE_DIR}/shaders_glsl3.pack"
        --shader-db "${CMAKE_CURRENT_SOURCE_DIR}/shaders.json"
        --shaderc "${RBX_SHADERC_EXECUTABLE}"
        --source-root "${CMAKE_CURRENT_SOURCE_DIR}/source"
        --platform linux
        --profile spirv
        --preprocessor "${CMAKE_C_COMPILER}"
        --output "${RBX_SPIRV_SCENE_SHADER_PACK}"
    DEPENDS
        "${RBX_SHADERC_DEPENDENCY}"
        "${CMAKE_SOURCE_DIR}/tools/shader_pipeline/compile_scene_pack.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders_glsl3.pack"
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders.json"
        ${RBX_SCENE_SHADER_SOURCES}
    COMMENT "Compiling the complete scene shader pack for bgfx Vulkan/WebGPU"
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

if(CMAKE_CROSSCOMPILING)
    function(rbx_compile_player_ui_shader source kind output_variable)
        get_filename_component(shader_name "${source}" NAME)
        set(outputs)
        set(profiles spirv 120 100_es metal)
        set(platforms linux ios ios ios)
        set(directories spirv glsl essl metal)
        foreach(profile platform directory IN ZIP_LISTS
                profiles platforms directories)
            set(output "${RBX_SHADER_OUTPUT_DIR}/${directory}/${shader_name}.bin")
            add_custom_command(
                OUTPUT "${output}"
                COMMAND "${CMAKE_COMMAND}" -E make_directory
                    "${RBX_SHADER_OUTPUT_DIR}/${directory}"
                COMMAND "${RBX_SHADERC_EXECUTABLE}"
                    -f "${source}"
                    -i "${BGFX_DIR}/src"
                    -i "${CMAKE_CURRENT_SOURCE_DIR}/include"
                    -o "${output}"
                    --platform "${platform}"
                    --type "${kind}"
                    --varyingdef "${RBX_VARYING_DEF}"
                    --profile "${profile}"
                    -O 3
                    --Werror
                MAIN_DEPENDENCY "${source}"
                DEPENDS "${RBX_VARYING_DEF}" "${RBX_SHADERC_DEPENDENCY}"
                VERBATIM)
            list(APPEND outputs "${output}")
        endforeach()
        set(${output_variable} "${outputs}" PARENT_SCOPE)
    endfunction()

    rbx_compile_player_ui_shader(
        "${CMAKE_CURRENT_SOURCE_DIR}/source/vs_player_ui.sc"
        vertex RBX_PLAYER_UI_VERTEX_OUTPUTS)
    rbx_compile_player_ui_shader(
        "${CMAKE_CURRENT_SOURCE_DIR}/source/fs_player_ui.sc"
        fragment RBX_PLAYER_UI_FRAGMENT_OUTPUTS)
else()
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
endif()

add_custom_target(rbx-player-shaders ALL
    DEPENDS
        ${RBX_PLAYER_UI_VERTEX_OUTPUTS}
        ${RBX_PLAYER_UI_FRAGMENT_OUTPUTS}
        "${RBX_SCENE_SHADER_PACK}"
        "${RBX_SPIRV_SCENE_SHADER_PACK}"
        "${RBX_SHADER_OUTPUT_DIR}/shaders.json")

if(WIN32)
    add_custom_target(rbx-d3d11-scene-shaders
        DEPENDS "${RBX_D3D11_SCENE_SHADER_PACK}")
    add_dependencies(rbx-player-shaders rbx-d3d11-scene-shaders)
endif()

if(EMSCRIPTEN)
    add_custom_target(rbx-wgsl-scene-shaders
        DEPENDS "${RBX_WGSL_SCENE_SHADER_PACK}")
    add_dependencies(rbx-player-shaders rbx-wgsl-scene-shaders)
endif()

if(APPLE AND CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    add_custom_target(rbx-metal-scene-shaders DEPENDS "${RBX_METAL_SCENE_SHADER_PACK}")
    add_dependencies(rbx-player-shaders rbx-metal-scene-shaders)
endif()

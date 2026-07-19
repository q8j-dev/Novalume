add_library(rbx-gfxcore-contract STATIC
    src/gfxcore/Device.cpp
    src/gfxcore/DeviceCreate.cpp
    src/gfxcore/Framebuffer.cpp
    src/gfxcore/Geometry.cpp
    src/gfxcore/Resource.cpp
    src/gfxcore/Shader.cpp
    src/gfxcore/States.cpp
    src/gfxcore/Texture.cpp
    src/gfxcore/pix.cpp
    src/bgfx/DeviceBgfx.cpp)
add_library(Roblox::GfxCoreContract ALIAS rbx-gfxcore-contract)
target_include_directories(rbx-gfxcore-contract PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/engine/core/include")
target_include_directories(rbx-gfxcore-contract PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(rbx-gfxcore-contract PRIVATE bgfx)
target_compile_features(rbx-gfxcore-contract PUBLIC cxx_std_17)
if(RBX_ENABLE_LEGACY_RENDERERS)
    target_compile_definitions(rbx-gfxcore-contract PRIVATE RBX_ENABLE_LEGACY_RENDERERS=1)
endif()

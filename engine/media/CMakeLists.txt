add_library(rbx-media-runtime STATIC
    src/MediaDecoder.cpp
    src/MediaPlayer.cpp
    include/media/MediaDecoder.h
    include/media/MediaPlayer.h)
add_library(Roblox::MediaRuntime ALIAS rbx-media-runtime)

target_include_directories(rbx-media-runtime
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(rbx-media-runtime
    PUBLIC Roblox::AudioContracts
    PRIVATE Roblox::AudioRuntime Roblox::FFmpeg)
target_compile_features(rbx-media-runtime PUBLIC cxx_std_20)
rbx_enable_warnings(rbx-media-runtime)

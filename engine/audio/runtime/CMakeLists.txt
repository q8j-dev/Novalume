add_library(rbx-audio-runtime STATIC
    ../src/AudioEngine.cpp
    ../src/FfmpegAudioDecoder.cpp
    ../src/FfmpegAudioDecoder.h
    ../src/MiniaudioImplementation.cpp)
add_library(Roblox::AudioRuntime ALIAS rbx-audio-runtime)

target_include_directories(rbx-audio-runtime
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_include_directories(rbx-audio-runtime SYSTEM PRIVATE
    "${MINIAUDIO_INCLUDE_DIR}")
target_link_libraries(rbx-audio-runtime PUBLIC Roblox::AudioContracts PkgConfig::RBX_FFMPEG)
target_compile_features(rbx-audio-runtime PUBLIC cxx_std_20)
rbx_enable_warnings(rbx-audio-runtime)

# The device is opened explicitly by SoundService; deterministic tests and
# headless tools never call startOutputDevice and therefore remain device-free.

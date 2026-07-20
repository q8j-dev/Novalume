add_library(rbx-platform-linux STATIC SdlHost.cpp)
add_library(Roblox::PlatformHost ALIAS rbx-platform-linux)
target_link_libraries(rbx-platform-linux
    PUBLIC Roblox::PlatformContract
    PRIVATE SDL3::SDL3-static)
target_compile_features(rbx-platform-linux PUBLIC cxx_std_20)
rbx_enable_warnings(rbx-platform-linux)

add_library(rbx-platform-clipboard-linux STATIC Clipboard.cpp)
add_library(Roblox::PlatformClipboard ALIAS rbx-platform-clipboard-linux)
target_link_libraries(rbx-platform-clipboard-linux
    PUBLIC Roblox::PlatformContract
    PRIVATE SDL3::SDL3-static)
target_compile_features(rbx-platform-clipboard-linux PUBLIC cxx_std_20)
rbx_enable_warnings(rbx-platform-clipboard-linux)

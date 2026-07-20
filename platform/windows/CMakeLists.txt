add_library(rbx-platform-windows STATIC WinHost.cpp)
add_library(Roblox::PlatformHost ALIAS rbx-platform-windows)
target_link_libraries(rbx-platform-windows
    PUBLIC Roblox::PlatformContract
    PRIVATE user32 shell32 ole32 shcore)
target_compile_features(rbx-platform-windows PUBLIC cxx_std_20)
target_compile_definitions(rbx-platform-windows PRIVATE
    NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE
    WINVER=0x0A00 _WIN32_WINNT=0x0A00)
rbx_enable_warnings(rbx-platform-windows)

add_library(rbx-platform-clipboard-windows STATIC Clipboard.cpp)
add_library(Roblox::PlatformClipboard ALIAS rbx-platform-clipboard-windows)
target_link_libraries(rbx-platform-clipboard-windows
    PUBLIC Roblox::PlatformContract
    PRIVATE user32)
target_compile_features(rbx-platform-clipboard-windows PUBLIC cxx_std_20)
target_compile_definitions(rbx-platform-clipboard-windows PRIVATE
    NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE
    WINVER=0x0A00 _WIN32_WINNT=0x0A00)
rbx_enable_warnings(rbx-platform-clipboard-windows)

add_library(rbx-platform-windows STATIC WinHost.cpp)
add_library(Roblox::PlatformHost ALIAS rbx-platform-windows)
target_link_libraries(rbx-platform-windows
    PUBLIC Roblox::PlatformContract
    PRIVATE user32 shell32 ole32 shcore xinput9_1_0)
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

rbx_add_portable_platform_runtime(rbx-platform-runtime-windows
    "${CMAKE_SOURCE_DIR}/platform/windows/src/FileSystem.cpp"
    "${CMAKE_SOURCE_DIR}/platform/windows/src/MemoryStats.cpp"
    "${CMAKE_SOURCE_DIR}/platform/windows/src/NamedMutex.cpp"
    "${CMAKE_SOURCE_DIR}/platform/windows/src/WinHeap.cpp"
    "${CMAKE_SOURCE_DIR}/apps/player/shared/client/StringConv.cpp")
add_library(Roblox::PlatformRuntime ALIAS rbx-platform-runtime-windows)
target_link_libraries(rbx-platform-runtime-windows PRIVATE psapi shell32)
target_compile_definitions(rbx-platform-runtime-windows PRIVATE
    NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE
    WINVER=0x0A00 _WIN32_WINNT=0x0A00)

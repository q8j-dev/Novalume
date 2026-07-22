add_library(rbx-platform-web STATIC WebHost.cpp)
add_library(Roblox::PlatformHost ALIAS rbx-platform-web)
target_link_libraries(rbx-platform-web
    PUBLIC Roblox::PlatformContract Roblox::PlatformRecentDocuments)
target_compile_features(rbx-platform-web PUBLIC cxx_std_20)
target_compile_options(rbx-platform-web PRIVATE -pthread)
rbx_enable_warnings(rbx-platform-web)

add_library(rbx-platform-clipboard-web STATIC Clipboard.cpp)
add_library(Roblox::PlatformClipboard ALIAS rbx-platform-clipboard-web)
target_link_libraries(rbx-platform-clipboard-web PUBLIC Roblox::PlatformContract)
target_compile_features(rbx-platform-clipboard-web PUBLIC cxx_std_20)
rbx_enable_warnings(rbx-platform-clipboard-web)

rbx_add_portable_platform_runtime(rbx-platform-runtime-web
    FileSystem.cpp
    MemoryStats.cpp
    "${CMAKE_SOURCE_DIR}/platform/linux/StringConv.cpp")
add_library(Roblox::PlatformRuntime ALIAS rbx-platform-runtime-web)
target_compile_options(rbx-platform-runtime-web PRIVATE -pthread)

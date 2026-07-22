add_library(rbx-platform-ios STATIC IOSHost.mm)
add_library(Roblox::PlatformHost ALIAS rbx-platform-ios)
target_include_directories(rbx-platform-ios PUBLIC include)
target_link_libraries(rbx-platform-ios
    PUBLIC Roblox::PlatformContract Roblox::PlatformRecentDocuments
    PRIVATE
        "-framework GameController"
        "-framework QuartzCore"
        "-framework UIKit"
        "-framework UniformTypeIdentifiers")
target_compile_features(rbx-platform-ios PUBLIC cxx_std_20)
target_compile_definitions(rbx-platform-ios PUBLIC RBX_PLATFORM_IOS=1)
rbx_enable_warnings(rbx-platform-ios)

add_library(rbx-platform-clipboard-ios STATIC Clipboard.mm)
add_library(Roblox::PlatformClipboard ALIAS rbx-platform-clipboard-ios)
target_link_libraries(rbx-platform-clipboard-ios
    PUBLIC Roblox::PlatformContract
    PRIVATE "-framework UIKit")
target_compile_features(rbx-platform-clipboard-ios PUBLIC cxx_std_20)
target_compile_definitions(rbx-platform-clipboard-ios PUBLIC RBX_PLATFORM_IOS=1)
rbx_enable_warnings(rbx-platform-clipboard-ios)

rbx_add_portable_platform_runtime(rbx-platform-runtime-ios
    "${CMAKE_SOURCE_DIR}/platform/macos/src/FileSystem.mm"
    "${CMAKE_SOURCE_DIR}/platform/macos/src/MachOBaseAddr.mm"
    "${CMAKE_SOURCE_DIR}/platform/macos/src/MemoryStats.cpp"
    "${CMAKE_SOURCE_DIR}/platform/macos/src/StringConv.cpp")
add_library(Roblox::PlatformRuntime ALIAS rbx-platform-runtime-ios)
target_compile_definitions(rbx-platform-runtime-ios
    PUBLIC RBX_PLATFORM_IOS=1
    PRIVATE _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR=1)
target_link_libraries(rbx-platform-runtime-ios PRIVATE "-framework Foundation")
target_compile_options(rbx-platform-runtime-ios PRIVATE
    $<$<COMPILE_LANG_AND_ID:CXX,AppleClang>:-Wno-enum-constexpr-conversion>
    $<$<COMPILE_LANG_AND_ID:OBJCXX,AppleClang>:-Wno-enum-constexpr-conversion>)

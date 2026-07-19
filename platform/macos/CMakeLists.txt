add_library(rbx-platform-macos STATIC MacHost.mm)
add_library(Roblox::PlatformHost ALIAS rbx-platform-macos)
target_include_directories(rbx-platform-macos PUBLIC include)
target_link_libraries(rbx-platform-macos
    PUBLIC Roblox::PlatformContract
    PRIVATE "-framework AppKit" "-framework QuartzCore")
target_compile_features(rbx-platform-macos PUBLIC cxx_std_20)
rbx_enable_warnings(rbx-platform-macos)

add_library(rbx-platform-clipboard-macos STATIC Clipboard.mm)
add_library(Roblox::PlatformClipboard ALIAS rbx-platform-clipboard-macos)
target_link_libraries(rbx-platform-clipboard-macos
    PUBLIC Roblox::PlatformContract
    PRIVATE "-framework AppKit")
target_compile_features(rbx-platform-clipboard-macos PUBLIC cxx_std_20)
rbx_enable_warnings(rbx-platform-clipboard-macos)

add_library(rbx-platform-runtime-macos STATIC EXCLUDE_FROM_ALL
    "${CMAKE_SOURCE_DIR}/App/util/FileSystemIndependent.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/MemoryStatsCommon.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/Darwin/FileSystem.mm"
    "${CMAKE_SOURCE_DIR}/App/util/Darwin/MemoryStats.cpp"
    "${CMAKE_SOURCE_DIR}/App/util/Darwin/StringConv.cpp")
add_library(Roblox::PlatformRuntime ALIAS rbx-platform-runtime-macos)
target_include_directories(rbx-platform-runtime-macos PRIVATE
    "${CMAKE_SOURCE_DIR}/App/include"
    "${CMAKE_SOURCE_DIR}/App"
    "${CMAKE_SOURCE_DIR}/engine/core/include"
    "${CMAKE_SOURCE_DIR}/engine/core/math/include"
    "${CMAKE_SOURCE_DIR}/engine/security/include"
    "${CMAKE_SOURCE_DIR}/engine/reflection/include"
    "${CMAKE_SOURCE_DIR}/engine/datamodel/runtime/include"
    "${CMAKE_SOURCE_DIR}/engine/ui/runtime/include"
    "${CMAKE_SOURCE_DIR}/engine/datamodel/tree/include"
    "${CMAKE_SOURCE_DIR}/engine/simulation/include"
    "${CMAKE_SOURCE_DIR}/engine/animation/include"
    "${CMAKE_SOURCE_DIR}/engine/scripting/include"
    "${CMAKE_SOURCE_DIR}/engine/scripting/lua/src"
    "${CMAKE_SOURCE_DIR}/engine/networking/include"
    "${CMAKE_SOURCE_DIR}/engine/rendering/foundation/include"
    "${CMAKE_SOURCE_DIR}/engine/rendering/adorn/include"
    "${CMAKE_SOURCE_DIR}/engine/interaction/tools/include"
    "${CMAKE_SOURCE_DIR}/apps/player/shared/client"
    "${CMAKE_SOURCE_DIR}/third_party/bullet_historical"
    "${CMAKE_SOURCE_DIR}/Library/boost/include"
    "${CMAKE_SOURCE_DIR}/Library/SDL2/include")
target_link_libraries(rbx-platform-runtime-macos PUBLIC Roblox::Core Roblox::ValueTypes)
target_compile_features(rbx-platform-runtime-macos PUBLIC cxx_std_20)
target_compile_definitions(rbx-platform-runtime-macos PRIVATE
    BOOST_NO_CXX98_FUNCTION_BASE=1
    _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR=1)
target_compile_options(rbx-platform-runtime-macos PRIVATE
    $<$<COMPILE_LANG_AND_ID:CXX,AppleClang>:-Wno-enum-constexpr-conversion>
    $<$<COMPILE_LANG_AND_ID:OBJCXX,AppleClang>:-Wno-enum-constexpr-conversion>)

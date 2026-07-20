include(FetchContent)
find_package(PkgConfig REQUIRED)

set(RBX_BGFX_CMAKE_REVISION "572868c0cb952add48019d267223453958e958b8")
set(RBX_BGFX_REVISION "759bdeb936ea95e4ac13d1ba8d4ce2e91c5c17d2")
set(RBX_BX_REVISION "c98e98cde15498250fedbc41ad6da75a896fbae0")
set(RBX_BIMG_REVISION "c3cf9afef058f99846de57050c74b183815d5ee7")
set(RBX_MINIAUDIO_REVISION "9634bedb5b5a2ca38c1ee7108a9358a4e233f14d")
set(RBX_ABSEIL_REVISION "76bb24329e8bf5f39704eb10d21b9a80befa7c81")
set(RBX_PROTOBUF_REVISION "35cd01f9fe9afbeea38cc7b979a3b6bfcde82c03")
set(RBX_GNS_REVISION "4fbfe83ef4d59a12dc32baeff2c33e511af93157")
set(RBX_CURL_REVISION "68720b4837284335b2d63cb358f8f6ce65f5bc55")
set(RBX_ZSTD_REVISION "v1.5.7")
set(RBX_LUAU_REVISION "5e76a0a162d82cbe13a09a4beacd09d7e53cc856")
set(RBX_UTF8PROC_REVISION "e5e799221b45bbb90f5fdc5c69b6b8dfbf017e78") # v2.11.3, Unicode 17
set(RBX_DRACO_REVISION "8786740086a9f4d83f44aa83badfbea4dce7a1b5") # 1.5.7
set(RBX_FREETYPE_REVISION "0a0221a1347e2f1e07c395263540026e9a0aa7c7") # 2.14.3
set(RBX_HARFBUZZ_REVISION "56feae4035bdd48f62ba2b8d8c16232d4d89b3a4") # 14.2.1
set(RBX_SHEENBIDI_REVISION "cfe430e7375a7845b679adae9d51dac6deaa8858") # 3.0.0

# VideoFrame uses the LGPL libav demux/decode/conversion libraries.  Keep GPL
# and nonfree codec add-ons out of the dependency contract; distributable
# platform packages provide the corresponding shared libraries and source.
pkg_check_modules(RBX_FFMPEG REQUIRED IMPORTED_TARGET
    libavformat>=62.0 libavcodec>=62.0 libavutil>=60.0
    libswscale>=9.0 libswresample>=6.0)

if(NOT RBX_FETCH_DEPENDENCIES)
    find_package(bgfx CONFIG REQUIRED)
    find_path(MINIAUDIO_INCLUDE_DIR miniaudio.h REQUIRED)
    find_package(GameNetworkingSockets CONFIG REQUIRED)
    pkg_check_modules(RBX_ZSTD REQUIRED IMPORTED_TARGET libzstd>=1.5.7)
    add_library(Roblox::Zstd ALIAS PkgConfig::RBX_ZSTD)
    find_package(Luau CONFIG REQUIRED)
    find_package(utf8proc CONFIG REQUIRED)
    add_library(Roblox::Utf8proc ALIAS utf8proc::utf8proc)
    find_package(Freetype 2.14.3 REQUIRED)
    add_library(Roblox::Freetype ALIAS Freetype::Freetype)
    pkg_check_modules(RBX_HARFBUZZ REQUIRED IMPORTED_TARGET harfbuzz>=14.2.1)
    add_library(Roblox::HarfBuzz ALIAS PkgConfig::RBX_HARFBUZZ)
    find_package(SheenBidi 3.0.0 CONFIG REQUIRED)
    add_library(Roblox::SheenBidi ALIAS SheenBidi::SheenBidi)
    find_package(draco CONFIG REQUIRED)
    return()
endif()

# Current binary place/model files use Zstandard-compressed chunks while older
# files use LZ4. Pin the codec in the shared dependency graph so every target
# platform reads the same format instead of relying on a host package manager.
set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_STATIC ON CACHE BOOL "" FORCE)
FetchContent_Declare(rbx_zstd
    GIT_REPOSITORY https://github.com/facebook/zstd.git
    GIT_TAG ${RBX_ZSTD_REVISION}
    GIT_SHALLOW FALSE
    SOURCE_SUBDIR build/cmake)
FetchContent_MakeAvailable(rbx_zstd)
add_library(Roblox::Zstd ALIAS libzstd_static)

# The authentic in-experience package contains Luau bytecode rather than Lua
# 5.1 source.  Pin the official VM/compiler revision used by the owned runtime
# migration; command-line tools, analysis, web, and upstream tests are not part
# of the Player dependency graph.
set(LUAU_BUILD_CLI OFF CACHE BOOL "" FORCE)
set(LUAU_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LUAU_BUILD_WEB OFF CACHE BOOL "" FORCE)
set(LUAU_WERROR OFF CACHE BOOL "" FORCE)
FetchContent_Declare(rbx_luau
    GIT_REPOSITORY https://github.com/luau-lang/luau.git
    GIT_TAG ${RBX_LUAU_REVISION}
    GIT_SHALLOW FALSE)
FetchContent_MakeAvailable(rbx_luau)

# Current CorePackages normalize player-entered text before enforcing limits.
# utf8proc provides the same full Unicode normalization tables on every target,
# avoiding platform-specific Foundation/Win32/Java string behavior.
set(UTF8PROC_INSTALL OFF CACHE BOOL "" FORCE)
set(UTF8PROC_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(_RBX_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS OFF)
FetchContent_Declare(rbx_utf8proc
    GIT_REPOSITORY https://github.com/JuliaStrings/utf8proc.git
    GIT_TAG ${RBX_UTF8PROC_REVISION}
    GIT_SHALLOW FALSE)
FetchContent_MakeAvailable(rbx_utf8proc)
set(BUILD_SHARED_LIBS ${_RBX_BUILD_SHARED_LIBS})
unset(_RBX_BUILD_SHARED_LIBS)
add_library(Roblox::Utf8proc ALIAS utf8proc)

# Shape and rasterize text with maintained, immutable upstream libraries on
# every target. FreeType deliberately does not discover HarfBuzz itself; the
# owned renderer drives HarfBuzz's FreeType bridge, avoiding a dependency
# cycle while retaining OpenType shaping and a single glyph rasterizer.
set(_RBX_TEXT_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS OFF)
set(FT_DISABLE_ZLIB TRUE CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 TRUE CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG TRUE CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ TRUE CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI TRUE CACHE BOOL "" FORCE)
set(FT_ENABLE_ERROR_STRINGS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(rbx_freetype
    GIT_REPOSITORY https://github.com/freetype/freetype.git
    GIT_TAG ${RBX_FREETYPE_REVISION}
    GIT_SHALLOW FALSE)
FetchContent_MakeAvailable(rbx_freetype)
add_library(Roblox::Freetype ALIAS freetype)

set(HB_HAVE_FREETYPE ON CACHE BOOL "" FORCE)
set(HB_HAVE_CORETEXT OFF CACHE BOOL "" FORCE)
set(HB_HAVE_GLIB OFF CACHE BOOL "" FORCE)
set(HB_HAVE_ICU OFF CACHE BOOL "" FORCE)
set(HB_BUILD_UTILS OFF CACHE BOOL "" FORCE)
set(HB_BUILD_SUBSET OFF CACHE BOOL "" FORCE)
set(HB_BUILD_RASTER OFF CACHE BOOL "" FORCE)
set(HB_BUILD_VECTOR OFF CACHE BOOL "" FORCE)
set(HB_BUILD_GPU OFF CACHE BOOL "" FORCE)
FetchContent_Declare(rbx_harfbuzz
    GIT_REPOSITORY https://github.com/harfbuzz/harfbuzz.git
    GIT_TAG ${RBX_HARFBUZZ_REVISION}
    GIT_SHALLOW FALSE)
FetchContent_MakeAvailable(rbx_harfbuzz)
add_library(Roblox::HarfBuzz ALIAS harfbuzz)

set(SB_CONFIG_EXPERIMENTAL_TEXT_API OFF CACHE BOOL "" FORCE)
set(SB_CONFIG_UNITY ON CACHE BOOL "" FORCE)
set(BUILD_GENERATOR OFF CACHE BOOL "" FORCE)
FetchContent_Declare(rbx_sheenbidi
    GIT_REPOSITORY https://github.com/Tehreer/SheenBidi.git
    GIT_TAG ${RBX_SHEENBIDI_REVISION}
    GIT_SHALLOW FALSE)
FetchContent_MakeAvailable(rbx_sheenbidi)
add_library(Roblox::SheenBidi ALIAS SheenBidi)
set(BUILD_SHARED_LIBS ${_RBX_TEXT_BUILD_SHARED_LIBS})
unset(_RBX_TEXT_BUILD_SHARED_LIBS)

# FileMesh v7 stores its COREMESH payload as a Google Draco bitstream.  Use
# the official pinned decoder on every platform; encoder tools, JavaScript
# glue, transcoding, plug-ins, tests, and install rules are not part of the
# runtime graph.
set(DRACO_JS_GLUE OFF CACHE BOOL "" FORCE)
set(DRACO_TESTS OFF CACHE BOOL "" FORCE)
set(DRACO_INSTALL OFF CACHE BOOL "" FORCE)
set(DRACO_TRANSCODER_SUPPORTED OFF CACHE BOOL "" FORCE)
set(DRACO_ANIMATION_ENCODING OFF CACHE BOOL "" FORCE)
set(DRACO_MAYA_PLUGIN OFF CACHE BOOL "" FORCE)
set(DRACO_UNITY_PLUGIN OFF CACHE BOOL "" FORCE)
FetchContent_Declare(rbx_draco
    GIT_REPOSITORY https://github.com/google/draco.git
    GIT_TAG ${RBX_DRACO_REVISION}
    GIT_SHALLOW FALSE)
FetchContent_MakeAvailable(rbx_draco)

# SOURCE_SUBDIR deliberately names a missing directory for the three upstream
# source trees: they are populated but only bgfx.cmake defines build targets.
FetchContent_Declare(rbx_bx
    GIT_REPOSITORY https://github.com/bkaradzic/bx.git
    GIT_TAG ${RBX_BX_REVISION}
    GIT_SHALLOW FALSE
    SOURCE_SUBDIR _rbx_populate_only)
FetchContent_Declare(rbx_bimg
    GIT_REPOSITORY https://github.com/bkaradzic/bimg.git
    GIT_TAG ${RBX_BIMG_REVISION}
    GIT_SHALLOW FALSE
    SOURCE_SUBDIR _rbx_populate_only)
FetchContent_Declare(rbx_bgfx
    GIT_REPOSITORY https://github.com/bkaradzic/bgfx.git
    GIT_TAG ${RBX_BGFX_REVISION}
    GIT_SHALLOW FALSE
    SOURCE_SUBDIR _rbx_populate_only)
FetchContent_Declare(rbx_miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG ${RBX_MINIAUDIO_REVISION}
    GIT_SHALLOW FALSE
    SOURCE_SUBDIR _rbx_populate_only)
FetchContent_Declare(rbx_abseil
    GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
    GIT_TAG ${RBX_ABSEIL_REVISION}
    GIT_SHALLOW FALSE)
FetchContent_Declare(rbx_protobuf
    GIT_REPOSITORY https://github.com/protocolbuffers/protobuf.git
    GIT_TAG ${RBX_PROTOBUF_REVISION}
    GIT_SHALLOW FALSE)

set(protobuf_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(protobuf_LOCAL_DEPENDENCIES_ONLY ON CACHE BOOL "" FORCE)
if(CMAKE_CROSSCOMPILING)
    set(RBX_HOST_PROTOC "" CACHE FILEPATH "Host protoc executable used while cross-compiling")
    if(NOT EXISTS "${RBX_HOST_PROTOC}")
        message(FATAL_ERROR "Cross-compiling requires -DRBX_HOST_PROTOC=/absolute/path/to/a host protoc 35.1 executable")
    endif()
    set(protobuf_BUILD_PROTOC_BINARIES OFF CACHE BOOL "" FORCE)
else()
    set(protobuf_BUILD_PROTOC_BINARIES ON CACHE BOOL "" FORCE)
endif()
set(protobuf_INSTALL OFF CACHE BOOL "" FORCE)
set(protobuf_WITH_ZLIB OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(rbx_bx rbx_bimg rbx_bgfx rbx_miniaudio rbx_abseil rbx_protobuf)

set(RBX_PROTOBUF_SOURCE_DIR "${rbx_protobuf_SOURCE_DIR}" CACHE INTERNAL "")
set(RBX_PROTOBUF_BINARY_DIR "${rbx_protobuf_BINARY_DIR}" CACHE INTERNAL "")

set(BX_DIR "${rbx_bx_SOURCE_DIR}" CACHE PATH "Pinned bx source" FORCE)
set(BIMG_DIR "${rbx_bimg_SOURCE_DIR}" CACHE PATH "Pinned bimg source" FORCE)
set(BGFX_DIR "${rbx_bgfx_SOURCE_DIR}" CACHE PATH "Pinned bgfx source" FORCE)
set(MINIAUDIO_INCLUDE_DIR "${rbx_miniaudio_SOURCE_DIR}" CACHE PATH
    "Pinned miniaudio include directory" FORCE)

# GameNetworkingSockets is the native transport for the Player.  Keep its
# public types behind engine/networking contracts so browser and future host
# adapters do not leak third-party socket types into replication code.
set(BUILD_STATIC_LIB ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIB OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(ENABLE_ICE OFF CACHE BOOL "" FORCE)
set(USE_STEAMWEBRTC OFF CACHE BOOL "" FORCE)
if(WIN32)
    # Use the Windows SDK crypto provider.  This avoids shipping or discovering
    # an unrelated host OpenSSL build on Windows x64 and arm64.
    set(USE_CRYPTO BCrypt CACHE STRING "" FORCE)
else()
    set(USE_CRYPTO OpenSSL CACHE STRING "" FORCE)
    set(OPENSSL_USE_STATIC_LIBS TRUE CACHE BOOL "" FORCE)
    set(RBX_OPENSSL_ROOT "" CACHE PATH
        "Pinned OpenSSL 3.5.7 installation built for the target platform and architecture")
    if(NOT EXISTS "${RBX_OPENSSL_ROOT}/include/openssl/evp.h" OR
       NOT EXISTS "${RBX_OPENSSL_ROOT}/lib/libcrypto.a")
        message(FATAL_ERROR
            "GameNetworkingSockets requires a target OpenSSL 3.5.7 build. "
            "Set -DRBX_OPENSSL_ROOT=/absolute/path/to/openssl; Apple builds can create one "
            "with tools/dependencies/build-openssl.sh.")
    endif()
    set(OPENSSL_ROOT_DIR "${RBX_OPENSSL_ROOT}" CACHE PATH "" FORCE)
    set(OPENSSL_INCLUDE_DIR "${RBX_OPENSSL_ROOT}/include" CACHE PATH "" FORCE)
    set(OPENSSL_CRYPTO_LIBRARY "${RBX_OPENSSL_ROOT}/lib/libcrypto.a" CACHE FILEPATH "" FORCE)
    set(OPENSSL_SSL_LIBRARY "${RBX_OPENSSL_ROOT}/lib/libssl.a" CACHE FILEPATH "" FORCE)

    # FindOpenSSL cannot infer every private dependency when callers pin a
    # static archive through OPENSSL_CRYPTO_LIBRARY.  Establish the imported
    # targets here and carry the platform thread/dynamic-loader requirements
    # with libcrypto so both configure-time symbol probes and final consumers
    # use the same complete link contract.
    find_package(OpenSSL 3.5.7 EXACT REQUIRED COMPONENTS Crypto SSL)
    find_package(Threads REQUIRED)
    set_property(TARGET OpenSSL::Crypto APPEND PROPERTY
        INTERFACE_LINK_LIBRARIES Threads::Threads)
    if(CMAKE_DL_LIBS)
        set_property(TARGET OpenSSL::Crypto APPEND PROPERTY
            INTERFACE_LINK_LIBRARIES "${CMAKE_DL_LIBS}")
    endif()
endif()
set(Protobuf_DIR "${rbx_protobuf_BINARY_DIR}" CACHE PATH "" FORCE)

FetchContent_Declare(rbx_gns
    GIT_REPOSITORY https://github.com/ValveSoftware/GameNetworkingSockets.git
    GIT_TAG ${RBX_GNS_REVISION}
    GIT_SHALLOW FALSE
    SOURCE_SUBDIR _rbx_populate_only)
FetchContent_MakeAvailable(rbx_gns)

# Keep HTTP on a current, pinned libcurl and the same target-architecture TLS
# provider as networking. Only the library and HTTP(S) protocols are needed by
# the Player; command-line tools, tests, examples, and non-HTTP protocols stay
# out of the default graph.
set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
set(BUILD_LIBCURL_DOCS OFF CACHE BOOL "" FORCE)
set(BUILD_MISC_DOCS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(CURL_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
set(CURL_ENABLE_EXPORT_TARGET OFF CACHE BOOL "" FORCE)
set(HTTP_ONLY ON CACHE BOOL "" FORCE)
set(CURL_ZLIB ON CACHE BOOL "" FORCE)
set(CURL_BROTLI OFF CACHE BOOL "" FORCE)
set(CURL_ZSTD OFF CACHE BOOL "" FORCE)
set(CURL_USE_LIBPSL OFF CACHE BOOL "" FORCE)
if(WIN32)
    set(CURL_USE_SCHANNEL ON CACHE BOOL "" FORCE)
    set(CURL_USE_OPENSSL OFF CACHE BOOL "" FORCE)
else()
    set(CURL_USE_SCHANNEL OFF CACHE BOOL "" FORCE)
    set(CURL_USE_OPENSSL ON CACHE BOOL "" FORCE)
endif()
FetchContent_Declare(rbx_curl
    GIT_REPOSITORY https://github.com/curl/curl.git
    GIT_TAG ${RBX_CURL_REVISION}
    GIT_SHALLOW FALSE)
FetchContent_MakeAvailable(rbx_curl)

include("${CMAKE_CURRENT_LIST_DIR}/patches/GameNetworkingSocketsPlatforms.cmake")
rbx_patch_gamenetworkingsockets_platforms("${rbx_gns_SOURCE_DIR}")
set(_rbx_skip_install_rules "${CMAKE_SKIP_INSTALL_RULES}")
set(CMAKE_SKIP_INSTALL_RULES TRUE)
add_subdirectory("${rbx_gns_SOURCE_DIR}" "${rbx_gns_BINARY_DIR}")
set(CMAKE_SKIP_INSTALL_RULES "${_rbx_skip_install_rules}")
unset(_rbx_skip_install_rules)

set(BGFX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS ON CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_BIN2C ON CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_SHADER ON CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_GEOMETRY OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_TEXTURE OFF CACHE BOOL "" FORCE)
set(BGFX_INSTALL OFF CACHE BOOL "" FORCE)
set(BGFX_CUSTOM_TARGETS OFF CACHE BOOL "" FORCE)
set(BGFX_CONFIG_RENDERER_METAL ON CACHE BOOL "" FORCE)

FetchContent_Declare(rbx_bgfx_cmake
    GIT_REPOSITORY https://github.com/bkaradzic/bgfx.cmake.git
    GIT_TAG ${RBX_BGFX_CMAKE_REVISION}
    GIT_SHALLOW FALSE
    GIT_SUBMODULES "")
FetchContent_MakeAvailable(rbx_bgfx_cmake)

include("${rbx_bgfx_cmake_SOURCE_DIR}/cmake/bgfxToolUtils.cmake")

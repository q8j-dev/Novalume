set(RBX_BOOST_ROOT "${CMAKE_SOURCE_DIR}/Library/boost")

add_library(rbx-boost-runtime STATIC
    "${RBX_BOOST_ROOT}/libs/filesystem/src/codecvt_error_category.cpp"
    "${RBX_BOOST_ROOT}/libs/filesystem/src/directory.cpp"
    "${RBX_BOOST_ROOT}/libs/filesystem/src/exception.cpp"
    "${RBX_BOOST_ROOT}/libs/filesystem/src/operations.cpp"
    "${RBX_BOOST_ROOT}/libs/filesystem/src/path.cpp"
    "${RBX_BOOST_ROOT}/libs/filesystem/src/path_traits.cpp"
    "${RBX_BOOST_ROOT}/libs/filesystem/src/portability.cpp"
    "${RBX_BOOST_ROOT}/libs/filesystem/src/unique_path.cpp"
    "${RBX_BOOST_ROOT}/libs/filesystem/src/utf8_codecvt_facet.cpp"
    "${RBX_BOOST_ROOT}/libs/filesystem/src/windows_file_codecvt.cpp"
    "${RBX_BOOST_ROOT}/libs/iostreams/src/gzip.cpp"
    "${RBX_BOOST_ROOT}/libs/iostreams/src/zlib.cpp")

if(WIN32)
    target_sources(rbx-boost-runtime PRIVATE
        "${RBX_BOOST_ROOT}/libs/thread/src/win32/thread.cpp"
        "${RBX_BOOST_ROOT}/libs/thread/src/win32/tss_dll.cpp"
        "${RBX_BOOST_ROOT}/libs/thread/src/win32/tss_pe.cpp"
        "${RBX_BOOST_ROOT}/libs/thread/src/win32/thread_primitives.cpp"
        "${RBX_BOOST_ROOT}/libs/thread/src/future.cpp")
    target_compile_definitions(rbx-boost-runtime PRIVATE BOOST_THREAD_WIN32=1)
else()
    target_sources(rbx-boost-runtime PRIVATE
        "${RBX_BOOST_ROOT}/libs/thread/src/pthread/thread.cpp"
        "${RBX_BOOST_ROOT}/libs/thread/src/pthread/once.cpp"
        "${RBX_BOOST_ROOT}/libs/thread/src/future.cpp")
    target_compile_definitions(rbx-boost-runtime PRIVATE
        BOOST_THREAD_POSIX=1 BOOST_THREAD_DONT_USE_CHRONO=1)
endif()

add_library(Roblox::BoostRuntime ALIAS rbx-boost-runtime)
find_package(Threads REQUIRED)
if(EMSCRIPTEN)
    add_library(rbx-emscripten-zlib INTERFACE)
    add_library(ZLIB::ZLIB ALIAS rbx-emscripten-zlib)
    target_compile_options(rbx-emscripten-zlib INTERFACE "SHELL:-sUSE_ZLIB=1")
    target_link_options(rbx-emscripten-zlib INTERFACE "SHELL:-sUSE_ZLIB=1")
else()
    find_package(ZLIB REQUIRED)
endif()
target_include_directories(rbx-boost-runtime PUBLIC
    "${RBX_BOOST_ROOT}/include")
target_compile_features(rbx-boost-runtime PUBLIC cxx_std_20)
target_compile_definitions(rbx-boost-runtime PRIVATE
    BOOST_FILESYSTEM_SOURCE=1
    BOOST_IOSTREAMS_SOURCE=1
    BOOST_THREAD_BUILD_LIB=1)
target_link_libraries(rbx-boost-runtime PUBLIC Threads::Threads ZLIB::ZLIB)

if(CMAKE_CXX_COMPILER_ID MATCHES "AppleClang|Clang")
    target_compile_options(rbx-boost-runtime PRIVATE
        -Wno-enum-constexpr-conversion)
endif()
if(APPLE)
    target_compile_definitions(rbx-boost-runtime PRIVATE
        _LIBCPP_ENABLE_CXX17_REMOVED_UNARY_BINARY_FUNCTION=1)
endif()

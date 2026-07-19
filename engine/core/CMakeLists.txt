set(RBX_CORE_SOURCES
    src/BuildInfo.cpp
    src/EngineFeatures.cpp
    src/RbxAssert.cpp
    src/RbxFormat.cpp
    src/cpucount.cpp
    src/rbx/CEvent.cpp
    src/rbx/MathUtil.cpp
    src/rbx/Memory.cpp
    src/rbx/ProcessPerfCounter.cpp
    src/rbx/Profiler.cpp
    src/rbx/Signal.cpp
    src/rbx/TaskScheduler.Job.cpp
    src/rbx/TaskScheduler.Thread.cpp
    src/rbx/TaskScheduler.cpp
    src/rbx/Tasks/Coordinator.cpp
    src/rbx/ThreadSafe.cpp
    src/rbx/Time.cpp
    src/rbx/boost.cpp
    src/util/Debug.cpp
    src/util/Log.cpp
    src/util/RbxCrash.cpp
    src/util/StreamHelpers.cpp
    include/rbx/RbxDbgInfo.cpp)

if(APPLE)
    list(APPEND RBX_CORE_SOURCES
        src/rbx/Darwin/SystemUtil.cpp
        src/rbx/Unix/Crypt.cpp)
elseif(ANDROID)
    list(APPEND RBX_CORE_SOURCES
        src/rbx/Android/SystemUtil.cpp
        src/rbx/Android/ifaddrs.c)
elseif(WIN32)
    list(APPEND RBX_CORE_SOURCES
        src/rbx/Crypt.cpp
        src/rbx/Win/SystemUtil.cpp
        src/util/RegistryUtil.cpp)
endif()

add_library(rbx-core STATIC ${RBX_CORE_SOURCES})
add_library(Roblox::Core ALIAS rbx-core)
target_include_directories(rbx-core
    PUBLIC
        include
        "${CMAKE_SOURCE_DIR}/Library/boost/include")
target_compile_features(rbx-core PUBLIC cxx_std_20)
rbx_enable_warnings(rbx-core)

# The vendored Boost headers predate Clang's enum constant range diagnostic.
# Keep the compatibility suppression local to this transitional dependency;
# owned engine code remains under the normal warning policy.
if(CMAKE_CXX_COMPILER_ID MATCHES "AppleClang|Clang")
    target_compile_options(rbx-core PRIVATE -Wno-enum-constexpr-conversion)
endif()

if(APPLE)
    # The 2016 Boost snapshot still derives hash adapters from these removed
    # standard-library base classes. This is confined to the compatibility
    # boundary until the pinned Boost replacement is complete.
    target_compile_definitions(rbx-core PRIVATE
        _LIBCPP_ENABLE_CXX17_REMOVED_UNARY_BINARY_FUNCTION)
endif()

add_subdirectory(math)

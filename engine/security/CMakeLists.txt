add_library(rbx-security STATIC
    src/FuzzyTokens.cpp
    src/SecurityContext.cpp)
add_library(Roblox::Security ALIAS rbx-security)

add_library(rbx-protection-markers INTERFACE)
add_library(Roblox::ProtectionMarkers ALIAS rbx-protection-markers)
target_sources(rbx-protection-markers INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/include/security/ProtectionMarkers.h")
target_include_directories(rbx-protection-markers INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/include")

if(RBX_ENABLE_VMPROTECT)
    if(NOT WIN32)
        message(FATAL_ERROR "RBX_ENABLE_VMPROTECT is only supported by Windows builds")
    endif()
    if(NOT EXISTS "${RBX_VMPROTECT_INCLUDE_DIR}/VMProtect/VMProtectSDK.h")
        message(FATAL_ERROR
            "RBX_ENABLE_VMPROTECT requires RBX_VMPROTECT_INCLUDE_DIR to contain "
            "VMProtect/VMProtectSDK.h")
    endif()
    if(NOT EXISTS "${RBX_VMPROTECT_LIBRARY}" OR
       IS_DIRECTORY "${RBX_VMPROTECT_LIBRARY}")
        message(FATAL_ERROR
            "RBX_ENABLE_VMPROTECT requires RBX_VMPROTECT_LIBRARY to name the "
            "target-architecture vendor import library")
    endif()
    get_filename_component(RBX_VMPROTECT_LIBRARY_DIR
        "${RBX_VMPROTECT_LIBRARY}" DIRECTORY)
    target_compile_definitions(rbx-protection-markers INTERFACE
        RBX_ENABLE_VMPROTECT=1)
    target_include_directories(rbx-protection-markers SYSTEM INTERFACE
        "${RBX_VMPROTECT_INCLUDE_DIR}")
    # The historical vendor header also emits a default-library directive, so
    # expose the validated library directory in addition to linking its exact
    # target-architecture import library.
    target_link_directories(rbx-protection-markers INTERFACE
        "${RBX_VMPROTECT_LIBRARY_DIR}")
    target_link_libraries(rbx-protection-markers INTERFACE
        "${RBX_VMPROTECT_LIBRARY}")
endif()

target_include_directories(rbx-security
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE
        "${CMAKE_SOURCE_DIR}/App/include"
        "${CMAKE_SOURCE_DIR}/Library/boost/include")
target_link_libraries(rbx-security PUBLIC Roblox::Core Roblox::Math)
target_compile_features(rbx-security PUBLIC cxx_std_20)
target_compile_definitions(rbx-security PRIVATE
    BOOST_NO_CXX98_FUNCTION_BASE=1
    _LIBCPP_ENABLE_CXX17_REMOVED_UNARY_BINARY_FUNCTION=1)
target_compile_options(rbx-security PRIVATE
    $<$<COMPILE_LANG_AND_ID:CXX,AppleClang>:-Wno-enum-constexpr-conversion>)
rbx_enable_warnings(rbx-security)

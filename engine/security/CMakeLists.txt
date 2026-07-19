add_library(rbx-security STATIC
    src/FuzzyTokens.cpp
    src/SecurityContext.cpp)
add_library(Roblox::Security ALIAS rbx-security)

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

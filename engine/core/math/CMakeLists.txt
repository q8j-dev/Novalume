file(GLOB RBX_G3D_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/g3d/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/rbx/*.cpp")
file(GLOB RBX_G3D_JPEG_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/Rendering/g3d/ijg/*.c")
file(GLOB RBX_G3D_PNG_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/Rendering/g3d/png/*.c")
file(GLOB RBX_G3D_ZIP_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/Rendering/g3d/zip/*.c")
list(FILTER RBX_G3D_JPEG_SOURCES EXCLUDE REGEX "/(jidctred|jdphuff|jmemansi|jcphuff)\\.c$")

add_library(rbx-math STATIC EXCLUDE_FROM_ALL
    ${RBX_G3D_SOURCES}
    ${RBX_G3D_JPEG_SOURCES}
    ${RBX_G3D_PNG_SOURCES}
    ${RBX_G3D_ZIP_SOURCES})
add_library(Roblox::Math ALIAS rbx-math)
find_package(ZLIB REQUIRED)

target_include_directories(rbx-math
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_SOURCE_DIR}/third_party/bullet_historical"
    PRIVATE
        "${CMAKE_SOURCE_DIR}/Rendering/g3d/include"
        "${CMAKE_SOURCE_DIR}/Rendering/g3d/include/png"
        "${CMAKE_SOURCE_DIR}/Rendering/g3d/include/zip"
        "${CMAKE_SOURCE_DIR}/Rendering/g3d/ijg"
        "${CMAKE_SOURCE_DIR}/engine/core/include"
        "${CMAKE_SOURCE_DIR}/engine/core/include"
        "${CMAKE_SOURCE_DIR}/App/include"
        "${CMAKE_SOURCE_DIR}/Library/boost/include")
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(RBX_G3D_CASE_COMPAT_DIR
        "${CMAKE_CURRENT_BINARY_DIR}/case-sensitive-include")
    file(MAKE_DIRECTORY "${RBX_G3D_CASE_COMPAT_DIR}")
    if(NOT EXISTS "${RBX_G3D_CASE_COMPAT_DIR}/G3D")
        file(CREATE_LINK "${CMAKE_CURRENT_SOURCE_DIR}/include/g3d"
            "${RBX_G3D_CASE_COMPAT_DIR}/G3D" SYMBOLIC)
    endif()
    target_include_directories(rbx-math BEFORE PUBLIC
        "${RBX_G3D_CASE_COMPAT_DIR}")
endif()
target_link_libraries(rbx-math PUBLIC Roblox::Core ZLIB::ZLIB)
target_compile_features(rbx-math PUBLIC cxx_std_20)
target_compile_definitions(rbx-math PRIVATE
    BOOST_NO_CXX98_FUNCTION_BASE=1
    _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR=1)
target_compile_options(rbx-math PRIVATE
    $<$<COMPILE_LANG_AND_ID:CXX,AppleClang>:-Wno-deprecated-declarations>)

if(APPLE AND CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_compile_definitions(rbx-math PRIVATE __FP__=1)
    target_link_libraries(rbx-math PUBLIC "-framework CoreServices")
endif()

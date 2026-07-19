file(GLOB_RECURSE RBX_BULLET_COLLISION_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/third_party/bullet_historical/BulletCollision/*.cpp")
file(GLOB RBX_BULLET_LINEAR_MATH_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/third_party/bullet_historical/LinearMath/*.cpp")
file(GLOB RBX_BULLET_CONVEX_DECOMPOSITION_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/third_party/bullet_historical/Extras/ConvexDecomposition/*.cpp")
set(RBX_BULLET_GIMPACT_UTILITY_SOURCES
    "${CMAKE_SOURCE_DIR}/third_party/bullet_historical/Extras/GIMPACTUtils/btGImpactConvexDecompositionShape.cpp")

add_library(rbx-physics STATIC EXCLUDE_FROM_ALL
    ${RBX_BULLET_COLLISION_SOURCES}
    ${RBX_BULLET_LINEAR_MATH_SOURCES}
    ${RBX_BULLET_CONVEX_DECOMPOSITION_SOURCES}
    ${RBX_BULLET_GIMPACT_UTILITY_SOURCES})
add_library(Roblox::Physics ALIAS rbx-physics)

target_include_directories(rbx-physics PUBLIC "${CMAKE_SOURCE_DIR}/third_party/bullet_historical")
target_compile_features(rbx-physics PUBLIC cxx_std_20)
target_compile_options(rbx-physics PRIVATE
    $<$<COMPILE_LANG_AND_ID:CXX,AppleClang>:-Wno-deprecated-declarations>)

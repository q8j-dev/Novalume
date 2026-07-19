include(App)

if(ANDROID)
add_definitions(-DGLEW_NO_GLU)
endif(ANDROID)

include_directories("${CMAKE_SOURCE_DIR}/Rendering/g3d/include")
include_directories("${CMAKE_SOURCE_DIR}/engine/core/math/include")
include_directories("${CMAKE_SOURCE_DIR}/engine/rendering/foundation/include")
include_directories("${CMAKE_SOURCE_DIR}/Rendering/RbxG3D/include")
include_directories("${CMAKE_SOURCE_DIR}/engine/rendering/adorn/include")
include_directories("${CMAKE_SOURCE_DIR}/Rendering/GfxCore/GL")
include_directories("${CMAKE_SOURCE_DIR}/Rendering/GfxCore/include")

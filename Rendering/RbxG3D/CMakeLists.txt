include(Boost)
include(App)

include_project_files(RbxG3D "*")

include_directories(include)
include_directories(${CMAKE_SOURCE_DIR}/engine/core/math/include)
include_directories(../g3d/include)

list(APPEND HEADERS ${CMAKE_SOURCE_DIR}/engine/core/math/include/RbxG3D/RbxRay.h)
list(APPEND HEADERS ${CMAKE_SOURCE_DIR}/engine/core/math/include/RbxG3D/RbxTime.h)
list(APPEND HEADERS ${CMAKE_SOURCE_DIR}/engine/core/math/include/RbxG3D/Frustum.h)
list(APPEND HEADERS ${CMAKE_SOURCE_DIR}/engine/core/math/include/RbxG3D/RbxCamera.h)

list(APPEND SOURCES ${CMAKE_SOURCE_DIR}/engine/core/math/src/rbx/RbxCamera.cpp)
list(APPEND SOURCES ${CMAKE_SOURCE_DIR}/engine/core/math/src/rbx/Frustum.cpp)
list(APPEND SOURCES ${CMAKE_SOURCE_DIR}/engine/core/math/src/rbx/RbxRay.cpp)

add_library(RbxG3D OBJECT ${SOURCES} ${HEADERS})

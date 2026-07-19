include(App)

include_directories(${CMAKE_SOURCE_DIR}/engine/rendering/adorn/include)
include_directories(../g3d/include)
include_directories(../RbxG3D/include)
include_directories(${CMAKE_SOURCE_DIR}/engine/rendering/foundation/include)

list(APPEND HEADERS ${CMAKE_SOURCE_DIR}/engine/rendering/adorn/include/AppDraw/Draw.h)
list(APPEND HEADERS ${CMAKE_SOURCE_DIR}/engine/rendering/adorn/include/AppDraw/HitTest.h)
list(APPEND HEADERS ${CMAKE_SOURCE_DIR}/engine/rendering/adorn/include/AppDraw/DrawAdorn.h)
list(APPEND HEADERS ${CMAKE_SOURCE_DIR}/engine/rendering/adorn/include/AppDraw/HandleType.h)
list(APPEND HEADERS ${CMAKE_SOURCE_DIR}/engine/rendering/adorn/include/AppDraw/DrawPrimitives.h)

list(APPEND SOURCES ${CMAKE_SOURCE_DIR}/engine/rendering/adorn/src/Draw.cpp)
list(APPEND SOURCES ${CMAKE_SOURCE_DIR}/engine/rendering/adorn/src/DrawAdorn.cpp)
list(APPEND SOURCES ${CMAKE_SOURCE_DIR}/engine/rendering/adorn/src/HitTest.cpp)

add_library(AppDraw OBJECT ${SOURCES} ${HEADERS})

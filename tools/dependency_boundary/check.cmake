file(GLOB_RECURSE shared_headers
    "${SOURCE_ROOT}/engine/*.h"
    "${SOURCE_ROOT}/engine/*.hpp"
    "${SOURCE_ROOT}/platform/common/*.h"
    "${SOURCE_ROOT}/platform/common/*.hpp")

set(forbidden
    "AppKit/" "Cocoa/" "Metal/" "UIKit/" "windows.h" "d3d11.h"
    "jni.h" "android/" "X11/" "wayland-" "emscripten/")

set(violations "")
foreach(header IN LISTS shared_headers)
    file(READ "${header}" contents)
    foreach(token IN LISTS forbidden)
        string(FIND "${contents}" "${token}" found)
        if(NOT found EQUAL -1)
            list(APPEND violations "${header}: forbidden platform token '${token}'")
        endif()
    endforeach()
endforeach()

if(violations)
    list(JOIN violations "\n" message)
    message(FATAL_ERROR "Dependency boundary violations:\n${message}")
endif()

message(STATUS "Dependency boundaries: ${shared_headers}")

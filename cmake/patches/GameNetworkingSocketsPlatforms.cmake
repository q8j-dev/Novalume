function(rbx_patch_gamenetworkingsockets_platforms source_dir)
    set(cmake_file "${source_dir}/CMakeLists.txt")
    file(READ "${cmake_file}" contents)
    if(NOT contents MATCHES "PUBLIC OSX IOS")
        string(REPLACE
            "elseif(CMAKE_SYSTEM_NAME MATCHES Darwin)\n\t\ttarget_compile_definitions(\${TGT} PUBLIC OSX)"
            "elseif(CMAKE_SYSTEM_NAME MATCHES Darwin)\n\t\ttarget_compile_definitions(\${TGT} PUBLIC OSX)\n\telseif(CMAKE_SYSTEM_NAME MATCHES iOS)\n\t\ttarget_compile_definitions(\${TGT} PUBLIC OSX IOS)\n\telseif(ANDROID OR CMAKE_SYSTEM_NAME MATCHES Android)\n\t\ttarget_compile_definitions(\${TGT} PUBLIC ANDROID)"
            patched "${contents}")

        if(patched STREQUAL contents)
            message(FATAL_ERROR "Pinned GameNetworkingSockets platform-definition patch no longer applies; audit the new upstream revision")
        endif()
        file(WRITE "${cmake_file}" "${patched}")
    endif()

    set(source_cmake_file "${source_dir}/src/CMakeLists.txt")
    file(READ "${source_cmake_file}" source_contents)
    if(NOT source_contents MATCHES "CMAKE_SYSTEM_NAME MATCHES iOS")
        string(REPLACE
            "elseif(CMAKE_SYSTEM_NAME MATCHES OpenBSD)\n\n\telseif(CMAKE_SYSTEM_NAME MATCHES Windows)"
            "elseif(CMAKE_SYSTEM_NAME MATCHES OpenBSD)\n\n\telseif(CMAKE_SYSTEM_NAME MATCHES iOS)\n\n\telseif(ANDROID OR CMAKE_SYSTEM_NAME MATCHES Android)\n\n\telseif(CMAKE_SYSTEM_NAME MATCHES Windows)"
            source_patched "${source_contents}")
        if(source_patched STREQUAL source_contents)
            message(FATAL_ERROR "Pinned GameNetworkingSockets target-OS patch no longer applies; audit the new upstream revision")
        endif()
        file(WRITE "${source_cmake_file}" "${source_patched}")
    endif()

    set(debug_source "${source_dir}/src/tier0/dbg.cpp")
    file(READ "${debug_source}" debug_contents)
    if(NOT debug_contents MATCHES "defined\\(IOS\\)")
        string(REPLACE
            "#elif IsNintendoSwitch()\n\treturn false;\n#else\n\t#error \"HALP\""
            "#elif IsNintendoSwitch()\n\treturn false;\n#elif defined(IOS) || IsAndroid()\n\treturn false;\n#else\n\t#error \"HALP\""
            debug_patched "${debug_contents}")
        if(debug_patched STREQUAL debug_contents)
            message(FATAL_ERROR "Pinned GameNetworkingSockets mobile debugger patch no longer applies; audit the new upstream revision")
        endif()
        file(WRITE "${debug_source}" "${debug_patched}")
    endif()
endfunction()

# Bridge an in-tree pinned protobuf build to dependencies that still use
# CMake's legacy FindProtobuf module API.
if(TARGET protobuf::libprotobuf AND (TARGET protobuf::protoc OR EXISTS "${RBX_HOST_PROTOC}"))
    set(Protobuf_FOUND TRUE)
    set(PROTOBUF_FOUND TRUE)
    set(Protobuf_VERSION "35.1.0")
    set(Protobuf_LIBRARIES protobuf::libprotobuf)
    set(PROTOBUF_LIBRARIES protobuf::libprotobuf)
    if(CMAKE_CROSSCOMPILING)
        set(Protobuf_PROTOC_EXECUTABLE "${RBX_HOST_PROTOC}")
        set(PROTOBUF_PROTOC_EXECUTABLE "${RBX_HOST_PROTOC}")
        set(protobuf_PROTOC_EXE "${RBX_HOST_PROTOC}")
    else()
        set(Protobuf_PROTOC_EXECUTABLE protobuf::protoc)
        set(PROTOBUF_PROTOC_EXECUTABLE protobuf::protoc)
    endif()
    set(Protobuf_INCLUDE_DIR "${RBX_PROTOBUF_SOURCE_DIR}/src")
    set(Protobuf_INCLUDE_DIRS
        "${RBX_PROTOBUF_SOURCE_DIR}/src"
        "${RBX_PROTOBUF_BINARY_DIR}")
    set(PROTOBUF_INCLUDE_DIRS "${Protobuf_INCLUDE_DIRS}")

    include("${RBX_PROTOBUF_SOURCE_DIR}/cmake/protobuf-generate.cmake")

    function(protobuf_generate_cpp SRCS HDRS)
        cmake_parse_arguments(PGC "" "EXPORT_MACRO" "" ${ARGN})
        set(_proto_files ${PGC_UNPARSED_ARGUMENTS})
        protobuf_generate(
            APPEND_PATH
            LANGUAGE cpp
            OUT_VAR _generated
            PROTOC_EXE "${Protobuf_PROTOC_EXECUTABLE}"
            PROTOS ${_proto_files})
        set(_sources)
        set(_headers)
        foreach(_file IN LISTS _generated)
            if(_file MATCHES "\\.cc$")
                list(APPEND _sources "${_file}")
            elseif(_file MATCHES "\\.h$")
                list(APPEND _headers "${_file}")
            endif()
        endforeach()
        set(${SRCS} "${_sources}" PARENT_SCOPE)
        set(${HDRS} "${_headers}" PARENT_SCOPE)
    endfunction()
    return()
endif()

include("${CMAKE_ROOT}/Modules/FindProtobuf.cmake")

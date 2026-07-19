add_library(rbx-lz4 STATIC
    lz4.c
    lz4hc.c)
add_library(Roblox::Lz4 ALIAS rbx-lz4)

target_include_directories(rbx-lz4 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
set_target_properties(rbx-lz4 PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF)

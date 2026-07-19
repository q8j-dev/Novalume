add_library(rbx-ui-input STATIC
    "${CMAKE_SOURCE_DIR}/engine/core/include/rbx/ui/PreferredInput.h"
    src/PreferredInput.cpp)
add_library(Roblox::UIInput ALIAS rbx-ui-input)

target_include_directories(rbx-ui-input PUBLIC
    "${CMAKE_SOURCE_DIR}/engine/core/include")
target_compile_features(rbx-ui-input PUBLIC cxx_std_20)
rbx_enable_warnings(rbx-ui-input)

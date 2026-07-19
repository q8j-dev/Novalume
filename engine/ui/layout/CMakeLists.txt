add_library(rbx-ui-layout STATIC
    include/rbx/ui/ScreenLayout.h
    src/ScreenLayout.cpp)
add_library(Roblox::UILayout ALIAS rbx-ui-layout)

target_include_directories(rbx-ui-layout PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_features(rbx-ui-layout PUBLIC cxx_std_20)
rbx_enable_warnings(rbx-ui-layout)

add_library(rbx-assets STATIC
    include/rbx/assets/AssetMountTable.h
    include/rbx/assets/PlacePackage.h
    src/AssetMountTable.cpp
    src/PlacePackage.cpp)
add_library(Roblox::Assets ALIAS rbx-assets)

target_include_directories(rbx-assets PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_features(rbx-assets PUBLIC cxx_std_20)
rbx_enable_warnings(rbx-assets)

add_library(rbx-network-transport STATIC
    src/GameNetworkingTransport.cpp)
add_library(Roblox::NetworkTransport ALIAS rbx-network-transport)

target_include_directories(rbx-network-transport PUBLIC
    include
    "${CMAKE_CURRENT_LIST_DIR}/../include")
target_link_libraries(rbx-network-transport
    PRIVATE GameNetworkingSockets::static)
target_compile_features(rbx-network-transport PUBLIC cxx_std_20)

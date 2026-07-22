set(RBX_NETWORK_TRANSPORT_SOURCE src/GameNetworkingTransport.cpp)
if(EMSCRIPTEN)
    set(RBX_NETWORK_TRANSPORT_SOURCE src/WebSocketNetworkingTransport.cpp)
endif()
add_library(rbx-network-transport STATIC
    ${RBX_NETWORK_TRANSPORT_SOURCE})
add_library(Roblox::NetworkTransport ALIAS rbx-network-transport)

target_include_directories(rbx-network-transport PUBLIC
    include
    "${CMAKE_CURRENT_LIST_DIR}/../include")
if(EMSCRIPTEN)
    target_compile_options(rbx-network-transport PRIVATE -pthread)
    target_link_options(rbx-network-transport PUBLIC "SHELL:-lwebsocket.js" -pthread)
else()
    target_link_libraries(rbx-network-transport
        PRIVATE GameNetworkingSockets::static)
endif()
target_compile_features(rbx-network-transport PUBLIC cxx_std_20)

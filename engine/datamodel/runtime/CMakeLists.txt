file(GLOB RBX_DATAMODEL_RUNTIME_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/v8datamodel/*.h")
file(GLOB RBX_DATAMODEL_RUNTIME_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
add_library(rbx-datamodel-runtime STATIC EXCLUDE_FROM_ALL
    ${RBX_DATAMODEL_RUNTIME_HEADERS} ${RBX_DATAMODEL_RUNTIME_SOURCES})
add_library(Roblox::DataModelRuntime ALIAS rbx-datamodel-runtime)

target_include_directories(rbx-datamodel-runtime
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE
        "${CMAKE_SOURCE_DIR}/engine/datamodel/tree/include"
        "${CMAKE_SOURCE_DIR}/engine/simulation/include"
        "${CMAKE_SOURCE_DIR}/engine/animation/include"
        "${CMAKE_SOURCE_DIR}/engine/interaction/tools/include"
        "${CMAKE_SOURCE_DIR}/engine/ui/runtime/include"
        "${CMAKE_SOURCE_DIR}/engine/reflection/include"
        "${CMAKE_SOURCE_DIR}/engine/serialization/include"
        "${CMAKE_SOURCE_DIR}/engine/core/include"
        "${CMAKE_SOURCE_DIR}/engine/core/include"
        "${CMAKE_SOURCE_DIR}/App/include"
        "${CMAKE_SOURCE_DIR}/engine/avatar/humanoid/include"
        "${CMAKE_SOURCE_DIR}/engine/terrain/include"
        "${CMAKE_SOURCE_DIR}/App"
        "${CMAKE_SOURCE_DIR}/engine/scripting/include"
        "${CMAKE_SOURCE_DIR}/third_party/bullet_historical"
        "${CMAKE_SOURCE_DIR}/apps/player/shared/client"
        "${CMAKE_SOURCE_DIR}/engine/networking/include"
        "${CMAKE_SOURCE_DIR}/Rendering/g3d/include"
        "${CMAKE_SOURCE_DIR}/Rendering/RbxG3D/include"
        "${CMAKE_SOURCE_DIR}/engine/rendering/foundation/include"
        "${CMAKE_SOURCE_DIR}/engine/rendering/adorn/include"
        "${CMAKE_SOURCE_DIR}/Library/boost/include"
        "${CMAKE_SOURCE_DIR}/Library/SDL2/include"
        "${OPENSSL_INCLUDE_DIR}")
target_link_libraries(rbx-datamodel-runtime
    PUBLIC Roblox::DataModelTree Roblox::Serialization Roblox::Reflection Roblox::ValueTypes Roblox::Assets Roblox::AudioContracts Roblox::MediaRuntime Roblox::UILayout Roblox::UIInput Roblox::Simulation Roblox::Animation Roblox::Terrain
    PRIVATE Roblox::PlatformClipboard Luau.VM "${OPENSSL_CRYPTO_LIBRARY}")
target_compile_features(rbx-datamodel-runtime PUBLIC cxx_std_20)
target_compile_definitions(rbx-datamodel-runtime PRIVATE
    RBX_LUAU_VM=1
    BOOST_NO_CXX98_FUNCTION_BASE=1
    _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR=1)
target_compile_options(rbx-datamodel-runtime PRIVATE
    $<$<COMPILE_LANG_AND_ID:CXX,AppleClang>:-Wno-enum-constexpr-conversion>)

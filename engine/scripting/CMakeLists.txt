file(GLOB RBX_SCRIPT_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/Script/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/lua/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/lua/*.hpp")
file(GLOB RBX_SCRIPT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
list(REMOVE_ITEM RBX_SCRIPT_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaVM.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaVMDummy.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/LuaVMServer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/LuauBytecode.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/LuauRuntime.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/LuauVM.cpp")
file(GLOB RBX_LUA_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/lua/src/*.h")
file(GLOB RBX_LUA_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/lua/src/*.c")
list(REMOVE_ITEM RBX_LUA_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/lua/src/lua.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lua/src/luac.c")
set_source_files_properties(${RBX_LUA_SOURCES} PROPERTIES LANGUAGE CXX)

add_library(rbx-luau-vm STATIC EXCLUDE_FROM_ALL
    include/Script/LuauBytecode.h
    src/LuauBytecode.cpp
    src/LuauVM.cpp)
add_library(Roblox::LuauVM ALIAS rbx-luau-vm)
target_include_directories(rbx-luau-vm PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/App/include"
    "${CMAKE_SOURCE_DIR}/Library/boost/include")
target_link_libraries(rbx-luau-vm PUBLIC Luau.VM Luau.Compiler)
target_compile_features(rbx-luau-vm PUBLIC cxx_std_20)
target_compile_definitions(rbx-luau-vm PUBLIC
    BOOST_NO_CXX98_FUNCTION_BASE=1
    _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR=1)

add_library(rbx-luau-runtime STATIC EXCLUDE_FROM_ALL
    include/Script/LuauRuntime.h
    src/LuauRuntime.cpp)
add_library(Roblox::LuauRuntime ALIAS rbx-luau-runtime)
target_include_directories(rbx-luau-runtime PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(rbx-luau-runtime PUBLIC Roblox::LuauVM)
target_compile_features(rbx-luau-runtime PUBLIC cxx_std_20)

# The engine scripting bridge is moved onto Luau independently from the
# shipping checkpoint so every legacy-VM dependency is exposed and removed.
add_library(rbx-luau-scripting STATIC EXCLUDE_FROM_ALL
    src/CoreScript.cpp
    src/LuaArguments.cpp
    src/LuaAtomicClasses.cpp
    src/LuaBridge.cpp
    src/LuaCoreFunctions.cpp
    src/LuaEnum.cpp
    src/LuaInstanceBridge.cpp
    src/LuaLibrary.cpp
    src/LuaMemory.cpp
    src/LuaSettings.cpp
    src/LuaSignalBridge.cpp
    src/LuaSourceContainer.cpp
    src/ModuleScript.cpp
    src/Script.cpp
    src/ScriptContext.cpp
    src/ScriptEvent.cpp
    src/ScriptStats.cpp
    src/ThreadRef.cpp)
add_library(Roblox::LuauScripting ALIAS rbx-luau-scripting)
target_include_directories(rbx-luau-scripting
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_SOURCE_DIR}/engine/datamodel/runtime/include"
        "${CMAKE_SOURCE_DIR}/engine/datamodel/tree/include"
        "${CMAKE_SOURCE_DIR}/engine/animation/include"
        "${CMAKE_SOURCE_DIR}/engine/simulation/include"
        "${CMAKE_SOURCE_DIR}/engine/interaction/tools/include"
        "${CMAKE_SOURCE_DIR}/engine/ui/runtime/include"
        "${CMAKE_SOURCE_DIR}/engine/rendering/adorn/include"
        "${CMAKE_SOURCE_DIR}/engine/rendering/foundation/include"
        "${CMAKE_SOURCE_DIR}/engine/reflection/include"
        "${CMAKE_SOURCE_DIR}/engine/serialization/include"
        "${CMAKE_SOURCE_DIR}/engine/core/math/include"
        "${CMAKE_SOURCE_DIR}/engine/core/include"
        "${CMAKE_SOURCE_DIR}/App/include"
        "${CMAKE_SOURCE_DIR}/App"
        "${CMAKE_SOURCE_DIR}/third_party/bullet_historical"
        "${CMAKE_SOURCE_DIR}/apps/player/shared/client"
        "${CMAKE_SOURCE_DIR}/engine/networking/include"
        "${CMAKE_SOURCE_DIR}/Library/SDL2/include"
        "${CMAKE_SOURCE_DIR}/Library/boost/include")
target_link_libraries(rbx-luau-scripting PRIVATE
    Roblox::LuauVM
    Roblox::DataModelRuntime Roblox::RuntimeUI Roblox::InteractionTools
    Roblox::RenderAdorn Roblox::RenderFoundation Roblox::Simulation
    Roblox::Animation Roblox::Serialization Roblox::Reflection Roblox::Math
    Roblox::Lz4 Roblox::Utf8proc Roblox::ProtectionMarkers)
target_compile_definitions(rbx-luau-scripting PUBLIC
    RBX_LUAU_VM=1
    BOOST_NO_CXX98_FUNCTION_BASE=1
    _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR=1)
target_compile_features(rbx-luau-scripting PRIVATE cxx_std_20)
target_compile_options(rbx-luau-scripting PRIVATE
    $<$<COMPILE_LANG_AND_ID:CXX,AppleClang>:-Wno-enum-constexpr-conversion>)

add_library(rbx-scripting STATIC EXCLUDE_FROM_ALL
    ${RBX_SCRIPT_HEADERS} ${RBX_SCRIPT_SOURCES}
    ${RBX_LUA_HEADERS} ${RBX_LUA_SOURCES})
add_library(Roblox::Scripting ALIAS rbx-scripting)

target_include_directories(rbx-scripting
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/lua/src"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_SOURCE_DIR}/engine/datamodel/runtime/include"
        "${CMAKE_SOURCE_DIR}/engine/datamodel/tree/include"
        "${CMAKE_SOURCE_DIR}/engine/animation/include"
        "${CMAKE_SOURCE_DIR}/engine/simulation/include"
        "${CMAKE_SOURCE_DIR}/engine/interaction/tools/include"
        "${CMAKE_SOURCE_DIR}/engine/ui/runtime/include"
        "${CMAKE_SOURCE_DIR}/engine/rendering/adorn/include"
        "${CMAKE_SOURCE_DIR}/engine/rendering/foundation/include"
        "${CMAKE_SOURCE_DIR}/engine/reflection/include"
        "${CMAKE_SOURCE_DIR}/engine/serialization/include"
        "${CMAKE_SOURCE_DIR}/engine/core/math/include"
        "${CMAKE_SOURCE_DIR}/engine/core/include"
        "${CMAKE_SOURCE_DIR}/engine/core/include"
        "${CMAKE_SOURCE_DIR}/App/include"
        "${CMAKE_SOURCE_DIR}/App"
        "${CMAKE_SOURCE_DIR}/third_party/bullet_historical"
        "${CMAKE_SOURCE_DIR}/apps/player/shared/client"
        "${CMAKE_SOURCE_DIR}/engine/networking/include"
        "${CMAKE_SOURCE_DIR}/Library/SDL2/include"
        "${CMAKE_SOURCE_DIR}/Library/boost/include")
target_link_libraries(rbx-scripting PUBLIC
    Roblox::DataModelRuntime Roblox::RuntimeUI Roblox::InteractionTools
    Roblox::RenderAdorn Roblox::RenderFoundation Roblox::Simulation
    Roblox::Animation Roblox::Serialization Roblox::Reflection Roblox::Math
    Roblox::Lz4 Roblox::Utf8proc)
target_link_libraries(rbx-scripting PRIVATE Roblox::ProtectionMarkers)
target_compile_features(rbx-scripting PUBLIC cxx_std_20)
target_compile_definitions(rbx-scripting PRIVATE
    BOOST_NO_CXX98_FUNCTION_BASE=1
    _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR=1)
target_compile_options(rbx-scripting PRIVATE
    $<$<COMPILE_LANG_AND_ID:CXX,AppleClang>:-Wno-enum-constexpr-conversion>)

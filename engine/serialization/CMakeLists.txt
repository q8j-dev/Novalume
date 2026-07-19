add_library(rbx-serialization STATIC EXCLUDE_FROM_ALL
    src/Base64BinaryInputStream.cpp
    src/Base64BinaryOutputStream.cpp
    src/SerializerBinary.cpp
    src/SerializerV2.cpp
    src/WebParser.cpp
    src/WebSerializer.cpp
    src/XmlElement.cpp
    src/XmlSerializer.cpp
    include/v8xml/Reference.h
    include/v8xml/Serializer.h
    include/v8xml/SerializerBinary.h
    include/v8xml/SerializerV2.h
    include/v8xml/WebParser.h
    include/v8xml/WebSerializer.h
    include/v8xml/XmlElement.h
    include/v8xml/XmlSerializer.h)
add_library(Roblox::Serialization ALIAS rbx-serialization)

target_include_directories(rbx-serialization
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE
        "${CMAKE_SOURCE_DIR}/engine/reflection/include"
        "${CMAKE_SOURCE_DIR}/engine/core/include"
        "${CMAKE_SOURCE_DIR}/engine/simulation/include"
        "${CMAKE_SOURCE_DIR}/engine/datamodel/runtime/include"
        "${CMAKE_SOURCE_DIR}/engine/animation/include"
        "${CMAKE_SOURCE_DIR}/engine/core/include"
        "${CMAKE_SOURCE_DIR}/App/include"
        "${CMAKE_SOURCE_DIR}/App"
        "${CMAKE_SOURCE_DIR}/third_party/bullet_historical"
        "${CMAKE_SOURCE_DIR}/apps/player/shared/client"
        "${CMAKE_SOURCE_DIR}/Rendering/g3d/include"
        "${CMAKE_SOURCE_DIR}/Rendering/RbxG3D/include"
        "${CMAKE_SOURCE_DIR}/engine/rendering/foundation/include"
        "${CMAKE_SOURCE_DIR}/engine/rendering/adorn/include"
        "${CMAKE_SOURCE_DIR}/engine/interaction/tools/include"
        "${CMAKE_SOURCE_DIR}/engine/ui/runtime/include"
        "${CMAKE_SOURCE_DIR}/engine/scripting/include"
        "${CMAKE_SOURCE_DIR}/engine/scripting/lua/src"
        "${CMAKE_SOURCE_DIR}/Library/boost/include"
        "${CMAKE_SOURCE_DIR}/Library/SDL2/include"
        )
target_link_libraries(rbx-serialization PUBLIC
    Roblox::Reflection Roblox::Lz4 Roblox::Zstd Roblox::AudioContracts)
target_compile_features(rbx-serialization PUBLIC cxx_std_20)
target_compile_definitions(rbx-serialization PRIVATE
    BOOST_NO_CXX98_FUNCTION_BASE=1
    _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR=1)
target_compile_options(rbx-serialization PRIVATE
    $<$<COMPILE_LANG_AND_ID:CXX,AppleClang>:-Wno-enum-constexpr-conversion>)

set(RBX_VALUE_TYPE_SOURCES
    "${CMAKE_SOURCE_DIR}/engine/value_types/src/AnimationId.cpp"
    "${CMAKE_SOURCE_DIR}/engine/value_types/src/Axes.cpp"
    "${CMAKE_SOURCE_DIR}/engine/value_types/src/BrickColor.cpp"
    "${CMAKE_SOURCE_DIR}/engine/value_types/src/CellID.cpp"
    "${CMAKE_SOURCE_DIR}/engine/value_types/src/Color.cpp"
    "${CMAKE_SOURCE_DIR}/engine/value_types/src/ContentId.cpp"
    "${CMAKE_SOURCE_DIR}/engine/value_types/src/Content.cpp"
    "${CMAKE_SOURCE_DIR}/engine/value_types/src/Extents.cpp"
    "${CMAKE_SOURCE_DIR}/engine/value_types/src/Face.cpp"
    "${CMAKE_SOURCE_DIR}/engine/value_types/src/Faces.cpp"
    "${CMAKE_SOURCE_DIR}/engine/value_types/src/Font.cpp"
    "${CMAKE_SOURCE_DIR}/engine/value_types/src/Guid.cpp"
    "${CMAKE_SOURCE_DIR}/engine/value_types/src/Hash.cpp"
    "${CMAKE_SOURCE_DIR}/engine/value_types/src/IndexBox.cpp"
    "${CMAKE_SOURCE_DIR}/engine/value_types/src/KeyCode.cpp"
    "${CMAKE_SOURCE_DIR}/engine/value_types/src/Math.cpp"
    "${CMAKE_SOURCE_DIR}/engine/value_types/src/MeshId.cpp"
    "${CMAKE_SOURCE_DIR}/engine/value_types/src/NormalId.cpp"
    "${CMAKE_SOURCE_DIR}/engine/value_types/src/PhysicalProperties.cpp"
    "${CMAKE_SOURCE_DIR}/engine/value_types/src/ProtectedString.cpp"
    "${CMAKE_SOURCE_DIR}/engine/value_types/src/Quaternion.cpp"
    "${CMAKE_SOURCE_DIR}/engine/value_types/src/Rect.cpp"
    "${CMAKE_SOURCE_DIR}/engine/value_types/src/Region2.cpp"
    "${CMAKE_SOURCE_DIR}/engine/value_types/src/Region3.cpp"
    "${CMAKE_SOURCE_DIR}/engine/value_types/src/Region3int16.cpp"
    "${CMAKE_SOURCE_DIR}/engine/value_types/src/Region3int32.cpp"
    "${CMAKE_SOURCE_DIR}/engine/value_types/src/RunningAverage.cpp"
    "${CMAKE_SOURCE_DIR}/engine/value_types/src/UDim.cpp"
    "${CMAKE_SOURCE_DIR}/engine/value_types/src/Units.cpp"
    "${CMAKE_SOURCE_DIR}/engine/value_types/src/Vector3int32.cpp"
    "${CMAKE_SOURCE_DIR}/engine/value_types/src/rbxrandom.cpp")

add_library(rbx-value-types STATIC EXCLUDE_FROM_ALL ${RBX_VALUE_TYPE_SOURCES})
add_library(Roblox::ValueTypes ALIAS rbx-value-types)
target_include_directories(rbx-value-types
    PUBLIC "${CMAKE_SOURCE_DIR}/App/include"
    PRIVATE
        "${CMAKE_SOURCE_DIR}/App"
        "${CMAKE_SOURCE_DIR}/engine/core/include"
        "${CMAKE_SOURCE_DIR}/engine/core/math/include"
        "${CMAKE_SOURCE_DIR}/engine/security/include"
        "${CMAKE_SOURCE_DIR}/engine/reflection/include"
        "${CMAKE_SOURCE_DIR}/engine/datamodel/runtime/include"
        "${CMAKE_SOURCE_DIR}/engine/animation/include"
        "${CMAKE_SOURCE_DIR}/engine/simulation/include"
        "${CMAKE_SOURCE_DIR}/engine/datamodel/tree/include"
        "${CMAKE_SOURCE_DIR}/engine/ui/runtime/include"
        "${CMAKE_SOURCE_DIR}/engine/interaction/tools/include"
        "${CMAKE_SOURCE_DIR}/engine/scripting/include"
        "${CMAKE_SOURCE_DIR}/engine/scripting/lua/src"
        "${CMAKE_SOURCE_DIR}/engine/networking/include"
        "${CMAKE_SOURCE_DIR}/engine/rendering/foundation/include"
        "${CMAKE_SOURCE_DIR}/engine/rendering/adorn/include"
        "${CMAKE_SOURCE_DIR}/third_party/bullet_historical"
        "${CMAKE_SOURCE_DIR}/Library/boost/include"
        "${CMAKE_SOURCE_DIR}/Library/SDL2/include")
target_link_libraries(rbx-value-types PUBLIC Roblox::Core Roblox::Math Roblox::GeometryCore Roblox::Security Roblox::Reflection)
target_compile_features(rbx-value-types PUBLIC cxx_std_20)
target_compile_definitions(rbx-value-types PRIVATE
    BOOST_NO_CXX98_FUNCTION_BASE=1
    _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR=1)
target_compile_options(rbx-value-types PRIVATE
    $<$<COMPILE_LANG_AND_ID:CXX,AppleClang>:-Wno-enum-constexpr-conversion>)

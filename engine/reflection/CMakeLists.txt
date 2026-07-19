add_library(rbx-reflection STATIC EXCLUDE_FROM_ALL
    src/Callback.cpp
    src/Event.cpp
    src/reflection_function.cpp
    src/reflection_object.cpp
    src/reflection_property.cpp
    src/type.cpp
    include/reflection/Callback.h
    include/reflection/Descriptor.h
    include/reflection/EnumConverter.h
    include/reflection/Event.h
    include/reflection/Function.h
    include/reflection/Object.h
    include/reflection/Property.h
    include/reflection/Type.h
    include/reflection/YieldFunction.h
    include/reflection/member.h
    include/reflection/reflection.h)
add_library(Roblox::Reflection ALIAS rbx-reflection)

target_include_directories(rbx-reflection
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE
        "${CMAKE_SOURCE_DIR}/engine/core/include"
        "${CMAKE_SOURCE_DIR}/engine/core/include"
        "${CMAKE_SOURCE_DIR}/App/include"
        "${CMAKE_SOURCE_DIR}/Rendering/g3d/include"
        "${CMAKE_SOURCE_DIR}/Rendering/RbxG3D/include"
        "${CMAKE_SOURCE_DIR}/engine/core/math/include"
        "${CMAKE_SOURCE_DIR}/engine/datamodel/runtime/include"
        "${CMAKE_SOURCE_DIR}/engine/animation/include"
        "${CMAKE_SOURCE_DIR}/engine/simulation/include"
        "${CMAKE_SOURCE_DIR}/Library/boost/include"
        "${CMAKE_SOURCE_DIR}/Library/SDL2/include")
target_link_libraries(rbx-reflection PUBLIC Roblox::Math Roblox::Security)
target_compile_features(rbx-reflection PUBLIC cxx_std_20)
target_compile_definitions(rbx-reflection PRIVATE
    BOOST_NO_CXX98_FUNCTION_BASE=1
    _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR=1)
target_compile_options(rbx-reflection PRIVATE
    $<$<COMPILE_LANG_AND_ID:CXX,AppleClang>:-Wno-enum-constexpr-conversion>)

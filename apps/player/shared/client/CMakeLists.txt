list(APPEND HEADERS rapidjson/prettywriter.h)
list(APPEND HEADERS rapidjson/stringbuffer.h)
list(APPEND HEADERS rapidjson/reader.h)
list(APPEND HEADERS rapidjson/rapidjson.h)
list(APPEND HEADERS rapidjson/writer.h)
list(APPEND HEADERS rapidjson/document.h)
list(APPEND HEADERS rapidjson/internal/stack.h)
list(APPEND HEADERS rapidjson/internal/pow10.h)
list(APPEND HEADERS rapidjson/internal/strfunc.h)
list(APPEND HEADERS rapidjson/filestream.h)
list(APPEND HEADERS format_string.h)
list(APPEND HEADERS SimpleJSON.h)
list(APPEND HEADERS CountersClient.h)
list(APPEND HEADERS CookiesEngine.h)
list(APPEND HEADERS StringConv.h)
list(APPEND HEADERS RobloxServicesTools.h)

list(APPEND SOURCES format_string.cpp)
list(APPEND SOURCES Win/CookiesEngine.cpp)
list(APPEND SOURCES RobloxServicesTools.cpp)
list(APPEND SOURCES CountersClient.cpp)
list(APPEND SOURCES Mobile/CookiesEngine.cpp)
list(APPEND SOURCES DataModelSerialize.cpp)
list(APPEND SOURCES StringConv.cpp)
list(APPEND SOURCES DataModelEmptySerialize.cpp)
list(APPEND SOURCES SimpleJSON.cpp)
list(APPEND SOURCES Mac/CookiesEngine.cpp)

# Will not be built, but it will list all project files in the IDE.
add_custom_target(ClientShared_unbuilt SOURCES ${SOURCES} ${HEADERS})

set(RBX_PLAYER_SHARED_SOURCES
    DataModelSerialize.cpp
    format_string.cpp
    RobloxServicesTools.cpp
    SimpleJSON.cpp)
if(APPLE)
    list(APPEND RBX_PLAYER_SHARED_SOURCES Mac/CookiesEngine.cpp)
elseif(WIN32)
    list(APPEND RBX_PLAYER_SHARED_SOURCES Win/CookiesEngine.cpp)
else()
    list(APPEND RBX_PLAYER_SHARED_SOURCES Mobile/CookiesEngine.cpp)
endif()

add_library(rbx-player-shared STATIC EXCLUDE_FROM_ALL ${RBX_PLAYER_SHARED_SOURCES})
add_library(Roblox::PlayerShared ALIAS rbx-player-shared)
target_include_directories(rbx-player-shared
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
    PRIVATE
        "${CMAKE_SOURCE_DIR}/App/include"
        "${CMAKE_SOURCE_DIR}/App"
        "${CMAKE_SOURCE_DIR}/engine/core/include"
        "${CMAKE_SOURCE_DIR}/engine/core/math/include"
        "${CMAKE_SOURCE_DIR}/engine/security/include"
        "${CMAKE_SOURCE_DIR}/engine/reflection/include"
        "${CMAKE_SOURCE_DIR}/engine/serialization/include"
        "${CMAKE_SOURCE_DIR}/engine/datamodel/tree/include"
        "${CMAKE_SOURCE_DIR}/engine/datamodel/runtime/include"
        "${CMAKE_SOURCE_DIR}/engine/audio/include"
        "${CMAKE_SOURCE_DIR}/engine/simulation/include"
        "${CMAKE_SOURCE_DIR}/engine/animation/include"
        "${CMAKE_SOURCE_DIR}/engine/interaction/tools/include"
        "${CMAKE_SOURCE_DIR}/engine/ui/runtime/include"
        "${CMAKE_SOURCE_DIR}/engine/scripting/include"
        "${CMAKE_SOURCE_DIR}/engine/scripting/lua/src"
        "${CMAKE_SOURCE_DIR}/engine/rendering/foundation/include"
        "${CMAKE_SOURCE_DIR}/engine/rendering/adorn/include"
        "${CMAKE_SOURCE_DIR}/engine/networking/include"
        "${CMAKE_SOURCE_DIR}/third_party/bullet_historical"
        "${CMAKE_SOURCE_DIR}/Library/boost/include"
        "${CMAKE_SOURCE_DIR}/Library/SDL2/include")

target_link_libraries(rbx-player-shared PUBLIC Roblox::Core)
if(APPLE)
    target_link_libraries(rbx-player-shared PRIVATE "-framework CoreFoundation")
endif()
target_compile_features(rbx-player-shared PUBLIC cxx_std_20)
target_compile_definitions(rbx-player-shared PRIVATE
    BOOST_NO_CXX98_FUNCTION_BASE=1
    _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR=1)
target_compile_options(rbx-player-shared PRIVATE
    $<$<COMPILE_LANG_AND_ID:CXX,AppleClang>:-Wno-enum-constexpr-conversion>)

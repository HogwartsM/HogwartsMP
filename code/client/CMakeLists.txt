set(UE4_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/src/sdk)

message(UE4_SOURCE_DIR="${UE4_SOURCE_DIR}")

set(HOGWARTSMP_CLIENT_FILES
    src/main.cpp
    src/core/application.cpp
    src/core/client_instance.cpp
    src/networking/network_client.cpp
    ../shared/networking/network_packet.cpp
    ../shared/logging/logger.cpp
    ${CMAKE_BINARY_DIR}/hogwartsmp_version.cpp
)

# Temporarily disabled until Framework dependencies are replaced:
# - hooks (need SDK + Framework logging)
# - modules (need SDK + ECS)
# - states (need state machine)
# - UI (need ImGui)
# - SDK files (need MSVC or stub implementation)

add_library(HogwartsMPClient SHARED ${HOGWARTSMP_CLIENT_FILES})
target_include_directories(HogwartsMPClient PRIVATE
    src
    ../shared
    ${CMAKE_SOURCE_DIR}/vendor/enet/include
    ${CMAKE_SOURCE_DIR}/vendor/flecs
    ${CMAKE_SOURCE_DIR}/vendor/glm
    ${CMAKE_SOURCE_DIR}/vendor/minhook/include
    ${UE4_SOURCE_DIR}/Runtime/Core/Public
    ${UE4_SOURCE_DIR}/Runtime/CoreUObject/Public
    ${UE4_SOURCE_DIR}/Runtime/TraceLog/Public
)

target_link_libraries(HogwartsMPClient flecs enet minhook shlwapi)

# Use MinGW-compatible link options (MSVC flags removed)
if(MSVC)
    target_link_options(HogwartsMPClient PRIVATE /SAFESEH:NO /MANIFEST:NO)
endif()

set_property(TARGET HogwartsMPClient PROPERTY CXX_STANDARD 17)
set_property(TARGET HogwartsMPClient PROPERTY CXX_STANDARD_REQUIRED ON)

target_compile_definitions(HogwartsMPClient PRIVATE 
    UBT_COMPILED_PLATFORM=Windows 
    UE_BUILD_SHIPPING=1 
    WITH_EDITOR=0 
    WITH_EDITORONLY_DATA=0
    WITH_ENGINE=1 
    WITH_UNREAL_DEVELOPER_TOOLS=0
    WITH_PLUGIN_SUPPORT=0
    WITH_SERVER_CODE=0
    IS_MONOLITHIC=1
    IS_PROGRAM=1
    PLATFORM_WINDOWS
    PLATFORM_64BITS
    CORE_API=
    COREUOBJECT_API=
    UNICODE
    _UNICODE
    NOMINMAX
    WINVER=0x0A00
)

add_custom_command(TARGET HogwartsMPClient POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/../../files" "${CMAKE_BINARY_DIR}/bin/files/"
)

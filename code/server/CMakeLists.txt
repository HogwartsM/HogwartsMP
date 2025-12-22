set(HOGWARTSMP_SERVER_FILES
    src/main.cpp
    src/core/server.cpp
    src/core/server_instance.cpp

    src/core/modules/human.cpp

    src/networking/network_server.cpp
    ../shared/networking/network_packet.cpp
    ../shared/logging/logger.cpp

    ${CMAKE_BINARY_DIR}/hogwartsmp_version.cpp
)

add_executable(HogwartsMPServer ${HOGWARTSMP_SERVER_FILES})
target_include_directories(HogwartsMPServer PRIVATE
    src
    ../shared
    ${CMAKE_SOURCE_DIR}/vendor/enet/include
    ${CMAKE_SOURCE_DIR}/vendor/flecs
    ${CMAKE_SOURCE_DIR}/vendor/glm
)
target_link_libraries(HogwartsMPServer flecs enet)
set_property(TARGET HogwartsMPServer PROPERTY CXX_STANDARD 17)
set_property(TARGET HogwartsMPServer PROPERTY CXX_STANDARD_REQUIRED ON)

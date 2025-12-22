set(HOGWARTSMP_SERVER_FILES
    src/main.cpp
    src/core/server.cpp
    src/core/server_instance.cpp
    src/cli/cli.cpp

    src/game/modules/human.cpp

    src/networking/network_server.cpp
    src/logging/network_logger.cpp
    ../shared/networking/network_packet.cpp
    ../shared/logging/logger.cpp

    ${CMAKE_BINARY_DIR}/hogwartsmp_version.cpp
)

add_executable(HogwartsMPServer ${HOGWARTSMP_SERVER_FILES})
target_include_directories(HogwartsMPServer PRIVATE
    src
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vendor/enet/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vendor/flecs
    ${CMAKE_CURRENT_SOURCE_DIR}/../../vendor/glm
)
target_link_libraries(HogwartsMPServer flecs enet)
target_compile_features(HogwartsMPServer PRIVATE cxx_std_17)

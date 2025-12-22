cmake_minimum_required(VERSION 3.10)
project(HogwartsMPClient)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROJECT_ROOT ${CMAKE_CURRENT_LIST_DIR}/../..)

set(HOGWARTSMP_CLIENT_FILES
    src/main.cpp
    src/Core/client_instance.cpp
    src/Services/network_client.cpp
    ../shared/logging/logger.cpp
    ../shared/networking/network_packet.cpp
    ../shared/version.cpp
)

add_library(HogwartsMPClient SHARED ${HOGWARTSMP_CLIENT_FILES})

target_include_directories(HogwartsMPClient PRIVATE
    src
    ${PROJECT_ROOT}/code/shared
    ${PROJECT_ROOT}/vendor/enet/include
)

target_link_libraries(HogwartsMPClient PRIVATE
    enet
    ws2_32
    winmm
    psapi
)

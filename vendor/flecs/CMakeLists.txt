# Flecs ECS Library
cmake_minimum_required(VERSION 3.20)

project(flecs LANGUAGES C)

add_library(flecs STATIC
    flecs.c
)

target_include_directories(flecs PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Disable warnings for third-party code
if(MSVC)
    target_compile_options(flecs PRIVATE /W0)
else()
    target_compile_options(flecs PRIVATE -w)
endif()

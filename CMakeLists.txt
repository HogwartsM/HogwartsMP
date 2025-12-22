cmake_minimum_required(VERSION 3.20...3.31)
cmake_policy(SET CMP0091 NEW)

project(HogwartsMP VERSION 1.0 LANGUAGES CXX)

# Global definitions
add_compile_definitions(_SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")

# Add vendor dependencies
add_subdirectory(vendor/enet)
add_subdirectory(vendor/flecs)
add_subdirectory(vendor/glm)
add_subdirectory(vendor/minhook)

add_subdirectory(code)

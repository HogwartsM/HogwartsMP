cmake_minimum_required(VERSION 3.20)
project(HogwartsMPClient VERSION 2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================================
# CLIENT v2.0 - Utilise entièrement le framework
# ============================================================================

# Source files (simplifié - plus d'UI, plus de services custom)
set(CLIENT_SOURCES
    src/client_main.cpp
)

# Créer la DLL client
add_library(HogwartsMPClient SHARED ${CLIENT_SOURCES})

# Include directories
target_include_directories(HogwartsMPClient PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/code
)

# Link avec le framework complet
target_link_libraries(HogwartsMPClient PRIVATE
    HogwartsMPFrameworkClient  # Tout le framework (Debug, Networking, Entities, Game, Events, Natives)
    ws2_32                      # Winsock
    winmm                       # Windows Multimedia
)

# Définir les macros
target_compile_definitions(HogwartsMPClient PRIVATE
    HOGWARTSMP_CLIENT
    $<$<CONFIG:Debug>:DEBUG_ENABLED>
)

# Options de compilation
if(MSVC)
    target_compile_options(HogwartsMPClient PRIVATE /W4)
else()
    target_compile_options(HogwartsMPClient PRIVATE -Wall -Wextra)
endif()

# Installation
install(TARGETS HogwartsMPClient
    LIBRARY DESTINATION bin
    RUNTIME DESTINATION bin
)

# Version info (simplified)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/shared/version.cpp" "${CMAKE_BINARY_DIR}/hogwartsmp_version.cpp" COPYONLY)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Always build client on 64-bit systems
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    add_subdirectory(client)
    # Launcher disabled - missing launcher/project.h
    # add_subdirectory(launcher)
endif()

add_subdirectory(server)

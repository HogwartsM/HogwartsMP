set(HOGWARTSMP_LAUNCHER_FILES
    src/main.cpp
    src/manual_map.cpp
    src/manual_map.h
)

add_executable(HogwartsMPLauncher ${HOGWARTSMP_LAUNCHER_FILES})
target_include_directories(HogwartsMPLauncher PRIVATE
    .
)
target_link_libraries(HogwartsMPLauncher PRIVATE)
set_property(TARGET HogwartsMPLauncher PROPERTY CXX_STANDARD 17)
set_property(TARGET HogwartsMPLauncher PROPERTY CXX_STANDARD_REQUIRED ON)

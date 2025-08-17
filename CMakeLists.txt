cmake_minimum_required(VERSION 3.15)

project(rpi0-weather 
    VERSION 1.0.0
    DESCRIPTION "Raspberry Pi Zero Weather Station"
    LANGUAGES C CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# CPM (CMake Package Manager) setup
include(cmake/CPM.cmake)

# Add SDL3 using CPM
CPMAddPackage(
    NAME SDL3
    GITHUB_REPOSITORY libsdl-org/SDL
    GIT_TAG main
    OPTIONS
        "SDL_SHARED OFF"
        "SDL_STATIC ON"
        "SDL_TEST OFF"
        "SDL_DISABLE_INSTALL ON"
)

if(SDL3_ADDED)
    message(STATUS "SDL3 added via CPM - enabling emulator support")
    add_definitions(-DHAVE_SDL3)
else()
    message(STATUS "SDL3 not available - building without emulator support")
endif()

# Build inky_c library
set(INKY_C_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/inky_c)
set(INKY_C_SOURCES
    ${INKY_C_DIR}/inky_common.c
    ${INKY_C_DIR}/inky_emulator.c
    ${INKY_C_DIR}/inky_hardware.c
    ${INKY_C_DIR}/inky_buttons.c
)

# Create inky_c static library
add_library(inky_c STATIC ${INKY_C_SOURCES})
target_include_directories(inky_c PUBLIC ${INKY_C_DIR})

# Include directories
include_directories(include)
include_directories(${INKY_C_DIR})

# Source files
set(SOURCES
    src/main.cpp
    src/weather_app.cpp
    src/weather_data.cpp
    src/sdl_emulator.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE inky_c)
if(SDL3_ADDED)
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)
endif()

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
endif()

# Install target
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
cmake_minimum_required(VERSION 3.15)

project(rpi0-weather 
    VERSION 1.0.0
    DESCRIPTION "Raspberry Pi Zero Weather Station"
    LANGUAGES C CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Detect target platform and set defaults
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm" OR CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    set(BUILD_EMULATOR_DEFAULT OFF)
    set(PLATFORM_MSG "ARM processor detected - SDL3 emulator disabled by default")
else()
    set(BUILD_EMULATOR_DEFAULT ON)  
    set(PLATFORM_MSG "x86/x64 processor detected - SDL3 emulator enabled by default")
endif()

# Build options
option(BUILD_EMULATOR "Build with SDL3 emulator support (not needed for Pi hardware)" ${BUILD_EMULATOR_DEFAULT})
message(STATUS ${PLATFORM_MSG})

# CPM (CMake Package Manager) setup
include(cmake/CPM.cmake)

# Only fetch SDL3 if emulator build is requested
if(BUILD_EMULATOR)
    message(STATUS "BUILD_EMULATOR=ON - Fetching and building SDL3 dependency")
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
else()
    message(STATUS "BUILD_EMULATOR=OFF - SDL3 completely skipped (no download, no build)")
endif()

# Option to use system packages instead of fetching from GitHub
option(USE_SYSTEM_PACKAGES "Use system packages instead of CPM for dependencies" OFF)

# Add cpp-httplib for HTTP requests
if(USE_SYSTEM_PACKAGES)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(HTTPLIB QUIET httplib)
    if(NOT HTTPLIB_FOUND)
        message(STATUS "System httplib not found, falling back to CPM")
        set(USE_CPM_HTTPLIB ON)
    else()
        message(STATUS "Using system httplib package")
        set(USE_CPM_HTTPLIB OFF)
    endif()
else()
    set(USE_CPM_HTTPLIB ON)
endif()

if(USE_CPM_HTTPLIB)
    CPMAddPackage(
        NAME httplib
        GITHUB_REPOSITORY yhirose/cpp-httplib
        VERSION 0.14.3
        OPTIONS
            "HTTPLIB_REQUIRE_OPENSSL OFF"
            "HTTPLIB_USE_POLL ON"
    )
endif()

# Add nlohmann/json for JSON parsing
if(USE_SYSTEM_PACKAGES)
    find_package(nlohmann_json QUIET)
    if(NOT nlohmann_json_FOUND)
        message(STATUS "System nlohmann_json not found, falling back to CPM")
        set(USE_CPM_JSON ON)
    else()
        message(STATUS "Using system nlohmann_json package")
        set(USE_CPM_JSON OFF)
    endif()
else()
    set(USE_CPM_JSON ON)
endif()

if(USE_CPM_JSON)
    CPMAddPackage(
        NAME nlohmann_json
        GITHUB_REPOSITORY nlohmann/json
        VERSION 3.11.3
    )
endif()

# Add stb libraries for image and font handling
if(USE_SYSTEM_PACKAGES)
    find_path(STB_INCLUDE_DIR stb_image.h PATH_SUFFIXES stb)
    if(NOT STB_INCLUDE_DIR)
        message(STATUS "System stb headers not found, falling back to CPM")
        set(USE_CPM_STB ON)
    else()
        message(STATUS "Using system stb headers at ${STB_INCLUDE_DIR}")
        set(USE_CPM_STB OFF)
    endif()
else()
    set(USE_CPM_STB ON)
endif()

if(USE_CPM_STB)
    CPMAddPackage(
        NAME stb
        GITHUB_REPOSITORY nothings/stb
        GIT_TAG master
        DOWNLOAD_ONLY YES
    )
endif()

# Set compile definitions based on SDL3 availability
if(BUILD_EMULATOR)
    if(SDL3_ADDED)
        message(STATUS "SDL3 successfully added - emulator support enabled")
        add_definitions(-DHAVE_SDL3)
    else()
        message(WARNING "BUILD_EMULATOR=ON but SDL3 failed to load - emulator support disabled")
    endif()
else()
    message(STATUS "SDL3 disabled - building for hardware-only (no emulator)")
endif()

# Build inky_c library
set(INKY_C_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/inky_c)
set(INKY_C_SOURCES
    ${INKY_C_DIR}/inky_common.c
    ${INKY_C_DIR}/inky_buttons.c
)

# Only include hardware OR emulator implementation, not both
if(BUILD_EMULATOR)
    message(STATUS "Building with inky_emulator.c (SDL3 emulator mode)")
    list(APPEND INKY_C_SOURCES ${INKY_C_DIR}/inky_emulator.c)
else()
    message(STATUS "Building with inky_hardware.c (Pi hardware mode)")
    list(APPEND INKY_C_SOURCES ${INKY_C_DIR}/inky_hardware.c)
endif()

# Create inky_c static library
add_library(inky_c STATIC ${INKY_C_SOURCES})
target_include_directories(inky_c PUBLIC ${INKY_C_DIR})

# Include directories
include_directories(include)
include_directories(${INKY_C_DIR})

# Handle stb include directories
if(USE_CPM_STB)
    if(stb_ADDED)
        include_directories(${stb_SOURCE_DIR})
    endif()
else()
    include_directories(${STB_INCLUDE_DIR})
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/weather_app.cpp
    src/weather_data.cpp
    src/sdl_emulator.cpp
    src/display_renderer.cpp
    src/nws_client.cpp
    src/weather_service.cpp
    src/bitmap_font.cpp
    src/font_renderer.cpp
    src/config.cpp
    src/logger.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE inky_c)

# SDL3 linking (emulator only)
if(BUILD_EMULATOR AND SDL3_ADDED)
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)
    message(STATUS "Linking SDL3 for emulator support")
endif()

# HttpLib linking
if(USE_CPM_HTTPLIB)
    if(httplib_ADDED)
        target_link_libraries(${PROJECT_NAME} PRIVATE httplib::httplib)
    endif()
else()
    if(HTTPLIB_FOUND)
        target_link_libraries(${PROJECT_NAME} PRIVATE ${HTTPLIB_LIBRARIES})
        target_include_directories(${PROJECT_NAME} PRIVATE ${HTTPLIB_INCLUDE_DIRS})
        target_compile_options(${PROJECT_NAME} PRIVATE ${HTTPLIB_CFLAGS_OTHER})
    endif()
endif()

# JSON linking
if(USE_CPM_JSON)
    if(nlohmann_json_ADDED)
        target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)
    endif()
else()
    if(nlohmann_json_FOUND)
        target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)
    endif()
endif()

# Create test executable for NWS API
add_executable(test_nws_api 
    src/test_nws_api.cpp
    src/nws_client.cpp
    src/weather_service.cpp
    src/weather_data.cpp
)
target_link_libraries(test_nws_api PRIVATE inky_c)

# HttpLib linking for test
if(USE_CPM_HTTPLIB)
    if(httplib_ADDED)
        target_link_libraries(test_nws_api PRIVATE httplib::httplib)
    endif()
else()
    if(HTTPLIB_FOUND)
        target_link_libraries(test_nws_api PRIVATE ${HTTPLIB_LIBRARIES})
        target_include_directories(test_nws_api PRIVATE ${HTTPLIB_INCLUDE_DIRS})
        target_compile_options(test_nws_api PRIVATE ${HTTPLIB_CFLAGS_OTHER})
    endif()
endif()

# JSON linking for test
if(USE_CPM_JSON)
    if(nlohmann_json_ADDED)
        target_link_libraries(test_nws_api PRIVATE nlohmann_json::nlohmann_json)
    endif()
else()
    if(nlohmann_json_FOUND)
        target_link_libraries(test_nws_api PRIVATE nlohmann_json::nlohmann_json)
    endif()
endif()

# Font converter tool
add_executable(font_converter tools/font_converter.cpp)
target_compile_features(font_converter PRIVATE cxx_std_17)
if(USE_CPM_STB)
    if(stb_ADDED)
        target_include_directories(font_converter PRIVATE ${stb_SOURCE_DIR})
    endif()
else()
    target_include_directories(font_converter PRIVATE ${STB_INCLUDE_DIR})
endif()

# Image converter tool
add_executable(image_converter tools/image_converter.cpp)
target_compile_features(image_converter PRIVATE cxx_std_17)
if(USE_CPM_STB)
    if(stb_ADDED)
        target_include_directories(image_converter PRIVATE ${stb_SOURCE_DIR})
    endif()
else()
    target_include_directories(image_converter PRIVATE ${STB_INCLUDE_DIR})
endif()

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
endif()

# Install target
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
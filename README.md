# rpi0-weather

Raspberry Pi Zero Weather Station with Inky Display - C++ Implementation

## Overview

This is a complete rewrite of the original Python-based weather station in clean C++. The application displays weather information on a 7-color Inky Impression display using a 6-panel layout.

## Features

- **Zero Dependencies**: Uses only the inky_c display library
- **Unified Codebase**: Single C++ implementation for both emulator and hardware
- **6-Panel Layout**: Weather icon, current temp, forecast, precipitation, wind, humidity
- **Cross-Platform**: Builds on Linux/macOS for development, ARM for Raspberry Pi

## Building

### Prerequisites
- CMake 3.15+
- C++17 compatible compiler (GCC 7+, Clang 5+)
- Git (for submodules)

### Build Steps
```bash
# Clone with submodules
git clone --recursive <repository-url>
cd rpi0-weather

# Or if already cloned, update submodules
git submodule update --init --recursive

# Build
mkdir build
cd build
cmake ..
make

# Run
./rpi0-weather
```

## Project Structure

```
rpi0-weather/
├── CMakeLists.txt           # CMake build configuration
├── lib/
│   └── inky_c/              # Display library submodule
├── src/
│   ├── main.cpp             # Application entry point
│   ├── weather_app.h/.cpp   # Main weather application
│   └── weather_data.h/.cpp  # Weather data structures
├── legacy/                  # Original Python/C++ implementation
└── test_nws_api.py         # NWS API validation script
```

## Development Status

- ✅ Project structure and build system
- ✅ Inky_c display integration
- ✅ Mock weather data display
- ⏳ NWS API integration
- ⏳ Font rendering system
- ⏳ Weather icon assets

## Hardware vs Emulator

The inky_c library automatically handles hardware detection:
- **Hardware**: Runs on Raspberry Pi with real Inky display
- **Emulator**: Runs on desktop with SDL2 window simulation

## Legacy Code

The original Python/C++ implementation has been preserved in the `legacy/` folder for reference. The new C++ implementation aims to be simpler and more maintainable while providing the same functionality.
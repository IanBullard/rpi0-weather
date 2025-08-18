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
cmake .
make

# Run main application (with SDL3 emulator)
./rpi0-weather

# Or test NWS API integration
./test_nws_api

# Or convert fonts
./font_converter fonts/Inter-Regular.ttf 24 fonts/inter24
```

### Available Executables
- `rpi0-weather` - Main weather application with SDL3 emulator
- `test_nws_api` - Standalone NWS API testing tool  
- `font_converter` - Font preprocessing tool for TTF/OTF → bitmap atlas

## Project Structure

```
rpi0-weather/
├── CMakeLists.txt           # CMake build configuration
├── lib/
│   └── inky_c/              # Display library submodule
├── src/
│   ├── main.cpp             # Application entry point
│   ├── weather_app.h/.cpp   # Main weather application
│   ├── weather_data.h/.cpp  # Weather data structures
│   ├── nws_client.h/.cpp    # National Weather Service API client
│   ├── weather_service.h/.cpp # High-level weather service
│   ├── sdl_emulator.h/.cpp  # SDL3 display emulator
│   ├── bitmap_font.h/.cpp   # Font data structures
│   ├── font_renderer.h/.cpp # Font rendering engine
│   └── test_nws_api.cpp     # API integration tests
├── tools/
│   └── font_converter.cpp   # Font preprocessing tool
├── fonts/                   # Generated font assets
│   ├── inter24.h/.png       # Inter Regular 24px
│   ├── inter32.h/.png       # Inter Regular 32px
│   └── inter48.h/.png       # Inter Regular 48px
├── legacy/                  # Original Python/C++ implementation
└── test_nws_api.py         # Python NWS API validation script
```

## Development Status

- ✅ Project structure and build system
- ✅ Inky_c display integration
- ✅ SDL3 emulator with real-time preview
- ✅ NWS API integration with live weather data
- ✅ Font rendering system (stb_truetype preprocessing)
- ⏳ Weather icon assets
- ⏳ Integration of fonts into display
- ⏳ Hardware testing on Raspberry Pi

## Hardware vs Emulator

The inky_c library automatically handles hardware detection:
- **Hardware**: Runs on Raspberry Pi with real Inky display
- **Emulator**: Runs on desktop with SDL2 window simulation

## Features

### Weather Data
- **Live NWS API integration** - Fetches real weather data from National Weather Service
- **Automatic station selection** - Finds nearest weather station to your coordinates  
- **Data caching** - 10-minute cache to avoid excessive API calls
- **Fallback support** - Mock data if API is unavailable

### Font System
- **stb_truetype preprocessing** - Converts TTF/OTF fonts to optimized bitmap atlases
- **Runtime bitmap rendering** - Fast text rendering with alignment support
- **Multiple font sizes** - Inter Regular at 24px, 32px, and 48px included
- **UTF-8 support** - Handles international characters

### Display
- **SDL3 emulator** - Real-time preview on desktop during development
- **Hardware compatibility** - Runs on Raspberry Pi with Inky Impression display
- **6-panel layout** - Weather icon, current temp, forecast, precipitation, wind, humidity
- **Button controls** - A=refresh, B=toggle API/mock, C/D=reserved

### Build System
- **Zero external dependencies** - Uses header-only libraries (cpp-httplib, nlohmann/json, stb)
- **Cross-platform** - Builds on macOS/Linux for development, ARM for Raspberry Pi
- **Automated font conversion** - Font preprocessing integrated into build

## Legacy Code

The original Python/C++ implementation has been preserved in the `legacy/` folder for reference. The new C++ implementation aims to be simpler and more maintainable while providing the same functionality.
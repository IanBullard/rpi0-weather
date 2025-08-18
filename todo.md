# rpi0-weather TODO List

## Current Project Status

The rpi0-weather project has been successfully refactored from Python to C++ with SDL3 integration. The project compiles and runs with a working SDL3 emulator showing a 6-panel weather display layout.

## Next Steps to Complete

### 1. **NWS API Integration** (Priority: High)
- [ ] Implement HTTP client for National Weather Service API calls
- [ ] Port the Python API logic from `test_nws_api.py` to C++
- [ ] Add JSON parsing for weather data responses
- [ ] Replace mock data with real weather data

### 2. **Font Rendering System** (Priority: High)
- [ ] Integrate a font rendering library (FreeType or stb_truetype)
- [ ] Load and render TrueType/OpenType fonts
- [ ] Replace placeholder text rendering with actual text
- [ ] Add support for different font sizes

### 3. **Weather Icons** (Priority: Medium)
- [ ] Convert existing PNG weather icons to embedded C++ data
- [ ] Implement image loading and rendering
- [ ] Map weather conditions to appropriate icons
- [ ] Display icons in the weather panel

### 4. **Configuration System** (Priority: Medium)
- [ ] Add JSON configuration file support
- [ ] Allow customizable location (latitude/longitude)
- [ ] Make update intervals configurable
- [ ] Add temperature unit preferences (F/C)

### 5. **Hardware Testing** (Priority: Low)
- [ ] Test on actual Raspberry Pi Zero hardware
- [ ] Verify Inky display rendering
- [ ] Optimize for low power consumption
- [ ] Add systemd service for auto-start

### 6. **Polish & Features** (Priority: Low)
- [ ] Implement button functionality (unit toggle, refresh, etc.)
- [ ] Add error handling and retry logic for API calls
- [ ] Improve visual layout and spacing
- [ ] Add logging system for debugging
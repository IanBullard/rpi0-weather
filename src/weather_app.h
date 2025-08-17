#pragma once

#include "weather_data.h"
#include "sdl_emulator.h"
#include <memory>

// Forward declaration for inky_c
typedef struct inky_display inky_t;

/**
 * Main weather application class
 * Handles weather data retrieval and display rendering
 */
class WeatherApp {
public:
    WeatherApp();
    ~WeatherApp();
    
    // Initialize the display
    bool initialize();
    
    // Update weather data and refresh display
    void update();
    
    // Run the main event loop
    void run();
    
    // Shutdown and cleanup
    void shutdown();
    
private:
    // Display functions
    void render_weather(const WeatherData& data);
    void draw_panel_border(int panel_x, int panel_y, int panel_w, int panel_h);
    void draw_text_centered(int x, int y, int w, int h, const std::string& text, int color);
    void clear_display();
    void show_display();
    
    // Display constants
    static constexpr int SCREEN_WIDTH = 600;
    static constexpr int SCREEN_HEIGHT = 448;
    static constexpr int PANEL_WIDTH = 196;
    static constexpr int PANEL_HEIGHT = 196;
    static constexpr int BORDER_WIDTH = 3;
    
    // Colors (matching inky_c 7-color palette)
    static constexpr int BLACK = 0;
    static constexpr int WHITE = 1;
    static constexpr int GREEN = 2;
    static constexpr int BLUE = 3;
    static constexpr int RED = 4;
    static constexpr int YELLOW = 5;
    static constexpr int ORANGE = 6;
    static constexpr int CLEAR = 7;
    
    // Panel positions (same 6-panel layout as original)
    struct Panel {
        int x, y;
    };
    static constexpr Panel PANELS[6] = {
        {BORDER_WIDTH, BORDER_WIDTH},                                    // Weather icon
        {BORDER_WIDTH * 2 + PANEL_WIDTH, BORDER_WIDTH},                 // Current temp
        {BORDER_WIDTH * 3 + PANEL_WIDTH * 2, BORDER_WIDTH},             // Min/Max temp
        {BORDER_WIDTH, BORDER_WIDTH * 2 + PANEL_HEIGHT},                // Precipitation
        {BORDER_WIDTH * 2 + PANEL_WIDTH, BORDER_WIDTH * 2 + PANEL_HEIGHT},  // Wind
        {BORDER_WIDTH * 3 + PANEL_WIDTH * 2, BORDER_WIDTH * 2 + PANEL_HEIGHT}  // Humidity
    };
    
    // Display handle
    inky_t* display_;
    
    // SDL3 emulator (for desktop testing)
    std::unique_ptr<SDL3Emulator> sdl_emulator_;
    bool use_sdl_emulator_;
    
    bool initialized_;
    
    // Button handling
    void on_button_pressed(int button);
};
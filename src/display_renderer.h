#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <memory>

// Forward declarations
class SDL3Emulator;
typedef struct inky_display inky_t;

/**
 * Unified display renderer that renders to a common backbuffer
 * and outputs to different target devices (SDL, Inky hardware, PNG files)
 */
class DisplayRenderer {
public:
    // Display dimensions
    static constexpr int SCREEN_WIDTH = 600;
    static constexpr int SCREEN_HEIGHT = 448;
    
    // Inky 7-color palette
    static constexpr uint8_t BLACK = 0;
    static constexpr uint8_t WHITE = 1;
    static constexpr uint8_t GREEN = 2;
    static constexpr uint8_t BLUE = 3;
    static constexpr uint8_t RED = 4;
    static constexpr uint8_t YELLOW = 5;
    static constexpr uint8_t ORANGE = 6;
    static constexpr uint8_t CLEAR = 7;
    
    DisplayRenderer();
    ~DisplayRenderer();
    
    // Initialize with target devices and debug flag
    bool initialize(bool use_sdl_emulator = true, inky_t* inky_display = nullptr, bool debug = false);
    void shutdown();
    
    // Common backbuffer operations
    void clear(uint8_t color = WHITE);
    void set_pixel(int x, int y, uint8_t color);
    uint8_t get_pixel(int x, int y) const;
    
    // High-level drawing operations
    void draw_rectangle(int x, int y, int w, int h, uint8_t color);
    void draw_weather_icon(int x, int y, int w, int h, const std::string& icon_name);
    void draw_text_centered(int x, int y, int w, int h, const std::string& text, uint8_t color);
    void draw_panel_border(int panel_x, int panel_y, int panel_w, int panel_h);
    
    // Output to target devices
    void present();  // Update SDL and/or Inky display
    bool save_png(const std::string& filename);  // Save backbuffer to PNG
    
    // Event handling for SDL
    void poll_events();
    bool should_quit() const;
    
private:
    // Backbuffer storage (one byte per pixel, Inky palette values)
    std::vector<uint8_t> backbuffer_;
    
    // Target devices
    std::unique_ptr<SDL3Emulator> sdl_emulator_;
    inky_t* inky_display_;
    bool use_sdl_;
    bool use_inky_;
    bool initialized_;
    bool debug_enabled_;
    
    // Helper methods
    bool is_valid_pixel(int x, int y) const;
    void update_sdl_display();
    void update_inky_display();
};
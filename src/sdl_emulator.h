#pragma once

#include <cstdint>
#include <functional>

/**
 * SDL3-based emulator for Inky display
 * Provides 600x448 window with 7-color palette and button emulation
 */
class SDL3Emulator {
public:
    // Display dimensions
    static constexpr int DISPLAY_WIDTH = 600;
    static constexpr int DISPLAY_HEIGHT = 448;
    static constexpr int WINDOW_SCALE = 1; // 1x scaling for smaller window
    
    // Inky 7-color palette
    struct Color {
        uint8_t r, g, b;
    };
    
    static constexpr Color PALETTE[8] = {
        {57, 48, 57},   // 0 - Black
        {255, 255, 255}, // 1 - White  
        {58, 91, 70},   // 2 - Green
        {61, 59, 94},   // 3 - Blue
        {156, 72, 75},  // 4 - Red
        {208, 190, 71}, // 5 - Yellow
        {177, 106, 73}, // 6 - Orange
        {255, 255, 255} // 7 - Clear (same as white)
    };
    
    // Button callback type
    using ButtonCallback = std::function<void(int button)>;
    
    SDL3Emulator();
    ~SDL3Emulator();
    
    // Initialize SDL3 window and renderer
    bool initialize();
    
    // Shutdown and cleanup
    void shutdown();
    
    // Display operations
    void clear(uint8_t color);
    void set_pixel(int x, int y, uint8_t color);
    void update();
    
    // Event handling
    void poll_events();
    bool should_quit() const { return quit_requested_; }
    
    // Button emulation
    void set_button_callback(ButtonCallback callback);
    
private:
    struct SDL_Window* window_;
    struct SDL_Renderer* renderer_;
    struct SDL_Texture* display_texture_;
    
    uint8_t* pixel_buffer_;
    bool initialized_;
    bool quit_requested_;
    
    ButtonCallback button_callback_;
    
    void handle_keydown(int scancode);
    void render_display();
};
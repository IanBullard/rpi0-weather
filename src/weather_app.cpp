#include "weather_app.h"

extern "C" {
#include <inky.h>
}
#include <iostream>
#include <ctime>
#include <iomanip>
#include <sstream>

// Declare the mock data function
WeatherData create_mock_weather_data();

WeatherApp::WeatherApp() : display_(nullptr), initialized_(false) {
}

WeatherApp::~WeatherApp() {
    shutdown();
}

bool WeatherApp::initialize() {
    if (initialized_) {
        return true;
    }
    
    // Initialize inky display (use emulator mode)
    display_ = inky_init(true);
    if (!display_) {
        std::cerr << "Failed to initialize inky display" << std::endl;
        return false;
    }
    
    initialized_ = true;
    std::cout << "Weather app initialized successfully" << std::endl;
    return true;
}

void WeatherApp::update() {
    if (!initialized_) {
        std::cerr << "App not initialized" << std::endl;
        return;
    }
    
    // Get weather data (using mock data for now)
    WeatherData data = create_mock_weather_data();
    
    if (!data.is_valid) {
        std::cerr << "Invalid weather data: " << data.error_message << std::endl;
        return;
    }
    
    // Render to display
    render_weather(data);
    
    std::cout << "Display updated with weather data" << std::endl;
}

void WeatherApp::shutdown() {
    if (display_) {
        inky_destroy(display_);
        display_ = nullptr;
    }
    initialized_ = false;
    std::cout << "Weather app shutdown" << std::endl;
}

void WeatherApp::render_weather(const WeatherData& data) {
    // Clear display
    clear_display();
    
    // Draw panel borders
    for (int i = 0; i < 6; i++) {
        draw_panel_border(PANELS[i].x, PANELS[i].y, PANEL_WIDTH, PANEL_HEIGHT);
    }
    
    // Panel 0: Weather icon placeholder
    draw_text_centered(PANELS[0].x, PANELS[0].y, PANEL_WIDTH, PANEL_HEIGHT, 
                      data.weather_icon, INKY_BLACK);
    
    // Panel 1: Current temperature
    std::string temp_str = std::to_string(data.temperature_f()) + "°F";
    draw_text_centered(PANELS[1].x, PANELS[1].y + 20, PANEL_WIDTH, 40, "Currently", INKY_BLACK);
    draw_text_centered(PANELS[1].x, PANELS[1].y + 80, PANEL_WIDTH, 60, temp_str, INKY_BLACK);
    
    // Panel 2: Min/Max temperature
    std::string max_str = "Hi " + std::to_string(data.temperature_max_f()) + "°F";
    std::string min_str = "Lo " + std::to_string(data.temperature_min_f()) + "°F";
    draw_text_centered(PANELS[2].x, PANELS[2].y + 20, PANEL_WIDTH, 40, "Forecast", INKY_BLACK);
    draw_text_centered(PANELS[2].x, PANELS[2].y + 80, PANEL_WIDTH, 40, max_str, INKY_BLACK);
    draw_text_centered(PANELS[2].x, PANELS[2].y + 120, PANEL_WIDTH, 40, min_str, INKY_BLACK);
    
    // Panel 3: Precipitation chance
    std::string precip_str = std::to_string(data.precipitation_chance_percent) + "%";
    draw_text_centered(PANELS[3].x, PANELS[3].y + 20, PANEL_WIDTH, 40, "Precip", INKY_BLACK);
    draw_text_centered(PANELS[3].x, PANELS[3].y + 80, PANEL_WIDTH, 60, precip_str, INKY_BLACK);
    
    // Panel 4: Wind
    std::string wind_speed_str = std::to_string(data.wind_speed_mph()) + " mph";
    std::string wind_dir_str = std::to_string(data.wind_direction_deg) + "°";
    draw_text_centered(PANELS[4].x, PANELS[4].y + 20, PANEL_WIDTH, 40, "Wind", INKY_BLACK);
    draw_text_centered(PANELS[4].x, PANELS[4].y + 80, PANEL_WIDTH, 40, wind_speed_str, INKY_BLACK);
    draw_text_centered(PANELS[4].x, PANELS[4].y + 120, PANEL_WIDTH, 40, wind_dir_str, INKY_BLACK);
    
    // Panel 5: Humidity/Dew
    std::string humidity_str = std::to_string(data.humidity_percent) + "%";
    std::string dew_str = std::to_string(data.dewpoint_f()) + "°F";
    draw_text_centered(PANELS[5].x, PANELS[5].y + 20, PANEL_WIDTH, 40, "Humidity", INKY_BLACK);
    draw_text_centered(PANELS[5].x, PANELS[5].y + 80, PANEL_WIDTH, 40, humidity_str, INKY_BLACK);
    draw_text_centered(PANELS[5].x, PANELS[5].y + 120, PANEL_WIDTH, 40, dew_str, INKY_BLACK);
    
    // Add timestamp at bottom
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%m/%d/%Y %I:%M%p");
    int date_y = BORDER_WIDTH * 3 + PANEL_HEIGHT * 2;
    int date_h = SCREEN_HEIGHT - date_y - BORDER_WIDTH;
    draw_text_centered(BORDER_WIDTH, date_y, SCREEN_WIDTH - BORDER_WIDTH * 2, date_h, 
                      oss.str(), INKY_BLACK);
    
    // Update display
    show_display();
}

void WeatherApp::draw_panel_border(int panel_x, int panel_y, int panel_w, int panel_h) {
    // Simple border drawing using inky_c
    for (int i = 0; i < BORDER_WIDTH; i++) {
        // Top and bottom borders
        for (int x = panel_x - i; x < panel_x + panel_w + i; x++) {
            if (x >= 0 && x < SCREEN_WIDTH) {
                if (panel_y - i >= 0) inky_set_pixel(display_, x, panel_y - i, INKY_BLACK);
                if (panel_y + panel_h + i < SCREEN_HEIGHT) inky_set_pixel(display_, x, panel_y + panel_h + i, INKY_BLACK);
            }
        }
        // Left and right borders
        for (int y = panel_y - i; y < panel_y + panel_h + i; y++) {
            if (y >= 0 && y < SCREEN_HEIGHT) {
                if (panel_x - i >= 0) inky_set_pixel(display_, panel_x - i, y, INKY_BLACK);
                if (panel_x + panel_w + i < SCREEN_WIDTH) inky_set_pixel(display_, panel_x + panel_w + i, y, INKY_BLACK);
            }
        }
    }
}

void WeatherApp::draw_text_centered(int x, int y, int w, int h, const std::string& text, int color) {
    // Simple text rendering - just place a pixel pattern for now
    // In a real implementation, this would use proper font rendering
    
    // Calculate center position for a simple 8x8 character approximation
    int char_width = 8;
    int char_height = 8;
    int text_width = text.length() * char_width;
    
    int start_x = x + (w - text_width) / 2;
    int start_y = y + (h - char_height) / 2;
    
    // Draw a simple rectangle to represent text for now
    int rect_w = std::min(text_width, w - 10);
    int rect_h = std::min(char_height, h - 10);
    
    for (int ty = 0; ty < rect_h; ty++) {
        for (int tx = 0; tx < rect_w; tx++) {
            int px = start_x + tx;
            int py = start_y + ty;
            if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                // Simple pattern to represent text
                if ((tx + ty) % 4 == 0) {
                    inky_set_pixel(display_, px, py, color);
                }
            }
        }
    }
}

void WeatherApp::clear_display() {
    inky_clear(display_, INKY_WHITE);
}

void WeatherApp::show_display() {
    inky_update(display_);
}
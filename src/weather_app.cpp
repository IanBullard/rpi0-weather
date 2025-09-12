#include "weather_app.h"
#include "weather_icons_large.h"

// For PNG output in renderAllIconsTest
#include "stb_image_write.h"

extern "C" {
#include <inky.h>
}

#include <iostream>
#include <chrono>
#include <thread>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <algorithm>

// Declare the mock data function
WeatherData create_mock_weather_data();

WeatherApp::WeatherApp()
    : renderer_(nullptr)
    , inky_display_(nullptr)
    , use_sdl_emulator_(true)
    , use_real_api_(false)
    , initialized_(false) 
{
    weather_service_ = std::make_unique<WeatherService>();
    renderer_ = std::make_unique<DisplayRenderer>();
}

WeatherApp::~WeatherApp() {
    shutdown();
}

bool WeatherApp::initialize(const std::string& config_file) {
    if (initialized_) {
        return true;
    }
    
    // Load configuration
    if (!config_.load_from_file(config_file)) {
        std::cout << "Config file not found, creating default: " << config_file << std::endl;
        if (!Config::create_default_config(config_file)) {
            std::cerr << "Failed to create default config file" << std::endl;
            return false;
        }
        if (!config_.load_from_file(config_file)) {
            std::cerr << "Failed to load default config file" << std::endl;
            return false;
        }
    }
    
    // Apply configuration
    use_sdl_emulator_ = config_.use_sdl_emulator;
    use_real_api_ = config_.use_real_api;
    
    // Set location in weather service
    if (weather_service_ && use_real_api_) {
        weather_service_->setLocation(config_.latitude, config_.longitude);
    }
    
    // Initialize unified renderer
    if (!renderer_->initialize(use_sdl_emulator_, inky_display_)) {
        std::cerr << "Failed to initialize display renderer" << std::endl;
        return false;
    }
    
    // Set button callback for SDL emulator if enabled
    if (use_sdl_emulator_) {
        // Note: Button handling would need to be added to DisplayRenderer
        // For now, just note that it's available
    }
    
    initialized_ = true;
    std::cout << "Weather app initialized successfully for " << config_.location_name << std::endl;
    return true;
}

void WeatherApp::update() {
    if (!initialized_) {
        return;
    }
    
    // Get weather data
    WeatherData data;
    if (use_real_api_ && weather_service_) {
        std::cout << "Fetching weather data from NWS API..." << std::endl;
        data = weather_service_->fetchWeatherData();
    } else {
        data = create_mock_weather_data();
    }
    
    if (!data.is_valid) {
        std::cerr << "Invalid weather data: " << data.error_message << std::endl;
        // Try to use mock data as fallback
        if (use_real_api_) {
            std::cout << "Falling back to mock data" << std::endl;
            data = create_mock_weather_data();
        }
        if (!data.is_valid) {
            return;
        }
    }
    
    // Render weather data to unified backbuffer
    render_weather(data);
    
    // Present to all target devices
    renderer_->present();
    
    std::cout << "Display updated with weather data" << std::endl;
}

void WeatherApp::run() {
    if (!initialized_) {
        std::cerr << "WeatherApp not initialized" << std::endl;
        return;
    }
    
    std::cout << "Weather app initialized. Starting main loop..." << std::endl;
    
    // Initial update
    update();
    
    // Main event loop
    while (!renderer_->should_quit()) {
        // Poll events (handles SDL events and quit requests)
        renderer_->poll_events();
        
        // Sleep for a short time to avoid busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Update weather data every minute (for now just once)
        // In a real implementation, this would check timestamps
        // For now, just keep the display updated
    }
    
    std::cout << "Exiting main loop" << std::endl;
}

void WeatherApp::shutdown() {
    if (!initialized_) {
        return;
    }
    
    if (renderer_) {
        renderer_->shutdown();
    }
    
    initialized_ = false;
    std::cout << "Weather app shutdown" << std::endl;
}

void WeatherApp::setLocation(double latitude, double longitude) {
    if (weather_service_) {
        weather_service_->setLocation(latitude, longitude);
        use_real_api_ = true;
    }
}

bool WeatherApp::renderTestFrame(const std::string& output_file) {
    if (!initialized_) {
        std::cerr << "App not initialized" << std::endl;
        return false;
    }
    
    // Get weather data
    WeatherData data;
    if (use_real_api_ && weather_service_) {
        std::cout << "Fetching weather data from NWS API..." << std::endl;
        data = weather_service_->fetchWeatherData();
    } else {
        std::cout << "Using mock weather data" << std::endl;
        data = create_mock_weather_data();
    }
    
    if (!data.is_valid) {
        std::cerr << "Invalid weather data: " << data.error_message << std::endl;
        // Try to use mock data as fallback
        if (use_real_api_) {
            std::cout << "Falling back to mock data" << std::endl;
            data = create_mock_weather_data();
        }
        if (!data.is_valid) {
            return false;
        }
    }
    
    // Render to unified backbuffer
    render_weather(data);
    
    // Save backbuffer as PNG
    return renderer_->save_png(output_file);
}

void WeatherApp::render_weather(const WeatherData& data) {
    // Clear display to white
    renderer_->clear(DisplayRenderer::WHITE);
    
    // Draw panel borders using unified renderer
    for (int i = 0; i < 6; i++) {
        renderer_->draw_panel_border(PANELS[i].x, PANELS[i].y, PANEL_WIDTH, PANEL_HEIGHT);
    }
    
    // Panel 0: Weather icon
    renderer_->draw_weather_icon(PANELS[0].x, PANELS[0].y, PANEL_WIDTH, PANEL_HEIGHT, data.weather_icon);
    
    // Panel 1: Current temperature (single value panel)
    std::string temp_str = std::to_string(data.temperature_f()) + "F";
    // Title area: full width, title font height (24pt font line_height = 24)
    constexpr int TITLE_HEIGHT = 24;
    renderer_->draw_text_centered(PANELS[1].x, PANELS[1].y, PANEL_WIDTH, TITLE_HEIGHT, "Currently", DisplayRenderer::BLACK);
    // Value area: use height of 60 pixels to force 48pt font, centered in remaining space
    constexpr int LARGE_VALUE_HEIGHT = 60;
    int remaining_height = PANEL_HEIGHT - TITLE_HEIGHT;
    int value_y = PANELS[1].y + TITLE_HEIGHT + (remaining_height - LARGE_VALUE_HEIGHT) / 2;
    renderer_->draw_text_centered(PANELS[1].x, value_y, PANEL_WIDTH, LARGE_VALUE_HEIGHT, temp_str, DisplayRenderer::BLACK);
    
    // Panel 2: Min/Max temperature (double value panel)
    std::string max_str = "Hi " + std::to_string(data.temperature_max_f()) + "F";
    std::string min_str = "Lo " + std::to_string(data.temperature_min_f()) + "F";
    // Title area: full width, title font height
    renderer_->draw_text_centered(PANELS[2].x, PANELS[2].y, PANEL_WIDTH, TITLE_HEIGHT, "Forecast", DisplayRenderer::BLACK);
    // Use 45px height for each value to force 32pt font, with smaller gaps
    constexpr int MEDIUM_VALUE_HEIGHT = 45;
    constexpr int VALUE_GAP = 10;  // Small gap between values
    remaining_height = PANEL_HEIGHT - TITLE_HEIGHT;
    int total_values_height = MEDIUM_VALUE_HEIGHT * 2 + VALUE_GAP;
    int values_start_y = PANELS[2].y + TITLE_HEIGHT + (remaining_height - total_values_height) / 2;
    renderer_->draw_text_centered(PANELS[2].x, values_start_y, PANEL_WIDTH, MEDIUM_VALUE_HEIGHT, max_str, DisplayRenderer::BLACK);
    renderer_->draw_text_centered(PANELS[2].x, values_start_y + MEDIUM_VALUE_HEIGHT + VALUE_GAP, PANEL_WIDTH, MEDIUM_VALUE_HEIGHT, min_str, DisplayRenderer::BLACK);
    
    // Panel 3: Precipitation chance (single value panel)
    std::string precip_str = std::to_string(data.precipitation_chance_percent) + "%";
    renderer_->draw_text_centered(PANELS[3].x, PANELS[3].y, PANEL_WIDTH, TITLE_HEIGHT, "Precip Chance", DisplayRenderer::BLACK);
    remaining_height = PANEL_HEIGHT - TITLE_HEIGHT;
    value_y = PANELS[3].y + TITLE_HEIGHT + (remaining_height - LARGE_VALUE_HEIGHT) / 2;
    renderer_->draw_text_centered(PANELS[3].x, value_y, PANEL_WIDTH, LARGE_VALUE_HEIGHT, precip_str, DisplayRenderer::BLACK);
    
    // Panel 4: Wind (double value panel)
    std::string wind_speed_str = std::to_string(data.wind_speed_mph()) + " mph";
    std::string wind_dir_str = std::to_string(data.wind_direction_deg) + "°";
    renderer_->draw_text_centered(PANELS[4].x, PANELS[4].y, PANEL_WIDTH, TITLE_HEIGHT, "Wind", DisplayRenderer::BLACK);
    remaining_height = PANEL_HEIGHT - TITLE_HEIGHT;
    total_values_height = MEDIUM_VALUE_HEIGHT * 2 + VALUE_GAP;
    values_start_y = PANELS[4].y + TITLE_HEIGHT + (remaining_height - total_values_height) / 2;
    renderer_->draw_text_centered(PANELS[4].x, values_start_y, PANEL_WIDTH, MEDIUM_VALUE_HEIGHT, wind_speed_str, DisplayRenderer::BLACK);
    renderer_->draw_text_centered(PANELS[4].x, values_start_y + MEDIUM_VALUE_HEIGHT + VALUE_GAP, PANEL_WIDTH, MEDIUM_VALUE_HEIGHT, wind_dir_str, DisplayRenderer::BLACK);
    
    // Panel 5: Humidity/Dew (double value panel)
    std::string humidity_str = std::to_string(data.humidity_percent) + "%";
    std::string dew_str = std::to_string(data.dewpoint_f()) + "F";
    renderer_->draw_text_centered(PANELS[5].x, PANELS[5].y, PANEL_WIDTH, TITLE_HEIGHT, "Humidity/Dew", DisplayRenderer::BLACK);
    remaining_height = PANEL_HEIGHT - TITLE_HEIGHT;
    total_values_height = MEDIUM_VALUE_HEIGHT * 2 + VALUE_GAP;
    values_start_y = PANELS[5].y + TITLE_HEIGHT + (remaining_height - total_values_height) / 2;
    renderer_->draw_text_centered(PANELS[5].x, values_start_y, PANEL_WIDTH, MEDIUM_VALUE_HEIGHT, humidity_str, DisplayRenderer::BLACK);
    renderer_->draw_text_centered(PANELS[5].x, values_start_y + MEDIUM_VALUE_HEIGHT + VALUE_GAP, PANEL_WIDTH, MEDIUM_VALUE_HEIGHT, dew_str, DisplayRenderer::BLACK);
    
    // Add timestamp at bottom
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%m/%d/%Y %I:%M%p");
    int date_y = BORDER_WIDTH * 3 + PANEL_HEIGHT * 2;
    int date_h = SCREEN_HEIGHT - date_y - BORDER_WIDTH;
    renderer_->draw_text_centered(BORDER_WIDTH, date_y, SCREEN_WIDTH - BORDER_WIDTH * 2, date_h, 
                                 oss.str(), DisplayRenderer::BLACK);
}

bool WeatherApp::renderAllIconsTest(const std::string& output_file) {
    if (!initialized_) {
        std::cerr << "App not initialized" << std::endl;
        return false;
    }
    
    // Grid layout configuration
    constexpr int ICON_SIZE = 160;
    constexpr int ICONS_PER_ROW = 8;
    constexpr int PADDING = 10;
    constexpr int TEXT_HEIGHT = 30;
    
    // Count total icons
    int total_icons = weather_icons::icons.size();
    
    // Calculate grid dimensions
    int rows = (total_icons + ICONS_PER_ROW - 1) / ICONS_PER_ROW;
    int cols = std::min(total_icons, ICONS_PER_ROW);
    
    // Calculate image dimensions
    int grid_width = cols * (ICON_SIZE + PADDING) + PADDING;
    int grid_height = rows * (ICON_SIZE + TEXT_HEIGHT + PADDING) + PADDING;
    
    std::cout << "Rendering " << total_icons << " icons in " << cols << "x" << rows << " grid" << std::endl;
    std::cout << "Grid image size: " << grid_width << "x" << grid_height << std::endl;
    
    // Create custom backbuffer for the full grid
    std::vector<uint8_t> grid_buffer(grid_width * grid_height, 1); // 1 = white
    
    // Helper function to set pixel in grid buffer
    auto set_grid_pixel = [&](int x, int y, uint8_t color) {
        if (x >= 0 && x < grid_width && y >= 0 && y < grid_height) {
            grid_buffer[y * grid_width + x] = color;
        }
    };
    
    // Helper function to render icon to grid buffer
    auto render_icon_to_grid = [&](int grid_x, int grid_y, const weather_icons::IconInfo& icon) {
        // Center the icon within the allocated space
        int icon_x = grid_x + (ICON_SIZE - icon.width) / 2;
        int icon_y = grid_y + (ICON_SIZE - icon.height) / 2;
        
        // Draw the icon to grid buffer
        for (int iy = 0; iy < icon.height; iy++) {
            for (int ix = 0; ix < icon.width; ix++) {
                int px = icon_x + ix;
                int py = icon_y + iy;
                
                if (px >= 0 && px < grid_width && py >= 0 && py < grid_height) {
                    uint8_t pixel = icon.data[iy * icon.width + ix];
                    
                    // Skip white pixels (background/transparent)
                    if (pixel != 1) {
                        set_grid_pixel(px, py, pixel);
                    }
                }
            }
        }
    };
    
    // Helper function to render simple text to grid buffer
    auto render_text_to_grid = [&](int grid_x, int grid_y, int width, int height, const std::string& text) {
        // Simple text rendering - just render as black pixels in a basic pattern
        int text_start_x = grid_x + (width - text.length() * 8) / 2;
        int text_y = grid_y + height / 2;
        
        for (size_t i = 0; i < text.length() && i < 20; i++) {
            int char_x = text_start_x + i * 10;
            
            // Simple 5x7 character rendering
            for (int dy = -3; dy <= 3; dy++) {
                for (int dx = 0; dx < 8; dx++) {
                    int px = char_x + dx;
                    int py = text_y + dy;
                    
                    // Simple pattern to make text visible
                    if ((dy == -3 || dy == 3 || dx == 0 || dx == 7) && (dx + dy) % 2 == 0) {
                        set_grid_pixel(px, py, 0); // Black
                    }
                }
            }
        }
    };
    
    // Render all icons to the grid
    int icon_index = 0;
    for (const auto& [name, icon] : weather_icons::icons) {
        int row = icon_index / ICONS_PER_ROW;
        int col = icon_index % ICONS_PER_ROW;
        
        int x = col * (ICON_SIZE + PADDING) + PADDING;
        int y = row * (ICON_SIZE + TEXT_HEIGHT + PADDING) + PADDING;
        
        // Render the icon
        render_icon_to_grid(x, y, icon);
        
        // Render the icon name below it
        render_text_to_grid(x, y + ICON_SIZE + 2, ICON_SIZE, TEXT_HEIGHT, name);
        
        icon_index++;
    }
    
    // Convert grid buffer to RGB for PNG output
    std::vector<uint8_t> rgb_buffer(grid_width * grid_height * 3);
    
    // Inky color palette for PNG conversion
    struct RGB { uint8_t r, g, b; };
    const RGB inky_palette[] = {
        {0, 0, 0},        // Black
        {255, 255, 255},  // White  
        {0, 255, 0},      // Green
        {0, 0, 255},      // Blue
        {255, 0, 0},      // Red
        {255, 255, 0},    // Yellow
        {255, 128, 0},    // Orange
        {224, 224, 224}   // Clear (light gray)
    };
    
    for (int i = 0; i < grid_width * grid_height; i++) {
        uint8_t color = grid_buffer[i];
        if (color < 8) {
            rgb_buffer[i * 3] = inky_palette[color].r;
            rgb_buffer[i * 3 + 1] = inky_palette[color].g;
            rgb_buffer[i * 3 + 2] = inky_palette[color].b;
        } else {
            // Default to white for invalid colors
            rgb_buffer[i * 3] = 255;
            rgb_buffer[i * 3 + 1] = 255;
            rgb_buffer[i * 3 + 2] = 255;
        }
    }
    
    // Save as PNG using stb_image_write
    int result = stbi_write_png(output_file.c_str(), grid_width, grid_height, 3, rgb_buffer.data(), grid_width * 3);
    return result != 0;
}
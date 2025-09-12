#include "weather_app.h"

extern "C" {
#include <inky.h>
}

#include <iostream>
#include <chrono>
#include <thread>
#include <ctime>
#include <iomanip>
#include <sstream>

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

bool WeatherApp::initialize() {
    if (initialized_) {
        return true;
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
    std::cout << "Weather app initialized successfully" << std::endl;
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
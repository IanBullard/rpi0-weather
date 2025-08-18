#include "weather_app.h"

extern "C" {
#include <inky.h>
}

#ifdef HAVE_SDL3
#include <SDL3/SDL.h>
#endif
#include <iostream>
#include <ctime>
#include <iomanip>
#include <sstream>

// Declare the mock data function
WeatherData create_mock_weather_data();

WeatherApp::WeatherApp() 
    : display_(nullptr)
    , use_sdl_emulator_(true) // Default to SDL emulator for now
    , use_real_api_(true) // Use real API by default
    , initialized_(false) 
{
    weather_service_ = std::make_unique<WeatherService>();
}

WeatherApp::~WeatherApp() {
    shutdown();
}

bool WeatherApp::initialize() {
    if (initialized_) {
        return true;
    }
    
    if (use_sdl_emulator_) {
        // Initialize SDL3 emulator
        sdl_emulator_ = std::make_unique<SDL3Emulator>();
        if (!sdl_emulator_->initialize()) {
            std::cerr << "Failed to initialize SDL3 emulator" << std::endl;
            return false;
        }
        
        // Set up button callback
        sdl_emulator_->set_button_callback([this](int button) {
            this->on_button_pressed(button);
        });
        
        display_ = nullptr; // Don't use inky_c when using SDL
    } else {
        // Initialize inky display (use emulator mode)
        display_ = inky_init(true);
        if (!display_) {
            std::cerr << "Failed to initialize inky display" << std::endl;
            return false;
        }
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
            return;
        }
    }
    
    // Render to display
    render_weather(data);
    
    std::cout << "Display updated with weather data" << std::endl;
}

void WeatherApp::run() {
    if (!initialized_) {
        std::cerr << "App not initialized" << std::endl;
        return;
    }
    
    if (use_sdl_emulator_) {
        // Update display once initially
        update();
        
        // SDL event loop
        while (!sdl_emulator_->should_quit()) {
            sdl_emulator_->poll_events();
            // Add small delay to prevent 100% CPU usage
#ifdef HAVE_SDL3
            SDL_Delay(16); // ~60 FPS
#else
            // In console mode, just run once and exit
            break;
#endif
        }
    } else {
        // Single update for console mode
        update();
    }
}

void WeatherApp::shutdown() {
    if (sdl_emulator_) {
        sdl_emulator_->shutdown();
        sdl_emulator_.reset();
    }
    
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
                if (panel_y - i >= 0) {
                    if (use_sdl_emulator_ && sdl_emulator_) {
                        sdl_emulator_->set_pixel(x, panel_y - i, 0); // Black
                    } else if (display_) {
                        inky_set_pixel(display_, x, panel_y - i, INKY_BLACK);
                    }
                }
                if (panel_y + panel_h + i < SCREEN_HEIGHT) {
                    if (use_sdl_emulator_ && sdl_emulator_) {
                        sdl_emulator_->set_pixel(x, panel_y + panel_h + i, 0); // Black
                    } else if (display_) {
                        inky_set_pixel(display_, x, panel_y + panel_h + i, INKY_BLACK);
                    }
                }
            }
        }
        // Left and right borders
        for (int y = panel_y - i; y < panel_y + panel_h + i; y++) {
            if (y >= 0 && y < SCREEN_HEIGHT) {
                if (panel_x - i >= 0) {
                    if (use_sdl_emulator_ && sdl_emulator_) {
                        sdl_emulator_->set_pixel(panel_x - i, y, 0); // Black
                    } else if (display_) {
                        inky_set_pixel(display_, panel_x - i, y, INKY_BLACK);
                    }
                }
                if (panel_x + panel_w + i < SCREEN_WIDTH) {
                    if (use_sdl_emulator_ && sdl_emulator_) {
                        sdl_emulator_->set_pixel(panel_x + panel_w + i, y, 0); // Black
                    } else if (display_) {
                        inky_set_pixel(display_, panel_x + panel_w + i, y, INKY_BLACK);
                    }
                }
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
                    if (use_sdl_emulator_ && sdl_emulator_) {
                        sdl_emulator_->set_pixel(px, py, color);
                    } else if (display_) {
                        inky_set_pixel(display_, px, py, color);
                    }
                }
            }
        }
    }
}

void WeatherApp::clear_display() {
    if (use_sdl_emulator_ && sdl_emulator_) {
        sdl_emulator_->clear(1); // White
    } else if (display_) {
        inky_clear(display_, INKY_WHITE);
    }
}

void WeatherApp::show_display() {
    if (use_sdl_emulator_ && sdl_emulator_) {
        sdl_emulator_->update();
    } else if (display_) {
        inky_update(display_);
    }
}

void WeatherApp::setLocation(double latitude, double longitude) {
    if (weather_service_) {
        weather_service_->setLocation(latitude, longitude);
    }
}

void WeatherApp::on_button_pressed(int button) {
    std::cout << "Button " << char('A' + button) << " action: ";
    
    switch (button) {
        case 0: // Button A
            std::cout << "Refresh weather data" << std::endl;
            if (weather_service_) {
                // Force refresh from API
                WeatherData data = weather_service_->forceFetch();
                if (data.is_valid) {
                    render_weather(data);
                }
            } else {
                update();
            }
            break;
        case 1: // Button B  
            std::cout << "Toggle API/Mock mode" << std::endl;
            use_real_api_ = !use_real_api_;
            std::cout << "Now using: " << (use_real_api_ ? "Real API" : "Mock data") << std::endl;
            update();
            break;
        case 2: // Button C
            std::cout << "Show detailed forecast" << std::endl;
            // TODO: Implement detailed view
            break;
        case 3: // Button D
            std::cout << "Settings menu" << std::endl;
            // TODO: Implement settings
            break;
        default:
            std::cout << "Unknown button" << std::endl;
            break;
    }
}
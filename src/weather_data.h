#pragma once

#include <string>

/**
 * Weather data structure containing all information needed for display
 */
struct WeatherData {
    // Current conditions
    int temperature_c = 0;
    int humidity_percent = 0;
    int wind_speed_kph = 0;
    int wind_direction_deg = 0;
    int dewpoint_c = 0;
    
    // Forecast
    int temperature_max_c = 0;
    int temperature_min_c = 0;
    int precipitation_chance_percent = 0;
    
    // Weather condition
    std::string weather_icon = "unknown";
    std::string location_name = "Unknown";
    
    // Status
    bool is_valid = false;
    std::string error_message;
    
    // Helper methods
    int temperature_f() const { return (temperature_c * 9 / 5) + 32; }
    int temperature_max_f() const { return (temperature_max_c * 9 / 5) + 32; }
    int temperature_min_f() const { return (temperature_min_c * 9 / 5) + 32; }
    int dewpoint_f() const { return (dewpoint_c * 9 / 5) + 32; }
    int wind_speed_mph() const { return static_cast<int>(wind_speed_kph * 0.621371); }
};
#pragma once

#include <string>
#include <ctime>

/**
 * Weather data structure containing all information needed for display
 */
struct WeatherData {
    // Current conditions (using double for more precision from API)
    double temperature_c = 0;
    int humidity_percent = 0;
    double wind_speed_kmh = 0;  // Changed from kph to kmh to match API
    int wind_direction_deg = 0;
    double dewpoint_c = 0;
    
    // Forecast
    double temperature_max_c = 0;
    double temperature_min_c = 0;
    int precipitation_chance_percent = 0;
    
    // Weather condition
    std::string weather_icon = "unknown";
    std::string weather_description = "";
    std::string location = "Unknown";  // Changed from location_name
    
    // Timestamp for caching
    std::time_t timestamp = 0;
    
    // Status
    bool is_valid = false;
    std::string error_message;
    
    // Helper methods (converting to int for display)
    int temperature_f() const { return static_cast<int>((temperature_c * 9.0 / 5.0) + 32); }
    int temperature_max_f() const { return static_cast<int>((temperature_max_c * 9.0 / 5.0) + 32); }
    int temperature_min_f() const { return static_cast<int>((temperature_min_c * 9.0 / 5.0) + 32); }
    int dewpoint_f() const { return static_cast<int>((dewpoint_c * 9.0 / 5.0) + 32); }
    int wind_speed_mph() const { return static_cast<int>(wind_speed_kmh * 0.621371); }
};
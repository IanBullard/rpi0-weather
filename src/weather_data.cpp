#include "weather_data.h"

/**
 * Create mock weather data for testing
 */
WeatherData create_mock_weather_data() {
    WeatherData data;
    
    // Mock current conditions (similar to what we got from NWS API test)
    data.temperature_c = 19;
    data.humidity_percent = 68;
    data.wind_speed_kph = 15;
    data.wind_direction_deg = 270;
    data.dewpoint_c = 13;
    
    // Mock forecast
    data.temperature_max_c = 23;
    data.temperature_min_c = 12;
    data.precipitation_chance_percent = 20;
    
    // Mock condition
    data.weather_icon = "clear_day";
    data.location_name = "Mount Marcy";
    
    data.is_valid = true;
    
    return data;
}
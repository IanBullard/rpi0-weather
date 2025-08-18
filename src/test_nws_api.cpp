#include "nws_client.h"
#include "weather_service.h"
#include <iostream>
#include <iomanip>
#include <ctime>

void print_separator() {
    std::cout << "=================================================" << std::endl;
}

void test_nws_client() {
    std::cout << "🌦️  NWS API Test for rpi0-weather (C++ version)" << std::endl;
    print_separator();
    
    // Test coordinates (Mount Marcy from original Python test)
    double latitude = 44.1076;
    double longitude = -73.9209;
    
    std::cout << "Testing location: Mount Marcy (" << latitude << ", " << longitude << ")" << std::endl;
    std::cout << std::endl;
    
    NWSClient client;
    client.setUserAgent("rpi0-weather-test/1.0");
    
    // Test 1: Get points data
    std::cout << "🔍 Testing NWS Points endpoint..." << std::endl;
    NWSPoints points = client.getPoints(latitude, longitude);
    
    if (!points.valid) {
        std::cerr << "❌ Points endpoint failed: " << client.getLastError() << std::endl;
        return;
    }
    
    std::cout << "✅ Points endpoint working" << std::endl;
    std::cout << "   Office: " << points.office_id << std::endl;
    std::cout << "   Grid: " << points.grid_x << "," << points.grid_y << std::endl;
    std::cout << std::endl;
    
    // Test 2: Get stations
    std::cout << "🔍 Testing NWS Stations endpoint..." << std::endl;
    auto stations = client.getStations(points.stations_url, latitude, longitude);
    
    if (stations.empty()) {
        std::cerr << "❌ Stations endpoint failed: " << client.getLastError() << std::endl;
    } else {
        std::cout << "✅ Stations endpoint working" << std::endl;
        std::cout << "   Found " << stations.size() << " stations" << std::endl;
        std::cout << "   Closest: " << stations[0].name << " (" << stations[0].id << ")" << std::endl;
        std::cout << "   Distance²: " << std::fixed << std::setprecision(6) << stations[0].distance_squared << std::endl;
        std::cout << std::endl;
        
        // Test 3: Get observations
        std::cout << "🔍 Testing NWS Observations endpoint..." << std::endl;
        NWSObservation obs = client.getLatestObservation(stations[0].id);
        
        if (!obs.valid) {
            std::cerr << "❌ Observations endpoint failed: " << client.getLastError() << std::endl;
        } else {
            std::cout << "✅ Observations endpoint working" << std::endl;
            if (obs.temperature_celsius.has_value()) {
                double temp_f = (obs.temperature_celsius.value() * 9.0 / 5.0) + 32;
                std::cout << "   Temperature: " << obs.temperature_celsius.value() 
                         << "°C (" << std::fixed << std::setprecision(1) << temp_f << "°F)" << std::endl;
            }
            if (obs.wind_speed_kmh.has_value() && obs.wind_direction_degrees.has_value()) {
                std::cout << "   Wind: " << obs.wind_speed_kmh.value() 
                         << " km/h @ " << obs.wind_direction_degrees.value() << "°" << std::endl;
            }
            if (obs.humidity_percent.has_value()) {
                std::cout << "   Humidity: " << obs.humidity_percent.value() << "%" << std::endl;
            }
            if (obs.dewpoint_celsius.has_value()) {
                std::cout << "   Dewpoint: " << obs.dewpoint_celsius.value() << "°C" << std::endl;
            }
            if (!obs.text_description.empty()) {
                std::cout << "   Conditions: " << obs.text_description << std::endl;
            }
        }
        std::cout << std::endl;
    }
    
    // Test 4: Get forecast
    std::cout << "🔍 Testing NWS Forecast Grid endpoint..." << std::endl;
    NWSForecast forecast = client.getForecast(points.forecast_grid_url);
    
    if (!forecast.valid) {
        std::cerr << "❌ Forecast Grid endpoint failed: " << client.getLastError() << std::endl;
    } else {
        std::cout << "✅ Forecast Grid endpoint working" << std::endl;
        if (forecast.temperature_max_celsius.has_value()) {
            std::cout << "   Max Temperature: " << forecast.temperature_max_celsius.value() << "°C" << std::endl;
        }
        if (forecast.temperature_min_celsius.has_value()) {
            std::cout << "   Min Temperature: " << forecast.temperature_min_celsius.value() << "°C" << std::endl;
        }
        if (forecast.precipitation_chance_percent.has_value()) {
            std::cout << "   Precipitation Chance: " << forecast.precipitation_chance_percent.value() << "%" << std::endl;
        }
        if (forecast.sky_cover_percent.has_value()) {
            std::cout << "   Sky Cover: " << forecast.sky_cover_percent.value() << "%" << std::endl;
        }
        if (!forecast.weather_condition.empty()) {
            std::cout << "   Weather: " << forecast.weather_condition;
            if (!forecast.weather_intensity.empty()) {
                std::cout << " (" << forecast.weather_intensity << ")";
            }
            std::cout << std::endl;
        }
    }
    
    print_separator();
    std::cout << "🏁 NWS API test complete" << std::endl;
}

void test_weather_service() {
    std::cout << std::endl;
    std::cout << "🌦️  Weather Service Test" << std::endl;
    print_separator();
    
    WeatherService service;
    service.setLocation(44.1076, -73.9209); // Mount Marcy
    
    std::cout << "Fetching complete weather data..." << std::endl;
    WeatherData data = service.fetchWeatherData();
    
    if (!data.is_valid) {
        std::cerr << "❌ Failed to fetch weather data: " << data.error_message << std::endl;
        return;
    }
    
    std::cout << "✅ Weather data fetched successfully" << std::endl;
    std::cout << std::endl;
    
    std::cout << "Current Conditions:" << std::endl;
    std::cout << "  Temperature: " << data.temperature_f() << "°F (" << data.temperature_c << "°C)" << std::endl;
    std::cout << "  Humidity: " << data.humidity_percent << "%" << std::endl;
    std::cout << "  Wind: " << data.wind_speed_mph() << " mph @ " << data.wind_direction_deg << "°" << std::endl;
    std::cout << "  Dewpoint: " << data.dewpoint_f() << "°F" << std::endl;
    
    std::cout << std::endl;
    std::cout << "Forecast:" << std::endl;
    std::cout << "  High: " << data.temperature_max_f() << "°F" << std::endl;
    std::cout << "  Low: " << data.temperature_min_f() << "°F" << std::endl;
    std::cout << "  Precipitation: " << data.precipitation_chance_percent << "%" << std::endl;
    std::cout << "  Icon: " << data.weather_icon << std::endl;
    
    if (!data.weather_description.empty()) {
        std::cout << "  Description: " << data.weather_description << std::endl;
    }
    
    std::cout << std::endl;
    std::cout << "Location: " << data.location << std::endl;
    
    // Test caching
    std::cout << std::endl;
    std::cout << "Testing cache (should return immediately)..." << std::endl;
    auto start = std::chrono::steady_clock::now();
    WeatherData cached = service.fetchWeatherData();
    auto end = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Cache fetch took: " << elapsed.count() << "ms" << std::endl;
    std::cout << "Data is " << (cached.is_valid ? "valid" : "invalid") << std::endl;
    
    print_separator();
    std::cout << "🏁 Weather Service test complete" << std::endl;
}

int main(int argc, char* argv[]) {
    bool test_service_only = false;
    
    if (argc > 1 && std::string(argv[1]) == "--service") {
        test_service_only = true;
    }
    
    if (!test_service_only) {
        test_nws_client();
    }
    
    test_weather_service();
    
    return 0;
}
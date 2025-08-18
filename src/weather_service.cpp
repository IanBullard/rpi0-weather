#include "weather_service.h"
#include <iostream>
#include <cmath>
#include <chrono>
#include <ctime>

WeatherService::WeatherService()
    : client_(std::make_unique<NWSClient>())
    , latitude_(44.1076)  // Default to Mount Marcy
    , longitude_(-73.9209)
    , cache_timeout_minutes_(10)
    , endpoints_initialized_(false)
{
}

WeatherService::~WeatherService() {
}

void WeatherService::setLocation(double latitude, double longitude) {
    if (std::abs(latitude_ - latitude) > 0.0001 || std::abs(longitude_ - longitude) > 0.0001) {
        // Location changed significantly, reset endpoints
        latitude_ = latitude;
        longitude_ = longitude;
        endpoints_initialized_ = false;
        cached_data_ = WeatherData(); // Clear cache
    }
}

void WeatherService::setUserAgent(const std::string& user_agent) {
    client_->setUserAgent(user_agent);
}

bool WeatherService::isCacheValid() const {
    if (!cached_data_.is_valid) {
        return false;
    }
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(now - last_fetch_time_);
    
    return elapsed.count() < cache_timeout_minutes_;
}

WeatherData WeatherService::fetchWeatherData() {
    if (isCacheValid()) {
        return cached_data_;
    }
    
    return forceFetch();
}

WeatherData WeatherService::forceFetch() {
    WeatherData data = fetchFromAPI();
    
    if (data.is_valid) {
        cached_data_ = data;
        last_fetch_time_ = std::chrono::steady_clock::now();
    }
    
    return data;
}

WeatherData WeatherService::fetchFromAPI() {
    WeatherData data;
    
    try {
        // Step 1: Get points data if not initialized
        if (!endpoints_initialized_) {
            std::cout << "Fetching NWS endpoints for location..." << std::endl;
            
            NWSPoints points = client_->getPoints(latitude_, longitude_);
            if (!points.valid) {
                last_error_ = "Failed to get NWS points: " + client_->getLastError();
                data.error_message = last_error_;
                return data;
            }
            
            forecast_grid_url_ = points.forecast_grid_url;
            stations_url_ = points.stations_url;
            
            // Get nearest station
            auto stations = client_->getStations(stations_url_, latitude_, longitude_);
            if (stations.empty()) {
                last_error_ = "No weather stations found";
                data.error_message = last_error_;
                return data;
            }
            
            nearest_station_id_ = stations[0].id;
            std::cout << "Using station: " << stations[0].name << " (" << nearest_station_id_ << ")" << std::endl;
            
            endpoints_initialized_ = true;
        }
        
        // Step 2: Get current observations
        NWSObservation obs = client_->getLatestObservation(nearest_station_id_);
        if (!obs.valid) {
            std::cout << "Warning: Could not get current observations" << std::endl;
        } else {
            if (obs.temperature_celsius.has_value()) {
                data.temperature_c = obs.temperature_celsius.value();
            }
            if (obs.dewpoint_celsius.has_value()) {
                data.dewpoint_c = obs.dewpoint_celsius.value();
            }
            if (obs.wind_speed_kmh.has_value()) {
                data.wind_speed_kmh = obs.wind_speed_kmh.value();
            }
            if (obs.wind_direction_degrees.has_value()) {
                data.wind_direction_deg = obs.wind_direction_degrees.value();
            }
            if (obs.humidity_percent.has_value()) {
                data.humidity_percent = static_cast<int>(obs.humidity_percent.value());
            }
            data.weather_description = obs.text_description;
        }
        
        // Step 3: Get forecast data
        NWSForecast forecast = client_->getForecast(forecast_grid_url_);
        if (!forecast.valid) {
            std::cout << "Warning: Could not get forecast data" << std::endl;
        } else {
            if (forecast.temperature_max_celsius.has_value()) {
                data.temperature_max_c = forecast.temperature_max_celsius.value();
            }
            if (forecast.temperature_min_celsius.has_value()) {
                data.temperature_min_c = forecast.temperature_min_celsius.value();
            }
            if (forecast.precipitation_chance_percent.has_value()) {
                data.precipitation_chance_percent = forecast.precipitation_chance_percent.value();
            }
            
            // Select appropriate weather icon
            int sky_cover = forecast.sky_cover_percent.value_or(0);
            auto now = std::time(nullptr);
            auto tm = *std::localtime(&now);
            bool is_day = (tm.tm_hour >= 6 && tm.tm_hour < 18);
            
            data.weather_icon = selectWeatherIcon(forecast.weather_condition, sky_cover, is_day);
        }
        
        // Mark as valid if we got at least some data
        if (obs.valid || forecast.valid) {
            data.is_valid = true;
            data.location = "Lat: " + std::to_string(latitude_) + ", Lon: " + std::to_string(longitude_);
            
            // Update timestamp
            auto now = std::time(nullptr);
            data.timestamp = std::time(nullptr);
        } else {
            last_error_ = "No valid data received from NWS";
            data.error_message = last_error_;
        }
        
    } catch (const std::exception& e) {
        last_error_ = std::string("Exception in fetchFromAPI: ") + e.what();
        data.error_message = last_error_;
        data.is_valid = false;
    }
    
    return data;
}

std::string WeatherService::selectWeatherIcon(const std::string& condition, int sky_cover, bool is_day) {
    // Map weather conditions to icon names
    // This follows the logic from the Python implementation
    
    if (condition == "thunderstorms") {
        return is_day ? "⛈️ day" : "⛈️ night";
    } else if (condition == "rain" || condition == "rain_showers") {
        return is_day ? "🌧️ day" : "🌧️ night";
    } else if (condition == "snow" || condition == "snow_showers") {
        return is_day ? "❄️ day" : "❄️ night";
    } else if (condition == "fog") {
        return "🌫️";
    } else if (sky_cover > 80) {
        return "☁️";
    } else if (sky_cover > 25) {
        return is_day ? "⛅ day" : "☁️ night";
    } else {
        return is_day ? "☀️" : "🌙";
    }
}
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
            forecast_url_ = points.forecast_url;
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
        
        // Step 3: Get forecast data with NWS icons
        NWSForecast forecast = client_->getForecastWithIcon(forecast_url_);
        if (!forecast.valid) {
            std::cout << "Warning: Could not get forecast with icon data" << std::endl;
            
            // Fallback to grid data for temperature info
            NWSForecast grid_forecast = client_->getForecast(forecast_grid_url_);
            if (grid_forecast.valid) {
                if (grid_forecast.temperature_max_celsius.has_value()) {
                    data.temperature_max_c = grid_forecast.temperature_max_celsius.value();
                }
                if (grid_forecast.temperature_min_celsius.has_value()) {
                    data.temperature_min_c = grid_forecast.temperature_min_celsius.value();
                }
                if (grid_forecast.precipitation_chance_percent.has_value()) {
                    data.precipitation_chance_percent = grid_forecast.precipitation_chance_percent.value();
                }
            }
        } else {
            // Use forecast data with NWS-provided icon
            if (forecast.temperature_max_celsius.has_value()) {
                data.temperature_max_c = forecast.temperature_max_celsius.value();
            }
            
            // Use NWS-provided weather icon directly
            if (!forecast.weather_icon.empty()) {
                data.weather_icon = forecast.weather_icon;
            } else {
                data.weather_icon = "na";  // Unknown/fallback icon
            }
            
            // Use detailed forecast as weather description
            data.weather_description = forecast.weather_condition;
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


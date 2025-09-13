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
        
        // Step 3: Get grid forecast data for detailed weather conditions
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

            // Determine the weather icon based on conditions like Python version
            data.weather_icon = determineWeatherIcon(grid_forecast, obs);

            if (!grid_forecast.weather_condition.empty()) {
                data.weather_description = grid_forecast.weather_condition;
            }
        } else {
            // Fallback to simple forecast with NWS icons
            NWSForecast forecast = client_->getForecastWithIcon(forecast_url_);
            if (forecast.valid) {
                if (forecast.temperature_max_celsius.has_value()) {
                    data.temperature_max_c = forecast.temperature_max_celsius.value();
                }
                if (forecast.precipitation_chance_percent.has_value()) {
                    data.precipitation_chance_percent = forecast.precipitation_chance_percent.value();
                }
                if (!forecast.weather_icon.empty()) {
                    data.weather_icon = forecast.weather_icon;
                } else {
                    data.weather_icon = "na";
                }
                data.weather_description = forecast.weather_condition;
            } else {
                data.weather_icon = "na";
            }
        }
        
        // Mark as valid if we got at least some data
        if (obs.valid || grid_forecast.valid) {
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



std::string WeatherService::determineWeatherIcon(const NWSForecast& forecast, const NWSObservation& obs) const {
    // Determine the appropriate weather icon based on weather conditions
    // This follows the logic from the Python version

    // Get current time to determine day/night
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm* local_time = std::localtime(&now_time);
    int hour = local_time->tm_hour;
    bool is_day = (hour >= 6 && hour < 18);

    // Get weather conditions
    std::string weather = forecast.weather_condition;
    std::string intensity = forecast.weather_intensity;
    int sky_cover = forecast.sky_cover_percent.value_or(0);

    // Check for various weather conditions
    bool is_foggy = (weather.find("fog") != std::string::npos) ||
                    (weather.find("Fog") != std::string::npos);
    bool is_blowing = (weather.find("blowing") != std::string::npos) ||
                      (weather.find("Blowing") != std::string::npos) ||
                      (weather.find("wind") != std::string::npos) ||
                      (weather.find("Wind") != std::string::npos);
    bool is_thunder = (weather.find("thunder") != std::string::npos) ||
                      (weather.find("Thunder") != std::string::npos);
    bool is_rain = (weather.find("rain") != std::string::npos) ||
                   (weather.find("Rain") != std::string::npos) ||
                   (weather.find("drizzle") != std::string::npos) ||
                   (weather.find("Drizzle") != std::string::npos) ||
                   (weather.find("shower") != std::string::npos) ||
                   (weather.find("Shower") != std::string::npos);
    bool is_snow = (weather.find("snow") != std::string::npos) ||
                   (weather.find("Snow") != std::string::npos);
    bool is_freezing = (weather.find("freezing") != std::string::npos) ||
                       (weather.find("Freezing") != std::string::npos);
    bool is_hail = (weather.find("hail") != std::string::npos) ||
                   (weather.find("Hail") != std::string::npos);
    bool is_sleet = (weather.find("sleet") != std::string::npos) ||
                    (weather.find("Sleet") != std::string::npos);

    // Check intensity
    bool is_light = (intensity == "very_light" || intensity == "light");
    bool is_moderate = (intensity == "moderate");
    bool is_heavy = (intensity == "heavy" || intensity == "very_heavy");

    // Map to icon numbers based on conditions
    // The icon mapping is based on analyzing the actual icon images:
    // 00-07: Heavy rain/storm variations
    // 08-15: Light to moderate rain/clouds
    // 16: Light clouds/sun day
    // 17: Thunderstorm
    // 18: Fog
    // 19: Clear sun
    // 20: Fog
    // 21: Moon (clear night)
    // 22: Sun hot
    // 23: Light clouds/moon night
    // 24-31: Night variations
    // 32-39: Day variations with weather
    // 40-47: Thunderstorms and special conditions

    if (is_hail) {
        return "04";  // Hail storm
    } else if (is_thunder) {
        if (is_light) {
            return is_day ? "03" : "38";  // Light thunderstorm day/night
        } else {
            return "17";  // Heavy thunderstorm
        }
    } else if (is_sleet || (is_rain && is_snow)) {
        return "05";  // Rain/snow mix
    } else if (is_freezing && is_rain) {
        if (is_heavy) {
            return "02";  // Heavy freezing rain
        } else {
            return "01";  // Light freezing rain
        }
    } else if (is_foggy) {
        if (is_light) {
            return is_day ? "18" : "20";  // Fog day/night
        } else {
            return "20";  // Heavy fog
        }
    } else if (is_snow) {
        if (is_light) {
            return is_day ? "14" : "46";  // Light snow day/night
        } else if (is_moderate) {
            return "13";  // Moderate snow
        } else {
            return "12";  // Heavy snow
        }
    } else if (is_rain) {
        if (is_light) {
            return is_day ? "09" : "45";  // Light rain day/night
        } else if (is_moderate) {
            return "10";  // Moderate rain
        } else {
            return "00";  // Heavy rain
        }
    } else if (is_blowing) {
        return "23";  // Windy
    } else {
        // Clear or cloudy conditions based on sky cover
        if (sky_cover > 80) {
            return "16";  // Overcast clouds
        } else if (sky_cover > 60) {
            return is_day ? "26" : "31";  // Mostly cloudy day/night
        } else if (sky_cover > 40) {
            return is_day ? "28" : "27";  // Partly cloudy day/night
        } else if (sky_cover > 25) {
            return is_day ? "30" : "29";  // Few clouds day/night
        } else {
            // Clear sky
            if (is_day) {
                // Check if it is hot (temperature > 100F / 38C)
                if (obs.temperature_celsius.value_or(0) > 38) {
                    return "22";  // Hot sun
                } else {
                    return "19";  // Clear day
                }
            } else {
                return "21";  // Clear night (moon)
            }
        }
    }
}

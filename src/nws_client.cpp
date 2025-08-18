#include "nws_client.h"
#include <httplib.h>
#include <iostream>
#include <sstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <algorithm>

NWSClient::NWSClient() 
    : user_agent_("rpi0-weather/1.0")
    , timeout_seconds_(10)
{
}

NWSClient::~NWSClient() {
}

void NWSClient::setUserAgent(const std::string& user_agent) {
    user_agent_ = user_agent;
}

void NWSClient::setTimeout(int timeout_seconds) {
    timeout_seconds_ = timeout_seconds;
}

std::optional<nlohmann::json> NWSClient::fetchJSON(const std::string& url) {
    try {
        httplib::Client client(std::string("https://") + NWS_BASE_URL);
        client.set_connection_timeout(timeout_seconds_);
        client.set_read_timeout(timeout_seconds_);
        client.set_follow_location(true);
        
        httplib::Headers headers = {
            {"User-Agent", user_agent_},
            {"Accept", "application/json"}
        };
        
        auto res = client.Get(url.c_str(), headers);
        
        if (!res) {
            last_error_ = "Network request failed";
            return std::nullopt;
        }
        
        if (res->status != 200) {
            last_error_ = "HTTP " + std::to_string(res->status) + ": " + res->reason;
            return std::nullopt;
        }
        
        return nlohmann::json::parse(res->body);
        
    } catch (const std::exception& e) {
        last_error_ = std::string("Exception: ") + e.what();
        return std::nullopt;
    }
}

NWSPoints NWSClient::getPoints(double latitude, double longitude) {
    NWSPoints result;
    
    std::stringstream url;
    url << "/points/" << std::fixed << std::setprecision(4) << latitude << "," << longitude;
    
    auto json_opt = fetchJSON(url.str());
    if (!json_opt.has_value()) {
        return result;
    }
    
    try {
        const auto& json = json_opt.value();
        const auto& props = json["properties"];
        
        result.valid = true;
        result.forecast_grid_url = props["forecastGridData"].get<std::string>();
        result.stations_url = props["observationStations"].get<std::string>();
        result.forecast_url = props["forecast"].get<std::string>();
        result.forecast_hourly_url = props["forecastHourly"].get<std::string>();
        result.office_id = props["gridId"].get<std::string>();
        result.grid_x = props["gridX"].get<int>();
        result.grid_y = props["gridY"].get<int>();
        
        // Convert full URLs to paths
        std::string base = "https://api.weather.gov";
        if (result.forecast_grid_url.find(base) == 0) {
            result.forecast_grid_url = result.forecast_grid_url.substr(base.length());
        }
        if (result.stations_url.find(base) == 0) {
            result.stations_url = result.stations_url.substr(base.length());
        }
        if (result.forecast_url.find(base) == 0) {
            result.forecast_url = result.forecast_url.substr(base.length());
        }
        if (result.forecast_hourly_url.find(base) == 0) {
            result.forecast_hourly_url = result.forecast_hourly_url.substr(base.length());
        }
        
    } catch (const std::exception& e) {
        last_error_ = std::string("Failed to parse points data: ") + e.what();
        result.valid = false;
    }
    
    return result;
}

std::vector<NWSStation> NWSClient::getStations(const std::string& stations_url, double lat, double lon) {
    std::vector<NWSStation> stations;
    
    auto json_opt = fetchJSON(stations_url);
    if (!json_opt.has_value()) {
        return stations;
    }
    
    try {
        const auto& json = json_opt.value();
        const auto& features = json["features"];
        
        for (const auto& feature : features) {
            NWSStation station;
            const auto& props = feature["properties"];
            const auto& coords = feature["geometry"]["coordinates"];
            
            station.id = props["stationIdentifier"].get<std::string>();
            station.name = props["name"].get<std::string>();
            station.longitude = coords[0].get<double>();
            station.latitude = coords[1].get<double>();
            
            // Calculate distance squared (avoiding sqrt for efficiency)
            double dlat = lat - station.latitude;
            double dlon = lon - station.longitude;
            station.distance_squared = dlat * dlat + dlon * dlon;
            
            stations.push_back(station);
        }
        
        // Sort by distance
        std::sort(stations.begin(), stations.end(), 
                  [](const NWSStation& a, const NWSStation& b) {
                      return a.distance_squared < b.distance_squared;
                  });
        
    } catch (const std::exception& e) {
        last_error_ = std::string("Failed to parse stations data: ") + e.what();
    }
    
    return stations;
}

NWSObservation NWSClient::getLatestObservation(const std::string& station_id) {
    NWSObservation result;
    
    std::string url = "/stations/" + station_id + "/observations";
    auto json_opt = fetchJSON(url);
    if (!json_opt.has_value()) {
        return result;
    }
    
    try {
        const auto& json = json_opt.value();
        const auto& features = json["features"];
        
        // Find first valid temperature reading
        for (const auto& feature : features) {
            const auto& props = feature["properties"];
            
            // Check if temperature has valid quality control
            if (props.contains("temperature") && 
                props["temperature"].contains("qualityControl") &&
                props["temperature"]["qualityControl"] == "V") {
                
                result.valid = true;
                result.timestamp = props["timestamp"].get<std::string>();
                
                if (props["temperature"].contains("value") && !props["temperature"]["value"].is_null()) {
                    result.temperature_celsius = props["temperature"]["value"].get<double>();
                }
                
                if (props.contains("dewpoint") && props["dewpoint"].contains("value") && !props["dewpoint"]["value"].is_null()) {
                    result.dewpoint_celsius = props["dewpoint"]["value"].get<double>();
                }
                
                if (props.contains("windSpeed") && props["windSpeed"].contains("value") && !props["windSpeed"]["value"].is_null()) {
                    result.wind_speed_kmh = props["windSpeed"]["value"].get<double>();
                }
                
                if (props.contains("windDirection") && props["windDirection"].contains("value") && !props["windDirection"]["value"].is_null()) {
                    result.wind_direction_degrees = props["windDirection"]["value"].get<int>();
                }
                
                if (props.contains("relativeHumidity") && props["relativeHumidity"].contains("value") && !props["relativeHumidity"]["value"].is_null()) {
                    result.humidity_percent = props["relativeHumidity"]["value"].get<double>();
                }
                
                if (props.contains("barometricPressure") && props["barometricPressure"].contains("value") && !props["barometricPressure"]["value"].is_null()) {
                    result.pressure_pa = props["barometricPressure"]["value"].get<double>();
                }
                
                if (props.contains("textDescription") && !props["textDescription"].is_null()) {
                    result.text_description = props["textDescription"].get<std::string>();
                }
                
                break; // Found valid observation
            }
        }
        
    } catch (const std::exception& e) {
        last_error_ = std::string("Failed to parse observation data: ") + e.what();
        result.valid = false;
    }
    
    return result;
}

std::optional<double> NWSClient::getValueAtTime(const nlohmann::json& data, const std::string& field) {
    try {
        if (!data.contains(field) || !data[field].contains("values")) {
            return std::nullopt;
        }
        
        const auto& values = data[field]["values"];
        if (values.empty()) {
            return std::nullopt;
        }
        
        // For now, just return the first value
        // In a more sophisticated implementation, we'd parse the validTime
        // and find the value closest to current time
        if (values[0].contains("value") && !values[0]["value"].is_null()) {
            return values[0]["value"].get<double>();
        }
        
    } catch (const std::exception& e) {
        // Silently fail for individual field parsing
    }
    
    return std::nullopt;
}

NWSForecast NWSClient::getForecast(const std::string& forecast_grid_url) {
    NWSForecast result;
    
    auto json_opt = fetchJSON(forecast_grid_url);
    if (!json_opt.has_value()) {
        return result;
    }
    
    try {
        const auto& json = json_opt.value();
        const auto& props = json["properties"];
        
        result.valid = true;
        
        // Get temperature values
        result.temperature_max_celsius = getValueAtTime(props, "maxTemperature");
        result.temperature_min_celsius = getValueAtTime(props, "minTemperature");
        
        // Get precipitation chance
        auto precip = getValueAtTime(props, "probabilityOfPrecipitation");
        if (precip.has_value()) {
            result.precipitation_chance_percent = static_cast<int>(precip.value());
        }
        
        // Get sky cover
        auto sky = getValueAtTime(props, "skyCover");
        if (sky.has_value()) {
            result.sky_cover_percent = static_cast<int>(sky.value());
        }
        
        // Get weather conditions
        if (props.contains("weather") && props["weather"].contains("values")) {
            const auto& weather_values = props["weather"]["values"];
            if (!weather_values.empty() && weather_values[0].contains("value")) {
                const auto& weather_array = weather_values[0]["value"];
                if (weather_array.is_array() && !weather_array.empty()) {
                    const auto& weather = weather_array[0];
                    if (weather.contains("weather") && !weather["weather"].is_null()) {
                        result.weather_condition = weather["weather"].get<std::string>();
                    }
                    if (weather.contains("intensity") && !weather["intensity"].is_null()) {
                        result.weather_intensity = weather["intensity"].get<std::string>();
                    }
                }
            }
        }
        
    } catch (const std::exception& e) {
        last_error_ = std::string("Failed to parse forecast data: ") + e.what();
        result.valid = false;
    }
    
    return result;
}
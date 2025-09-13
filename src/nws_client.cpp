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

NWSForecast NWSClient::getForecastWithIcon(const std::string& forecast_url) {
    NWSForecast result;
    
    auto json_opt = fetchJSON(forecast_url);
    if (!json_opt.has_value()) {
        return result;
    }
    
    try {
        const auto& json = json_opt.value();
        const auto& properties = json["properties"];
        const auto& periods = properties["periods"];
        
        if (periods.empty()) {
            last_error_ = "No forecast periods found";
            return result;
        }
        
        // Get the first period (current/today's forecast)
        const auto& period = periods[0];
        
        result.valid = true;
        
        // Extract temperature
        if (period.contains("temperature") && !period["temperature"].is_null()) {
            double temp_f = period["temperature"].get<double>();
            result.temperature_max_celsius = (temp_f - 32.0) * 5.0 / 9.0;  // Convert F to C
        }
        
        // Extract weather icon URL and extract icon name
        if (period.contains("icon") && !period["icon"].is_null()) {
            std::string icon_url = period["icon"].get<std::string>();
            result.weather_icon = extractIconName(icon_url);
        }
        
        // Extract detailed forecast and short forecast
        if (period.contains("detailedForecast") && !period["detailedForecast"].is_null()) {
            result.weather_condition = period["detailedForecast"].get<std::string>();
        }
        
    } catch (const std::exception& e) {
        last_error_ = std::string("Failed to parse forecast with icon data: ") + e.what();
        result.valid = false;
    }
    
    return result;
}

std::string NWSClient::extractIconName(const std::string& icon_url) {
    // NWS icon URLs are like: https://api.weather.gov/icons/land/day/skc?size=medium
    // We want to extract the weather condition part (e.g., "skc")
    
    // Find the last slash before the query parameters
    size_t query_pos = icon_url.find('?');
    std::string url_without_query = (query_pos != std::string::npos) ? 
                                   icon_url.substr(0, query_pos) : icon_url;
    
    size_t last_slash = url_without_query.find_last_of('/');
    if (last_slash == std::string::npos) {
        return "na";  // fallback to unknown icon
    }
    
    std::string icon_name = url_without_query.substr(last_slash + 1);
    
    // Map NWS icon names to our numbered icon system
    return mapNWSIconToNumber(icon_name);
}

std::string NWSClient::mapNWSIconToNumber(const std::string& nws_icon) {
    // Map NWS icon names to our numbered weather icons (00-47)
    // Based on https://api.weather.gov/icons documentation
    
    if (nws_icon == "skc") return "01";  // Sky Clear -> Clear Day
    if (nws_icon == "few") return "02";  // Few Clouds -> Partly Cloudy Day
    if (nws_icon == "sct") return "02";  // Scattered Clouds -> Partly Cloudy Day
    if (nws_icon == "bkn") return "03";  // Broken Clouds -> Mostly Cloudy
    if (nws_icon == "ovc") return "04";  // Overcast -> Cloudy
    
    // Rain conditions
    if (nws_icon == "ra" || nws_icon == "rain") return "09";  // Rain -> Rain Day
    if (nws_icon == "shra") return "09";  // Showers -> Rain Day
    if (nws_icon == "hi_shwrs") return "09";  // Heavy Showers -> Rain Day
    
    // Snow conditions
    if (nws_icon == "sn" || nws_icon == "snow") return "13";  // Snow -> Snow Day
    if (nws_icon == "mix") return "13";  // Rain/Snow Mix -> Snow Day
    
    // Thunderstorm conditions
    if (nws_icon == "tsra") return "17";  // Thunderstorm -> Thunderstorm Day
    if (nws_icon == "hi_tsra") return "17";  // Heavy Thunderstorm -> Thunderstorm Day
    
    // Fog/Haze conditions
    if (nws_icon == "fg") return "20";  // Fog -> Fog
    if (nws_icon == "haze") return "20";  // Haze -> Fog
    
    // Wind conditions
    if (nws_icon == "wind") return "02";  // Windy -> Partly Cloudy (no specific wind icon)
    
    // Default fallback based on common patterns
    if (nws_icon.find("rain") != std::string::npos) return "09";
    if (nws_icon.find("snow") != std::string::npos) return "13";
    if (nws_icon.find("storm") != std::string::npos) return "17";
    if (nws_icon.find("cloud") != std::string::npos) return "03";
    
    return "na";  // Unknown condition
}
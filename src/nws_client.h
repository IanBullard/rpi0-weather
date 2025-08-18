#ifndef NWS_CLIENT_H
#define NWS_CLIENT_H

#include <string>
#include <optional>
#include <vector>
#include <nlohmann/json.hpp>

struct NWSStation {
    std::string id;
    std::string name;
    double latitude;
    double longitude;
    double distance_squared;  // Distance squared from target location
};

struct NWSObservation {
    bool valid = false;
    std::optional<double> temperature_celsius;
    std::optional<double> dewpoint_celsius;
    std::optional<double> wind_speed_kmh;
    std::optional<int> wind_direction_degrees;
    std::optional<double> humidity_percent;
    std::optional<double> pressure_pa;
    std::string timestamp;
    std::string text_description;
};

struct NWSForecast {
    bool valid = false;
    std::optional<double> temperature_max_celsius;
    std::optional<double> temperature_min_celsius;
    std::optional<int> precipitation_chance_percent;
    std::optional<int> sky_cover_percent;
    std::string weather_condition;
    std::string weather_intensity;
};

struct NWSPoints {
    bool valid = false;
    std::string forecast_grid_url;
    std::string stations_url;
    std::string forecast_url;
    std::string forecast_hourly_url;
    std::string office_id;
    int grid_x = 0;
    int grid_y = 0;
};

class NWSClient {
public:
    NWSClient();
    ~NWSClient();
    
    // Main API methods
    NWSPoints getPoints(double latitude, double longitude);
    std::vector<NWSStation> getStations(const std::string& stations_url, double lat, double lon);
    NWSObservation getLatestObservation(const std::string& station_id);
    NWSForecast getForecast(const std::string& forecast_grid_url);
    
    // Utility methods
    void setUserAgent(const std::string& user_agent);
    void setTimeout(int timeout_seconds);
    std::string getLastError() const { return last_error_; }
    
private:
    std::optional<nlohmann::json> fetchJSON(const std::string& url);
    std::optional<double> getValueAtTime(const nlohmann::json& data, const std::string& field);
    
    std::string user_agent_;
    int timeout_seconds_;
    std::string last_error_;
    
    static constexpr const char* NWS_BASE_URL = "api.weather.gov";
};

#endif // NWS_CLIENT_H
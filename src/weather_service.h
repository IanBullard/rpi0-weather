#ifndef WEATHER_SERVICE_H
#define WEATHER_SERVICE_H

#include "weather_data.h"
#include "nws_client.h"
#include <memory>
#include <chrono>
#include <string>

class WeatherService {
public:
    WeatherService();
    ~WeatherService();
    
    // Set location for weather data
    void setLocation(double latitude, double longitude);
    
    // Fetch weather data from NWS API
    // Returns cached data if recent enough (< 10 minutes old)
    WeatherData fetchWeatherData();
    
    // Force a fresh fetch from the API (ignores cache)
    WeatherData forceFetch();
    
    // Configuration
    void setCacheTimeout(int minutes) { cache_timeout_minutes_ = minutes; }
    void setUserAgent(const std::string& user_agent);
    
    // Get last error message
    std::string getLastError() const { return last_error_; }
    
private:
    WeatherData fetchFromAPI();
    std::string selectWeatherIcon(const std::string& condition, int sky_cover, bool is_day);
    bool isCacheValid() const;
    
    std::unique_ptr<NWSClient> client_;
    
    double latitude_;
    double longitude_;
    
    WeatherData cached_data_;
    std::chrono::steady_clock::time_point last_fetch_time_;
    int cache_timeout_minutes_;
    
    std::string last_error_;
    
    // Cached API endpoints from points lookup
    std::string forecast_grid_url_;
    std::string stations_url_;
    std::string nearest_station_id_;
    bool endpoints_initialized_;
};

#endif // WEATHER_SERVICE_H
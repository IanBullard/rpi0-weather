#pragma once

#include <string>
#include <fstream>
#include <chrono>
#include <mutex>

/**
 * Simple file logger for weather station events
 * Logs in a format suitable for log monitoring systems
 */
class Logger {
public:
    static Logger& getInstance();
    
    // Initialize logger with log file path
    bool initialize(const std::string& log_file = "/var/log/rpi0-weather.log");
    
    // Log display update with weather data
    void logDisplayUpdate(const std::string& location, 
                         int temperature,
                         const std::string& conditions,
                         const std::string& source = "NWS");
    
    // Log button press events
    void logButtonPress(char button);
    
    // Log errors
    void logError(const std::string& message);
    
    // Log informational messages
    void logInfo(const std::string& message);
    
    // Close the log file
    void close();
    
private:
    Logger() = default;
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    void writeLog(const std::string& level, const std::string& message);
    std::string getCurrentTimestamp();
    
    std::ofstream log_file_;
    std::mutex log_mutex_;
    bool initialized_ = false;
};
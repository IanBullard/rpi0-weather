#include "logger.h"
#include <iostream>
#include <iomanip>
#include <sstream>

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::~Logger() {
    close();
}

bool Logger::initialize(const std::string& log_file) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    
    if (initialized_) {
        return true;
    }
    
    // Try to open log file in append mode
    log_file_.open(log_file, std::ios::app);
    if (!log_file_.is_open()) {
        // If /var/log is not writable, try current directory
        log_file_.open("rpi0-weather.log", std::ios::app);
        if (!log_file_.is_open()) {
            std::cerr << "Failed to open log file: " << log_file << std::endl;
            return false;
        }
    }
    
    initialized_ = true;
    return true;
}

void Logger::logDisplayUpdate(const std::string& location, 
                              int temperature,
                              const std::string& conditions,
                              const std::string& source) {
    if (!initialized_) return;
    
    std::stringstream ss;
    ss << "DISPLAY_UPDATE location=\"" << location << "\" "
       << "temp=" << temperature << " "
       << "conditions=\"" << conditions << "\" "
       << "source=\"" << source << "\"";
    
    writeLog("INFO", ss.str());
}

void Logger::logButtonPress(char button) {
    if (!initialized_) return;
    
    std::stringstream ss;
    ss << "BUTTON_PRESS button=" << button;
    
    writeLog("INFO", ss.str());
}

void Logger::logError(const std::string& message) {
    writeLog("ERROR", message);
}

void Logger::logInfo(const std::string& message) {
    writeLog("INFO", message);
}

void Logger::close() {
    std::lock_guard<std::mutex> lock(log_mutex_);
    
    if (log_file_.is_open()) {
        logInfo("Weather station shutting down");
        log_file_.close();
    }
    initialized_ = false;
}

void Logger::writeLog(const std::string& level, const std::string& message) {
    if (!initialized_) return;
    
    std::lock_guard<std::mutex> lock(log_mutex_);
    
    log_file_ << getCurrentTimestamp() << " "
              << "[" << level << "] "
              << message << std::endl;
    log_file_.flush();
}

std::string Logger::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}
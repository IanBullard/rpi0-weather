#pragma once

#include <string>

struct Config {
    std::string location_name = "Round Rock, TX";
    double latitude = 30.5084;
    double longitude = -97.6781;
    bool use_real_api = true;
    bool use_sdl_emulator = true;
    
    // Load configuration from JSON file
    bool load_from_file(const std::string& config_path);
    
    // Save current configuration to JSON file
    bool save_to_file(const std::string& config_path) const;
    
    // Create default configuration file
    static bool create_default_config(const std::string& config_path);
};
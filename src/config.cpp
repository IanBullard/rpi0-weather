#include "config.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

bool Config::load_from_file(const std::string& config_path) {
    std::ifstream file(config_path);
    if (!file.is_open()) {
        std::cerr << "Could not open config file: " << config_path << std::endl;
        return false;
    }
    
    try {
        json config_json;
        file >> config_json;
        
        if (config_json.contains("location_name")) {
            location_name = config_json["location_name"];
        }
        
        if (config_json.contains("latitude")) {
            latitude = config_json["latitude"];
        }
        
        if (config_json.contains("longitude")) {
            longitude = config_json["longitude"];
        }
        
        if (config_json.contains("use_real_api")) {
            use_real_api = config_json["use_real_api"];
        }
        
        if (config_json.contains("use_sdl_emulator")) {
            use_sdl_emulator = config_json["use_sdl_emulator"];
        }
        
        if (config_json.contains("timezone")) {
            timezone = config_json["timezone"];
        }
        
        std::cout << "Loaded configuration from " << config_path << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error parsing config file: " << e.what() << std::endl;
        return false;
    }
}

bool Config::save_to_file(const std::string& config_path) const {
    try {
        json config_json;
        config_json["location_name"] = location_name;
        config_json["latitude"] = latitude;
        config_json["longitude"] = longitude;
        config_json["use_real_api"] = use_real_api;
        config_json["use_sdl_emulator"] = use_sdl_emulator;
        config_json["timezone"] = timezone;
        
        std::ofstream file(config_path);
        if (!file.is_open()) {
            std::cerr << "Could not create config file: " << config_path << std::endl;
            return false;
        }
        
        file << config_json.dump(4) << std::endl;
        std::cout << "Saved configuration to " << config_path << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error saving config file: " << e.what() << std::endl;
        return false;
    }
}

bool Config::create_default_config(const std::string& config_path) {
    Config default_config;
    return default_config.save_to_file(config_path);
}
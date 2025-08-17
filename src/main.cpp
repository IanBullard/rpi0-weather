#include "weather_app.h"
#include <iostream>
#include <chrono>
#include <thread>

int main() {
    std::cout << "Starting rpi0-weather application..." << std::endl;
    
    // Create and initialize weather app
    WeatherApp app;
    if (!app.initialize()) {
        std::cerr << "Failed to initialize weather app" << std::endl;
        return 1;
    }
    
    std::cout << "Weather app initialized. Updating display..." << std::endl;
    
    // Update the display once
    app.update();
    
    std::cout << "Display updated. Press Enter to exit..." << std::endl;
    std::cin.get();
    
    // Cleanup
    app.shutdown();
    
    std::cout << "Application terminated." << std::endl;
    return 0;
}
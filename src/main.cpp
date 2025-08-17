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
    
    std::cout << "Weather app initialized. Starting main loop..." << std::endl;
    
    // Run the main event loop (will handle SDL events or single update)
    app.run();
    
    // Cleanup
    app.shutdown();
    
    std::cout << "Application terminated." << std::endl;
    return 0;
}
#include "weather_app.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <string>

void print_usage(const char* prog_name) {
    std::cout << "Usage: " << prog_name << " [options]\n";
    std::cout << "Options:\n";
    std::cout << "  --test <output.png>  Render one frame and save as PNG\n";
    std::cout << "  --config <file>      Use specified config file (default: config.json)\n";
    std::cout << "  --help               Show this help message\n";
}

int main(int argc, char* argv[]) {
    bool test_mode = false;
    std::string output_file;
    std::string config_file = "config.json";
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--test" && i + 1 < argc) {
            test_mode = true;
            output_file = argv[++i];
        } else if (arg == "--config" && i + 1 < argc) {
            config_file = argv[++i];
        } else if (arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            print_usage(argv[0]);
            return 1;
        }
    }
    
    std::cout << "Starting rpi0-weather application..." << std::endl;
    
    // Create and initialize weather app with config file
    WeatherApp app;
    if (!app.initialize(config_file)) {
        std::cerr << "Failed to initialize weather app" << std::endl;
        return 1;
    }
    
    if (test_mode) {
        std::cout << "Test mode: Rendering single frame to " << output_file << std::endl;
        if (!app.renderTestFrame(output_file)) {
            std::cerr << "Failed to render test frame" << std::endl;
            return 1;
        }
        std::cout << "Test frame saved successfully" << std::endl;
    } else {
        std::cout << "Weather app initialized. Starting main loop..." << std::endl;
        // Run the main event loop (will handle SDL events or single update)
        app.run();
    }
    
    // Cleanup
    app.shutdown();
    
    std::cout << "Application terminated." << std::endl;
    return 0;
}
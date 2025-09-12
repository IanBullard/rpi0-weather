#include "weather_app.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <string>

void print_usage(const char* prog_name) {
    std::cout << "Usage: " << prog_name << " [options]\n";
    std::cout << "Options:\n";
    std::cout << "  --test <output.png>       Render one frame and save as PNG\n";
    std::cout << "  --test-icons <output.png> Render all weather icons in grid and save as PNG\n";
    std::cout << "  --config <file>           Use specified config file (default: config.json)\n";
    std::cout << "  --debug                   Enable verbose debug output\n";
    std::cout << "  --help                    Show this help message\n";
}

int main(int argc, char* argv[]) {
    bool test_mode = false;
    bool test_icons_mode = false;
    bool debug_mode = false;
    std::string output_file;
    std::string config_file = "config.json";
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--test" && i + 1 < argc) {
            test_mode = true;
            output_file = argv[++i];
        } else if (arg == "--test-icons" && i + 1 < argc) {
            test_icons_mode = true;
            output_file = argv[++i];
        } else if (arg == "--config" && i + 1 < argc) {
            config_file = argv[++i];
        } else if (arg == "--debug") {
            debug_mode = true;
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
    
    // Create and initialize weather app with config file and debug flag
    WeatherApp app;
    if (!app.initialize(config_file, debug_mode)) {
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
    } else if (test_icons_mode) {
        std::cout << "Test icons mode: Rendering all icons grid to " << output_file << std::endl;
        if (!app.renderAllIconsTest(output_file)) {
            std::cerr << "Failed to render icons test" << std::endl;
            return 1;
        }
        std::cout << "Icons test saved successfully" << std::endl;
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
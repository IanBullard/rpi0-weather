// Image converter tool - converts PNG images to embedded C++ header format
// This runs at build time to preprocess weather icons and other images

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <iomanip>
#include <filesystem>
#include <map>

namespace fs = std::filesystem;

// Inky Impression 7-color palette (matches the display)
struct Color {
    uint8_t r, g, b;
    uint8_t inky_value;  // The value used by Inky display
};

const Color INKY_PALETTE[] = {
    {0, 0, 0, 0},        // Black
    {255, 255, 255, 1},  // White  
    {0, 255, 0, 2},      // Green
    {0, 0, 255, 3},      // Blue
    {255, 0, 0, 4},      // Red
    {255, 255, 0, 5},    // Yellow
    {255, 128, 0, 6},    // Orange
    {224, 224, 224, 7}   // Clean (light gray, used for cleaning)
};

class ImageConverter {
public:
    ImageConverter() {}
    
    bool load_image(const std::string& filename) {
        int channels;
        unsigned char* data = stbi_load(filename.c_str(), &width_, &height_, &channels, 4);
        
        if (!data) {
            std::cerr << "Failed to load image: " << filename << std::endl;
            std::cerr << "Error: " << stbi_failure_reason() << std::endl;
            return false;
        }
        
        // Store RGBA data
        rgba_data_.clear();
        rgba_data_.reserve(width_ * height_ * 4);
        for (int i = 0; i < width_ * height_ * 4; i++) {
            rgba_data_.push_back(data[i]);
        }
        
        stbi_image_free(data);
        
        // Extract name from filename
        fs::path path(filename);
        image_name_ = path.stem().string();
        
        // Convert to Inky palette
        convert_to_inky_palette();
        
        return true;
    }
    
    bool save_header(const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open output file: " << filename << std::endl;
            return false;
        }
        
        // Create header guard
        std::string guard_name = "IMAGE_" + image_name_ + "_H";
        std::transform(guard_name.begin(), guard_name.end(), guard_name.begin(), ::toupper);
        
        file << "// Auto-generated image data for " << image_name_ << "\n";
        file << "// Dimensions: " << width_ << "x" << height_ << "\n";
        file << "#ifndef " << guard_name << "\n";
        file << "#define " << guard_name << "\n\n";
        file << "#include <cstdint>\n\n";
        
        file << "namespace image_" << image_name_ << " {\n\n";
        file << "constexpr int width = " << width_ << ";\n";
        file << "constexpr int height = " << height_ << ";\n\n";
        
        // Write palette-indexed data (one byte per pixel)
        file << "// Palette-indexed data (0=Black, 1=White, 2=Green, 3=Blue, 4=Red, 5=Yellow, 6=Orange, 7=Clean)\n";
        file << "const uint8_t data[] = {\n";
        
        for (size_t i = 0; i < inky_data_.size(); i++) {
            if (i % 16 == 0) file << "    ";
            file << "0x" << std::hex << std::setw(2) << std::setfill('0') 
                 << (int)inky_data_[i];
            if (i < inky_data_.size() - 1) file << ",";
            if (i % 16 == 15) file << "\n";
            else if (i < inky_data_.size() - 1) file << " ";
        }
        if (inky_data_.size() % 16 != 0) file << "\n";
        
        file << "};\n\n";
        file << "} // namespace image_" << image_name_ << "\n\n";
        file << "#endif // " << guard_name << "\n";
        
        file.close();
        std::cout << "Saved header to: " << filename << std::endl;
        return true;
    }
    
    bool save_all_headers(const std::string& directory, const std::string& output_file) {
        std::vector<std::string> image_files;
        
        // Collect all PNG files
        for (const auto& entry : fs::directory_iterator(directory)) {
            if (entry.path().extension() == ".png") {
                image_files.push_back(entry.path().string());
            }
        }
        
        if (image_files.empty()) {
            std::cerr << "No PNG files found in: " << directory << std::endl;
            return false;
        }
        
        // Sort for consistent ordering
        std::sort(image_files.begin(), image_files.end());
        
        std::ofstream file(output_file);
        if (!file.is_open()) {
            std::cerr << "Failed to open output file: " << output_file << std::endl;
            return false;
        }
        
        // Create header guard
        std::string guard_name = "WEATHER_ICONS_H";
        
        file << "// Auto-generated weather icon data\n";
        file << "// Contains all weather icons as embedded C++ data\n";
        file << "#ifndef " << guard_name << "\n";
        file << "#define " << guard_name << "\n\n";
        file << "#include <cstdint>\n";
        file << "#include <map>\n";
        file << "#include <string>\n\n";
        
        file << "namespace weather_icons {\n\n";
        
        // Process each image
        std::map<std::string, std::pair<int, int>> icon_info;
        
        for (const auto& img_file : image_files) {
            if (!load_image(img_file)) {
                std::cerr << "Skipping: " << img_file << std::endl;
                continue;
            }
            
            std::string icon_name = image_name_;
            std::cout << "DEBUG: Loaded " << img_file << " as " << icon_name << " - " << width_ << "x" << height_ << std::endl;
            // Prefix numeric names with "icon_" to make valid C++ identifiers
            std::string namespace_name = icon_name;
            if (!namespace_name.empty() && std::isdigit(namespace_name[0])) {
                namespace_name = "icon_" + namespace_name;
            }
            icon_info[icon_name] = {width_, height_};
            
            file << "// Icon: " << icon_name << " (" << width_ << "x" << height_ << ")\n";
            file << "namespace " << namespace_name << " {\n";
            file << "    constexpr int width = " << width_ << ";\n";
            file << "    constexpr int height = " << height_ << ";\n";
            file << "    const uint8_t data[] = {\n";
            
            for (size_t i = 0; i < inky_data_.size(); i++) {
                if (i % 16 == 0) file << "        ";
                file << "0x" << std::hex << std::setw(2) << std::setfill('0') 
                     << (int)inky_data_[i];
                if (i < inky_data_.size() - 1) file << ",";
                if (i % 16 == 15) file << "\n";
                else if (i < inky_data_.size() - 1) file << " ";
            }
            if (inky_data_.size() % 16 != 0) file << "\n";
            
            file << "    };\n";
            file << "} // namespace " << namespace_name << "\n\n";
        }
        
        // Create icon info structure
        file << "struct IconInfo {\n";
        file << "    int width;\n";
        file << "    int height;\n";
        file << "    const uint8_t* data;\n";
        file << "};\n\n";
        
        // Create icon map
        file << "const std::map<std::string, IconInfo> icons = {\n";
        for (const auto& [name, dims] : icon_info) {
            // Use prefixed namespace name for numeric icons
            std::string namespace_name = name;
            if (!namespace_name.empty() && std::isdigit(namespace_name[0])) {
                namespace_name = "icon_" + namespace_name;
            }
            std::cout << "DEBUG: Map entry for " << name << " - " << dims.first << "x" << dims.second << std::endl;
            std::cout << "DEBUG: Writing to file: " << dims.first << "x" << dims.second << std::endl;
            file << "    {\"" << name << "\", {" 
                 << dims.first << ", " << dims.second 
                 << ", " << namespace_name << "::data}},\n";
        }
        file << "};\n\n";
        
        file << "// Helper function to get icon by name\n";
        file << "inline const IconInfo* get_icon(const std::string& name) {\n";
        file << "    auto it = icons.find(name);\n";
        file << "    return (it != icons.end()) ? &it->second : nullptr;\n";
        file << "}\n\n";
        
        file << "} // namespace weather_icons\n\n";
        file << "#endif // " << guard_name << "\n";
        
        file.close();
        std::cout << "Saved combined header to: " << output_file << std::endl;
        std::cout << "Processed " << icon_info.size() << " icons\n";
        
        return true;
    }
    
private:
    void convert_to_inky_palette() {
        inky_data_.clear();
        inky_data_.reserve(width_ * height_);
        
        for (int y = 0; y < height_; y++) {
            for (int x = 0; x < width_; x++) {
                int idx = (y * width_ + x) * 4;
                uint8_t r = rgba_data_[idx];
                uint8_t g = rgba_data_[idx + 1];
                uint8_t b = rgba_data_[idx + 2];
                uint8_t a = rgba_data_[idx + 3];
                
                // Handle transparency - make transparent pixels white
                if (a < 128) {
                    inky_data_.push_back(1);  // White
                    continue;
                }
                
                // Find closest color in Inky palette
                uint8_t closest_idx = find_closest_color(r, g, b);
                inky_data_.push_back(closest_idx);
            }
        }
    }
    
    uint8_t find_closest_color(uint8_t r, uint8_t g, uint8_t b) {
        int min_distance = INT_MAX;
        uint8_t closest_idx = 0;
        
        for (size_t i = 0; i < sizeof(INKY_PALETTE) / sizeof(Color); i++) {
            const Color& c = INKY_PALETTE[i];
            int dr = r - c.r;
            int dg = g - c.g;
            int db = b - c.b;
            int distance = dr * dr + dg * dg + db * db;
            
            if (distance < min_distance) {
                min_distance = distance;
                closest_idx = c.inky_value;
            }
        }
        
        return closest_idx;
    }
    
private:
    int width_, height_;
    std::string image_name_;
    std::vector<uint8_t> rgba_data_;
    std::vector<uint8_t> inky_data_;
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage:\n";
        std::cout << "  Single image: image_converter <image.png> [output.h]\n";
        std::cout << "  Directory:    image_converter --dir <directory> <output.h>\n";
        std::cout << "\nExamples:\n";
        std::cout << "  image_converter weather_icon.png weather_icon.h\n";
        std::cout << "  image_converter --dir legacy/weather-icons/ src/weather_icons.h\n";
        return 1;
    }
    
    ImageConverter converter;
    
    if (std::string(argv[1]) == "--dir") {
        if (argc < 4) {
            std::cerr << "Directory mode requires: --dir <directory> <output.h>\n";
            return 1;
        }
        
        std::string directory = argv[2];
        std::string output = argv[3];
        
        if (!converter.save_all_headers(directory, output)) {
            return 1;
        }
    } else {
        // Single image mode
        std::string input = argv[1];
        std::string output;
        
        if (argc > 2) {
            output = argv[2];
        } else {
            // Auto-generate output filename
            fs::path path(input);
            output = path.stem().string() + ".h";
        }
        
        if (!converter.load_image(input)) {
            return 1;
        }
        
        if (!converter.save_header(output)) {
            return 1;
        }
    }
    
    std::cout << "Image conversion complete!\n";
    return 0;
}
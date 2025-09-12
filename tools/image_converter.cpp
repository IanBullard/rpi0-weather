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
    ImageConverter() : target_width_(-1), target_height_(-1), add_border_(false) {}
    
    void set_target_size(int width, int height) {
        target_width_ = width;
        target_height_ = height;
    }
    
    void set_border(bool add_border) {
        add_border_ = add_border;
    }
    
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
        
        // Scale if target dimensions are specified
        if (target_width_ > 0 && target_height_ > 0 && 
            (target_width_ != width_ || target_height_ != height_)) {
            scale_image();
        }
        
        // Add border if requested
        if (add_border_) {
            add_border_to_image();
        }
        
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
            
            file << "// Icon: " << icon_name << " (" << std::dec << width_ << "x" << height_ << ")\n";
            file << "namespace " << namespace_name << " {\n";
            file << "    constexpr int width = " << std::dec << width_ << ";\n";
            file << "    constexpr int height = " << std::dec << height_ << ";\n";
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
                 << std::dec << dims.first << ", " << std::dec << dims.second 
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
    void scale_image() {
        if (target_width_ <= 0 || target_height_ <= 0) return;
        
        std::vector<uint8_t> scaled_data(target_width_ * target_height_ * 4);
        
        // Dithering-aware scaling algorithm
        for (int y = 0; y < target_height_; y++) {
            for (int x = 0; x < target_width_; x++) {
                // Map target coordinates to source coordinates with sub-pixel precision
                float src_x_f = (x * (float)width_) / target_width_;
                float src_y_f = (y * (float)height_) / target_height_;
                
                int src_x = (int)src_x_f;
                int src_y = (int)src_y_f;
                
                // Clamp to source bounds
                src_x = std::max(0, std::min(src_x, width_ - 1));
                src_y = std::max(0, std::min(src_y, height_ - 1));
                
                uint8_t final_r, final_g, final_b, final_a;
                
                // Check if we're in a dithered area by examining neighborhood
                if (is_dithered_area(src_x, src_y)) {
                    // Use dithering-preserving sampling
                    sample_dithered_pixel(src_x_f, src_y_f, final_r, final_g, final_b, final_a);
                } else {
                    // Use high-quality sampling for smooth areas
                    sample_smooth_pixel(src_x_f, src_y_f, final_r, final_g, final_b, final_a);
                }
                
                int dst_idx = (y * target_width_ + x) * 4;
                scaled_data[dst_idx] = final_r;
                scaled_data[dst_idx + 1] = final_g;
                scaled_data[dst_idx + 2] = final_b;
                scaled_data[dst_idx + 3] = final_a;
            }
        }
        
        // Replace original data with scaled data
        rgba_data_ = std::move(scaled_data);
        width_ = target_width_;
        height_ = target_height_;
    }
    
    // Check if a pixel is in a dithered area by examining local variance
    bool is_dithered_area(int x, int y) {
        if (x < 1 || x >= width_ - 1 || y < 1 || y >= height_ - 1) return false;
        
        int variance_count = 0;
        uint8_t center_r = rgba_data_[(y * width_ + x) * 4];
        
        // Check 3x3 neighborhood for high variance (indicating dithering)
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dy == 0) continue;
                
                int nx = x + dx;
                int ny = y + dy;
                uint8_t neighbor_r = rgba_data_[(ny * width_ + nx) * 4];
                
                // Count significant color differences
                if (abs(center_r - neighbor_r) > 30) {
                    variance_count++;
                }
            }
        }
        
        // If more than 4 neighbors are significantly different, likely dithered
        return variance_count >= 4;
    }
    
    // Sample pixel for dithered areas - preserve patterns
    void sample_dithered_pixel(float src_x, float src_y, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) {
        // For dithered areas, use nearest neighbor to preserve crisp patterns
        int x = (int)(src_x + 0.5f);
        int y = (int)(src_y + 0.5f);
        
        x = std::max(0, std::min(x, width_ - 1));
        y = std::max(0, std::min(y, height_ - 1));
        
        int idx = (y * width_ + x) * 4;
        r = rgba_data_[idx];
        g = rgba_data_[idx + 1];
        b = rgba_data_[idx + 2];
        a = rgba_data_[idx + 3];
    }
    
    // Sample pixel for smooth areas - use bilinear interpolation
    void sample_smooth_pixel(float src_x, float src_y, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) {
        int x1 = (int)src_x;
        int y1 = (int)src_y;
        int x2 = std::min(x1 + 1, width_ - 1);
        int y2 = std::min(y1 + 1, height_ - 1);
        
        x1 = std::max(0, x1);
        y1 = std::max(0, y1);
        
        float fx = src_x - x1;
        float fy = src_y - y1;
        
        // Get four surrounding pixels
        int idx_tl = (y1 * width_ + x1) * 4; // top-left
        int idx_tr = (y1 * width_ + x2) * 4; // top-right
        int idx_bl = (y2 * width_ + x1) * 4; // bottom-left
        int idx_br = (y2 * width_ + x2) * 4; // bottom-right
        
        // Bilinear interpolation for each channel
        for (int c = 0; c < 4; c++) {
            float top = rgba_data_[idx_tl + c] * (1.0f - fx) + rgba_data_[idx_tr + c] * fx;
            float bottom = rgba_data_[idx_bl + c] * (1.0f - fx) + rgba_data_[idx_br + c] * fx;
            float result = top * (1.0f - fy) + bottom * fy;
            
            switch (c) {
                case 0: r = (uint8_t)(result + 0.5f); break;
                case 1: g = (uint8_t)(result + 0.5f); break;
                case 2: b = (uint8_t)(result + 0.5f); break;
                case 3: a = (uint8_t)(result + 0.5f); break;
            }
        }
    }
    
    void add_border_to_image() {
        // Create a copy of the original data for processing
        std::vector<uint8_t> original_data = rgba_data_;
        
        // Helper function to check if a pixel is transparent/background
        auto is_transparent = [&](int x, int y) -> bool {
            if (x < 0 || x >= width_ || y < 0 || y >= height_) return true;
            int idx = (y * width_ + x) * 4;
            uint8_t alpha = original_data[idx + 3];
            uint8_t r = original_data[idx];
            uint8_t g = original_data[idx + 1];
            uint8_t b = original_data[idx + 2];
            
            // Consider pixel transparent if alpha is low or if it's very close to white
            return (alpha < 128) || (r > 240 && g > 240 && b > 240 && alpha > 128);
        };
        
        // Helper function to check if a pixel is yellow or yellow-like
        auto is_yellow_pixel = [&](int x, int y) -> bool {
            if (x < 0 || x >= width_ || y < 0 || y >= height_) return false;
            if (is_transparent(x, y)) return false;
            
            int idx = (y * width_ + x) * 4;
            uint8_t r = original_data[idx];
            uint8_t g = original_data[idx + 1];
            uint8_t b = original_data[idx + 2];
            
            // Check for yellow-like colors: high red and green, low blue
            return (r > 200 && g > 200 && b < 100);
        };
        
        // Helper function to check if a yellow pixel is on the exterior edge
        auto is_yellow_exterior_edge = [&](int x, int y) -> bool {
            if (!is_yellow_pixel(x, y)) return false;
            
            // Check 8-directional neighbors - if any neighbor is transparent, this is exterior edge
            int dx[] = {-1, -1, -1, 0, 0, 1, 1, 1};
            int dy[] = {-1, 0, 1, -1, 1, -1, 0, 1};
            
            for (int i = 0; i < 8; i++) {
                int nx = x + dx[i];
                int ny = y + dy[i];
                if (is_transparent(nx, ny)) {
                    return true; // Adjacent to transparent = exterior edge
                }
            }
            return false;
        };
        
        // First pass: identify yellow exterior edge pixels
        std::vector<bool> yellow_edge_pixels(width_ * height_, false);
        for (int y = 0; y < height_; y++) {
            for (int x = 0; x < width_; x++) {
                if (is_yellow_exterior_edge(x, y)) {
                    yellow_edge_pixels[y * width_ + x] = true;
                }
            }
        }
        
        // Second pass: add black border ONLY around yellow exterior edges
        for (int y = 0; y < height_; y++) {
            for (int x = 0; x < width_; x++) {
                int idx = (y * width_ + x) * 4;
                
                // Only add borders to transparent pixels adjacent to yellow edges
                if (is_transparent(x, y)) {
                    bool should_be_border = false;
                    
                    // Check 8-directional neighbors for yellow edge pixels
                    int dx[] = {-1, -1, -1, 0, 0, 1, 1, 1};
                    int dy[] = {-1, 0, 1, -1, 1, -1, 0, 1};
                    
                    for (int i = 0; i < 8; i++) {
                        int nx = x + dx[i];
                        int ny = y + dy[i];
                        if (nx >= 0 && nx < width_ && ny >= 0 && ny < height_) {
                            if (yellow_edge_pixels[ny * width_ + nx]) {
                                should_be_border = true;
                                break;
                            }
                        }
                    }
                    
                    if (should_be_border) {
                        // Set border pixel to black (opaque)
                        rgba_data_[idx] = 0;       // R
                        rgba_data_[idx + 1] = 0;   // G
                        rgba_data_[idx + 2] = 0;   // B
                        rgba_data_[idx + 3] = 255; // A (fully opaque)
                    }
                }
                // Keep all original pixels unchanged
            }
        }
    }

    int width_, height_;
    int target_width_, target_height_;
    bool add_border_;
    std::string image_name_;
    std::vector<uint8_t> rgba_data_;
    std::vector<uint8_t> inky_data_;
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage:\n";
        std::cout << "  Single image: image_converter <image.png> [output.h] [--width W] [--height H] [--border]\n";
        std::cout << "  Directory:    image_converter --dir <directory> <output.h> [--width W] [--height H] [--border]\n";
        std::cout << "\nOptions:\n";
        std::cout << "  --width W   : Scale images to width W pixels\n";
        std::cout << "  --height H  : Scale images to height H pixels\n";
        std::cout << "  --border    : Add 1-pixel black border around images\n";
        std::cout << "\nExamples:\n";
        std::cout << "  image_converter weather_icon.png weather_icon.h\n";
        std::cout << "  image_converter --dir legacy/weather-icons/ src/weather_icons.h --width 160 --height 160 --border\n";
        return 1;
    }
    
    ImageConverter converter;
    
    // Parse width/height/border parameters
    int target_width = -1, target_height = -1;
    bool add_border = false;
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--width" && i + 1 < argc) {
            target_width = std::stoi(argv[i + 1]);
        }
        if (std::string(argv[i]) == "--height" && i + 1 < argc) {
            target_height = std::stoi(argv[i + 1]);
        }
        if (std::string(argv[i]) == "--border") {
            add_border = true;
        }
    }
    
    if (target_width > 0 && target_height > 0) {
        converter.set_target_size(target_width, target_height);
        std::cout << "Scaling icons to " << target_width << "x" << target_height << std::endl;
    }
    
    if (add_border) {
        converter.set_border(true);
        std::cout << "Adding black border to icons" << std::endl;
    }
    
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
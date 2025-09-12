// Font converter tool - converts TTF/OTF fonts to bitmap atlas format
// This runs at build time to preprocess fonts

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <set>
#include <iomanip>

// Default character set to include
const std::string DEFAULT_CHARSET = 
    " !\"#$%&'()*+,-./0123456789:;<=>?@"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`"
    "abcdefghijklmnopqrstuvwxyz{|}~°";

struct CharInfo {
    int codepoint;
    int x, y, w, h;
    int xoff, yoff;
    float advance;
};

class FontConverter {
public:
    FontConverter() : font_size_(24), atlas_size_(512) {}
    
    bool load_font(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            std::cerr << "Failed to open font file: " << filename << std::endl;
            return false;
        }
        
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        font_data_.resize(size);
        file.read(reinterpret_cast<char*>(font_data_.data()), size);
        file.close();
        
        if (!stbtt_InitFont(&font_info_, font_data_.data(), 0)) {
            std::cerr << "Failed to initialize font" << std::endl;
            return false;
        }
        
        font_name_ = filename.substr(filename.find_last_of("/\\") + 1);
        font_name_ = font_name_.substr(0, font_name_.find_last_of("."));
        // Replace hyphens with underscores for valid C++ identifiers
        std::replace(font_name_.begin(), font_name_.end(), '-', '_');
        
        return true;
    }
    
    bool create_atlas(int font_size, const std::string& charset = DEFAULT_CHARSET) {
        font_size_ = font_size;
        
        // Get font scale
        float scale = stbtt_ScaleForPixelHeight(&font_info_, font_size);
        
        // Get font metrics
        int ascent, descent, line_gap;
        stbtt_GetFontVMetrics(&font_info_, &ascent, &descent, &line_gap);
        baseline_ = (int)(ascent * scale);
        line_height_ = (int)((ascent - descent + line_gap) * scale);
        
        // Collect unique characters
        std::set<int> unique_chars;
        for (char c : charset) {
            unique_chars.insert((unsigned char)c);
        }
        
        // Clear previous data
        char_info_.clear();
        atlas_data_.clear();
        
        // Determine atlas size (start with 256x256, grow as needed)
        atlas_size_ = 256;
        while (!pack_characters(unique_chars, scale)) {
            atlas_size_ *= 2;
            if (atlas_size_ > 2048) {
                std::cerr << "Atlas size too large, reduce font size or charset" << std::endl;
                return false;
            }
        }
        
        std::cout << "Created font atlas: " << atlas_size_ << "x" << atlas_size_ 
                  << " for " << char_info_.size() << " characters" << std::endl;
        
        return true;
    }
    
    bool save_atlas_png(const std::string& filename) {
        if (atlas_data_.empty()) {
            std::cerr << "No atlas data to save" << std::endl;
            return false;
        }
        
        if (!stbi_write_png(filename.c_str(), atlas_size_, atlas_size_, 1, 
                           atlas_data_.data(), atlas_size_)) {
            std::cerr << "Failed to write PNG file: " << filename << std::endl;
            return false;
        }
        
        std::cout << "Saved atlas to: " << filename << std::endl;
        return true;
    }
    
    bool save_header(const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open header file: " << filename << std::endl;
            return false;
        }
        
        std::string guard_name = font_name_;
        std::transform(guard_name.begin(), guard_name.end(), guard_name.begin(), ::toupper);
        guard_name += "_" + std::to_string(font_size_) + "_H";
        
        file << "// Auto-generated font data for " << font_name_ << " size " << font_size_ << "\n";
        file << "#ifndef " << guard_name << "\n";
        file << "#define " << guard_name << "\n\n";
        file << "#include <cstdint>\n\n";
        
        // Font metadata
        file << "namespace font_" << font_name_ << "_" << font_size_ << " {\n\n";
        file << "constexpr int size = " << font_size_ << ";\n";
        file << "constexpr int line_height = " << line_height_ << ";\n";
        file << "constexpr int baseline = " << baseline_ << ";\n";
        file << "constexpr int atlas_width = " << atlas_size_ << ";\n";
        file << "constexpr int atlas_height = " << atlas_size_ << ";\n\n";
        
        // Character data
        file << "struct CharData {\n";
        file << "    uint32_t codepoint;\n";
        file << "    int x, y, w, h;\n";
        file << "    int xoff, yoff;\n";
        file << "    int advance;\n";
        file << "};\n\n";
        
        file << "constexpr CharData char_data[] = {\n";
        for (const auto& ch : char_info_) {
            file << "    {" << ch.codepoint << ", " 
                 << ch.x << ", " << ch.y << ", " 
                 << ch.w << ", " << ch.h << ", "
                 << ch.xoff << ", " << ch.yoff << ", "
                 << (int)ch.advance << "},\n";
        }
        file << "};\n\n";
        
        file << "constexpr int char_count = " << char_info_.size() << ";\n\n";
        
        // Atlas data as C array
        file << "const uint8_t atlas_data[] = {\n";
        for (size_t i = 0; i < atlas_data_.size(); i++) {
            if (i % 16 == 0) file << "    ";
            file << "0x" << std::hex << std::setw(2) << std::setfill('0') 
                 << (int)atlas_data_[i];
            if (i < atlas_data_.size() - 1) file << ",";
            if (i % 16 == 15) file << "\n";
            else if (i < atlas_data_.size() - 1) file << " ";
        }
        file << "\n};\n\n";
        
        file << "} // namespace font_" << font_name_ << "_" << font_size_ << "\n\n";
        file << "#endif // " << guard_name << "\n";
        
        file.close();
        std::cout << "Saved header to: " << filename << std::endl;
        return true;
    }
    
private:
    bool pack_characters(const std::set<int>& chars, float scale) {
        atlas_data_.resize(atlas_size_ * atlas_size_);
        std::fill(atlas_data_.begin(), atlas_data_.end(), 0);
        
        char_info_.clear();
        
        int x = 2, y = 2;  // Start with small padding
        int row_height = 0;
        
        for (int codepoint : chars) {
            int advance, lsb;
            stbtt_GetCodepointHMetrics(&font_info_, codepoint, &advance, &lsb);
            
            int x0, y0, x1, y1;
            stbtt_GetCodepointBitmapBox(&font_info_, codepoint, scale, scale, 
                                        &x0, &y0, &x1, &y1);
            
            int w = x1 - x0;
            int h = y1 - y0;
            
            // Check if we need to move to next row
            if (x + w + 2 > atlas_size_) {
                x = 2;
                y += row_height + 2;
                row_height = 0;
            }
            
            // Check if we've run out of space
            if (y + h + 2 > atlas_size_) {
                return false;
            }
            
            // Render the character
            if (w > 0 && h > 0) {
                stbtt_MakeCodepointBitmap(&font_info_, 
                                          atlas_data_.data() + y * atlas_size_ + x,
                                          w, h, atlas_size_, scale, scale, codepoint);
            }
            
            // Store character info
            CharInfo info;
            info.codepoint = codepoint;
            info.x = x;
            info.y = y;
            info.w = w;
            info.h = h;
            info.xoff = x0;
            info.yoff = y0 + baseline_;
            info.advance = advance * scale;
            char_info_.push_back(info);
            
            x += w + 2;
            row_height = std::max(row_height, h);
        }
        
        return true;
    }
    
private:
    std::vector<uint8_t> font_data_;
    stbtt_fontinfo font_info_;
    std::string font_name_;
    
    int font_size_;
    int atlas_size_;
    int line_height_;
    int baseline_;
    
    std::vector<uint8_t> atlas_data_;
    std::vector<CharInfo> char_info_;
};

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cout << "Usage: font_converter <font.ttf> <size> <output_prefix> [charset_file]\n";
        std::cout << "  Example: font_converter Inter.ttf 24 inter24\n";
        std::cout << "  This will create inter24.png and inter24.h\n";
        return 1;
    }
    
    std::string font_file = argv[1];
    int font_size = std::atoi(argv[2]);
    std::string output_prefix = argv[3];
    
    std::string charset = DEFAULT_CHARSET;
    if (argc > 4) {
        std::ifstream charset_file(argv[4]);
        if (charset_file.is_open()) {
            std::getline(charset_file, charset, '\0');
            charset_file.close();
        } else {
            std::cerr << "Warning: Could not open charset file, using default\n";
        }
    }
    
    FontConverter converter;
    
    if (!converter.load_font(font_file)) {
        return 1;
    }
    
    if (!converter.create_atlas(font_size, charset)) {
        return 1;
    }
    
    if (!converter.save_atlas_png(output_prefix + ".png")) {
        return 1;
    }
    
    if (!converter.save_header(output_prefix + ".h")) {
        return 1;
    }
    
    std::cout << "Font conversion complete!\n";
    return 0;
}
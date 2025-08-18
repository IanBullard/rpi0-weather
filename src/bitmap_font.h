#ifndef BITMAP_FONT_H
#define BITMAP_FONT_H

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

// Character glyph information
struct FontGlyph {
    uint32_t codepoint;     // Unicode codepoint
    int x, y;               // Position in atlas
    int width, height;      // Size of glyph
    int xoffset, yoffset;   // Rendering offset
    int advance;            // Horizontal advance
};

// Bitmap font data structure
struct BitmapFont {
    std::string name;
    int size;               // Font size in pixels
    int line_height;        // Line spacing
    int baseline;           // Baseline offset
    
    // Atlas data
    int atlas_width;
    int atlas_height;
    std::vector<uint8_t> atlas_data;  // Grayscale bitmap data
    
    // Character map
    std::unordered_map<uint32_t, FontGlyph> glyphs;
    
    // Helper methods
    const FontGlyph* get_glyph(uint32_t codepoint) const {
        auto it = glyphs.find(codepoint);
        if (it != glyphs.end()) {
            return &it->second;
        }
        // Return '?' or space as fallback
        it = glyphs.find('?');
        if (it != glyphs.end()) {
            return &it->second;
        }
        it = glyphs.find(' ');
        if (it != glyphs.end()) {
            return &it->second;
        }
        return nullptr;
    }
    
    // Calculate text dimensions
    void measure_text(const std::string& text, int& width, int& height) const;
};

// Font manager for handling multiple fonts
class FontManager {
public:
    FontManager();
    ~FontManager();
    
    // Load a font from embedded data
    bool load_embedded_font(const std::string& name, const uint8_t* data, size_t data_size);
    
    // Load a font from file (for preprocessing)
    bool load_font_from_file(const std::string& name, const std::string& path);
    
    // Get a loaded font
    const BitmapFont* get_font(const std::string& name) const;
    
    // Clear all fonts
    void clear();
    
private:
    std::unordered_map<std::string, BitmapFont> fonts_;
};

#endif // BITMAP_FONT_H
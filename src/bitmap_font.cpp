#include "bitmap_font.h"
#include <cstring>
#include <algorithm>

// UTF-8 decoding helper
static uint32_t utf8_decode_char(const char*& str) {
    const uint8_t* s = (const uint8_t*)str;
    uint32_t codepoint = 0;
    
    if ((*s & 0x80) == 0) {
        // 1-byte sequence
        codepoint = *s;
        str += 1;
    } else if ((*s & 0xE0) == 0xC0) {
        // 2-byte sequence
        codepoint = ((s[0] & 0x1F) << 6) | (s[1] & 0x3F);
        str += 2;
    } else if ((*s & 0xF0) == 0xE0) {
        // 3-byte sequence
        codepoint = ((s[0] & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);
        str += 3;
    } else if ((*s & 0xF8) == 0xF0) {
        // 4-byte sequence
        codepoint = ((s[0] & 0x07) << 18) | ((s[1] & 0x3F) << 12) | 
                   ((s[2] & 0x3F) << 6) | (s[3] & 0x3F);
        str += 4;
    } else {
        // Invalid sequence, skip byte
        str += 1;
        codepoint = '?';
    }
    
    return codepoint;
}

void BitmapFont::measure_text(const std::string& text, int& width, int& height) const {
    width = 0;
    height = line_height;
    
    int current_width = 0;
    int lines = 1;
    
    const char* str = text.c_str();
    while (*str) {
        if (*str == '\n') {
            width = std::max(width, current_width);
            current_width = 0;
            lines++;
            str++;
            continue;
        }
        
        uint32_t codepoint = utf8_decode_char(str);
        const FontGlyph* glyph = get_glyph(codepoint);
        
        if (glyph) {
            current_width += glyph->advance;
        }
    }
    
    width = std::max(width, current_width);
    height = lines * line_height;
}

FontManager::FontManager() {
}

FontManager::~FontManager() {
    clear();
}

bool FontManager::load_embedded_font(const std::string& name, const uint8_t* data, size_t data_size) {
    // This would deserialize font data from embedded binary
    // For now, this is a placeholder
    // The actual implementation would parse the serialized font data
    return false;
}

bool FontManager::load_font_from_file(const std::string& name, const std::string& path) {
    // This will be implemented in the font converter tool
    // It's not needed at runtime
    return false;
}

const BitmapFont* FontManager::get_font(const std::string& name) const {
    auto it = fonts_.find(name);
    if (it != fonts_.end()) {
        return &it->second;
    }
    return nullptr;
}

void FontManager::clear() {
    fonts_.clear();
}
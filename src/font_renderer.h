#ifndef FONT_RENDERER_H
#define FONT_RENDERER_H

#include <string>
#include <cstdint>

// Font alignment options
enum class TextAlign {
    LEFT,
    CENTER,
    RIGHT
};

enum class VerticalAlign {
    TOP,
    MIDDLE,
    BOTTOM
};

// Font renderer class
class FontRenderer {
public:
    FontRenderer();
    ~FontRenderer();
    
    // Set the target buffer for rendering
    void set_target(uint8_t* buffer, int width, int height);
    
    // Draw text at position with given color (palette index)
    void draw_text(int x, int y, const std::string& text, uint8_t color,
                   const uint8_t* font_atlas, int atlas_width, int atlas_height,
                   const void* char_data, int char_count, int line_height, int baseline);
    
    // Draw text aligned within a rectangle
    void draw_text_aligned(int x, int y, int width, int height,
                          const std::string& text, uint8_t color,
                          TextAlign h_align, VerticalAlign v_align,
                          const uint8_t* font_atlas, int atlas_width, int atlas_height,
                          const void* char_data, int char_count, int line_height, int baseline);
    
    // Measure text dimensions
    void measure_text(const std::string& text,
                     int& out_width, int& out_height,
                     const void* char_data, int char_count, int line_height);
    
private:
    // Helper to draw a single character
    void draw_char(int x, int y, uint32_t codepoint, uint8_t color,
                  const uint8_t* font_atlas, int atlas_width, int atlas_height,
                  const void* char_data, int char_count);
    
    // Helper to find character data
    const void* find_char_data(uint32_t codepoint, const void* char_data, int char_count);
    
    // UTF-8 decoding
    uint32_t decode_utf8(const char*& str);
    
    uint8_t* target_buffer_;
    int target_width_;
    int target_height_;
};

#endif // FONT_RENDERER_H
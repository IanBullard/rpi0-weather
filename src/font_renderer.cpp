#include "font_renderer.h"
#include <algorithm>
#include <vector>
#include <sstream>

// Character data structure matching font_converter output
struct CharData {
    uint32_t codepoint;
    int x, y, w, h;
    int xoff, yoff;
    int advance;
};

FontRenderer::FontRenderer() 
    : target_buffer_(nullptr)
    , target_width_(0)
    , target_height_(0) {
}

FontRenderer::~FontRenderer() {
}

void FontRenderer::set_target(uint8_t* buffer, int width, int height) {
    target_buffer_ = buffer;
    target_width_ = width;
    target_height_ = height;
}

uint32_t FontRenderer::decode_utf8(const char*& str) {
    const uint8_t* s = (const uint8_t*)str;
    uint32_t codepoint = 0;
    
    if ((*s & 0x80) == 0) {
        codepoint = *s;
        str += 1;
    } else if ((*s & 0xE0) == 0xC0) {
        codepoint = ((s[0] & 0x1F) << 6) | (s[1] & 0x3F);
        str += 2;
    } else if ((*s & 0xF0) == 0xE0) {
        codepoint = ((s[0] & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);
        str += 3;
    } else if ((*s & 0xF8) == 0xF0) {
        codepoint = ((s[0] & 0x07) << 18) | ((s[1] & 0x3F) << 12) | 
                   ((s[2] & 0x3F) << 6) | (s[3] & 0x3F);
        str += 4;
    } else {
        str += 1;
        codepoint = '?';
    }
    
    return codepoint;
}

const void* FontRenderer::find_char_data(uint32_t codepoint, const void* char_data, int char_count) {
    const CharData* chars = static_cast<const CharData*>(char_data);
    
    for (int i = 0; i < char_count; i++) {
        if (chars[i].codepoint == codepoint) {
            return &chars[i];
        }
    }
    
    // Fallback to '?' or space
    for (int i = 0; i < char_count; i++) {
        if (chars[i].codepoint == '?') {
            return &chars[i];
        }
    }
    
    for (int i = 0; i < char_count; i++) {
        if (chars[i].codepoint == ' ') {
            return &chars[i];
        }
    }
    
    return nullptr;
}

void FontRenderer::draw_char(int x, int y, uint32_t codepoint, uint8_t color,
                            const uint8_t* font_atlas, int atlas_width, int atlas_height,
                            const void* char_data, int char_count) {
    const CharData* ch = static_cast<const CharData*>(find_char_data(codepoint, char_data, char_count));
    if (!ch || !target_buffer_) {
        return;
    }
    
    int draw_x = x + ch->xoff;
    int draw_y = y + ch->yoff;
    
    // Clip and draw the character
    for (int cy = 0; cy < ch->h; cy++) {
        int target_y = draw_y + cy;
        if (target_y < 0 || target_y >= target_height_) continue;
        
        for (int cx = 0; cx < ch->w; cx++) {
            int target_x = draw_x + cx;
            if (target_x < 0 || target_x >= target_width_) continue;
            
            // Get alpha from font atlas
            int atlas_idx = (ch->y + cy) * atlas_width + (ch->x + cx);
            uint8_t alpha = font_atlas[atlas_idx];
            
            // For now, use simple threshold (could do alpha blending later)
            if (alpha > 128) {
                target_buffer_[target_y * target_width_ + target_x] = color;
            }
        }
    }
}

void FontRenderer::draw_text(int x, int y, const std::string& text, uint8_t color,
                            const uint8_t* font_atlas, int atlas_width, int atlas_height,
                            const void* char_data, int char_count, int line_height, int baseline) {
    int cursor_x = x;
    int cursor_y = y + baseline;  // Start at baseline
    
    const char* str = text.c_str();
    while (*str) {
        if (*str == '\n') {
            cursor_x = x;
            cursor_y += line_height;
            str++;
            continue;
        }
        
        uint32_t codepoint = decode_utf8(str);
        const CharData* ch = static_cast<const CharData*>(find_char_data(codepoint, char_data, char_count));
        
        if (ch) {
            draw_char(cursor_x, cursor_y, codepoint, color,
                     font_atlas, atlas_width, atlas_height, char_data, char_count);
            cursor_x += ch->advance;
        }
    }
}

void FontRenderer::measure_text(const std::string& text,
                               int& out_width, int& out_height,
                               const void* char_data, int char_count, int line_height) {
    out_width = 0;
    out_height = line_height;
    
    int current_width = 0;
    int lines = 1;
    
    const char* str = text.c_str();
    while (*str) {
        if (*str == '\n') {
            out_width = std::max(out_width, current_width);
            current_width = 0;
            lines++;
            str++;
            continue;
        }
        
        uint32_t codepoint = decode_utf8(str);
        const CharData* ch = static_cast<const CharData*>(find_char_data(codepoint, char_data, char_count));
        
        if (ch) {
            current_width += ch->advance;
        }
    }
    
    out_width = std::max(out_width, current_width);
    out_height = lines * line_height;
}

void FontRenderer::draw_text_aligned(int x, int y, int width, int height,
                                    const std::string& text, uint8_t color,
                                    TextAlign h_align, VerticalAlign v_align,
                                    const uint8_t* font_atlas, int atlas_width, int atlas_height,
                                    const void* char_data, int char_count, int line_height, int baseline) {
    // Measure text first
    int text_width, text_height;
    measure_text(text, text_width, text_height, char_data, char_count, line_height);
    
    // Calculate position based on alignment
    int draw_x = x;
    int draw_y = y;
    
    switch (h_align) {
        case TextAlign::CENTER:
            draw_x = x + (width - text_width) / 2;
            break;
        case TextAlign::RIGHT:
            draw_x = x + width - text_width;
            break;
        default:
            break;
    }
    
    switch (v_align) {
        case VerticalAlign::MIDDLE:
            draw_y = y + (height - text_height) / 2;
            break;
        case VerticalAlign::BOTTOM:
            draw_y = y + height - text_height;
            break;
        default:
            break;
    }
    
    // Draw the text at calculated position
    draw_text(draw_x, draw_y, text, color,
             font_atlas, atlas_width, atlas_height,
             char_data, char_count, line_height, baseline);
}
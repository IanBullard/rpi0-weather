#include "display_renderer.h"
#include "sdl_emulator.h"
#include "weather_icons_large.h"

// Include generated font headers
#include "../fonts/inter24.h"
#include "../fonts/inter32.h"
#include "../fonts/inter48.h"

// For PNG output
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

extern "C" {
#include <inky.h>
}

#include <iostream>
#include <cstring>

DisplayRenderer::DisplayRenderer()
    : backbuffer_(SCREEN_WIDTH * SCREEN_HEIGHT, WHITE)
    , sdl_emulator_(nullptr)
    , inky_display_(nullptr)
    , use_sdl_(false)
    , use_inky_(false)
    , initialized_(false)
    , debug_enabled_(false)
{
}

DisplayRenderer::~DisplayRenderer() {
    shutdown();
}

bool DisplayRenderer::initialize(bool use_sdl_emulator, inky_t* inky_display, bool debug) {
    if (initialized_) {
        return true;
    }
    
    debug_enabled_ = debug;
    use_sdl_ = use_sdl_emulator;
    use_inky_ = (inky_display != nullptr);
    inky_display_ = inky_display;
    
    // Initialize SDL emulator if requested
    if (use_sdl_) {
        sdl_emulator_ = std::make_unique<SDL3Emulator>();
        if (!sdl_emulator_->initialize()) {
            std::cerr << "Failed to initialize SDL3 emulator" << std::endl;
            return false;
        }
    }
    
    // Clear backbuffer to white
    clear(WHITE);
    
    initialized_ = true;
    return true;
}

void DisplayRenderer::shutdown() {
    if (!initialized_) {
        return;
    }
    
    if (sdl_emulator_) {
        sdl_emulator_->shutdown();
        sdl_emulator_.reset();
    }
    
    // Don't shutdown inky_display_ as we don't own it
    
    initialized_ = false;
    std::cout << "Display renderer shutdown" << std::endl;
}

void DisplayRenderer::clear(uint8_t color) {
    std::fill(backbuffer_.begin(), backbuffer_.end(), color);
}

void DisplayRenderer::set_pixel(int x, int y, uint8_t color) {
    if (is_valid_pixel(x, y)) {
        backbuffer_[y * SCREEN_WIDTH + x] = color;
    }
}

uint8_t DisplayRenderer::get_pixel(int x, int y) const {
    if (is_valid_pixel(x, y)) {
        return backbuffer_[y * SCREEN_WIDTH + x];
    }
    return WHITE;  // Default to white for out-of-bounds
}

void DisplayRenderer::draw_rectangle(int x, int y, int w, int h, uint8_t color) {
    for (int dy = 0; dy < h; dy++) {
        for (int dx = 0; dx < w; dx++) {
            set_pixel(x + dx, y + dy, color);
        }
    }
}

void DisplayRenderer::draw_weather_icon(int x, int y, int w, int h, const std::string& icon_name) {
    // Try to get the icon from our embedded data
    const auto* icon = weather_icons::get_icon(icon_name);
    
    if (!icon) {
        // Fall back to "na" icon if not found
        icon = weather_icons::get_icon("na");
        if (!icon) {
            // If even "na" is not found, just draw placeholder text
            draw_text_centered(x, y, w, h, "?", BLACK);
            return;
        }
    }
    
    // Calculate centering position
    int icon_x = x + (w - icon->width) / 2;
    int icon_y = y + (h - icon->height) / 2;
    
    // Draw the icon to backbuffer
    for (int iy = 0; iy < icon->height; iy++) {
        for (int ix = 0; ix < icon->width; ix++) {
            int px = icon_x + ix;
            int py = icon_y + iy;
            
            if (is_valid_pixel(px, py)) {
                uint8_t pixel = icon->data[iy * icon->width + ix];
                
                // Skip white pixels (background/transparent)
                if (pixel != WHITE) {
                    set_pixel(px, py, pixel);
                }
            }
        }
    }
}

void DisplayRenderer::draw_text_centered(int x, int y, int w, int h, const std::string& text, uint8_t color) {
    // Choose font based on available height
    const uint8_t* font_atlas = nullptr;
    const font_Inter_Regular_24::CharData* char_data_24 = nullptr;
    const font_Inter_Regular_32::CharData* char_data_32 = nullptr;
    const font_Inter_Regular_48::CharData* char_data_48 = nullptr;
    int atlas_width = 0, atlas_height = 0;
    int char_count = 0, line_height = 0, baseline = 0;
    int font_size = 24;
    
    if (h >= 60) {
        // Use 48pt font
        font_atlas = font_Inter_Regular_48::atlas_data;
        char_data_48 = font_Inter_Regular_48::char_data;
        atlas_width = font_Inter_Regular_48::atlas_width;
        atlas_height = font_Inter_Regular_48::atlas_height;
        char_count = font_Inter_Regular_48::char_count;
        line_height = font_Inter_Regular_48::line_height;
        baseline = font_Inter_Regular_48::baseline;
        font_size = 48;
    } else if (h >= 40) {
        // Use 32pt font
        font_atlas = font_Inter_Regular_32::atlas_data;
        char_data_32 = font_Inter_Regular_32::char_data;
        atlas_width = font_Inter_Regular_32::atlas_width;
        atlas_height = font_Inter_Regular_32::atlas_height;
        char_count = font_Inter_Regular_32::char_count;
        line_height = font_Inter_Regular_32::line_height;
        baseline = font_Inter_Regular_32::baseline;
        font_size = 32;
    } else {
        // Use 24pt font
        font_atlas = font_Inter_Regular_24::atlas_data;
        char_data_24 = font_Inter_Regular_24::char_data;
        atlas_width = font_Inter_Regular_24::atlas_width;
        atlas_height = font_Inter_Regular_24::atlas_height;
        char_count = font_Inter_Regular_24::char_count;
        line_height = font_Inter_Regular_24::line_height;
        baseline = font_Inter_Regular_24::baseline;
        font_size = 24;
    }
    
    // Calculate text width for centering with UTF-8 decoding
    int text_width = 0;
    for (size_t i = 0; i < text.size();) {
        uint32_t codepoint;
        
        // Simple UTF-8 decoding for common characters
        unsigned char byte1 = static_cast<unsigned char>(text[i]);
        if (byte1 < 0x80) {
            // ASCII character (0xxxxxxx)
            codepoint = byte1;
            i++;
        } else if ((byte1 & 0xE0) == 0xC0 && i + 1 < text.size()) {
            // 2-byte UTF-8 character (110xxxxx 10xxxxxx)
            unsigned char byte2 = static_cast<unsigned char>(text[i + 1]);
            if ((byte2 & 0xC0) == 0x80) {
                codepoint = ((byte1 & 0x1F) << 6) | (byte2 & 0x3F);
                i += 2;
            } else {
                // Invalid UTF-8, skip this byte
                i++;
                continue;
            }
        } else {
            // Unsupported UTF-8 sequence or invalid, skip this byte
            i++;
            continue;
        }
        
        // Find character in font data
        bool found = false;
        int advance = 0;
        
        if (font_size == 48 && char_data_48) {
            for (int i = 0; i < char_count; i++) {
                if (char_data_48[i].codepoint == codepoint) {
                    advance = char_data_48[i].advance;
                    found = true;
                    break;
                }
            }
        } else if (font_size == 32 && char_data_32) {
            for (int i = 0; i < char_count; i++) {
                if (char_data_32[i].codepoint == codepoint) {
                    advance = char_data_32[i].advance;
                    found = true;
                    break;
                }
            }
        } else if (font_size == 24 && char_data_24) {
            for (int i = 0; i < char_count; i++) {
                if (char_data_24[i].codepoint == codepoint) {
                    advance = char_data_24[i].advance;
                    found = true;
                    break;
                }
            }
        }
        
        if (found) {
            text_width += advance;
        } else {
            // Default character width
            text_width += font_size / 2;
        }
    }
    
    // Center the text vertically in the available space
    int start_x = x + (w - text_width) / 2;
    // Center based on actual font height, accounting for ascent/descent
    // Move the baseline up by approximately half the font size for better visual centering
    int start_y = y + h / 2 - font_size / 4;
    
    int cur_x = start_x;
    
    // Render each character to backbuffer with UTF-8 decoding
    for (size_t i = 0; i < text.size();) {
        uint32_t codepoint;
        
        // Simple UTF-8 decoding for common characters
        unsigned char byte1 = static_cast<unsigned char>(text[i]);
        if (byte1 < 0x80) {
            // ASCII character (0xxxxxxx)
            codepoint = byte1;
            i++;
        } else if ((byte1 & 0xE0) == 0xC0 && i + 1 < text.size()) {
            // 2-byte UTF-8 character (110xxxxx 10xxxxxx)
            unsigned char byte2 = static_cast<unsigned char>(text[i + 1]);
            if ((byte2 & 0xC0) == 0x80) {
                codepoint = ((byte1 & 0x1F) << 6) | (byte2 & 0x3F);
                i += 2;
            } else {
                // Invalid UTF-8, skip this byte
                i++;
                continue;
            }
        } else {
            // Unsupported UTF-8 sequence or invalid, skip this byte
            i++;
            continue;
        }
        
        // Find character in font data
        bool found = false;
        int char_x = 0, char_y = 0, char_w = 0, char_h = 0;
        int xoff = 0, yoff = 0, advance = 0;
        
        if (font_size == 48 && char_data_48) {
            for (int i = 0; i < char_count; i++) {
                if (char_data_48[i].codepoint == codepoint) {
                    char_x = char_data_48[i].x;
                    char_y = char_data_48[i].y;
                    char_w = char_data_48[i].w;
                    char_h = char_data_48[i].h;
                    xoff = char_data_48[i].xoff;
                    yoff = char_data_48[i].yoff;
                    advance = char_data_48[i].advance;
                    found = true;
                    break;
                }
            }
        } else if (font_size == 32 && char_data_32) {
            for (int i = 0; i < char_count; i++) {
                if (char_data_32[i].codepoint == codepoint) {
                    char_x = char_data_32[i].x;
                    char_y = char_data_32[i].y;
                    char_w = char_data_32[i].w;
                    char_h = char_data_32[i].h;
                    xoff = char_data_32[i].xoff;
                    yoff = char_data_32[i].yoff;
                    advance = char_data_32[i].advance;
                    found = true;
                    break;
                }
            }
        } else if (font_size == 24 && char_data_24) {
            for (int i = 0; i < char_count; i++) {
                if (char_data_24[i].codepoint == codepoint) {
                    char_x = char_data_24[i].x;
                    char_y = char_data_24[i].y;
                    char_w = char_data_24[i].w;
                    char_h = char_data_24[i].h;
                    xoff = char_data_24[i].xoff;
                    yoff = char_data_24[i].yoff;
                    advance = char_data_24[i].advance;
                    found = true;
                    break;
                }
            }
        }
        
        if (found && char_w > 0 && char_h > 0) {
            // Render the glyph from the atlas to backbuffer
            for (int gy = 0; gy < char_h; gy++) {
                for (int gx = 0; gx < char_w; gx++) {
                    int atlas_idx = (char_y + gy) * atlas_width + (char_x + gx);
                    uint8_t alpha = font_atlas[atlas_idx];
                    
                    // Only draw visible pixels (threshold alpha)
                    if (alpha > 128) {
                        int px = cur_x + xoff + gx;
                        int py = start_y + yoff + gy;
                        
                        if (is_valid_pixel(px, py)) {
                            set_pixel(px, py, color);
                        }
                    }
                }
            }
        }
        
        // Move to next character position
        if (found) {
            cur_x += advance;
        } else {
            // Default advance for unknown characters
            cur_x += font_size / 2;
        }
    }
}

void DisplayRenderer::draw_panel_border(int panel_x, int panel_y, int panel_w, int panel_h) {
    constexpr int BORDER_WIDTH = 3;
    
    // Simple border drawing to backbuffer
    for (int i = 0; i < BORDER_WIDTH; i++) {
        // Top and bottom borders
        for (int x = panel_x - i; x < panel_x + panel_w + i; x++) {
            if (x >= 0 && x < SCREEN_WIDTH) {
                if (panel_y - i >= 0) {
                    set_pixel(x, panel_y - i, BLACK);
                }
                if (panel_y + panel_h + i < SCREEN_HEIGHT) {
                    set_pixel(x, panel_y + panel_h + i, BLACK);
                }
            }
        }
        // Left and right borders
        for (int y = panel_y - i; y < panel_y + panel_h + i; y++) {
            if (y >= 0 && y < SCREEN_HEIGHT) {
                if (panel_x - i >= 0) {
                    set_pixel(panel_x - i, y, BLACK);
                }
                if (panel_x + panel_w + i < SCREEN_WIDTH) {
                    set_pixel(panel_x + panel_w + i, y, BLACK);
                }
            }
        }
    }
}

void DisplayRenderer::present() {
    if (!initialized_) {
        return;
    }
    
    // Update SDL display
    if (use_sdl_ && sdl_emulator_) {
        update_sdl_display();
    }
    
    // Update Inky display
    if (use_inky_ && inky_display_) {
        update_inky_display();
    }
}

bool DisplayRenderer::save_png(const std::string& filename) {
    // Convert backbuffer to RGB for PNG output
    std::vector<uint8_t> rgb_buffer(SCREEN_WIDTH * SCREEN_HEIGHT * 3);
    
    // Inky color palette for PNG conversion
    struct RGB { uint8_t r, g, b; };
    const RGB inky_palette[] = {
        {0, 0, 0},        // Black
        {255, 255, 255},  // White  
        {0, 255, 0},      // Green
        {0, 0, 255},      // Blue
        {255, 0, 0},      // Red
        {255, 255, 0},    // Yellow
        {255, 128, 0},    // Orange
        {224, 224, 224}   // Clear (light gray)
    };
    
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        uint8_t color = backbuffer_[i];
        if (color < 8) {
            rgb_buffer[i * 3] = inky_palette[color].r;
            rgb_buffer[i * 3 + 1] = inky_palette[color].g;
            rgb_buffer[i * 3 + 2] = inky_palette[color].b;
        } else {
            // Default to white for invalid colors
            rgb_buffer[i * 3] = 255;
            rgb_buffer[i * 3 + 1] = 255;
            rgb_buffer[i * 3 + 2] = 255;
        }
    }
    
    int result = stbi_write_png(filename.c_str(), SCREEN_WIDTH, SCREEN_HEIGHT, 3, rgb_buffer.data(), SCREEN_WIDTH * 3);
    return result != 0;
}

void DisplayRenderer::poll_events() {
    if (use_sdl_ && sdl_emulator_) {
        sdl_emulator_->poll_events();
    }
}

bool DisplayRenderer::should_quit() const {
    if (use_sdl_ && sdl_emulator_) {
        return sdl_emulator_->should_quit();
    }
    return false;
}

bool DisplayRenderer::is_valid_pixel(int x, int y) const {
    return x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT;
}

void DisplayRenderer::update_sdl_display() {
    // Copy backbuffer to SDL emulator
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            uint8_t color = backbuffer_[y * SCREEN_WIDTH + x];
            sdl_emulator_->set_pixel(x, y, color);
        }
    }
    sdl_emulator_->update();
}

void DisplayRenderer::update_inky_display() {
    // Copy backbuffer to Inky display
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            uint8_t color = backbuffer_[y * SCREEN_WIDTH + x];
            inky_set_pixel(inky_display_, x, y, color);
        }
    }
    
    inky_update(inky_display_);
}
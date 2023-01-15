// Copyright 2023 Ian Bullard
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

#include "convert_font.h"

#include <ft2build.h>
#include <freetype/ftbitmap.h>
#include FT_FREETYPE_H

#include "log.h"
#include "utils/zipfile.h"

const std::string font_characters("`1234567890-=~!@#$%^&*()_+qwertyuiop[]\\QWERTYUIOP{}|asdfghjkl;'ASDFGHJKL:\"zxcvbnm,./ZXCVBNM<>?");

bool convert_font()
{
    FT_Library library;
    FT_Face face;

    FT_Open_Args open_args;
    open_args.flags = FT_OPEN_MEMORY;
    open_args.stream = 0;
    open_args.driver = nullptr;
    open_args.num_params = 0;
    open_args.params = nullptr;

    auto error = FT_Init_FreeType(&library);
    if (error)
    {
        log("Failed to init freetype");
        return false;
    }

    ZipFile fonts("source_assets/Inter.zip");

    open_args.memory_base = (const FT_Byte *)fonts.contents("Inter-VariableFont_slnt,wght.ttf");

    if(!open_args.memory_base)
    {
        log("Cound not load font zip file...");
        return false;
    }

    open_args.memory_size = fonts.size("Inter-VariableFont_slnt,wght.ttf");

    error = FT_Open_Face(library, &open_args, 0, &face);
    if (error)
    {
        log("Failed to load font");
        return false;
    }

    error = FT_Set_Pixel_Sizes(face, 0, 16);
    if (error)
    {
        log("Failed to set pixel size");
        return false;
    }

    FT_UInt glyph_index = 0;
    for(char const &c: font_characters){
        glyph_index = FT_Get_Char_Index(face, (FT_ULong)c);
        if(glyph_index)
        {
            error = FT_Load_Glyph(face, glyph_index, FT_RENDER_MODE_NORMAL);
            if (error)
            {
                log(fmt::format("Could not load glyph {}", c));
                return false;
            }
            error = FT_Render_Glyph(face->glyph,
                                    FT_RENDER_MODE_MONO);
            if (error)
            {
                log(fmt::format("Could not render glyph {}", c));
                return false;
            }

            FT_Bitmap mono;
            FT_Bitmap_Init(&mono);

            error = FT_Bitmap_Convert(library, &face->glyph->bitmap, &mono, 1);
            if (error)
            {
                log(fmt::format("Could not convert glyph {}", c));
                return false;
            }

            int left = face->glyph->bitmap_left;
            int top = face->glyph->bitmap_top;
            int advance_x = face->glyph->advance.x;
            int advance_y = face->glyph->advance.y;
            
            for(int y = 0; y < mono.rows; ++y)
            {
                for(int x = 0; x < mono.width; ++x)
                {
                    int bit = mono.buffer[x + y * mono.pitch];
                }
            }
        }
    }

    return true;
}

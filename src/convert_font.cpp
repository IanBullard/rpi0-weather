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

#include <rapidjson/document.h>
#include <utf8cpp/utf8.h>

#include "log.h"
#include "utils/zipfile.h"

#define xstr(s) str(s)
#define str(s) #s
#pragma message(xstr(__cplusplus))

const std::string font_characters("`1234567890-=~!@#$%^&*()_+qwertyuiop[]\\QWERTYUIOP{}|asdfghjkl;'ASDFGHJKL:\"zxcvbnm,./ZXCVBNM<>? °");

bool convert_font(AssetDb& db, const std::string& settings)
{
    FT_Library library;
    FT_Face face;

    FT_Open_Args open_args;
    open_args.flags = FT_OPEN_MEMORY;
    open_args.stream = 0;
    open_args.driver = nullptr;
    open_args.num_params = 0;
    open_args.params = nullptr;

    db.reset_fonts();

    auto error = FT_Init_FreeType(&library);
    if (error)
    {
        log("Failed to init freetype");
        return false;
    }

    rapidjson::Document config;
    if (config.Parse(settings.c_str()).HasParseError())
    {
        log(fmt::format("Failed to parse settings json"));
        return false;
    }

    const rapidjson::Value& font_settings = config["fontSettings"];
    for(auto iter = font_settings.MemberBegin(); iter != font_settings.MemberEnd(); ++iter)
    {
        std::string name = iter->name.GetString();
        const rapidjson::Value& data = font_settings[name.c_str()];
        std::string source_zip = data["sourceZip"].GetString();
        std::string source_file = data["sourceFile"].GetString();
        int height = data["height"].GetInt();

        ZipFile fonts(source_zip);

        open_args.memory_base = (const FT_Byte *)fonts.contents(source_file);

        if(!open_args.memory_base)
        {
            log("Cound not load font zip file...");
            return false;
        }

        open_args.memory_size = fonts.size(source_file);

        error = FT_Open_Face(library, &open_args, 0, &face);
        if (error)
        {
            log("Failed to load font");
            return false;
        }

        error = FT_Set_Pixel_Sizes(face, 0, height);
        if (error)
        {
            log("Failed to set pixel size");
            return false;
        }

        db.add_font(name, height, face->height >> 6);
        std::string font_table = db.font_table_name(name, height);

        FT_UInt glyph_index = 0;
        std::u32string utf32_characters = utf8::utf8to32(font_characters);

        for(auto c: utf32_characters){
            glyph_index = FT_Get_Char_Index(face, (FT_ULong)c);
            std::u32string char32_string;
            char32_string.push_back(c);
            std::string char8_string = utf8::utf32to8(char32_string);
            if(glyph_index)
            {
                error = FT_Load_Glyph(face, glyph_index, FT_RENDER_MODE_NORMAL);
                if (error)
                {
                    log(fmt::format("Could not load glyph {}", char8_string));
                    return false;
                }
                error = FT_Render_Glyph(face->glyph,
                                        FT_RENDER_MODE_MONO);
                if (error)
                {
                    log(fmt::format("Could not render glyph {}", char8_string));
                    return false;
                }

                FT_Bitmap mono;
                FT_Bitmap_Init(&mono);

                error = FT_Bitmap_Convert(library, &face->glyph->bitmap, &mono, 1);
                if (error)
                {
                    log(fmt::format("Could not convert glyph {}", char8_string));
                    return false;
                }

                int left = face->glyph->bitmap_left;
                int top = face->glyph->bitmap_top;
                int advance_x = face->glyph->advance.x >> 6;
                int advance_y = face->glyph->advance.y >> 6;
                char* data = new char[mono.width * mono.rows];
                
                for(int y = 0; y < mono.rows; ++y)
                {
                    for(int x = 0; x < mono.width; ++x)
                    {
                        data[x + y * mono.width] = mono.buffer[x + y * mono.pitch];
                    }
                }
                db.add_glyph(font_table, char8_string, mono.width, mono.rows, left, top, advance_x, advance_y, data, mono.width * mono.rows);
                delete[] data;
            }
        }
    }

    return true;
}

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

#include <string>
#include "utils/collector.h"
#include "utils/palette.h"
#include "utils/zipfile.h"
#include <fmt/core.h>
#include <rapidjson/document.h>

#include "convert_images.h"
#include "log.h"

Collector colors;

FIBITMAP* load_image(FIMEMORY* contents)
{
    FREE_IMAGE_FORMAT format = FreeImage_GetFileTypeFromMemory(contents);
    FIBITMAP* bitmap = FreeImage_LoadFromMemory(format, contents);
    FIBITMAP* normalized = FreeImage_ConvertTo32Bits(bitmap);
    FreeImage_Unload(bitmap);
    return normalized;
}

void quantize_and_save(FIBITMAP* bitmap, AssetDb& db, const std::string& id)
{
    unsigned width = FreeImage_GetWidth(bitmap);
    unsigned height = FreeImage_GetHeight(bitmap);
    RGBQUAD in;
    int index = 0;
    uint8_t *data = new uint8_t[width * height];

    for(unsigned y = 0; y < height; ++y)
    {
        for(unsigned x = 0; x < width; ++x)
        {
            FreeImage_GetPixelColor(bitmap, x, y, &in);
            colors.add_color(convert(in));
            if(is_transparent(in))
                index = 7;
            else
                index = convert_color(convert(in), x, y);
            data[x + (height-y-1) * width] = index;
        }
    }
    db.add_image(id, width, height, (const char*)data, width * height);
    delete[] data;
}

FIBITMAP* resize(FIBITMAP* bitmap, size_t target_width, size_t target_height)
{
    FIBITMAP* result = FreeImage_Rescale(bitmap, target_width, target_height, FILTER_BILINEAR);
    FreeImage_Unload(bitmap);
    return result;
}

void save_image(const char* filename, FIBITMAP* bitmap, AssetDb& db)
{
    FIBITMAP* normalized = FreeImage_ConvertTo32Bits(bitmap);
    int width = FreeImage_GetWidth(bitmap), height = FreeImage_GetHeight(bitmap);
    RGBQUAD color;
    uint8_t *data = new uint8_t[width * height];
    for(int y = 0; y < height; ++y)
    {
        for(int x = 0; x < width; ++x)
        {
            FreeImage_GetPixelColor(normalized, x, y, &color);
            data[x + (height-y-1) * width] = closest_color(rgba8888(color.rgbRed, color.rgbGreen, color.rgbBlue, color.rgbReserved));
        }
    }
    db.add_image(filename, width, height, (const char*)data, width * height);
    delete[] data;
    FreeImage_Unload(normalized);
}

void convert_weather_icon(const std::string name, FIMEMORY* contents, AssetDb& db, int width, int height)
{
    auto img = load_image(contents);
    img = resize(img, width, height);
    quantize_and_save(img, db, name);
    FreeImage_Unload(img);
}

FIMEMORY* convert_to_fimem(char* data, size_t size)
{
    FIMEMORY* results = FreeImage_OpenMemory((BYTE*)data, (size_t)size);
    return results;
}

bool convert_image(ZipFile* zip, const std::string& path, const std::string& file, AssetDb& db, int width, int height)
{
    char* contents = zip->contents(path);
    size_t content_size = zip->size(path);

    if(!contents)
    {
        log(fmt::format("Cound not load {}...", path));
        return false;
    }

    FIMEMORY* icon = convert_to_fimem(contents, content_size);
    convert_weather_icon(file, icon, db, width, height);
    FreeImage_CloseMemory(icon);
    delete[] contents;

    return true;
}

bool convert_images(AssetDb& db, const std::string& settings)
{
    db.reset_images();
    init_palette();
    rapidjson::Document config;
    if (config.Parse(settings.c_str()).HasParseError())
    {
        log(fmt::format("Failed to parse settings json"));
        return false;
    }

    const rapidjson::Value& icon_settings = config["iconSettings"];

    int width = icon_settings["width"].GetInt();
    int height = icon_settings["height"].GetInt();
    std::string source_zip = icon_settings["sourceZip"].GetString();
    std::string zip_folder = icon_settings["sourceFolder"].GetString();

    FreeImage_Initialise();
    ZipFile icons(source_zip);

    const rapidjson::Value& icon_names = icon_settings["iconNames"];
    for(auto iter = icon_names.MemberBegin(); iter != icon_names.MemberEnd(); ++iter)
    {
        std::string name = iter->name.GetString();
        std::string filename = iter->value.GetString();
        std::string path = fmt::format("{}{}", zip_folder, filename);
        convert_image(&icons, path, name, db, width, height);
    }

    FreeImage_DeInitialise();

    return true;
}

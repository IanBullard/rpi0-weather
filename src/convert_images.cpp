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

FIBITMAP* quantize(FIBITMAP* bitmap)
{
    unsigned width = FreeImage_GetWidth(bitmap);
    unsigned height = FreeImage_GetHeight(bitmap);
    FIBITMAP* result = FreeImage_Allocate(width, height, FreeImage_GetBPP(bitmap));
    RGBQUAD in;
    int index = 0;

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
            FreeImage_SetPixelColor(result, x, y, &inky_palette[index]);
        }
    }
    FreeImage_Unload(bitmap);
    return result;
}

FIBITMAP* resize(FIBITMAP* bitmap, size_t target_width, size_t target_height)
{
    FIBITMAP* result = FreeImage_Rescale(bitmap, target_width, target_height, FILTER_BILINEAR);
    FreeImage_Unload(bitmap);
    return result;
}

void save_image(const char* filename, FIBITMAP* bitmap)
{
    FIBITMAP* normalized = FreeImage_ConvertTo32Bits(bitmap);
    FreeImage_Save(FIF_PNG, normalized, filename);
    FreeImage_Unload(normalized);
}

void convert_weather_icon(const std::string name, FIMEMORY* contents)
{
    std::string dest_folder("./assets/");

    auto dest = dest_folder + name;

    auto img = load_image(contents);
    img = resize(img, 112, 112);
    img = quantize(img);
    save_image(dest.c_str(), img);
    FreeImage_Unload(img);
}

FIMEMORY* convert_to_fimem(char* data, size_t size)
{
    FIMEMORY* results = FreeImage_OpenMemory((BYTE*)data, (size_t)size);
    return results;
}

bool convert_image(ZipFile* zip, const std::string& path, const std::string& file)
{
    char* contents = zip->contents(path);
    size_t content_size = zip->size(path);

    if(!contents)
    {
        log(fmt::format("Cound not load {}...", path));
        return false;
    }

    FIMEMORY* icon = convert_to_fimem(contents, content_size);
    convert_weather_icon(file, icon);
    FreeImage_CloseMemory(icon);
    delete[] contents;

    return true;
}

bool convert_images()
{
    init_palette();
    FreeImage_Initialise();
    ZipFile icons("source_assets/plain_weather_icons_by_merlinthered_d2lkj4g.zip");

    for(int i = 0; i < 48; ++i)
    {
        std::string filename = fmt::format("{:02d}.png", i);
        std::string path = fmt::format("plain_weather/flat_colorful/png/{}", filename);
        convert_image(&icons, path, filename);
    }
    convert_image(&icons, "plain_weather/flat_colorful/png/na.png", "na.png");

    FreeImage_DeInitialise();

    return true;
}

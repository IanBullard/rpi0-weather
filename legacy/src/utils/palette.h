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
#pragma once

#include "FreeImage.h"
#include <cstdint>
#include <cmath>
#include <limits>
#include <glm/vec4.hpp>

typedef glm::vec<4, uint8_t, glm::defaultp> rgba8888;

const rgba8888 Black(0, 0, 0, 255);
const rgba8888 White(255, 255, 255, 255);
const rgba8888 Green(0, 255, 0, 255);
const rgba8888 Blue(0, 0, 255, 255);
const rgba8888 Red(255, 0, 0, 255);
const rgba8888 Yellow(255, 255, 0, 255);
const rgba8888 Orange(255, 140, 0, 255);
const rgba8888 Clean(255, 255, 255, 0);

static const size_t PALETTE_SIZE = 7;

RGBQUAD inky_palette[PALETTE_SIZE];

const rgba8888 palette_order[] = {
    Black, White, Green, Blue, Red, Yellow, Orange, Clean
};

const size_t BlackIndex = 0;
const size_t WhiteIndex = 1;

rgba8888 convert(const RGBQUAD& in)
{
    return rgba8888(in.rgbRed, in.rgbGreen, in.rgbBlue, in.rgbReserved);
}

RGBQUAD convert(const rgba8888& in)
{
    RGBQUAD result;
    result.rgbRed = in.x;
    result.rgbGreen = in.y;
    result.rgbBlue = in.z;
    result.rgbReserved = in.w;
    return result;
}

uint32_t hash(const rgba8888& in)
{
    uint32_t result = in.x;
    result = (result << 8) + in.y;
    result = (result << 8) + in.z;

    return result;
}

rgba8888 unhash(const uint32_t in)
{
    rgba8888 result;
    result.x = (in >> 16)& 0xFF;
    result.y = (in >> 8) & 0xFF;
    result.z = (in >> 0)& 0xFF;
    return result;
}

void init_palette()
{
    for(int i = 0; i < PALETTE_SIZE; ++i) {
        inky_palette[i] = convert(palette_order[i]);
    }
}

float distance_axis(float left, float right)
{
    return (right - left) * (right - left);
}

float distance(const rgba8888& left, const rgba8888& right)
{
    float result = distance_axis((float)left.x, (float)right.x);
    result += distance_axis((float)left.y, (float)right.y);
    result += distance_axis((float)left.z, (float)right.z);
    return sqrtf(result);
}

bool is_greyscale(const rgba8888& color)
{
    float red = color.x, green = color.y, blue = color.z;
    float average = (red + green + blue) / 3.0f;
    float diff = fabsf(red - average) + fabsf(green - average) + fabsf(blue - average);
    return diff < 1.0f;
}

bool is_transparent(const RGBQUAD& color)
{
    return color.rgbReserved < 200;
}

bool is_transparent(const rgba8888& color)
{
    return color.w < 200;
}

int closest_color(const rgba8888& color)
{
    size_t closest = 0;
    float closest_distance = std::numeric_limits<float>::max();

    if(is_greyscale(color))
    {
        float black_dist = distance(palette_order[BlackIndex], color);
        float white_dist = distance(palette_order[WhiteIndex], color);
        if(black_dist < white_dist)
            closest = BlackIndex;
        else
            closest = WhiteIndex;
    }
    else if(is_transparent(color))
    {
        return PALETTE_SIZE-1;
    }
    else
    {
        for(int i = 2; i < PALETTE_SIZE; ++i) {
            float dist = distance(palette_order[i], color);

            if(dist < closest_distance)
            {
                closest = i;
                closest_distance = dist;
            }
        }
    }

    return closest;
}

namespace source_color
{
    const rgba8888 Yellow(255, 234, 49, 255);
    const rgba8888 Blue(34, 160, 239, 255);
    const rgba8888 Orange(254, 180, 47, 255);
    const rgba8888 Red(250, 80, 46, 255);
    const rgba8888 Rorange(254, 84, 22, 255);
    const rgba8888 White(255, 255, 255, 255);
    const rgba8888 Black(0, 0, 0, 255);

    const rgba8888 PaleYellow(224, 217, 181, 255);

    const rgba8888 Grey0(229, 229, 229, 255);
    const rgba8888 Grey1(198, 198, 198, 255);
    const rgba8888 Grey2(174, 170, 179, 255);
    const rgba8888 Grey3(120, 120, 120, 255);
    const rgba8888 Grey4(111, 111, 111, 255);
}

typedef int dither2x2[2][2];

dither2x2 dither_pattern[5] =
{
    {{0, 0},
     {0, 0}},
    {{1, 0},
     {0, 0}},
    {{0, 1},
     {1, 0}},
    {{0, 1},
     {1, 1}},
    {{1, 1},
     {1, 1}},
};


struct color_map
{
    rgba8888 source;
    rgba8888 dest0;
    rgba8888 dest1;
    int dither_index;
};

color_map convert_map[] =
{
    {source_color::Yellow, Yellow, Yellow, 0},
    {source_color::Blue, Blue, Blue, 0},
    {source_color::Orange, Orange, Orange, 0},
    {source_color::Red, Red, Red, 0},
    {source_color::Rorange, Red, Red, 0},
    {source_color::White, White, White, 0},
    {source_color::Black, Black, Black, 0},

    {source_color::PaleYellow, Yellow, White, 2},

    {source_color::Grey0, Black, White, 1},
    {source_color::Grey1, Black, White, 1},
    {source_color::Grey2, Black, White, 2},
    {source_color::Grey3, Black, White, 2},
    {source_color::Grey3, Black, White, 2},
};

int lookup_palette(const rgba8888& in)
{
    for(int i=0; i < PALETTE_SIZE; ++i)
    {
        if(in == palette_order[i])
            return i;
    }
    return 4;
}

#include <fmt/core.h>

int convert_color(const rgba8888& in, int x, int y)
{
    int result = PALETTE_SIZE;

    for(int i=0; i < (sizeof(convert_map)/sizeof(convert_map[0])); i++)
    {
        float dist = distance(convert_map[i].source, in);
        if(dist < 3.0f)
        {
            rgba8888 dithered;
            if (dither_pattern[convert_map[i].dither_index][x&1][y&1])
            {
                dithered = convert_map[i].dest0;
            }
            else
            {
                dithered = convert_map[i].dest1;
            }

            return lookup_palette(dithered);
        }
    }

    return result;
}

void test(const rgba8888& color)
{
    auto result = inky_palette[closest_color(color)];
    bool is_grey = is_greyscale(color);
    fmt::print("({}, {}, {}): ({}, {}, {}) {}\n", color.x, color.y, color.z, result.rgbRed, result.rgbGreen, result.rgbBlue, is_grey);
}
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

#include "palette.h"
#include <unordered_map>

class Collector
{
private:
    std::unordered_map<uint32_t, int> m_histogram;
    float m_tolerance;
public:
    Collector(float tolerance = 5.0f);
    ~Collector();

    void add_color(const rgba8888& color);
    void report_palette();
};

Collector::Collector(float tolerance) :
    m_tolerance(tolerance)
{
}

Collector::~Collector()
{
}

void Collector::add_color(const rgba8888& color)
{
    auto h = hash(color);
    if(m_histogram.find(h) != m_histogram.end())
    {
        m_histogram[h] += 1;
    }
    else
    {
        m_histogram[h] = 1;
    }
}

void Collector::report_palette()
{
    for(auto c: m_histogram)
    {
        if(c.second > 500)
        {
            auto color = unhash(c.first);
            fmt::print("{}, {}, {}, {}\n", color.x, color.y, color.z, c.second);
        }
    }
}

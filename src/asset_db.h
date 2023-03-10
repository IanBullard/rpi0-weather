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

#include <sqlite3.h>
#include <string>

class AssetDb
{
public:
    AssetDb();
    ~AssetDb();

    void reset_images();
    void add_image(const std::string id, int width, int height, const char *data, size_t size);

    void reset_fonts();
    void add_font(const std::string id, int size, int height);
    const std::string font_table_name(const std::string id, int height);
    void add_glyph(const std::string font_table, const std::string id, int width, int height, int top, int left, int advance_x, int advance_y, const char *data, size_t size);

private:
    sqlite3 *m_db;

    bool simple_sql(const char* sql);
};
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

#include "asset_db.h"

#include "log.h"

AssetDb::AssetDb()
    : m_db(nullptr)
{
    int rc = sqlite3_open("src/python/assets.db", &m_db);
    if(rc)
    {
        log("Could not open asset database");
        sqlite3_close(m_db);
    }
}
AssetDb::~AssetDb()
{
    sqlite3_close(m_db);
}

void AssetDb::reset_images()
{
    if(this->simple_sql("DROP TABLE IF EXISTS images"))
        this->simple_sql("CREATE TABLE images(id TEXT PRIMARY KEY, width INT, height INT, data BLOB)");
}

void AssetDb::add_image(const std::string id, int width, int height, const char *data)
{
    this->simple_sql("INSERT INTO images VALUES (");
}

void AssetDb::reset_fonts()
{
    if(this->simple_sql("DROP TABLE IF EXISTS fonts"))
        this->simple_sql("CREATE TABLE fonts(id TEXT PRIMARY KEY, width INT, height INT, top INT, left INT, advance_x INT, advance_y INT, data BLOB)");
}

bool AssetDb::simple_sql(const char* sql)
{
    int result = sqlite3_exec(m_db, sql, nullptr, nullptr, nullptr);
    if(result)
    {
        log(fmt::format("Failed SQL statement: {}", sql));
    }
    return result == 0;
}

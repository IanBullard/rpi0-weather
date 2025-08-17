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

#include <vector>

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

void AssetDb::add_image(const std::string id, int width, int height, const char *data, size_t size)
{
    std::string insert("INSERT INTO images VALUES (?, ?, ?, ?)");
    sqlite3_stmt* statement;
    sqlite3_prepare_v2(m_db, insert.c_str(), -1, &statement, nullptr);
    if (statement == nullptr)
    {
        log("Failed to prepare add image SQL");
        sqlite3_finalize(statement);
        return;
    }
    sqlite3_bind_text(statement, 1, id.c_str(), -1, nullptr);
    sqlite3_bind_int(statement, 2, width);
    sqlite3_bind_int(statement, 3, height);
    sqlite3_bind_blob(statement, 4, data, size, nullptr);
    sqlite3_step(statement);
    sqlite3_finalize(statement);
}

void AssetDb::reset_fonts()
{
    std::string loaded_fonts("SELECT id, height, table_name FROM fonts");
    sqlite3_stmt* statement;
    sqlite3_prepare_v2(m_db, loaded_fonts.c_str(), -1, &statement, nullptr);
    std::vector<std::string> tables;
    int result = SQLITE_ROW;

    if (statement == nullptr)
    {
        log("Failed to prepare font table query");
    }
    else
    {
        int wtf = 0;
        while(result == SQLITE_ROW) {
            result = sqlite3_step(statement);
            if (result == SQLITE_ROW)
                tables.push_back(std::string((const char*)sqlite3_column_text(statement, 2)));
            wtf++;
        }
    }
    sqlite3_finalize(statement);

    for (std::string table: tables) {
        this->simple_sql(fmt::format("DROP TABLE IF EXISTS {}", table).c_str());
    }

    this->simple_sql("DROP TABLE IF EXISTS fonts");
    this->simple_sql("CREATE TABLE fonts(id TEXT PRIMARY KEY, size INT, height INT, table_name TEXT)");
}

void AssetDb::add_font(const std::string id, int size, int height)
{
    std::string table_name = this->font_table_name(id, size);
    std::string insert("INSERT INTO fonts VALUES (?, ?, ?, ?)");
    sqlite3_stmt* statement;
    sqlite3_prepare_v2(m_db, insert.c_str(), -1, &statement, nullptr);
    if (statement == nullptr)
    {
        log("Failed to prepare add font SQL");
        sqlite3_finalize(statement);
        return;
    }
    sqlite3_bind_text(statement, 1, id.c_str(), -1, nullptr);
    sqlite3_bind_int(statement, 2, size);
    sqlite3_bind_int(statement, 3, height);
    sqlite3_bind_text(statement, 4, table_name.c_str(), -1, nullptr);
    sqlite3_step(statement);
    sqlite3_finalize(statement);

    std::string drop_font(fmt::format("DROP TABLE IF EXISTS {}", table_name));
    this->simple_sql(drop_font.c_str());
    std::string create_font(fmt::format("CREATE TABLE {}(id TEXT PRIMARY KEY, width INT, height INT, top INT, left INT, advance_x INT, advance_y INT, data BLOB)", table_name));
    this->simple_sql(create_font.c_str());
}

const std::string AssetDb::font_table_name(const std::string id, int height)
{
    return fmt::format("{}_{}", id, height);
}

void AssetDb::add_glyph(const std::string font_table, const std::string id, int width, int height, int top, int left, int advance_x, int advance_y, const char *data, size_t size)
{
    std::string insert(fmt::format("INSERT INTO {} VALUES (?, ?, ?, ?, ?, ?, ?, ?)", font_table));
    sqlite3_stmt* statement;
    sqlite3_prepare_v2(m_db, insert.c_str(), -1, &statement, nullptr);
    if (statement == nullptr)
    {
        log("Failed to prepare add font SQL");
        sqlite3_finalize(statement);
        return;
    }
    sqlite3_bind_text(statement, 1, id.c_str(), -1, nullptr);
    sqlite3_bind_int(statement, 2, width);
    sqlite3_bind_int(statement, 3, height);
    sqlite3_bind_int(statement, 4, top);
    sqlite3_bind_int(statement, 5, left);
    sqlite3_bind_int(statement, 6, advance_x);
    sqlite3_bind_int(statement, 7, advance_y);
    sqlite3_bind_blob(statement, 8, data, size, nullptr);
    if(sqlite3_step(statement) != SQLITE_DONE)
    {
        log(fmt::format("Failed to add glyph {} to table", id));
        return;
    }
    sqlite3_finalize(statement);
}

bool AssetDb::simple_sql(const char* sql)
{
    int result = sqlite3_exec(m_db, sql, nullptr, nullptr, nullptr);
    if(result)
    {
        log(fmt::format("Failed SQL statement: {} result = {}", sql, result));
    }
    return result == 0;
}

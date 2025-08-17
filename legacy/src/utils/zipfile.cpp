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

#include "zipfile.h"

#include "../log.h"

ZipFile::ZipFile(const std::string& path) :
    m_zip(nullptr)
{
    int err = 0;
    m_zip = zip_open(path.c_str(), 0, &err);
    if(m_zip == nullptr)
    {
        switch(err) {
            case ZIP_ER_EXISTS:
                log("The file specified by path exists and ZIP_EXCL is set.");
                break;
            case ZIP_ER_INCONS:
                log("Inconsistencies were found in the file specified by path. This error is often caused by specifying ZIP_CHECKCONS but can also happen without it.");
                break;
            case ZIP_ER_INVAL:
                log("The path argument is NULL.");
                break;
            case ZIP_ER_MEMORY:
                log("Required memory could not be allocated.");
                break;
            case ZIP_ER_NOENT:
                log("The file specified by path does not exist and ZIP_CREATE is not set.");
                break;
            case ZIP_ER_NOZIP:
                log("The file specified by path is not a zip archive.");
                break;
            case ZIP_ER_OPEN:
                log("The file specified by path could not be opened.");
                break;
            case ZIP_ER_READ:
                log("A read error occurred; see errno for details.");
                break;
            case ZIP_ER_SEEK:
                log("The file specified by path does not allow seeks");
                break;
            default:
                log("Unknown zip file error");
                break;
        }
    }
}

ZipFile::~ZipFile()
{
    if(m_zip)
    {
        zip_close(m_zip);
    }
}

char* ZipFile::contents(const std::string& path)
{
    char* results = nullptr;

    if(m_zip)
    {
        struct zip_stat st;
        
        zip_stat_init(&st);
        int stat_result = zip_stat(m_zip, path.c_str(), 0, &st);
        if(stat_result == 0)
        {
            results = new char[st.size];
            zip_file* f = zip_fopen(m_zip, path.c_str(), 0);
            zip_fread(f, results, st.size);
            zip_fclose(f);
        }
        else
        {
            log(fmt::format("Cannot unzip {}", path));
        }
    }

    return results;
}

size_t ZipFile::size(const std::string& path)
{
    size_t results = 0;

    if(m_zip)
    {
        struct zip_stat st;
        
        zip_stat_init(&st);
        int stat_result = zip_stat(m_zip, path.c_str(), 0, &st);
        if(stat_result == 0)
        {
            results = (size_t)st.size;
        }
        else
        {
            log(fmt::format("Cannot stat {}", path));
        }
    }

    return results;
}

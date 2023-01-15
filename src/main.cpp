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
#include "emulate.h"
#include "convert_font.h"
#include "convert_images.h"
#include <filesystem>

int main(int argc, char** argv)
{
    std::filesystem::path cwd = std::filesystem::current_path();
    std::string base_path = cwd.string();
    AssetDb assets;

    if(argc > 1)
    {
        std::string action(argv[1]);
        
        if(action == "icons")
            convert_images(assets);
        if(action == "fonts")
            convert_font(assets);
        else
            emulate(argc, argv);
    }
    else
    {
        emulate(argc, argv);
    }

    return 0;
}
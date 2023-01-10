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

#include <pybind11/embed.h>
#include <fmt/core.h>

namespace py = pybind11;

void emulate(const std::string& base_path)
{
    py::scoped_interpreter guard{};
    std::string module_path = fmt::format("{}/python");
    py::module_ sys = py::module_::import("sys");
    py::print(sys.attr("path"));
    //std::string executable(fmt::format("import sys\nsys.path.insert(0, {})\nimport weather", module_path));

    //py::exec(executable);
    // py::object scope = py::module_::import("__main__").attr("__dict__");
    // py::eval_file(".src/python/weather.py", scope);
}

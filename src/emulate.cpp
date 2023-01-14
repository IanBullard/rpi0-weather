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

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <fmt/core.h>
#include <string_view>
#include <fstream>

#include "log.h"
#include "inky_mock.h"

static PyObject* py_inky_setup(PyObject *self, PyObject *args)
{
    inky_setup();
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* py_inky_set_pixel(PyObject *self, PyObject *args)
{
    int x;
    int y;
    int color;

    int result = PyArg_ParseTuple(args, "lll", &x, &y, &color);
    inky_set_pixel(x, y, (uint8_t)color);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* py_inky_display(PyObject *self, PyObject *args)
{
    inky_display();
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef mock_inky_display[] = {
    {"setup", py_inky_setup, METH_VARARGS,
     "."},
    {"set_pixel", py_inky_set_pixel, METH_VARARGS,
     "."},
    {"show", py_inky_display, METH_VARARGS,
     "."},
    {NULL, NULL, 0, NULL}
};

static PyModuleDef mock_inky_module = {
    PyModuleDef_HEAD_INIT, "mock_inky", NULL, -1, mock_inky_display,
    NULL, NULL, NULL, NULL
};

void emulate(int argc, char** argv)
{
    inky_setup();
    for(int i = 0; i < 448; ++i)
        inky_set_pixel(i, i, 6);
    inky_display();
    PyStatus status;

    PyConfig config;
    PyConfig_InitPythonConfig(&config);

    /* Decode command line arguments.
       Implicitly preinitialize Python (in isolated mode). */
    status = PyConfig_SetBytesArgv(&config, argc, argv);
    if (PyStatus_Exception(status)) {
        goto exception;
    }

    status = Py_InitializeFromConfig(&config);
    if (PyStatus_Exception(status)) {
        goto exception;
    }
    PyConfig_Clear(&config);

    PyRun_SimpleString("import sys, os\n"
                       "sys.path.append(os.path.abspath('./src/python'))\n"
                       "import weather\n");
    if (Py_FinalizeEx() < 0) {
        exit(120);
    }
    inky_shutdown();
    return;

exception:
    PyConfig_Clear(&config);
    if (PyStatus_IsExit(status)) {
        // return status.exitcode;
        return;
    }
    /* Display the error message and exit the process with
       non-zero exit code */
    Py_ExitStatusException(status);
}

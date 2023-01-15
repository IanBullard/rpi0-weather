# Copyright 2023 Ian Bullard
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.

import time

import mock_inky
import sqlite3
import os

import imageio.v3 as iio
import numpy

_THIS_DIR = os.path.dirname(__file__)
# con = sqlite3.connect("test.db")

"""
im = iio.imread('assets/00.png')
image = numpy.array(im, dtype=numpy.uint8)

color_lookup = [
    [0, 0, 0, 255],
    [255, 255, 255, 255],
    [0, 255, 0, 255],
    [0, 0, 255, 255],
    [255, 0, 0, 255],
    [255, 255, 0, 255],
    [255, 140, 0, 255],
    [255, 255, 255, 0]
]


def color_to_index(color):
    for i in range(len(color_lookup)):
        if color[0] == color_lookup[i][0] and color[1] == color_lookup[i][1] and color[2] == color_lookup[i][2]:
            return i


mock_inky.setup()
for x in range(im.shape[0]):
    for y in range(im.shape[1]):
        mock_inky.set_pixel(x, y, color_to_index(image[y][x]))
mock_inky.show()

time.sleep(5)
"""
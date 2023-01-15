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

def blit(pos_x, pos_y, width, height, data):
    for y in range(height):
        for x in range(width):
            color = data[x + y * width]
            if color != 7:
                mock_inky.set_pixel(pos_x + x, pos_y + y, data[x + y * width])

def draw_rectangle(pos_x, pos_y, width, height, color):
    for y in range(height):
        for x in range(width):
            mock_inky.set_pixel(pos_x + x, pos_y + y, color)

def draw_lineX(start_x, end_x, y, color):
    for x in range(start_x, end_x):
        mock_inky.set_pixel(x, y, color)

def draw_liney(x, start_y, end_y, color):
    for y in range(start_y, end_y):
        mock_inky.set_pixel(x, y, color)

_THIS_DIR = os.path.dirname(__file__)

con = sqlite3.connect(f"{_THIS_DIR}/assets.db")

cur = con.cursor()

res = cur.execute("SELECT width, height, data FROM images WHERE id='00.png'")

width, height, data = res.fetchone()

mock_inky.setup()

# 600x448
draw_rectangle(0, 0, 600, 448, 0)
blit(600-width, 0, width, height, data)
draw_lineX(0, 600, height+1, 1)
draw_liney(600-width-1, 0, 448, 1)

mock_inky.show()

time.sleep(5)
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


for x in range(im.shape[0]):
    for y in range(im.shape[1]):
        mock_inky.set_pixel(x, y, color_to_index(image[y][x]))
mock_inky.show()


"""
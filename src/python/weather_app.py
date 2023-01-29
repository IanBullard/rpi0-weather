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
import sqlite3
import logging
import os
from datetime import datetime

import forcast, forcast_nws

logging.basicConfig(filename="weather.log",
                    filemode='a',
                    format='%(asctime)s,%(msecs)d %(name)s %(levelname)s %(message)s',
                    datefmt='%H:%M:%S',
                    level=logging.DEBUG)

SHOW_WARNING = False

class Image:
    def __init__(self, id, width, height, data):
        self._id = id;
        self._width = width
        self._height = height
        self._data = data

    @property
    def width(self):
        return self._width

    @property
    def height(self):
        return self._height

    @property
    def data(self):
        return self._data

    def __str__(self):
        result = f"{self._id} ({self.width}, {self.height})"
        return result


class Glyph:
    def __init__(self, id, width, height, top, left, advance_x, advance_y, data):
        self._id = id
        self._size = (width, height)
        self._offset = (top, left)
        self._advance = (advance_x, advance_y)
        self._data = data

    @property
    def size(self):
        return self._size

    @property
    def offset(self):
        return self._offset

    @property
    def advance(self):
        return self._advance

    @property
    def data(self):
        return self._data

    def __str__(self):
        result = f"{self._id} size({self.size[0]}, {self.size[1]}), offset({self.offset[0]}, {self.offset[1]}) advance({self.advance[0]}, {self.advance[1]})"
        return result


class Font:
    def __init__(self, id, height, glyphs):
        self._id = id
        self._height = height
        self._glyphs = {}
        for glyph in glyphs:
            self._glyphs[glyph._id] = glyph

    def glyph(self, code):
        return self._glyphs[code]

    @property
    def height(self):
        return self._height

class AssetDb:
    def __init__(self):
        this_dir = os.path.dirname(__file__)
        self._connection = sqlite3.connect(f"{this_dir}/assets.db")
        self._cursor = self._connection.cursor()

    def load_image(self, id):
        res = self._cursor.execute(f"SELECT width, height, data FROM images WHERE id='{id}'")
        width, height, data = res.fetchone()
        return Image(id, width, height, data)

    def load_font(self, id):
        res = self._cursor.execute(f"SELECT height, table_name FROM fonts WHERE id='{id}'")
        height, table_name = res.fetchone()
        res = self._cursor.execute(f"SELECT id, width, height, top, left, advance_x, advance_y, data FROM {table_name}")
        glyphs = []
        for result in res.fetchall():
            glyphs.append(Glyph(*result))
        return Font(id, height, glyphs)


class InkyWrapper:
    def __init__(self, inky, setup, set_pixel, show):
        self._inky = inky
        self.setup = setup
        self.set_pixel = set_pixel
        self.show = show


class Renderer:
    SCREEN_WIDTH = 600
    SCREEN_HEIGHT = 448

    BLACK = 0
    WHITE = 1
    GREEN = 2
    BLUE = 3
    RED = 4
    YELLOW = 5
    ORANGE = 6
    CLEAR = 7

    def __init__(self, wrapper):
        self._inky = wrapper
        self._inky.setup()

    def show(self):
        self._inky.show()

    def clear(self, color):
        for y in range(self.SCREEN_HEIGHT):
            for x in range(self.SCREEN_WIDTH):
                self._inky.set_pixel(x, y, color)

    def blit(self, pos_x, pos_y, image):
        for y in range(image.height):
            for x in range(image.width):
                color = image.data[x + y * image.width]
                if image.width == 12:
                    print(color)
                if color != 7:
                    self._inky.set_pixel(pos_x + x, pos_y + y, image.data[x + y * image.width])

    def rectangle(self, pos_x, pos_y, width, height, color):
        for y in range(height):
            for x in range(width):
                self._inky.set_pixel(pos_x + x, pos_y + y, color)

    def lineX(self, start_x, end_x, y, color):
        for x in range(start_x, end_x):
            self._inky.set_pixel(x, y, color)

    def lineY(self, x, start_y, end_y, color):
        for y in range(start_y, end_y):
            self._inky.set_pixel(x, y, color)

    def box(self, x, y, width, height, color):
        self.lineX(x, x + width, y, color)
        self.lineX(x, x + width, y + height, color)
        self.lineY(x, y, y + height, color)
        self.lineY(x + width, y, y + height, color)

    def print(self, x, y, font, color, string):
        cur_pos = (int(x), int(y + font.height))
        for char in string:
            glyph = font.glyph(char)
            for y in range(glyph.size[1]):
                for x in range(glyph.size[0]):
                    if glyph.data[x + y * glyph.size[0]]:
                        self._inky.set_pixel(cur_pos[0] - glyph.offset[0] + int(x),
                                             cur_pos[1] - glyph.offset[1] + int(y), 
                                             color)
            cur_pos = (cur_pos[0] + glyph.advance[0], cur_pos[1] + glyph.advance[1])

    def bbox_of_string(self, font, string):
        bbox = [65536, 65536, 0, 0]
        def update_bbox(bbox, x, y):
            if x < bbox[0]:
                bbox[0] = x
            if y < bbox[1]:
                bbox[1] = y
            if x > bbox[2]:
                bbox[2] = x
            if y > bbox[3]:
                bbox[3] = y

        cur_pos = (0, font.height)
        for char in string:
            glyph = font.glyph(char)
            for y in range(glyph.size[1]):
                for x in range(glyph.size[0]):
                    if glyph.data[x + y * glyph.size[0]]:
                        update_bbox(bbox,
                                    cur_pos[0] - glyph.offset[0] + x,
                                    cur_pos[1] - glyph.offset[1] + y)
            cur_pos = (cur_pos[0] + glyph.advance[0], cur_pos[1] + glyph.advance[1])
        return bbox

    def print_center(self, rect, font, color, string):
        bbox = self.bbox_of_string(font, string)
        target_size = (rect[2], rect[3])
        text_size = (bbox[2] - bbox[0], bbox[3] - bbox[1])
        centered_position = [
            int((target_size[0] - text_size[0]) / 2),
            int((target_size[1] - text_size[1]) / 2),
        ]
        x_pos = int(rect[0])
        y_pos = int(rect[1])
        self.print(x_pos + centered_position[0] - bbox[0], y_pos + centered_position[1] - bbox[1], font, color, string)


def get_forcast_impl(config: dict):
    if config.get("forcast_source", "NWS") == "NWS":
        return forcast_nws.ForcastNWS


class WeatherApp:
    BACKGROUND_COLOR = Renderer.WHITE
    TEXT_COLOR = Renderer.BLACK
    PANEL_SIZE = (196, 196)
    BORDER_WIDTH = 3
    BORDER_COLOR = Renderer.BLACK
    PANEL_COORDS = [
        (BORDER_WIDTH, BORDER_WIDTH),
        (BORDER_WIDTH * 2 + PANEL_SIZE[0], BORDER_WIDTH),
        (BORDER_WIDTH * 3 + PANEL_SIZE[0] * 2, BORDER_WIDTH),
        (BORDER_WIDTH, BORDER_WIDTH * 2 + PANEL_SIZE[1]),
        (BORDER_WIDTH * 2 + PANEL_SIZE[0], BORDER_WIDTH * 2 + PANEL_SIZE[1]),
        (BORDER_WIDTH * 3 + PANEL_SIZE[0] * 2, BORDER_WIDTH * 2 + PANEL_SIZE[1]),
    ]
    DATE_TIME_SIZE = (Renderer.SCREEN_WIDTH - BORDER_WIDTH * 2, Renderer.SCREEN_HEIGHT - (BORDER_WIDTH * 4 + PANEL_SIZE[1] * 2))
    DATE_TIME_COORDS = (BORDER_WIDTH, BORDER_WIDTH * 3 + PANEL_SIZE[1] * 2)

    def __init__(self, inky, config):
        self._render = Renderer(inky)
        self._forcast = get_forcast_impl(config)(config)
        self._assets = AssetDb()
        self._weather_icon_name = "unknown"
        self._weather_icon = None
        self._warning_image = self._assets.load_image("warning")
        self._update_on_the_minutes = config.get("update_on_the_minutes", 0)
        self._small_font = self._assets.load_font("small")
        self._medium_font = self._assets.load_font("medium")
        self._large_font = self._assets.load_font("large")
        self._title_font = self._assets.load_font("titles")
        self._testing = False

    @property
    def testing(self):
        return self._testing

    @testing.setter
    def testing(self, enabled):
        self._testing = enabled

    def draw_borders(self):
        # aliases to make draw calls easier to read
        color = WeatherApp.BORDER_COLOR
        width = Renderer.SCREEN_WIDTH
        height = Renderer.SCREEN_HEIGHT
        thickness = WeatherApp.BORDER_WIDTH

        # Screen border
        self._render.rectangle(0, 0, width, thickness, color)
        self._render.rectangle(0, height - thickness, width, thickness, color)
        self._render.rectangle(0, 0, thickness, height, color)
        self._render.rectangle(width - thickness, 0, thickness, height, color)

        # horizontal divisions
        y_pos = thickness + WeatherApp.PANEL_SIZE[1]
        self._render.rectangle(0, y_pos, width, thickness, color)
        y_pos = y_pos + thickness + WeatherApp.PANEL_SIZE[1]
        self._render.rectangle(0, y_pos, width, thickness, color)

        # vertical divisions
        height = Renderer.SCREEN_HEIGHT - thickness * 2 - WeatherApp.DATE_TIME_SIZE[1]
        x_pos = thickness + WeatherApp.PANEL_SIZE[0]
        self._render.rectangle(x_pos, 0, thickness, height, color)
        x_pos = x_pos + thickness + WeatherApp.PANEL_SIZE[0]
        self._render.rectangle(x_pos, 0, thickness, height, color)

    def draw_forcast_icon(self, panel):
        pos = WeatherApp.PANEL_COORDS[panel]
        self._render.blit(pos[0], pos[1], self._weather_icon)

    def draw_single_value_panel(self, panel, title, text):
        pos = WeatherApp.PANEL_COORDS[panel]
        title_area = [pos[0], pos[1], WeatherApp.PANEL_SIZE[0], self._title_font.height]
        self._render.print_center(title_area, self._title_font, WeatherApp.TEXT_COLOR, title)
        value_area = [pos[0], pos[1] + self._title_font.height, WeatherApp.PANEL_SIZE[0], WeatherApp.PANEL_SIZE[1] -  + self._title_font.height]
        self._render.print_center(value_area, self._large_font, WeatherApp.TEXT_COLOR, text)

    def draw_double_value_panel(self, panel, title, text1, text2):
        pos = WeatherApp.PANEL_COORDS[panel]
        title_area = [pos[0], pos[1], WeatherApp.PANEL_SIZE[0], self._title_font.height]
        self._render.print_center(title_area, self._title_font, WeatherApp.TEXT_COLOR, title)
        value_area1 = [pos[0], pos[1] + self._title_font.height, WeatherApp.PANEL_SIZE[0], (WeatherApp.PANEL_SIZE[1] - self._title_font.height)/2]
        self._render.print_center(value_area1, self._medium_font, WeatherApp.TEXT_COLOR, text1)
        value_area2 = [pos[0], pos[1] + self._title_font.height + (WeatherApp.PANEL_SIZE[1] - self._title_font.height)/2, WeatherApp.PANEL_SIZE[0], (WeatherApp.PANEL_SIZE[1] - self._title_font.height)/2]
        self._render.print_center(value_area2, self._medium_font, WeatherApp.TEXT_COLOR, text2)

    def draw_current_temp(self, panel):
        self.draw_single_value_panel(panel, "Currently", f"{self._forcast.temperature}{self._forcast.temperature_text}")

    def draw_min_max_temp(self, panel):
        max_temp_text = f"Hi {self._forcast.temperature_max}{self._forcast.temperature_text}"
        min_temp_text = f"Lo {self._forcast.temperature_min}{self._forcast.temperature_text}"
        self.draw_double_value_panel(panel, "Forcasted", max_temp_text, min_temp_text)

    def draw_precip_chance(self, panel):
        self.draw_single_value_panel(panel, "Precip Chance", f"{self._forcast.precipitation_chance}%")

    def draw_wind(self, panel):
        wind_speed_text = f"{self._forcast.wind_speed}{self._forcast.speed_text}"
        wind_heading_text = f"{self._forcast.wind_heading}°"
        self.draw_double_value_panel(panel, "Wind", wind_speed_text, wind_heading_text)

    def draw_humidity(self, panel):
        humidity_text = f"{self._forcast.humidity}%"
        dew_text = f"{self._forcast.dewpoint}{self._forcast.temperature_text}"
        self.draw_double_value_panel(panel, "Humidity/Dew", humidity_text, dew_text)

    def draw_date_time(self):
        pos = WeatherApp.DATE_TIME_COORDS
        time_area = [pos[0], pos[1], WeatherApp.DATE_TIME_SIZE[0], WeatherApp.DATE_TIME_SIZE[1]]
        time_string = datetime.now().strftime("%m/%d/%Y, %a %I:%M%p")
        self._render.print_center(time_area, self._small_font, WeatherApp.TEXT_COLOR, time_string)

    def update(self):
        self._forcast.update()
        if not self._weather_icon or self._weather_icon_name != self._forcast.weather_icon:
            self._weather_icon_name = self._forcast.weather_icon
            self._weather_icon = self._assets.load_image(self._weather_icon_name)

        self._render.clear(WeatherApp.BACKGROUND_COLOR)
        self.draw_borders()
        self.draw_forcast_icon(0)
        self.draw_current_temp(1)
        self.draw_min_max_temp(2)
        self.draw_precip_chance(3)
        self.draw_wind(4)
        self.draw_humidity(5)
        self.draw_date_time()
        self._render.show()

    def run(self):
        if self._testing:
            # We're debugging, just run once with a short wait
            self.update()
            time.sleep(5)
        else:
            while True:
                self.update()
                time.sleep(60)
                now = datetime.now()
                while now.minute not in self._update_on_the_minutes:
                    time.sleep(55)
                    now = datetime.now()

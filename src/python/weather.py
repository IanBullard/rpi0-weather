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
import os
from datetime import datetime

import mock_inky

import requests
from dateutil import parser

class Forcast:
    WEATHER_URL = "https://api.weather.gov/"

    def __init__(self, config):
        self._location_name = config.get("locationName", "Unknown")
        self._latitude = config.get("latitude", 44.1076)
        self._longitude = config.get("longitude", -73.9209)
        self._timezone = config.get("timezone", "America/New_York")
        self._forecast = None
        self._temperature = None
        self._dewpoint = None
        self._temperature_max = None
        self._temperature_min = None
        self._humidity = None
        self._apparent_temperature = None
        self._sky_cover = None
        self._wind_dir = None
        self._wind_speed = None
        self._coverage = None
        self._weather = None
        self._intensity = None
        # weather
        self._percip = None
        self._wind_speed_20 = None
        self._wind_dir_20 = None
        self._get_forcast_url()
        self._get_forcast()

        # https://api.weather.gov/gridpoints/BTV/67,35
        #   everything

    def _convert_temp(self, src_unit, value):
        if src_unit == "wmoUnit:degC":
            return (value * 9 / 5) + 32
        return value

    def _get_forcast_url(self):
        try:
            request_string = f"{Forcast.WEATHER_URL}points/{self._latitude},{self._longitude}"
            response = requests.get(request_string)
        except:
            print("failed to query location")
            return
        json = response.json()
        self._forecast = json["properties"]["forecastGridData"]

    def _cur_value_from_list(self, list):
        current_time = datetime.now(None)
        result = list[0]["value"]
        for value in list:
            valid_time = parser.parse(value["validTime"].split("+")[0])
            if valid_time < current_time:
                result = value["value"]
        return result

    def _cur_value(self, data, measurement):
        return self._cur_value_from_list(data[measurement]["values"])

    def _cur_f_value(self, data, measurement):
        unit = data[measurement]["uom"]
        return self._convert_temp(unit, self._cur_value(data, measurement))

    def _get_forcast(self):
        try:
            response = requests.get(self._forecast)
        except:
            print("failed to query forcast")
            return
        data = response.json()["properties"]
        self._temperature = self._cur_f_value(data, "temperature")
        self._dewpoint = self._cur_f_value(data, "dewpoint")
        self._temperature_max = self._cur_f_value(data, "maxTemperature")
        self._temperature_min = self._cur_f_value(data, "minTemperature")
        self._humidity = self._cur_value(data, "relativeHumidity")
        self._apparent_temperature = self._cur_f_value(data, "apparentTemperature")
        self._sky_cover = self._cur_value(data, "skyCover")
        self._wind_dir = self._cur_value(data, "windDirection")
        self._wind_speed = self._cur_value(data, "windSpeed")

        weather = self._cur_value(data, "weather")[0]
        self._coverage = weather["coverage"]
        self._weather = weather["weather"]
        self._intensity = weather["intensity"]
        # weather
        #   coverage(null): areas, brief, chance, definite, few, frequent, intermittent, isolated, likely, numerous, occasional, patchy, periods, scattered, slight_chance, widespread
        #   weather(null): blowing_dust, blowing_sand, blowing_snow, drizzle, fog, freezing_fog, freezing_drizzle, freezing_rain, freezing_spray, frost, hail, haze, ice_crystals, ice_fog, rain, rain_showers, sleet, smoke, snow, snow_showers, thunderstorms, volcanic_ash, water_spouts
        #   intensity(null): very_light, light, moderate, heavy
        self._percip = self._cur_value(data, "probabilityOfPrecipitation")
        self._wind_speed_20 = self._cur_value(data, "twentyFootWindSpeed")
        self._wind_dir_20 = self._cur_value(data, "twentyFootWindDirection")

    @property
    def date(self):
        return (self._current_time[1], self._current_time[2], self._current_time[0])

    @property
    def time(self):
        return (self._current_time[3], self._current_time[5], self._current_time[4])

    def update(self):
        pass


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
    def __init__(self, id, glyphs):
        self._id = id
        self._glyphs = {}
        for glyph in glyphs:
            self._glyphs[glyph._id] = glyph

    def glyph(self, code):
        return self._glyphs[code]

    def string_size(self, string):
        width = 0
        height = 0
        for code in string:
            glyph = self.glyph(code)
            width = width + glyph.advance[0]
            if glyph.advance[1] > height:
                height = glyph.advance[1]

        return (width, height)


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
        res = self._cursor.execute(f"SELECT id, width, height, top, left, advance_x, advance_y, data FROM fonts")
        glyphs = []
        for result in res.fetchall():
            glyphs.append(Glyph(*result))
        return Font(id, glyphs)


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
                if color != 7:
                    self._inky.set_pixel(pos_x + x, pos_y + y, image.data[x + y * image.width])

    def rectangle(self, pos_x, pos_y, width, height, color):
        for y in range(height):
            for x in range(width):
                self._inky.set_pixel(pos_x + x, pos_y + y, color)

    def lineX(self, start_x, end_x, y, color):
        for x in range(start_x, end_x):
            self._inky.set_pixel(x, y, color)

    def liney(self, x, start_y, end_y, color):
        for y in range(start_y, end_y):
            self._inky.set_pixel(x, y, color)

    def print(self, x, y, font, color, string):
        cur_pos = (x, y)
        for char in string:
            glyph = font.glyph(char)
            for y in range(glyph.size[1]):
                for x in range(glyph.size[0]):
                    if glyph.data[x + y * glyph.size[0]]:
                        self._inky.set_pixel(cur_pos[0] - glyph.offset[0] + x,
                                             cur_pos[1] - glyph.offset[1] + y, 
                                             color)
            cur_pos = (cur_pos[0] + glyph.advance[0], cur_pos[1] + glyph.advance[1])


class WeatherApp:
    def __init__(self, inky):
        self._render = Renderer(inky)
        self._forcast = Forcast({})
        self._assets = AssetDb()
        self._test_image = self._assets.load_image("unknown")
        self._test_font = self._assets.load_font("test")

    def update(self):
        self._render.blit(self._render.SCREEN_WIDTH-self._test_image.width, 0, self._test_image)
        self._render.print(5, 15, self._test_font, Renderer.RED, "This is a test")
        self._render.show()

    def run(self):
        self.update()
        time.sleep(5)


app = WeatherApp(mock_inky)
app.run()
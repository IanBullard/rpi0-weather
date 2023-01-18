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

from datetime import datetime
from dateutil import parser
from forcast import Forcast
import requests
from typing import Any

class ForcastNWS(Forcast):
    WEATHER_URL = "https://api.weather.gov/"

    def __init__(self, config):
        super().__init__(config)
        self._temperature = 0
        self._temperature_max = 0
        self._temperature_min = 0
        self._precipitation_chance = 0
        self._wind_speed = 0
        self._wind_heading = 0
        self._humidity = 0
        self._dewpoint = 0
        self._weather_icon = "unknown"
        self._error_message = ""

        self._forcast_office_url = None
        if config.get("forcast_nws", None):
            self._use_20ft_wind = config["forcast_nws"].get("use_20ft_wind", False)
        else:
            self._use_20ft_wind = False

    def _request_forcast_url(self) -> bool:
        try:
            request_string = f"{ForcastNWS.WEATHER_URL}points/{self._latitude},{self._longitude}"
            response = requests.get(request_string)
            json = response.json()
            self._forcast_office_url = json["properties"]["forecastGridData"]
        except:
            self._error_message = f"Failed to retrieve NWS forcast office"
            return False
        return True

    def _cur_value_from_list(self, list: list) -> Any:
        try:
            current_time = datetime.now(None)
            result = list[0]["value"]
            for value in list:
                valid_time = parser.parse(value["validTime"].split("+")[0])
                if valid_time < current_time:
                    result = value["value"]
        except:
            return None
        return result

    def _cur_value(self, data, measurement):
        return self._cur_value_from_list(data[measurement]["values"])

    def update(self) -> bool:
        try:
            if not self._forcast_office_url:
                self._request_forcast_url()
            response = requests.get(self._forcast_office_url)
            data = response.json()["properties"]

            self._temperature = self._cur_value(data, "temperature")
            self._temperature_max = self._cur_value(data, "maxTemperature")
            self._temperature_min = self._cur_value(data, "minTemperature")
            self._precipitation_chance = self._cur_value(data, "probabilityOfPrecipitation")
            if self._use_20ft_wind:
                self._wind_speed = self._cur_value(data, "twentyFootWindSpeed")
                self._wind_heading = self._cur_value(data, "twentyFootWindDirection")
            else:
                self._wind_speed = self._cur_value(data, "windSpeed")
                self._wind_heading = self._cur_value(data, "windDirection")
            self._humidity = self._cur_value(data, "relativeHumidity")
            self._dewpoint = self._cur_value(data, "dewpoint")

            self._weather_icon = "unknown"

            current_time = datetime.now(None)
            is_day = current_time.hour > 6 and current_time.hour < 18

            weather = self._cur_value(data, "weather")[0]["weather"]
            intensity = self._cur_value(data, "weather")[0]["intensity"]
            sky_cover_percent = self._cur_value(data, "skyCover")
            heat_index = self._cur_value(data, "heatIndex")
            is_hot = heat_index > 100
            is_foggy = weather == "fog" or weather == "freezing_fog" or weather == "ice_fog"
            is_blowing = weather == "blowing_dust" or weather == "blowing_sand" or weather == "blowing_snow"
            is_thunder = weather == "thunderstorms"
            is_unknown = weather == "volcanic_ash" or weather == "water_spouts" or weather == "smoke"
            is_rain = weather == "drizzle" or weather == "rain" or weather == "rain_showers"
            is_snow = weather == "snow" or weather == "snow_showers"
            is_rain_freezing = weather == "freezing_drizzle" or weather == "freezing_rain" or "freezing_spray"
            is_hail = weather == "hail"
            is_sleet = weather == "sleet"

            if is_unknown:
                self._weather_icon = "unknown"
            if is_hail:
                self._weather_icon = "hail"
            elif is_thunder:
                if intensity == "very_light" or intensity == "light":
                    if is_day:
                        self._weather_icon = "thunderstorm_partial_day"
                    else:
                        self._weather_icon = "thunderstorm_partial_night"
                else:
                    self._weather_icon = "thuderstorm_full"
            elif is_sleet:
                self._weather_icon = "rain_snow"
            elif is_rain_freezing:
                if intensity == "very_light" or intensity == "light":
                    self._weather_icon = "rain_freezing"
                else:
                    self._weather_icon = "rain_freezing_heavy"
            elif is_foggy:
                if intensity == "very_light" or intensity == "light":
                    if is_day:
                        self._weather_icon = "fog_day"
                    else:
                        self._weather_icon = "fog_night"
                else:
                    self._weather_icon = "fog"
            elif is_snow:
                if intensity == "very_light" or intensity == "light":
                    if is_day:
                        self._weather_icon = "snow_partial_day"
                    else:
                        self._weather_icon = "snow_partial_night"
                elif intensity == "moderate":
                    self._weather_icon = "snow_medium"
                else:
                    self._weather_icon = "snow_heavy"
            elif is_rain:
                if intensity == "very_light" or intensity == "light":
                    if is_day:
                        self._weather_icon = "rain_partial_day"
                    else:
                        self._weather_icon = "rain_parital_night"
                elif intensity == "moderate":
                    self._weather_icon = "rain_light"
                else:
                    self._weather_icon = "rain_heavy"
            elif is_blowing:
                self._weather_icon = "windy"
            else:
                if sky_cover_percent > 80:
                    self._weather_icon = "clouds_full"
                elif sky_cover_percent > 60:
                    if is_day:
                        self._weather_icon = "cloud_heavy_day"
                    else:
                        self._weather_icon = "cloud_heavy_night"
                elif sky_cover_percent > 40:
                    if is_day:
                        self._weather_icon = "cloud_medium_day"
                    else:
                        self._weather_icon = "cloud_medium_night"
                elif sky_cover_percent > 25:
                    if is_day:
                        self._weather_icon = "cloud_light_day"
                    else:
                        self._weather_icon = "cloud_light_night"
                else:
                    if is_day and is_hot:
                        self._weather_icon = "clear_day_hot"
                    elif is_day:
                        self._weather_icon = "clear_day"
                    else:
                        self._weather_icon = "clear_night"
        except:
            self._error_message = f"Failed to retrieve NWS forcast"
            return False
        return True

    def error(self) -> str:
        return self._error_message
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

class Forcast:
    """
    Base forcast API used by the app to query the current and forcasted
    weather conditions.  This class should be subclassed with implementations
    based on whatever source you want to use.
    """
    def __init__(self, config: dict) -> None:
        """
        Initialize the base class using the settings in config.
        This class will parse and set three internal variables:
        "locationName" -> self.location_name
        "latitude" -> self.latitude
        "longitude" -> self.longitude
        "measurement_system" -> 
                self.measurement_system, either "metric" or "imperial"

        Subclasses may need more config settings.

        Properties that subclasses need to supply are listed below.  All results
            are assumed to be metric and this class will convert as
            necessary.

        _temperature -> int
        _temperature_max -> int
        _temperature_min -> int
        _precipitation_chance -> int
        _wind_speed -> int
        _wind_heading -> int
        _humidity -> int
        _dewpoint -> int
        _weather_icon -> str
            Must be one of the following:
                "snow_light"
                "snow_medium"
                "snow_heavy"
                "snow_blowing"
                "snow_rain"
                "rain_snow"
                "mixed_precipitation"
                "rain_hail"
                "hail"
                "clouds_full"
                "windy"
                "fog"
                "rain_light"
                "rain_heavy"
                "rain_blowing"
                "thuderstorm_full"
                "rain_freezing"
                "rain_freezing_heavy"
                "clear_night"
                "fog_night"
                "cloud_light_night"
                "cloud_medium_night"
                "cloud_heavy_night"
                "rain_parital_night"
                "snow_partial_night"
                "thunderstorm_partial_night"
                "clear_day_hot"
                "clear_day"
                "fog_day"
                "cloud_light_day"
                "cloud_medium_day"
                "cloud_heavy_day"
                "rain_partial_day"
                "snow_partial_day"
                "thunderstorm_partial_day"
                "unknown"
        """
        self._location_name = config.get("locationName", "Unknown")
        self._latitude = config.get("latitude", 44.1076)
        self._longitude = config.get("longitude", -73.9209)
        self._imperial = config.get("measurement_system", "imperial") == "imperial"

    def _celsius_to_fahrenheit(self, celsius: int) -> int:
        """
        Helper function to convert temperatures.
        """
        return (celsius * 9 / 5) + 32

    def _fahrenheit_to_celsius(self, fahrenheit: int) -> int:
        """
        Helper function to convert temperatures.
        """
        return (fahrenheit - 32) * 5 / 9

    def _kph_to_mph(self, kph) -> int:
        """
        Helper function to convert speed
        """
        return int(kph * 0.62137119223733)

    def _mph_to_kph(self, mph) -> int:
        """
        Helper function to convert speed
        """
        return int(mph * 1.609344)

    @property
    def location(self) -> str:
        return self._location_name

    @property
    def temperature(self) -> int:
        if self._imperial:
            return self._celsius_to_fahrenheit(self._temperature())
        return self._temperature()

    @property
    def temperature_max(self) -> int:
        if self._imperial:
            return self._celsius_to_fahrenheit(self._temperature_max())
        return self._temperature_max()

    @property
    def temperature_min(self) -> int:
        if self._imperial:
            return self._celsius_to_fahrenheit(self._temperature_min())
        return self._temperature_min()

    @property
    def precipitation_chance(self) -> int:
        return self._precipitation_chance()

    @property
    def wind_speed(self) -> int:
        if self._imperial:
            return self._kph_to_mph(self._wind_speed())
        return self._wind_speed()

    @property
    def wind_heading(self) -> int:
        return self._wind_heading()

    @property
    def humidity(self) -> int:
        return self._humidity()

    @property
    def dewpoint(self) -> int:
        if self._imperial:
            return self._celsius_to_fahrenheit(self._dewpoint())
        return self._dewpoint()

    @property
    def weather_icon(self) -> str:
        return self._weather_icon()

    def update(self) -> bool:
        """
        Update is called periodically as per settings.

        On false, self.error() will return a string describing the error for the user
        """
        pass

    @property
    def error(self) -> str:
        return ""

    def __str__(self) -> str:
        return f"{self.location}, {self.temperature}, {self.temperature_max}, {self.temperature_min}, {self.precipitation_chance}, {self.wind_speed}, {self.wind_heading}, {self.humidity}, {self.dewpoint}, {self.weather_icon}"

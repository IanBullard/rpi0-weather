import sys, os, json
sys.path.append(os.path.abspath('./src/python'))

from .src.python.weather_app import WeatherApp
import inky

# load base configuration
base_file = open("src/python/configuration.json")
config = json.load(base_file)

try:
    local_file = open("local_configuration.json")
    local_config = json.load(local_file)
    config.update(local_config)

app = WeatherApp(inky, config)
app.run()

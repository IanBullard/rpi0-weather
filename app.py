import sys, os, json
sys.path.append(os.path.abspath('./src/python'))

from weather_app import WeatherApp
from inky import Inky

# load base configuration
base_file = open("src/python/configuration.json")
config = json.load(base_file)

try:
    local_file = open("local_configuration.json")
    local_config = json.load(local_file)
    config.update(local_config)
except:
    exit(-1)

app = WeatherApp(Inky, config)
app.run()

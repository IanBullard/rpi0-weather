import sys, os, json
app_dir = os.path.abspath('./src/python')
print(app_dir)
sys.path.append(app_dir)

from weather_app import WeatherApp
from inky.auto import auto

inky = auto(verbose=True)

# load base configuration
base_file = open("src/python/configuration.json")
config = json.load(base_file)

try:
    local_file = open("local_configuration.json")
    local_config = json.load(local_file)
    config.update(local_config)
except:
    print("Failed to parse local config")
    exit(-1)

app = WeatherApp(inky, config)
app.run()

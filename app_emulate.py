import sys, os, json
import mock_inky
sys.path.append(os.path.abspath('./src/python'))

import weather_app

# load base configuration
base_file = open("src/python/configuration.json")
config = json.load(base_file)

local_file = open("local_configuration.json")
local_config = json.load(local_file)
config.update(local_config)
config["update_delay_sec"] = 0

app = weather_app.WeatherApp(mock_inky, config)
app.run()

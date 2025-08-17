#!/usr/bin/env python3
"""
Simple test script to verify National Weather Service API endpoints
are still functional for the rpi0-weather project.
"""

import requests
import json
from datetime import datetime
from dateutil import parser

# Test coordinates (Mount Marcy from configuration.json)
LATITUDE = 44.1076
LONGITUDE = -73.9209
WEATHER_URL = "https://api.weather.gov/"

def test_points_endpoint():
    """Test the /points endpoint to get forecast office URLs."""
    print("🔍 Testing NWS Points endpoint...")
    try:
        request_string = f"{WEATHER_URL}points/{LATITUDE},{LONGITUDE}"
        print(f"   URL: {request_string}")
        
        response = requests.get(request_string, timeout=10)
        response.raise_for_status()
        
        data = response.json()
        forecast_office_url = data["properties"]["forecastGridData"]
        stations_url = data["properties"]["observationStations"]
        
        print(f"✅ Points endpoint working")
        print(f"   Forecast Office: {forecast_office_url}")
        print(f"   Stations URL: {stations_url}")
        
        return forecast_office_url, stations_url
        
    except Exception as e:
        print(f"❌ Points endpoint failed: {e}")
        return None, None

def test_stations_endpoint(stations_url):
    """Test station lookup and find nearest station."""
    print("\n🔍 Testing NWS Stations endpoint...")
    try:
        response = requests.get(stations_url, timeout=10)
        response.raise_for_status()
        
        data = response.json()
        
        # Find closest station (same logic as forcast_nws.py)
        closest_distance2 = 90 * 90 + 180 * 180
        closest_id = ""
        closest_name = ""
        
        for station_data in data["features"]:
            lon, lat = station_data["geometry"]["coordinates"]
            dist2 = (LATITUDE - lat) * (LATITUDE - lat) + (LONGITUDE - lon) * (LONGITUDE - lon)
            if dist2 < closest_distance2:
                closest_distance2 = dist2
                closest_id = station_data["properties"]["stationIdentifier"]
                closest_name = station_data["properties"]["name"]
        
        observation_url = f"{WEATHER_URL}stations/{closest_id}/observations"
        
        print(f"✅ Stations endpoint working")
        print(f"   Closest Station: {closest_name} ({closest_id})")
        print(f"   Distance²: {closest_distance2:.6f}")
        print(f"   Observations URL: {observation_url}")
        
        return observation_url
        
    except Exception as e:
        print(f"❌ Stations endpoint failed: {e}")
        return None

def test_observations_endpoint(observation_url):
    """Test current weather observations."""
    print("\n🔍 Testing NWS Observations endpoint...")
    try:
        response = requests.get(observation_url, timeout=10)
        response.raise_for_status()
        
        data = response.json()
        
        # Find first valid reading (same logic as forcast_nws.py)
        readings = None
        for feature in data["features"]:
            if feature["properties"]["temperature"]["qualityControl"] == "V":
                readings = feature["properties"]
                break
        
        if readings is None:
            print("❌ No valid temperature readings found")
            return
        
        # Extract current conditions
        temp_c = readings["temperature"]["value"]
        temp_f = (temp_c * 9 / 5) + 32 if temp_c else None
        wind_speed = readings["windSpeed"]["value"]
        wind_dir = readings["windDirection"]["value"]
        humidity = readings["relativeHumidity"]["value"]
        dewpoint = readings["dewpoint"]["value"]
        
        print(f"✅ Observations endpoint working")
        print(f"   Temperature: {temp_c}°C ({temp_f:.1f}°F)" if temp_c else "   Temperature: Not available")
        print(f"   Wind: {wind_speed} km/h @ {wind_dir}°" if wind_speed and wind_dir else "   Wind: Not available")
        print(f"   Humidity: {humidity}%" if humidity else "   Humidity: Not available")
        print(f"   Dewpoint: {dewpoint}°C" if dewpoint else "   Dewpoint: Not available")
        
    except Exception as e:
        print(f"❌ Observations endpoint failed: {e}")

def test_forecast_endpoint(forecast_office_url):
    """Test forecast grid data."""
    print("\n🔍 Testing NWS Forecast Grid endpoint...")
    try:
        response = requests.get(forecast_office_url, timeout=10)
        response.raise_for_status()
        
        data = response.json()["properties"]
        
        # Test key forecast data (same logic as forcast_nws.py)
        current_time = datetime.now(None)
        
        def get_current_value(measurement):
            try:
                values = data[measurement]["values"]
                result = values[0]["value"]
                for value in values:
                    valid_time = parser.parse(value["validTime"].split("+")[0])
                    if valid_time < current_time:
                        result = value["value"]
                return result
            except:
                return None
        
        temp_max = get_current_value("maxTemperature")
        temp_min = get_current_value("minTemperature")
        precip_chance = get_current_value("probabilityOfPrecipitation")
        sky_cover = get_current_value("skyCover")
        weather_data = get_current_value("weather")
        
        print(f"✅ Forecast Grid endpoint working")
        print(f"   Max Temperature: {temp_max}°C" if temp_max else "   Max Temperature: Not available")
        print(f"   Min Temperature: {temp_min}°C" if temp_min else "   Min Temperature: Not available")
        print(f"   Precipitation Chance: {precip_chance}%" if precip_chance else "   Precipitation Chance: Not available")
        print(f"   Sky Cover: {sky_cover}%" if sky_cover else "   Sky Cover: Not available")
        
        # Test weather condition parsing
        if weather_data and len(weather_data) > 0:
            weather_condition = weather_data[0]["weather"]
            intensity = weather_data[0]["intensity"]
            print(f"   Weather: {weather_condition} ({intensity})")
            
            # Test icon selection logic
            current_hour = datetime.now().hour
            is_day = 6 < current_hour < 18
            
            if weather_condition == "thunderstorms":
                icon = "thunderstorm_partial_day" if is_day else "thunderstorm_partial_night"
            elif weather_condition in ["rain", "rain_showers"]:
                icon = "rain_partial_day" if is_day else "rain_parital_night"
            elif weather_condition in ["snow", "snow_showers"]:
                icon = "snow_partial_day" if is_day else "snow_partial_night"
            elif sky_cover and sky_cover > 80:
                icon = "clouds_full"
            elif sky_cover and sky_cover > 25:
                icon = "cloud_light_day" if is_day else "cloud_light_night"
            else:
                icon = "clear_day" if is_day else "clear_night"
                
            print(f"   Suggested Icon: {icon}")
        else:
            print("   Weather: Not available")
        
    except Exception as e:
        print(f"❌ Forecast Grid endpoint failed: {e}")

def main():
    """Run all NWS API tests."""
    print("🌦️  NWS API Test for rpi0-weather")
    print("=" * 50)
    print(f"Testing location: Mount Marcy ({LATITUDE}, {LONGITUDE})")
    print()
    
    # Test each endpoint in sequence
    forecast_office_url, stations_url = test_points_endpoint()
    
    if stations_url:
        observation_url = test_stations_endpoint(stations_url)
        if observation_url:
            test_observations_endpoint(observation_url)
    
    if forecast_office_url:
        test_forecast_endpoint(forecast_office_url)
    
    print("\n" + "=" * 50)
    print("🏁 NWS API test complete")

if __name__ == "__main__":
    main()
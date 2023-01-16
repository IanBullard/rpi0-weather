#!/bin/sh
# This script is used to setup the Pi 0 with the proper software needed to run this project.
#   The end result should be a rpi0-weather that is ready to run
sudo apt install git
git clone https://github.com/IanBullard/rpi0-weather.git
python -m ensurepip --upgrade
pip install requests
pip install python-dateutil

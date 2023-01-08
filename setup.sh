#!/bin/sh
# This script is used to setup the Pi 0 with the proper software needed to build and run this project.
#   The end result should be a rpi0-weather that is ready to build
sudo apt install build-essential clang llvm git
git clone https://github.com/IanBullard/rpi0-weather.git
export CC=clang
bash <(curl -fsSL https://xmake.io/shget.text)

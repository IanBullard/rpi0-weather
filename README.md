# rpi0-weather

Raspberry Pi Zero  with Inky Display weather station

The Pi Zero application is entirely Python and uses [Inky modules](https://github.com/pimoroni/inky) to display weather information.

There is also a supporting C++ application which converts assets and emulates the Inky display on a PC.  The Pi Zero is very slow so using the C++ app

## Building C++ app

You need [xmake](https://xmake.io/#/) to build.  Run `xmake` in the project folder to build the application.


## Development notes

While debugging, Pi0 needs to disable WiFi power management (sleep).  That can be done with the command:

```
sudo /sbin/iw wlan0 set power_save off
```

To check the status:
```
/sbin/iw wlan0 get power_save
```

## Development Todo

1. Pull in weather_image_converter code
2. Create mock Inky Impression display 600x448 7 color display
    1. There's an existing mock display in Inky's repo, how hard is it to get working?
        1. Does not work, make C++ version
3. Create font->image converter
4. Create Python line, box drawing routines
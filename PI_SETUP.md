# Raspberry Pi Zero Weather Station Setup

This guide will help you set up the weather station application to run automatically on boot on a Raspberry Pi Zero with the Inky Impression 7-color e-paper display.

## Hardware Requirements

- Raspberry Pi Zero (or Zero 2 W)
- Pimoroni Inky Impression 7.3" 7-color e-paper display
- MicroSD card (16GB or larger recommended)
- Power supply (5V micro USB)
- Internet connection (WiFi or Ethernet adapter)

## Initial Pi Zero Setup

### 1. Install Raspberry Pi OS

1. Download Raspberry Pi Imager from https://www.raspberrypi.com/software/
2. Flash Raspberry Pi OS Lite to your SD card
3. Before ejecting, enable SSH and configure WiFi:
   - Create empty file named `ssh` in boot partition
   - Create `wpa_supplicant.conf` in boot partition:
```
country=US
ctrl_interface=DIR=/var/run/wpa_supplicant GROUP=netdev
update_config=1

network={
    ssid="YourWiFiNetwork"
    psk="YourWiFiPassword"
}
```

### 2. Boot and Initial Configuration

1. Insert SD card and boot the Pi Zero
2. SSH into your Pi: `ssh pi@raspberrypi.local` (password: raspberry)
3. Update the system:
```bash
sudo apt update
sudo apt upgrade -y
```

4. Enable SPI for the e-paper display:
```bash
sudo raspi-config
# Navigate to: Interface Options > SPI > Enable
# Navigate to: Interface Options > I2C > Enable
```

5. Reboot: `sudo reboot`

## Install Dependencies

### 1. Install System Dependencies
```bash
sudo apt install -y build-essential cmake git pkg-config
```

### 2. Install Weather Station Dependencies

**Option A: Fast Build (Recommended for Pi) - Use System Packages**
```bash
# Install pre-compiled dependencies to avoid long build times
sudo apt install -y nlohmann-json3-dev libstb-dev
# Note: libhttplib-dev may not be available on all Pi OS versions
# If available: sudo apt install -y libhttplib-dev
```

**Option B: Standard Build - Download from Source**
```bash
# No additional packages needed - CMake will download dependencies
# This takes much longer but works on all systems
```

### 3. Install Inky Library
```bash
curl -sSL https://get.pimoroni.com/inky | bash
```

## Build and Deploy Application

### 1. Clone and Build
```bash
cd /home/pi
git clone <your-repository-url> rpi0-weather
cd rpi0-weather

# Update submodules
git submodule update --init --recursive

# Build with fast system packages (recommended for Pi)
cmake -DUSE_SYSTEM_PACKAGES=ON -DBUILD_EMULATOR=OFF .
make -j$(nproc)

# Alternative: Standard build (downloads dependencies, takes longer)
# cmake -DBUILD_EMULATOR=OFF .
# make -j$(nproc)

# Note: Build options
# -DUSE_SYSTEM_PACKAGES=ON   # Use system packages for faster builds
# -DBUILD_EMULATOR=OFF       # Disable SDL3 (automatic on ARM)
# -DBUILD_EMULATOR=ON        # Enable SDL3 (requires desktop packages)
```

### 2. Configuration
```bash
cd /home/pi/rpi0-weather
cp config.json.example config.json  # if exists, or create:
```

Create `config.json`:
```json
{
    "latitude": 30.5084,
    "longitude": -97.6781,
    "location_name": "Your Location",
    "use_real_api": true,
    "use_sdl_emulator": false
}
```

### 3. Test the Application
```bash
cd /home/pi/rpi0-weather
./rpi0-weather --test test.png
```

If this works without errors, proceed to the auto-start setup.

## Setup Auto-Start on Boot

### 1. Install Service File
The `rpi0-weather.service` systemd service file is included in the repository and configured for the standard Pi setup.

**Important**: Before installing, you may need to customize the service file for your system:

```bash
# Edit the service file to match your username and installation path
nano rpi0-weather.service

# Update these lines if needed:
# User=your_username          (change from 'pi' to your actual username)
# WorkingDirectory=/path/to/rpi0-weather
# ExecStart=/path/to/rpi0-weather/rpi0-weather

# Then copy to systemd directory
sudo cp rpi0-weather.service /etc/systemd/system/
sudo systemctl daemon-reload
```

### 2. Enable the Service
```bash
sudo systemctl enable rpi0-weather.service
sudo systemctl start rpi0-weather.service
```

### 3. Check Service Status
```bash
sudo systemctl status rpi0-weather.service
```

You should see the service running. To view logs:
```bash
sudo journalctl -u rpi0-weather.service -f
```

## Service Management Commands

- **Start service**: `sudo systemctl start rpi0-weather.service`
- **Stop service**: `sudo systemctl stop rpi0-weather.service`
- **Restart service**: `sudo systemctl restart rpi0-weather.service`
- **Check status**: `sudo systemctl status rpi0-weather.service`
- **View logs**: `sudo journalctl -u rpi0-weather.service -f`
- **Disable auto-start**: `sudo systemctl disable rpi0-weather.service`

## Operation

The weather station will:
1. Start automatically on boot
2. Display current weather immediately
3. Update weather data every 10 minutes
4. Allow manual weather updates via any of the 4 hardware buttons (A, B, C, D)
5. Restart automatically if the service crashes

### Button Controls

The Inky Impression display has 4 hardware buttons that can be used to interact with the weather station:
- **Button A, B, C, or D**: Pressing any button will immediately trigger a weather update
- The 10-minute timer resets after a manual update to prevent duplicate updates
- Button presses are logged to the system journal for monitoring

## Troubleshooting

### Service Won't Start
1. Check logs: `sudo journalctl -u rpi0-weather.service`
2. Verify executable permissions: `chmod +x /home/pi/rpi0-weather/rpi0-weather`
3. Test manually: `cd /home/pi/rpi0-weather && ./rpi0-weather`

### Display Issues
1. Verify SPI is enabled: `lsmod | grep spi`
2. Check Inky library installation
3. Test with Pimoroni examples

### Network Issues
1. Check internet connection: `ping 8.8.8.8`
2. Verify config.json has correct coordinates
3. Check API access (National Weather Service doesn't require API key)

### Performance Issues
- Pi Zero is single-core and limited; expect slower response times
- Consider Pi Zero 2 W for better performance
- Monitor system resources: `htop`

## Configuration Notes

- The application uses the National Weather Service API (no API key required)
- Weather updates occur every 10 minutes to balance freshness with API usage
- The service will restart automatically if it crashes
- Logs are written to systemd journal for monitoring

## Security Recommendations

1. Change default password: `passwd`
2. Update system regularly: `sudo apt update && sudo apt upgrade`
3. Consider firewall setup if needed: `sudo ufw enable`
4. Use SSH keys instead of password authentication

The weather station should now run automatically and reliably on your Raspberry Pi Zero!
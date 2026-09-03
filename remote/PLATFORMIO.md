# OpenRemote PlatformIO

Open this folder in Visual Studio Code:

`/Users/phillipcarlson/Documents/Arduino/OpenRemote/OpenRemote/OpenRemote_1.0`

The PlatformIO project builds the existing `OpenRemote_1.0.ino`; the INO file
remains the authoritative firmware source.

## PlatformIO toolbar

- Checkmark: compile the firmware.
- Right arrow: compile and upload over USB.
- Plug icon: open the serial monitor at 460800 baud.

The project is configured for the OMOTE Rev5 ESP32-S3 N16R8 hardware:

- 16 MB flash
- OPI PSRAM using the `qio_opi` memory configuration
- Existing dual-OTA 8 MB partition layout
- 460800 baud upload
- Timestamped serial output with automatic ESP32 crash decoding

The libraries are pinned in `platformio.ini`. PlatformIO installs them into the
project automatically; Arduino IDE's global library folder is not used.

The single `openremote_rev5` environment is the tested release target. Its
pinned ESP32 framework already includes tickless scheduling and Bluetooth
controller modem sleep; no experimental SDK configuration is required.

## Command-line equivalents

```sh
pio run
pio run --target upload
pio device monitor
```

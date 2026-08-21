# OpenRemote

An open-source universal remote firmware, desktop companion app, and web-based
configurator, built on the [OMOTE](https://github.com/OMOTE-Community) hardware
platform.

This repo covers the software side. Hardware lives in a separate repo:
[OpenRemote-Hardware](https://github.com/LORDSn1per/OpenRemote-Hardware).

## What's here

| Folder | What it is |
|---|---|
| `/` (this folder) | The ESP32-S3 firmware - a PlatformIO/Arduino project. `OpenRemote_1.0.ino` is the main source; see the changelog comment at the top of that file for full version history. |
| [`studio/`](studio/) | OpenRemote Studio - the desktop companion app (Mac/Windows). Builds the IR code database, flashes firmware, and provides USB recovery. |
| [`webconfig/`](webconfig/) | WebConfig - the remote's own on-device web configurator, served from the SD card. |

## Firmware

Built with PlatformIO. From this folder:

```
platformio run
platformio run --target upload --upload-port /dev/cu.usbserial-XXXX
platformio device monitor -p /dev/cu.usbserial-XXXX
```

`OpenRemote_1.0.ino`'s changelog comment (top of the file, newest first) is the
authoritative version history - every change is documented there with the reasoning
behind it, not just what changed.

## Studio and WebConfig

See [`studio/README.md`](studio/README.md) and [`webconfig/README.md`](webconfig/README.md).

## Status

Actively developed, running on real hardware. Not affiliated with the original
OMOTE-Community project - this is an independent fork with substantial firmware,
Studio, and WebConfig additions on top of the original OMOTE hardware design.

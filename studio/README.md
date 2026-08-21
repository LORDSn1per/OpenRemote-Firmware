<p align="center">
  <img src="../images/openremote-studio.jpg" alt="OpenRemote Studio — Design, Configure, Control" width="100%">
</p>

<h1 align="center">OpenRemote Studio</h1>

<p align="center">
  <strong>Build your remote. Keep it local.</strong><br>
  The desktop companion for OpenRemote — Mac and Windows.
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Version-2.67-2f8cff?style=flat-square" alt="v2.67">
  <img src="https://img.shields.io/badge/macOS-Universal-30d158?style=flat-square" alt="macOS Universal">
  <img src="https://img.shields.io/badge/Windows-10%20%7C%2011-30d158?style=flat-square" alt="Windows 10/11">
  <img src="https://img.shields.io/badge/Python-Bundled-30d158?style=flat-square" alt="Python bundled">
</p>

---

## What it does

Studio handles everything that happens **outside** the remote — the jobs that need
a real computer, a USB cable, or an internet connection. Day-to-day configuration
lives in [WebConfig](../webconfig/) on the remote itself; Studio is the toolkit
behind it.

It's a real desktop app, not a script you run from a terminal. Python is bundled —
you don't need to install anything.

---

## Features

### 📚 IRDB Builder
Builds a single searchable IR code database (`OpenRemote.irdb`) for the remote's SD
card, scraped from four community sources:

| Source | Format |
|---|---|
| [Flipper-IRDB](https://github.com/Lucaslhm/Flipper-IRDB) | Flipper `.ir` |
| [probonopd/irdb](https://github.com/probonopd/irdb) | Compact protocol/device/function |
| [Flipper Zero Firmware](https://github.com/flipperdevices/flipperzero-firmware) | Flipper `.ir` |
| [LIRC Remotes](https://github.com/probonopd/lirc-remotes) | LIRC config |

Over 100,000 codes, merged and deduplicated into one file. Once it's on the card,
the remote works **entirely offline** — no lookup service, nothing to shut down.

### 🔎 IRDB Browser
Search the built database by brand and model, preview the commands, and send a
device straight to the remote over USB.

### ⚡ Setup New Remote
Takes a bare ESP32-S3 to a working remote:
1. Verifies the board over USB
2. Flashes the firmware `.bin` you choose
3. Prepares the SD card — folders, WebConfig, default icons and themes

No BOOT button to hold, no Arduino IDE, no PlatformIO.

### 🔧 Recovery
A rescue path when the remote's own web page can't be reached — everything below
works over **USB serial only**, no Wi-Fi required, and the SD card stays inside
the remote:

- **Install WebConfig** — repair a broken or half-installed web interface
- **Prepare SD card** — rebuild missing folders and default libraries in place
- **Load backup** — copy a backup to the remote and apply it
- **Load `.IR` file** — push a single device file straight to `/devices`
- **Restore factory settings** — wipe user data, keep firmware and libraries

### 🩺 Hardware diagnostic
Installs a bundled Rev 5 sensor-test firmware that exercises the LCD, touch, I²C
bus, accelerometer, fuel gauge and IR — without touching the SD card. Restore your
real firmware when you're done.

---

## Running it

### From a release (recommended)
Grab the build for your platform from [Releases](../../../releases).

- **macOS** — universal binary, runs natively on both Apple Silicon and Intel
- **Windows** — extract the whole folder and run the `.exe`. Keep the `runtime`
  and `app` folders beside it; the `.exe` alone won't start without them.

> **macOS note:** builds are ad-hoc signed rather than notarised, so the first
> launch needs **right-click → Open** (or Privacy & Security → "Open Anyway").
> Once per copy, then it opens normally.

### From source

```bash
pip install -r requirements.txt --target vendor
python3 openremote_studio.py
```

Studio opens in a native app window using the OS's own web engine — WKWebView on
macOS, WebView2 on Windows — with an automatic fallback to your default browser if
that engine isn't available.

---

## What's in this folder

| File | Purpose |
|---|---|
| `openremote_studio.py` | The app — local server, USB serial protocol, IRDB builder, esptool flashing |
| `studio.html` | The interface |
| `requirements.txt` | Python dependencies |
| `OpenRemote*.png` / `.icns` | Branding and icon assets |

Third-party packages (`vendor/`) and compiled builds aren't tracked here — source
only. `requirements.txt` documents exactly what's needed to rebuild.

---

<p align="center">
  <sub>Part of the <a href="../">OpenRemote</a> ecosystem · Open source · 100% local · No cloud · No accounts</sub>
</p>

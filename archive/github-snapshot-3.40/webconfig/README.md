<p align="center">
  <img src="../images/openremote-webconfig.jpg" alt="OpenRemote Web Config — Configure From Any Screen" width="100%">
</p>

<h1 align="center">OpenRemote WebConfig</h1>

<p align="center">
  <strong>Configure from any screen.</strong><br>
  Computer or phone. Same local control.
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Version-2.47-2f8cff?style=flat-square" alt="v2.47">
  <img src="https://img.shields.io/badge/Install-Nothing-30d158?style=flat-square" alt="No install">
  <img src="https://img.shields.io/badge/Cloud-None-30d158?style=flat-square" alt="No cloud">
  <img src="https://img.shields.io/badge/Single-HTML%20file-2f8cff?style=flat-square" alt="Single HTML file">
</p>

---

## The whole configurator, served by the remote

WebConfig is a **single self-contained HTML file** that lives on the remote's SD
card and is served by the firmware itself. Point any browser on your network at
the remote's IP address and you're in.

- **No app to install** — it's a web page
- **No account** — there's nothing to sign into
- **No internet** — your browser talks directly to the remote on your own network
- **Works on anything** — desktop, laptop, tablet, phone

This is where the "no programming required" promise actually lives. Everything you
can configure about the remote, you configure here, visually.

---

## Features

### 🎨 Live screen designer
Drag activities, buttons and macros onto a canvas that mirrors the real remote —
and see the layout appear on the LCD as you build it. Page through multi-screen
device layouts exactly as they'll appear in your hand.

### 📺 Devices and commands
Add devices from the built-in IR database, or import your own codes:

- **IR database** — search thousands of devices by brand and model
- **Learned IR** — capture codes straight from an original remote using the receiver
- **Pronto Hex** — paste or drop industry-standard Pronto codes
- **Global Caché** — import `sendir` code sets and full code-set files
- **Flipper `.ir` files** — import community device files

### 🎬 Activities and macros
Build an activity like "Watch TV" that powers on the TV, switches the amplifier to
the right input and starts the player — in order, with per-step delays. Macros
chain multiple commands behind a single button.

### 🖼️ Icons and themes
Upload custom icons and wallpapers. Crop and position artwork, choose gradient
colours, set the glass overlay's colour and transparency, and control how many
control rows fit on screen — with a live preview.

### 🏠 Smart home and connectivity
- **Homebridge** — bind buttons to lights, switches and scenes on your own network
- **Bluetooth** — pair with a Chromecast / Android TV for native remote control
- **Wi-Fi** — manage networks, with QR-code setup for first-time connection
- **Clock** — network time sync with timezone selection

### 💾 Backup and restore
Full configuration backup and restore in the browser — devices, activities, macros,
plus **embedded copies of your custom icons and themes**, so a restore brings back
everything, not just the settings. Export individual categories, or a single
learned device as a portable `.ir` file.

The remote can also back up and restore **entirely on its own**, straight from the
LCD menu to the SD card — no computer involved at all.

### ⚙️ Remote settings
Brightness, sleep and deep-sleep timing, wake mode and sensitivity, display gamma
and saturation, button repeat timing, physical button remapping, and a debug menu
with live on-screen diagnostics.

---

## Using it

1. Connect the remote to your Wi-Fi (QR-code setup on first run)
2. Open the remote's IP address in any browser — the remote shows it on-screen
3. Configure anything
4. Hit **Sync** — changes are written to the remote's SD card over Wi-Fi

If the remote isn't on Wi-Fi yet, or WebConfig ever becomes unreachable,
[OpenRemote Studio](../studio/) can install or repair it over USB.

---

## What's in this folder

The `.html` file **is** the entire application — HTML, CSS and JavaScript in one
file, no build step, no dependencies, no external assets. It's installed to the SD
card as `/www/index.html`.

Versions are kept side by side rather than overwritten, so any earlier build stays
recoverable.

---

<p align="center">
  <sub>Part of the <a href="../">OpenRemote</a> ecosystem · Open source · 100% local · No cloud · No accounts</sub>
</p>

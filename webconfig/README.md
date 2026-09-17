<div align="center">
  <img src="../docs/images/openremote-webconfig.jpg" alt="OpenRemote WebConfig — configure from any screen" width="100%">

  ### Configure from any screen

  The remote serves its own configurator over your Wi-Fi. Open it on a laptop
  or a phone — nothing to install, no app, no account.

  <a href="#opening-it">Opening it</a> ·
  <a href="#what-you-can-do">What you can do</a> ·
  <a href="#updating-webconfig">Updating</a> ·
  <a href="#the-dock">The dock</a>
</div>

---

## Opening it

The remote must be on your Wi-Fi. Then either:

- **Scan the QR code** on the remote — *Settings → Wi-Fi → QR code*, or
- **Type its address** into a browser, shown on the same screen
  (for example `192.168.1.170`).

That is the whole setup. WebConfig is a single HTML file living on the remote's
SD card, served by the remote itself, so it works on any device with a browser
and keeps working when your internet does not.

---

## What you can do

### Screen designer
Build the pages the remote shows and watch them update live on the device as
you edit. Drag activities, buttons and macros onto a page, and set its theme.

### Widgets

Place the **Weather**, **Battery** and **Media** widgets, or build your own on
the **Widgets** page: up to three elements — time and date, weather, battery and
media — under a name of your choosing, on a Widget Wallpaper. Any widget can be
**large** (three columns, two rows) or **slim** (one row, the height of a
button).

- A media element can show **only while something is playing**, and can **take
  over the whole widget** while it plays, with artwork on a large widget. Both
  changes animate on the remote.
- The time element can show the **date first** — left of the time on a slim
  widget, above it on a large one.
- A media-only widget can **auto hide**: it takes no rows until playback starts,
  then grows into place and pushes the rest of the page down. The designer shows
  it at full size.

The editor previews every widget large and slim, and switches between *Nothing
playing* and *Playing* so you can see the animation before saving. Custom
widgets are included in backups and restores.

### Activities & Macros
An activity turns several devices on, switches the right inputs and hands you a
matching screen — one slide to start watching something. A macro is a sequence
of commands on a single button.

### Devices & Commands
Add equipment and its commands, learn a signal from an original remote, or pull
codes from the infrared database. Assign anything to the physical buttons.

The database lives on the remote's own SD card, so searching 14,000-plus
devices needs nothing but this page — no computer and no Studio. The search
runs on the remote and the matching devices come straight from its SD card, with
no Wi-Fi needed. Click
any command to fire the real infrared through the remote or the dock, so a code
set can be confirmed before you keep it.

Any single device can also be saved as one file — infrared, RF433, Homebridge,
MQTT or Bluetooth — and imported into another remote from
**Add device > Import a device file**.

### Icons & Themes
Upload your own icons and switch themes. What you choose appears on the remote
immediately.

### Backup & Restore
Save the whole configuration to a file and restore it later — useful before
changing something large, or to copy a setup to a second remote.

The remote writes the backup itself and restores it the same way, so a file is
identical whether it was made here or from the remote's own screen, and either
one can restore the other. Backups can be kept on the computer or on the SD
card, and progress is shown on both the page and the remote's display.

There is also a file manager for the SD card, with guards that refuse to delete
anything the remote needs to start.

### Settings
Wi-Fi, Bluetooth pairing for Android TV and Chromecast, display and sleep
behaviour, physical button repeat, battery, and firmware updates.

---

## Updating

Both the remote's firmware and WebConfig itself update from here — no cable.

**Firmware** — *Settings → Firmware → Install compiled firmware*. Choose a
`.bin`; the remote checks it, installs it and restarts.

**WebConfig** — *Settings → WebConfig*. Choose a newer `.html` and it replaces
itself. The upload is checksummed on arrival **and read back off the SD card
afterwards**, so a card that drops bytes is caught rather than leaving you with
a half-written configurator you would need a cable to fix.

[**Download the latest WebConfig →**](https://github.com/LORDSn1per/OpenRemote-Firmware/releases/latest/download/OpenRemote-WebConfig-3.42.html)

---

## The dock

If a [dock](../dock/README.md) is paired, WebConfig gains its controls:

- **Transmit IR from** — this remote, the dock, or both at once.
- **Dock RF433** — enable the 433 MHz transmitter, and **learn** a signal from
  an existing gate, garage or socket remote.
- **Dock LED**, **Dock LED brightness** and **Tx Power** — whether the LED
  flashes on transmit, how brightly it glows (5–100%), and range behaviour.
- **Homebridge via dock** — let the dock send your Homebridge commands instead
  of the remote. It is mains powered, so it stays on Wi-Fi permanently and the
  command goes out on a connection that is already open; the remote has to wake
  its radio and join the network first, which takes seconds. Turning it on
  sends the dock your Wi-Fi and Homebridge details automatically.
- **Update the dock wirelessly** — send new dock firmware over the radio link.
  A dock only ever needs a cable for its very first flash.

The dock's name, firmware version and address appear beside the remote's own
status, and a green dot shows when it is connected.

---

## For developers

Every released version is kept here, newest last. They are plain single-file
HTML with no build step and no external dependencies — open one in a browser
and it runs.

    webconfig/WebConfig 3.42.html    current
    webconfig/WebConfig 3.41.html    previous, and so on

The whole page — markup, styles, scripts and images — is one file because the
remote serves it from an SD card over a small embedded web server, where each
extra request costs far more than the bytes it saves.

**Editing note:** version numbers must be changed by exact literal string
replacement, never a regular expression. A stray `.` in a pattern once matched
inside an embedded base64 image and corrupted the page.

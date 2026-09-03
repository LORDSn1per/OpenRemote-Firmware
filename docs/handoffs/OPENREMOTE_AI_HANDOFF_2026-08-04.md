# OpenRemote Handoff

**Date:** 2026-08-04
**Reason for handoff:** Long working session reached a natural stopping point. Nothing is broken or mid-edit; everything below builds and is installed.

## Current Versions

| Component | Version | Location |
|---|---|---|
| Firmware | **2.84** | `SOFTWARE/FIRMWARE/BIN/OpenRemote_2.84.bin` |
| WebConfig | **2.38** | `SOFTWARE/WebConfig/WebConfig 2.38.html` |
| Studio (Mac) | **2.62** | `SOFTWARE/OpenRemote Studio/Mac/OpenRemote Studio 2.62.app` |
| Studio (Win) | **2.62** | `SOFTWARE/OpenRemote Studio/Windows/OpenRemote Studio 2.62.exe` |

**These three are a matched set.** Several fixes span two of them; mixing old and new will reproduce bugs that are already fixed.

## Working Rules (the user's conventions — follow these)

1. **Every change bumps the version by 0.01** and adds an entry to the changelog comment block at the very top of `OpenRemote_1.0.ino` (newest first, verbose, root-cause-first).
3. **Save a compiled `.bin` to `SOFTWARE/FIRMWARE/BIN/OpenRemote_<version>.bin`** after every firmware change.
4. **COPY to the new version, never rename/`mv`.** The user keeps a full running history on disk. Earlier in this session `mv` destroyed WebConfig 2.26 and 2.27 permanently — do not repeat that.
4. WebConfig has three version markers to update: `<meta name="openremote-webconfig-version">`, `<title>`, and the sidebar (now read dynamically from `/app/ping`, so only the first two matter).
5. Studio version lives in `APP_VERSION` in `openremote_studio.py`.
7. **All handoff files live in `SOFTWARE/AI Handoff/`**, named
   `OPENREMOTE_AI_HANDOFF_<YYYY-MM-DD>.md`. Write new ones there; do not
   scatter them at the repo root.

## Build and Install

```bash
cd "/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/Platformio:Arduino/OpenRemote/OpenRemote_1.0"
~/.platformio/penv/bin/platformio run
~/.platformio/penv/bin/platformio run --target upload --upload-port /dev/cu.usbserial-1330
~/.platformio/penv/bin/platformio device monitor -p /dev/cu.usbserial-1330   # 460800 baud
```

**Studio is source + PyInstaller.** The real source is `OpenRemote Studio/Windows/app/` (`openremote_studio.py` + `studio.html`) — shared by both platforms.

- **Windows:** the `.exe` is a *generic launcher*; 2.44/2.45 are byte-identical. Ship a new version by copying the previous `.exe` and updating `app/`. Note all `.exe`s share `app/`, so they all run the newest code.
- **Mac:** a frozen PyInstaller bundle that must be rebuilt. Working command:

```bash
python3 -m PyInstaller --noconfirm --clean --windowed \
  --name "OpenRemote Studio <ver>" --icon "$APP/OpenRemoteIcon.icns" \
  --paths "$APP/vendor" \
  --hidden-import esptool --hidden-import serial --hidden-import serial.tools.list_ports \
  --hidden-import configparser --hidden-import argparse --hidden-import shlex \
  --collect-submodules esptool \
  --add-data "$APP/studio.html:." --add-data "$APP/vendor:vendor" \
  --add-data "$APP/factory:factory" --add-data "$APP/OpenRemote.png:." \
  --add-data "$APP/openremote_product.png:." --add-data "$APP/OpenRemoteIcon.png:."
```

**`--paths vendor` + esptool hidden-imports are mandatory.** esptool/pyserial are vendored and added to `sys.path` at runtime, so PyInstaller cannot see them. Omitting these produces a build where Detect/Install fail with *"The bundled ESP32 flashing engine could not load: No module named 'configparser'"*. This regression was introduced and fixed once already.

After building: set `CFBundleShortVersionString`/`CFBundleVersion` via `plutil`, `codesign --force --deep --sign -`, and strip quarantine. The build is **arm64-only, ad-hoc signed** (the original 2.45 was universal2 — the spec was not available to reproduce that).

**Always verify a Mac build functionally** — run it and POST `/setup/detect` with a bogus port. A *port* error means esptool loaded; an *import* error means the build is broken.

## Hardware Constraint (settled — do not re-investigate)

**USB Mass Storage is impossible on this hardware.** Confirmed from `HARDWARE/OMOTE-Hardware-main/PCB/Omote.kicad_sch`:

- The USB-C connector goes to a **CH340C USB-to-UART bridge** — the host never sees the ESP32-S3 (hence `/dev/cu.usbserial-*`, not `usbmodem`).
- MSC needs the S3's native USB peripheral, hard-wired to **GPIO19/GPIO20**, which are not routable.
- Those pins are the shared **I2C bus** (`PIN_I2C_SCL=19`, `PIN_I2C_SDA=20`) — touch, keypad, accelerometer, fuel gauge.

The mitigation is Studio's USB file transfer + the firmware's boot-time self-heal.

## Major Work This Session

### Backup / restore (many real bugs, all fixed)
- Backups embed custom icons + theme wallpapers (2.50) **and `/devices/*.ir` files (2.61)** as base64. A backup made **before 2.61** cannot restore file-backed Studio/IRDB devices — the bytes aren't in the file.
- `POST /api/devices/file` added (was DELETE-only) so WebConfig can write `.ir` files back.
- **SD-card restore is now server-side** (`POST /api/backups/restore`, 2.67) running the same `restoreLcdFullBackup()` the LCD menu uses. Previously WebConfig downloaded the whole backup, rebuilt it in-browser and re-uploaded everything — megabytes for data that never left the card. **Zero upload now.**
- WebConfig no longer overwrites embedded icon/theme data with dead remote URLs on restore.
- Only **custom** themes are force re-uploaded on restore. Marking Default themes pending made them re-render from images that (by design) aren't in the backup — that was the "N themes skipped" message.

### Transfer reliability
- **Resumable chunked upload** (firmware 2.66 / WebConfig 2.34): `/api/upload/begin|status|chunk|finish`, 192 KB chunks, per-chunk retry, resume from the remote's actual byte count. Used for sync, WebConfig and firmware. Old single-shot endpoints retained for backward compatibility.
- **CRC-32 + size verification** on finish (2.68). Previously *nothing* verified completeness — `uploadedWebConfigLooksValid()` only read the first 768 bytes, so a truncated HTML passed and was installed. That caused a corrupted WebConfig and locked the user out.
- Runtime-config upload cap raised 2 MB → 6 MB; validation switched to a **filtered parse** (was materialising the whole document into PSRAM and failing with NoMemory on large configs).
- All 8 upload handlers now yield (`serviceUiDuringLongHttpTransfer()`); only the runtime path did.
- WebConfig sync timeout is now size-aware (was a flat 45 s, which aborted at ~2.4 MB every time — the "always fails at 2.44 MB" symptom).

### Recovery / setup
- Firmware self-heals a truncated WebConfig at boot from `/backups/index.previous.html` (2.69).
- `ORUSB STAT` (2.71), `ORUSB FACTORYRESET` (2.70), `PREPARESD` folder counts (2.72), `/themes/Default/` added to the USB write allowlist (2.73).
- Studio: new **Setup New Remote** and **Recovery** tabs; Remote Config is connect-only. Single Install button does firmware → reboot wait → SD prep. Recovery is ungated (no firmware/WebConfig selection needed).

### Studio UI architecture (refactor, 2.55)
Every job now carries the **view that started it** (`reset_factory_state(op, status, view)`), and the front end has one renderer:

```js
renderJobProgress(view, state)   // writes only <view>Status / <view>Bar / <view>Log
```

All five tabs have their own panel. This replaced hand-routed per-element updates that leaked Recovery output into the Setup tab **three separate times** (bar, status, log — each patched individually). Do not reintroduce per-element routing.

## Open Items / Not Verified

1. **UNRESOLVED: USB file transfer to the SD card dies a few seconds in.**
   Reported repeatedly. Latest observation with trustworthy UI (Studio 2.57):
   WebConfig install from Recovery reached **31 KB / 1.53 MB (2%)** then failed
   with *"No OpenRemote USB response"*. That message text (without the
   handshake suffix) comes from `serial_readline()` in
   `serial_upload_payload()`, i.e. the remote **stopped ACKing mid-transfer** -
   the handshake and the first ~31 windows succeeded. Attempted fixes so far,
   none of which resolved it: 2.74 (refuse QR page during transfer), 2.77
   (all sleep guards honour the USB link), 2.78 (stamp activity during bulk
   transfer, not just on commands). **LIKELY ROOT CAUSE FOUND in 2.81**: UART was never a light-sleep
   wake source, so a sleeping remote could not answer Studio at all. If it
   still fails on 2.81, capture serial - specifically whether the remote reboots (a
   fresh boot banner mid-copy = watchdog/crash) or simply goes quiet. Do not
   attempt another blind fix.
3. **No end-to-end restore test on hardware.** Many restore bugs were found in paths that *looked* correct. A full cycle — backup → wipe → restore → sync → verify icons/themes/devices — has not been done.
3. **`themes.json` in the SD template declares `rgb565Revision: 3`; current is 5.** Self-corrects (WebConfig re-renders on first sync) but the bundled bitmaps are one dithering generation stale. Worth regenerating.
4. **`_source.png` theme originals are excluded from USB SD prep** (5.6 MB over 460800 baud). Only WebConfig's re-crop editor reads them. The computer-card route still installs them.
5. **Firmware reboot wait in the combined install is a fixed 9 seconds**, not a poll. Should be ample; polling the USB handshake would be more robust.
6. **WebConfig's `FIRMWARE_CHANGELOG` is stale** — stops at 2.38 while firmware is 2.73.
7. **Studio Recovery still needs firmware+WebConfig chosen on another tab** for *some* paths, though Prepare SD itself is now ungated.
8. **The QR page opening by itself is mitigated, not root-caused.** On a
   fresh boot it appeared unprompted and aborted a USB transfer. 2.74 refuses
   to enter that page while `usbSdTransferActive()`, which protects the
   transfer, but *why* it navigated there was never established. The empty
   Activities screen has a large centred "Open WebConfig" button and this
   board has a long phantom-touch history — a ghost touch landing on it is the
   leading theory and worth confirming.
8. The user reported a firmware file named `OpenRemote_2.78.bin` that Studio read as **version 2.57** — Studio reads the version marker inside the binary, so that file may be an older build renamed. Worth confirming.

## Constraints

- `Platformio:Arduino/OpenRemote/OpenRemote_1.0` is a git repo; `SOFTWARE/WebConfig`, `SD Card Structure` and `OpenRemote Studio` are **not** — they rely on the copy-per-version convention.
- Preserve the user's SD card data: devices, activities, themes, icons, Wi-Fi credentials, backups.
- Do not erase flash or format the SD card unless explicitly asked.
- The user tests on real hardware. A successful build proves nothing about runtime behaviour — say so rather than implying verification that didn't happen.

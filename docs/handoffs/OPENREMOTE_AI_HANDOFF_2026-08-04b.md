# OpenRemote Handoff

**Date:** 2026-08-04 (second handoff of the day — supersedes `OPENREMOTE_AI_HANDOFF_2026-08-04.md`)
**Reason for handoff:** The long-running USB backup fault is **fixed and confirmed on hardware**. Nothing is mid-edit.

## Current Versions

| Component | Version | Location |
|---|---|---|
| Firmware | **2.92** | `SOFTWARE/FIRMWARE/BIN/OpenRemote_2.92.bin` |
| WebConfig | 2.38 | `SOFTWARE/WebConfig/WebConfig 2.38.html` |
| Studio (Mac) | **2.65** | `SOFTWARE/OpenRemote Studio/Mac/OpenRemote Studio 2.65.app` |
| Studio (Win) | **2.65** | `SOFTWARE/OpenRemote Studio/Windows/OpenRemote Studio 2.65.exe` |
| Sensor Test | **2026-08-04 (rot 0)** | `SOFTWARE/Sensor_Test/BIN/OpenRemote_Sensor_Test_factory_2026-08-04_rot0.bin` |

The previous handoff's version table said firmware 2.84 — it was already stale at 2.90 when this session started.

## SOLVED: backup restore over USB

This had been open across many versions and four blind fixes. It is now closed, and the
user confirmed a successful restore on hardware.

**Symptoms:** copying a 1.38 MB backup from Studio's Recovery tab either stalled part-way,
or reached 100% and was rejected, or copied and then "could not apply".

**Two separate faults, found in order:**

### 1. Validation and restore needed a contiguous PSRAM block (fixed in 2.91)

`uploadedRuntimeConfigLooksValid()` and `restoreLcdFullBackup()` both called
`readSdFileToPsramBuffer()`, which needs one *contiguous* block the size of the file.
Both now stream through `BufferedSdJsonStream` (1 KB reads, yields every 16 KB — preserving
the yielding that `readSdFileToPsramBuffer()` was introduced to guarantee, because a plain
`deserializeJson(doc, file)` reads a byte at a time, never yields, and rebooted the remote
historically). Validation additionally parses through an **empty filter**, so it
materialises nothing and costs a few KB regardless of backup size.

ArduinoJson 7 has **no zero-copy mode** (`BoundedReader` copies every string into the
document pool), so the raw file buffer was ~1.4 MB held *in addition to* a tree that already
contained all of it. Removing it was a strict win, not a trade-off.

### 2. ROOT CAUSE — ArduinoJson's 16-bit string length limit (fixed in 2.92)

2.91's new diagnostic named it in one run:

```
NoMemory at byte 365688 of 1446399 on the card, 7848012 PSRAM free, largest block 7733236
```

`NoMemory` with 7.7 MB **contiguous** free is not a memory shortage — and it disproved the
fragmentation theory from 2.91. ArduinoJson sizes two internal types from the pointer
width, so a 32-bit target silently gets `uint16_t` for both:

- `ARDUINOJSON_STRING_LENGTH_SIZE 2` → a single string value caps at **65,535 characters**
- `ARDUINOJSON_SLOT_ID_SIZE 2` → a document caps at 65,535 slots

`StringNode::create()` checks `if (length > maxLength) return nullptr;` **before** calling
the allocator, and a null return is reported as `NoMemory` — which is exactly why the
free-heap figures looked irrelevant. A backup embeds theme wallpapers as base64: a 240×320
RGB565 image is 153,600 bytes = **204,800 base64 characters**, over 3× the ceiling. Byte
365,688 is simply where the first wallpaper starts.

**Both sizes are now 4, set in `platformio.ini`**, with `static_assert`s in the .ino that
fail the build if either is ever lost. Do not remove them — this failure presents as an
out-of-memory error and would cost the same investigation twice.

**Why it copied but would not apply:** validation parses through an empty filter, so no
oversized string is ever constructed and the file passes; restore has to build the real
document and hits the limit on the first wallpaper. That asymmetry is deliberate —
validation confirms the bytes parse, not that the tree fits.

This never appeared before because the pre-2.91 backups on file top out at 285-character
strings; they predate asset embedding (2.50 and 2.61).

### Diagnostics added along the way (keep these)
- Upload rejection and restore failure both report the `DeserializationError`, the byte
  offset, file size and largest free PSRAM block, on serial and in Studio's message.
- The upload timeout used to report `usbSerialRxOverflows` **since boot**, so a failed
  transfer's "2 UART receive overflow(s)" included earlier attempts in the same session.
  It is now snapshotted per transfer.

## Sensor Test rotated (this session)

The user asked to rotate the sensor test screen 180°. Studio ships only the **compiled**
image; the source is a standalone sketch at `SOFTWARE/Sensor_Test/Sensor_Test.ino`.

- Display: `Arduino_ILI9341(..., 2, ...)` → **`0`** (was 180°).
- Touch: the controller reports in its own fixed orientation and knows nothing about the
  panel rotation, so `readTouchPoint()` now flips both axes
  (`x = LCD_WIDTH-1-x; y = LCD_HEIGHT-1-y;`) after clamping. Without it every press lands
  mirrored through the centre of the screen.
- `SOFTWARE/Sensor_Test/platformio.ini` pointed `board_build.partitions` at
  `../../OpenRemote/OpenRemote_1.0/partitions.csv`, which stopped resolving when the
  firmware moved under `Platformio:Arduino`. It now uses a **local copy**,
  `Sensor_Test/partitions.csv` — a path containing a colon is not worth reaching across
  for. Keep it in step with the firmware's if that layout changes.
- Old and new images archived in `SOFTWARE/Sensor_Test/BIN/`.

**Studio verifies the image by SHA-256 in `factory/manifest.json`.** After rebuilding you
must update `sensorTestImage`, `sensorTestImageBytes` **and** `sensorTestImageSha256`, or
Studio refuses the image. Done for this build (`242a93c1…`, 456560 bytes).

**Each Mac `.app` carries its own frozen copy** of `factory/`. Updating the shared
`Windows/app/factory/` does *not* reach a built Mac bundle — which is why Studio 2.65 was
built. 2.62/2.63/2.64 still embed the old 180° image.

## Working Rules (the user's conventions — follow these)

1. **Every change bumps the version by 0.01** and adds an entry to the changelog comment
   block at the very top of `OpenRemote_1.0.ino` (newest first, verbose, root-cause-first).
2. **Save a compiled `.bin` to `SOFTWARE/FIRMWARE/BIN/OpenRemote_<version>.bin`** after
   every firmware change.
3. **COPY to the new version, never rename/`mv`.** The user keeps a full running history on
   disk. `mv` destroyed WebConfig 2.26 and 2.27 permanently — do not repeat that.
4. WebConfig has version markers in `<meta name="openremote-webconfig-version">` and
   `<title>` (the sidebar reads `/app/ping` dynamically).
5. Studio version lives in `APP_VERSION` in `openremote_studio.py`.
6. **All handoff files live in `SOFTWARE/AI Handoff/`**, named
   `OPENREMOTE_AI_HANDOFF_<YYYY-MM-DD>.md`.
7. **Do not blind-fix.** Four attempts at the USB fault went after the wrong cause. What
   finally worked was shipping a diagnostic (2.90/2.91) and letting it name the fault.

## Build and Install

```bash
cd "/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/Platformio:Arduino/OpenRemote/OpenRemote_1.0"
~/.platformio/penv/bin/platformio run
~/.platformio/penv/bin/platformio run --target upload --upload-port /dev/cu.usbserial-1330
~/.platformio/penv/bin/platformio device monitor -p /dev/cu.usbserial-1330   # 460800 baud
```

Sensor test: same, from `SOFTWARE/Sensor_Test/`. Ship
`.pio/build/sensor_test_rev5/firmware.factory.bin`.

**Studio is source + PyInstaller.** The real source is `OpenRemote Studio/Windows/app/`
(`openremote_studio.py` + `studio.html`) — shared by both platforms.

- **Windows:** the `.exe` is a *generic launcher*; ship a new version by copying the
  previous `.exe`. All `.exe`s share `app/`, so they all run the newest code.
- **Mac:** a frozen PyInstaller bundle that must be rebuilt:

```bash
python3 -m PyInstaller --noconfirm --clean --windowed \
  --name "OpenRemote Studio <ver>" --icon "$APP/OpenRemoteIcon.icns" \
  --paths "$APP/vendor" \
  --hidden-import esptool --hidden-import serial --hidden-import serial.tools.list_ports \
  --hidden-import configparser --hidden-import argparse --hidden-import shlex \
  --collect-submodules esptool \
  --add-data "$APP/studio.html:." --add-data "$APP/vendor:vendor" \
  --add-data "$APP/factory:factory" --add-data "$APP/OpenRemote.png:." \
  --add-data "$APP/openremote_product.png:." --add-data "$APP/OpenRemoteIcon.png:." \
  "$APP/openremote_studio.py"
```

**`--paths vendor` + esptool hidden-imports are mandatory.** esptool/pyserial are vendored
and added to `sys.path` at runtime, so PyInstaller cannot see them. Omitting these produces
a build where Detect/Install fail with *"No module named 'configparser'"*.

After building: `plutil -replace CFBundleShortVersionString/CFBundleVersion`,
`codesign --force --deep --sign -`, `xattr -cr`. arm64-only, ad-hoc signed.

**Always verify a Mac build functionally.** Useful trick discovered this session: `DATA_DIR`
derives from `Path.home()`, so running the bundle with `HOME=<tempdir>` starts an isolated
instance that does **not** fight the instance lock of a Studio the user already has open:

```bash
HOME=/tmp/fakehome BROWSER=/usr/bin/true "OpenRemote Studio 2.65.app/Contents/MacOS/OpenRemote Studio 2.65" &
# read the port from $HOME/Library/Application Support/OpenRemote Studio/instance.json
curl -s -X POST http://127.0.0.1:<port>/setup/detect -d '{"port":"/dev/cu.bogus"}'
```

A **port** error means esptool loaded. An **import** error means the build is broken.
2.65 passed this check.

## Hardware Constraint (settled — do not re-investigate)

**USB Mass Storage is impossible on this hardware.** The USB-C connector goes to a **CH340C
USB-to-UART bridge**, so the host never sees the ESP32-S3. MSC needs the S3's native USB
peripheral on **GPIO19/GPIO20**, which are the shared I2C bus (`PIN_I2C_SCL=19`,
`PIN_I2C_SDA=20`) — touch, keypad, accelerometer, fuel gauge.

Also settled: the 2.85 RX-buffer fix is intact and correct — `CDC_ON_BOOT=0` so `Serial` is
UART0/CH340, `setRxBufferSize(8192)` precedes `begin()`, and nothing re-inits the driver
later. Verified this session; do not re-check.

## Open Items / Not Verified

1. **Intermittent mid-transfer stall is still undiagnosed.** Separate from the fault fixed
   above. The user's first copy attempt of the 1.38 MB backup failed and the second
   succeeded; an earlier one died at 83% with the remote not answering even a follow-up
   ping. The per-transfer overflow counter added in 2.91 will now say whether it is lost
   bytes on the wire. **Capture serial before attempting a fix** — specifically whether the
   remote reboots (a fresh boot banner mid-copy = watchdog/crash) or simply goes quiet.
2. **The rotated sensor test has not been run on hardware.** It builds and is bundled in
   Studio 2.65, but the rotation and the touch flip are unverified. The flip assumes the
   touch controller matched the panel at rotation 2; if presses land mirrored, invert the
   two lines in `readTouchPoint()`.
3. **`WebConfig`'s `FIRMWARE_CHANGELOG` is stale** — stops at 2.38 while firmware is 2.92.
4. **No end-to-end restore test of every category.** A full backup now restores, but
   backup → wipe → restore → sync → verify icons/themes/devices has not been done as one
   cycle.
5. **`themes.json` in the SD template declares `rgb565Revision: 3`; current is 5.**
   Self-corrects, but the bundled bitmaps are a dithering generation stale.
6. **`_source.png` theme originals are excluded from USB SD prep** (5.6 MB over 460800
   baud). Only WebConfig's re-crop editor reads them.
7. **Firmware reboot wait in the combined install is a fixed 9 seconds**, not a poll.
8. **The QR page opening by itself is mitigated, not root-caused.** 2.74 refuses that page
   while `usbSdTransferActive()`. A ghost touch on the large centred "Open WebConfig" button
   remains the leading theory, given this board's phantom-touch history.
9. The user reported a firmware file named `OpenRemote_2.78.bin` that Studio read as
   **2.57** — Studio reads the version marker inside the binary, so that file may be an
   older build renamed. Still unconfirmed.

## Constraints

- `Platformio:Arduino/OpenRemote/OpenRemote_1.0` is a git repo; `SOFTWARE/WebConfig`,
  `SD Card Structure`, `Sensor_Test` and `OpenRemote Studio` are **not** — they rely on the
  copy-per-version convention.
- Preserve the user's SD card data: devices, activities, themes, icons, Wi-Fi credentials,
  backups.
- Do not erase flash or format the SD card unless explicitly asked.
- The user tests on real hardware. A successful build proves nothing about runtime
  behaviour — say so rather than implying verification that didn't happen.

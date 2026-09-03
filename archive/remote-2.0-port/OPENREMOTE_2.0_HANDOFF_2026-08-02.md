# OpenRemote 2.0 — AI Handoff, 2 August 2026

Handoff for continuing work on **OpenRemote_2.0** in a fresh workspace. Written
immediately before the project folder was moved to a NAS.

OpenRemote_2.0 is a fork of the **official OMOTE-Community firmware** (modular,
multi-file) and is now the intended foundation going forward. It is *not* a
continuation of `OpenRemote_1.0`, which is a single ~14,700-line `.ino`. Nothing
in `OpenRemote_1.0` was modified in this session and it should stay read-only —
it is useful as a *reference* for behaviour and prior investigation only.

The most important section is **[The phantom-touch
investigation](#4-the-phantom-touch-investigation)**.

---

## 0. Read this first if you just moved the folder

- **The `.pio/` directory is ~2 GB and is NOT path-portable.** PlatformIO bakes
  absolute paths into its build metadata. After moving, delete it and let it
  rebuild:
  ```bash
  rm -rf .pio
  ~/.platformio/penv/bin/platformio run -e esp32-s3-Rev5andHigher
  ```
  First rebuild re-downloads toolchains/libs and takes several minutes.
- **The path contains a literal colon** (`Platformio:Arduino`). Always quote
  paths in shell commands. Some NAS filesystems (and SMB/AFP shares) disallow
  `:` in names — if the share rejects it, the folder may silently rename. Check
  before assuming a build failure is a code problem.
- **Git state: everything below is uncommitted.** One commit exists
  (`8cfb0c6 Initial commit`). Twelve modified files and five untracked files.
  **Commit before moving**, or the move is the only copy.

---

## 1. Hardware

ESP32-S3 based remote, **OMOTE Rev 5** hardware. Build env selects this via
`-D OMOTE_HARDWARE_REV=5`.

| Part | Detail |
|---|---|
| MCU | ESP32-S3, 16 MB flash, OPI PSRAM (`qio_opi`) |
| Display | ILI9341 240×320, **8-bit parallel** via LCD_CAM + GDMA |
| Touch | **FT6206** (cipher `0x06`, vendor `0x11`), I²C `0x38` |
| Keypad | TCA8418, I²C `0x34` |
| Accelerometer | LIS3DH, I²C `0x19` (wake-on-motion) |
| Fuel gauge | MAX17048, I²C `0x36` |
| Radios | Wi-Fi + BLE (NimBLE) |

### Pin map (confirmed in `hardware/ESP32/tft_hal_esp32.cpp`)

```
I2C            SDA=20   SCL=19          <-- note: ESP32-S3 native USB D+/D- pins
LCD control    EN=38  BL=9  CS=39  DC=40  WR=41  RD=42
LCD data       D0=48 D1=47 D2=21 D3=14 D4=13 D5=12 D6=11 D7=10
SD             MISO=7 SCK=15 EN=16 MOSI=17 CS=18
ACC_INT=2      "user LED"/mic power = 45
```

**Two facts that matter for the touch bug:**

1. `LCD_EN` (GPIO 38) powers **both the LCD and the FT6206 touch controller**.
   `enterSleep()` cuts it, so touch is power-cycled on every sleep/wake.
2. I²C runs on GPIO 19/20, which are the ESP32-S3's native USB D-/D+ pins.
   Investigated as a possible conflict; **not pursued to a conclusion**. The
   board uses an external CH340 for USB, so the native USB peripheral is
   nominally unused, but the pad configuration was never explicitly verified.
   Still an open thread if you run out of other ideas.

---

## 2. Repo layout, build, and serial

Official OMOTE architecture — a hardware abstraction layer under `hardware/`,
application code under `src/`:

```
hardware/ESP32/*_hal_esp32.cpp        real hardware
hardware/windows_linux/*              LVGL desktop simulator
src/applicationInternal/              GUI base, command/scene handling, HAL presenter
src/guis/                             screens (settings, activities, numpad, ...)
src/devices/  src/devices_pool/       per-device IR/BLE/MQTT drivers
src/scenes/                           scene definitions
```

**Architecture rule to respect:** only `hardwarePresenter.cpp` includes
`hardwareLayer.h`. Application code must go through `hardwarePresenter.h` and
must not touch hardware directly. Diagnostics that need raw hardware access
belong in `hardware/ESP32/`, which is why `phantomTouchDiag_hal_esp32.cpp` lives
there.

### Build environments

| Env | Purpose |
|---|---|
| `esp32-s3-Rev5andHigher` | **The real firmware.** Default target |
| `esp32-s3-soaktest` | Diagnostic: logs raw FT6206 registers, cycles DMA-stress/idle 5 min, never sleeps |
| `esp32-s3-sweeptest` | Steps the FT6206 `ID_G_THGROUP` threshold every 3 min under DMA load |
| `esp32-s3-soaktest-nofilter` | As soaktest but with the rejection filter compiled out (A/B control) |
| `macOS` / `linux_64bit` / `windows_*` | LVGL simulator — **macOS build currently fails, missing SDL2** (pre-existing, unrelated) |

All four ESP32 envs verified building as of this handoff.

```bash
cd "…/OpenRemote_2.0"
~/.platformio/penv/bin/platformio run -e esp32-s3-Rev5andHigher
~/.platformio/penv/bin/platformio run -e esp32-s3-Rev5andHigher --target upload --upload-port /dev/cu.usbserial-1330
```

### Serial capture (this firmware is **115200**, not 460800)

`pio device monitor` does not work in a non-interactive shell (needs a TTY).
`cat`/`stty` fail too — CH340 drivers reset baud on each `open()`. Use pyserial,
opening and reading in one process. This version reconnects, which matters
because deep-sleep wake reboots the device:

```python
import serial, sys, time
port, baud, outpath, duration = sys.argv[1], int(sys.argv[2]), sys.argv[3], float(sys.argv[4])
start = time.time()
with open(outpath, "ab", buffering=0) as f:
    while time.time() - start < duration:
        try:
            ser = serial.Serial(port, baudrate=baud, timeout=0.5)
            while time.time() - start < duration:
                data = ser.read(4096)
                if data: f.write(data)
            ser.close()
        except Exception as e:
            f.write(("PYERR %s\n" % e).encode()); time.sleep(2.0)
```

Run with `~/.platformio/penv/bin/python cap.py /dev/cu.usbserial-1330 115200 out.log 3600`.
If esptool or capture fails with "device reports readiness to read but returned
no data", check `lsof /dev/cu.usbserial-1330` and kill the holder (VS Code likes
to grab it).

---

## 3. What changed this session (UI work)

Goal was to make OpenRemote_2.0 the new foundation: strip its menus back to a
testable Settings page modelled on OpenRemote_1.0's, styled entirely with
OpenRemote_2.0's own widgets.

- **`src/main.cpp`** — only `Settings` and `Activities` are registered as tabs.
  All other GUIs (scene selection, IR receiver, Apple TV, numpad, BLE pairing,
  smart home, Yamaha) are commented out of registration; their device/scene code
  still compiles because `sceneRegistry`'s default key map references those
  commands.
- **`src/guis/gui_settings.cpp`** — kept everything genuinely functional
  (brightness, lift-to-wake, sensitivity, timeout, keyboard brightness, battery,
  memory usage, Debug FPS/Touch) and added **non-functional placeholder** boxes
  for Wi-Fi, Bluetooth, Clock, Buttons, Backup/Restore, About. Content mirrors
  OpenRemote_1.0; every widget is a native OpenRemote_2.0 control.
- **`src/guis/gui_activities.{h,cpp}`** *(new)* — fake Activities tab, 3
  placeholder activities with slide-to-activate sliders. Logs and snaps back.
- **Debug menu** is FPS + Touch only, and both now **persist** to NVS
  (`dbgFPS`/`dbgTouch` in the existing `settings` namespace), restored in
  `main.cpp` after `init_gui()`.
- **`user_led_hal_esp32.cpp`** — the stock OMOTE 1 Hz "user LED" blink on GPIO 45
  is disabled for Rev5+. On this board GPIO 45 is **mic power**, not a spare LED.
  (Investigated as a ghost-touch cause; **not** the cause.)

**Known loose end:** physical keys still map to scene commands (RED/GREEN/
YELLOW/BLUE → TV/FireTV/Chromecast/AppleTV). Pressing them tries to navigate to
GUIs no longer in the tab list. It logs `showSpecificGUI: … not found` and does
not crash, but may show a blank tab.

---

## 4. The phantom-touch investigation

**Full detail is in [`PHANTOM_TOUCH_FINDINGS.md`](PHANTOM_TOUCH_FINDINGS.md).**
Summary here.

### What is solid

| Finding | Evidence |
|---|---|
| **DMA is necessary** | 0 ghosts in **300,000+ polls** with a static screen. Every run, no exceptions |
| **DMA is not sufficient** | 245,486 polls under max DMA load overnight → **1 ghost** |
| Ghosts are real sensor events, not I²C corruption | LovyanGFX's `Touch_FT5x06` already double-reads and compares; ghosts survive it |
| **Weight/area cannot discriminate** | Real fingertips read `weight=16, area=0` — **identical** to every ghost. Settled by user tap test. Do not revisit |
| Ghost position is structured | X clustered at middle of sense axis (mean 106, stdev 5); Y spread across panel (stdev 66) |
| Controller is not resetting | ID registers read back stable throughout |

Highest ghost rate ever observed: **288 in 7,287 polls (3.95%)** in one 5-minute
DMA phase. **Never reproduced since**, despite 500,000+ polls in every
combination tried.

### Theories killed — do not re-litigate

| Theory | How it died |
|---|---|
| Backlight PWM harmonics | Ghosts persist at 100% brightness where PWM stops switching entirely |
| GPIO 45 blink | Blink disabled; ghosts remained |
| **Stock OMOTE is immune** | User sees ghosts on stock OMOTE too. This false premise anchored the *entire* prior investigation |
| I²C corruption / double-read validation | Driver already does it; ghosts survive |
| Weight/area filtering | Real touches identical to ghosts |
| User proximity | 0 ghosts in 7,491 polls under DMA while held; and ghosts occur on a table |
| USB charging current | Ghosts occur plugged **and** unplugged |
| Ghost→UI→DMA feedback loop | Filter compiled out, same conditions: 0 ghosts in 7,465 polls |
| WiFi retry-storm timing | Apparent correlation was an artifact of when serial capture attached |
| Drive strength, lower LCD clock, Arduino_GFX | From prior sessions; Arduino_GFX works but is sluggish/dark and was rejected |

### Leading hypothesis (UNPROVEN)

User reported: **"ghosts tend to come shortly after waking from sleep."**

`enterSleep()` cuts `LCD_EN`, which also powers the FT6206. On wake `init_tft()`
did `delay(5)` before `tft.init()` — that 5 ms was sized for the *LCD driver*
(OMOTE issue #70), but an FT6x06 needs ~**300 ms** from power-up to valid
operation. So it was initialised mid-boot and established its capacitive
baseline while (a) LCD DMA was already running and (b) — because wake is
motion-triggered by picking the remote up — a hand was usually near the glass. A
baseline captured under those conditions is wrong; the sensor is left
hypersensitive until it re-tracks.

Fits: episodic ~1-in-500, independent of table/couch/plugged, needs DMA, and
**explains why stock OMOTE ghosts too** (identical code).

**Status: applied but NOT validated. The user reported ghosts again on the build
containing this fix**, so it is at best incomplete.

### Mitigations currently in the firmware

1. **`TOUCH_POWER_ON_SETTLE_MS = 300`** in `tft_hal_esp32.cpp` — quiet settle
   window before the panel is driven.
2. **Rejection filter** in `lvgl_hal_esp32.cpp` (`PHANTOM_TOUCH_FILTER`, default
   on). Accepts a touch immediately when the screen has been quiet; requires a
   second spatially-coherent sample only within 150 ms of a panel flush. Tunables:
   `TOUCH_DMA_BUSY_MS` (150), `TOUCH_MAX_JUMP_PX` (30). User tested it under
   maximum DMA stress and said it "felt lovely to use" — no responsiveness cost.

### **The single most important process lesson**

**Every soak test disabled sleep.** The bug lives on the sleep/wake path, so
months of testing were structurally incapable of reproducing it. *Any* future
test must exercise sleep/wake.

A `PHANTOM_TOUCH_FIELD` mode was started for exactly this — normal behaviour,
sleep enabled, logs ghosts with `millis()` (which resets on deep-sleep wake, so
it directly reads as time-since-wake). The `#if !defined(PHANTOM_TOUCH_FIELD)`
guards are in place in `phantomTouchDiag_hal_esp32.cpp` and everything compiles,
but **no env defines the flag yet** and the wake-reason logging was never added.
Finishing it is the obvious next step.

### If you pick this up again

1. Finish `PHANTOM_TOUCH_FIELD`, add an env, capture during real use with sleep
   enabled. Correlate ghosts against time-since-wake.
2. If confirmed: force an explicit FT6206 re-calibration once the screen has
   settled and no touch is present, instead of trusting the power-on baseline.
3. Untested: raising `ID_G_THGROUP` (register `0x80`). Writes verified working
   (readback confirmed). The sweep env exists but was inconclusive because the
   ghost rate had already collapsed.
4. Unresolved: GPIO 19/20 vs native USB pads (§1).
5. True fix is likely hardware — a grounded shield between LCD and touch sensor,
   or better bulk decoupling on the shared rail.

---

## 5. Working conventions (from OpenRemote_1.0, keep these)

- **Git**: single branch `main`. One commit per version bump, message
  `<ver>: <summary>`. Commit only when a change is complete and builds.
- **Changelogs explain *why*, including dead ends**, so the next session does not
  repeat them. Rejected versions stay documented, marked `REJECTED / DO NOT USE`.
- **WebConfig** (OpenRemote_1.0 only): never edit in place; copy `<n>.html` to
  `<n+1>.html`. Four version strings to update.
- **Archiving to `BIN/`**: only on explicit instruction.
- **Testing**: the user tests on real hardware and reports back. **Never claim a
  fix works from a successful build alone.**
- **Verify with real evidence** — serial captures, measured values, headless
  renders — not by reasoning over source. This session produced two wrong
  conclusions that only measurement caught.
- **Investigate before implementing**; say so when a requested approach looks
  unlikely to work.

---

## 6. Honest state of play

- The remote is running `esp32-s3-Rev5andHigher` with both mitigations.
- The Settings/Activities UI foundation is done and builds.
- **The phantom touch bug is not solved.** The mechanism is plausible and
  specific, but unproven, and ghosts were still reported afterwards.
- Two conclusions were over-claimed during this session and later corrected in
  writing (DMA as sole root cause; the feedback-loop theory). Treat any
  single-run measurement here with suspicion — the ghost rate varies by four
  orders of magnitude between runs for reasons still unknown.

# OpenRemote — AI Handoff, 2 August 2026

Handoff for continuing work on the OpenRemote custom remote control in a fresh
workspace. Written at firmware **2.46** / WebConfig **2.24**.

The single most important section is **[The phantom-touch
investigation](#the-phantom-touch-investigation)** — it is the long-running
open problem, and it has a strong active lead that is mid-test on hardware.

---

## 1. Hardware

ESP32-S3 based remote, built on **OMOTE Rev 5** hardware.

| Part | Detail |
|---|---|
| MCU | ESP32-S3, 16 MB flash, OPI PSRAM (`qio_opi`) |
| Display | ILI9341 240×320, **8-bit parallel** |
| Touch | FT5x06-compatible capacitive, I²C `0x38` |
| Keypad | TCA8418, I²C `0x34` |
| Accelerometer | LIS3DH, I²C `0x19` (wake-on-motion) |
| Fuel gauge | MAX17048, I²C `0x36` |
| Storage | microSD |
| Radios | Wi-Fi + BLE HID (Bluedroid, Chromecast/Android TV) |

### Pin map (from `OpenRemote_1.0.ino`)

```
I2C            SDA=20   SCL=19
LCD control    EN=38  BL=9(active-low)  CS=39  DC=40  WR=41  RD=42  RST=-1
LCD data       D0=48 D1=47 D2=21 D3=14 D4=13 D5=12 D6=11 D7=10
Button LEDs    46 (SW_BL, active-high)
IR             LED=5(active-low)  RX=4  VCC=6
SD             MISO=7 SCK=15 EN=16 MOSI=17 CS=18
Interrupts     ACC_INT=2   TCA_INT=8
Charge status  1 (active-low)
Mic            POWER=45, shares SD SCK/MOSI/MISO
```

**Note for noise work:** I²C SDA/SCL are GPIO 20/19, directly adjacent to LCD
data line D2 on GPIO 21. The touch controller also shares the LCD power rail
(`PIN_LCD_EN`), and the backlight LEDs sit physically behind the touch sensor.

---

## 2. Repo layout and conventions

Project root (the PlatformIO project, and the only git repo):

```
SOFTWARE/Platformio:Arduino/OpenRemote/OpenRemote_1.0/
├── OpenRemote_1.0.ino     ← entire firmware, ~14,300 lines, single file
├── platformio.ini
├── lv_conf.h
├── partitions.csv
└── pio_src/main.cpp       ← thin shim; real code is the .ino
```

Note the literal colon in `Platformio:Arduino` — quote paths in shell commands.

Surrounding non-git folders:

```
SOFTWARE/FIRMWARE/BIN/                          OpenRemote_<ver>.bin per release
SOFTWARE/FIRMWARE/Other/Release Archives/Versions/   OpenRemote_<ver>_source.zip
SOFTWARE/FIRMWARE/Other/Rejected Releases/      <ver>_BAD_DO_NOT_USE.bin
SOFTWARE/WebConfig/WebConfig <ver>.html         self-contained web UI, one file
SOFTWARE/Platformio:Arduino/Reference Projects/Official OMOTE Firmware References/
                                                ← official OMOTE source, invaluable
```

### Working conventions (established, please keep)

- **Git**: single branch `main`. One commit per version bump, message
  `<ver>: <summary>`. Commit only when a change is complete and builds.
- **Versioning**: bump `OPENREMOTE_VERSION` / `OPENREMOTE_VERSION_TEXT` near
  line 1004 **and** prepend a changelog block at the very top of the `.ino`.
  The changelog is detailed and explains *why*, not just what — keep that.
  Rejected versions stay in the changelog marked `REJECTED / DO NOT USE`.
- **WebConfig**: never edit in place. Copy `WebConfig <n>.html` to `<n+1>.html`,
  then edit. Four version strings to update: `<meta ...version>`, `<title>`,
  `.sidebarVersion`, and `webConfigVersion:` in the JS.
- **Archiving to BIN**: only when the user explicitly says to publish. Archived
  through **2.45**; 2.46 is *not* archived yet.
- **Testing**: the user tests on real hardware and reports back. Do not claim a
  fix works from a successful build alone.

### Build / upload / serial

```bash
cd "…/OpenRemote_1.0"
~/.platformio/penv/bin/platformio run
~/.platformio/penv/bin/platformio run --target upload --upload-port /dev/cu.usbserial-1330
```

`pio device monitor` **does not work** in a non-interactive shell (needs a TTY;
raises `termios.error`). `cat`/`stty` also fail — CH340-style drivers reset baud
to 9600 on each `open()`. Use a pyserial script that opens, configures and reads
in one call. A working reset-and-capture helper:

```python
import serial, sys, time
port, baud, outpath = sys.argv[1], int(sys.argv[2]), sys.argv[3]
duration = float(sys.argv[4]) if len(sys.argv) > 4 else 8.0
ser = serial.Serial(port, baudrate=baud, timeout=0.2)
ser.dtr = False; ser.rts = True; time.sleep(0.1)   # pulse EN, stay out of
ser.rts = False; time.sleep(0.1)                   # download mode
start = time.time()
with open(outpath, "ab", buffering=0) as f:
    while time.time() - start < duration:
        data = ser.read(4096)
        if data: f.write(data)
ser.close()
```

Run with `~/.platformio/penv/bin/python <script> /dev/cu.usbserial-1330 460800 out.log 8`.
Firmware serial is **460800**; the official OMOTE reference firmware is 115200
(garbled output usually means you have the wrong one).

Note: VS Code sometimes auto-starts a serial monitor and holds the port. If
esptool or a capture fails with "device reports readiness to read but returned
no data", check `lsof /dev/cu.usbserial-1330` and kill the holder.

---

## 3. Firmware architecture notes

- **UI**: LVGL 8.3.11 on a persistent `lv_tileview` page strip (`pageUi[]`,
  `PAGE_SLOT_COUNT` slots), so pages are built once and scrolled between rather
  than rebuilt. `renderAllPageSlots()` / `changePage()` / `bindPageUi()`.
- **Settings pages**: `settingsView` enum + `openSettingsView()` /
  `backToSettings()`, each page rendered by a `render*Page()` function.
- **Two menu styles**: `menuStyle` global — 0 = original bordered rows,
  1 = OMOTE-style dark rounded cards. Applies live, no reboot. Currently
  implemented for **Settings home + Display only** (a deliberate sample);
  extending to the remaining Settings pages is an open task.
- **Persistence**: ESP32 NVS via `Preferences` for device settings;
  `/config/runtime.json` on SD for the WebConfig-authored model.
- **Sleep**: display sleep → light sleep → deep sleep (EXT1 wake on LIS3DH
  motion / TCA8418 keypad), plus a BLE-connected idle mode that keeps a
  Chromecast HID link up while the screen is off.

### Debug menu options (Settings → Debug)

All persisted; the display ones need a reboot, the last two apply instantly.

| Option | Values | Effect |
|---|---|---|
| LCD Driver | LovyanGFX / Arduino_GFX | DMA vs synchronous GPIO |
| LCD Clock | 40/27/20/16/10 MHz | LovyanGFX bus write clock |
| Buffering | Single / Double | LVGL draw buffers |
| Drive Strength | Weakest…Strongest | GPIO pad drive on WR + D0–D7 |
| Touch Driver | Adafruit / FT5x06 | Wire-based vs LovyanGFX's own I²C |
| **Backlight PWM** | **640 Hz / 1 k / 5 k / 25 k** | **live, no reboot — see §4** |
| Menu Style | OpenRemote / OMOTE | live, no reboot |

Plus overlays: Split Line, Touch (reticle + trail), CPU/RAM, Accelerometer, FPS.

---

## 4. The phantom-touch investigation

**The problem.** Random phantom touches fire across the screen during normal
use. They are not reproducible on demand — roughly "1 in 500" events — which
makes every test a long soak rather than a quick check.

**The decisive comparison.** The user has the **official OMOTE firmware** on
disk and can flash it to the same physical remote. On identical hardware, with
identical LovyanGFX + 40 MHz + 8-bit parallel DMA, **OMOTE has zero phantom
touches**. So this is not "the hardware is marginal" and not "DMA is inherently
unusable here" — something in this firmware differs.

### Ruled out (do not re-litigate)

| Hypothesis | Result |
|---|---|
| FT5x06 register re-init at runtime | Removed. Helped slightly, not a fix. |
| Switch to Arduino_GFX (no DMA) | **100 % effective** — but sluggish, and colours look dark. Not acceptable to the user as a solution. |
| Lower LCD clock | 16 MHz "not so bad"; partial only. |
| GPIO drive strength | No improvement at **any** level. |
| Replacing LovyanGFX with another DMA driver | Investigated and dropped: LCD_CAM + GDMA is the *only* DMA path on ESP32-S3, so any DMA driver uses the same peripheral. |
| Back-to-back double-read validation (2.43) | No fix. **See why below — this is a key insight.** |
| LovyanGFX's own `Touch_FT5x06` (bypasses Wire) | Available as an option; did not eliminate them. |

### Current leading theory (2.46, mid-test)

Direct diff of the two firmwares' backlight code:

```
OMOTE   hardware/ESP32/tft_hal_esp32.cpp   ledc_timer.freq_hz = 640
        …and ledc_stop() outright at full brightness
Ours    OpenRemote_1.0.ino:~1176           BACKLIGHT_PWM_HZ = 25000
```

**39× higher.** Raised from 5 kHz to ~25–30 kHz back in firmware 1.05 to silence
audible backlight whine — i.e. deliberately pushed above the audible band, which
also pushed its harmonics into a much worse place.

Mechanism: the backlight LEDs sit behind the touch sensor on the shared LCD
rail. A 25 kHz square wave has strong odd harmonics at **75 / 125 / 175 kHz**,
and FT5x06-class mutual-capacitance sensing runs in roughly the **100–200 kHz**
band. The 5th and 7th harmonics land directly in it. At 640 Hz you would need
about the 195th harmonic to reach that band, with negligible amplitude left —
which is why OMOTE is clean on the same silicon.

**Why 2.43's double-read validation didn't help** — important: it catches
*corrupted I²C reads* (two reads disagreeing). These are not corrupted reads.
The FT5x06 genuinely believes it is being touched, so it reports the same
phantom coordinate twice and passes validation cleanly. We were filtering the
wrong failure mode.

**Proposed model — two additive noise sources:**

1. Backlight PWM harmonics in the sense band (present here, absent in OMOTE)
2. 40 MHz LCD_CAM DMA coupling into adjacent I²C lines / the shared rail
   (present in both; note SDA=20, SCL=19 sit right next to LCD D2=21)

OMOTE has only #2 → below threshold. Arduino_GFX leaves only #1 → below
threshold. LovyanGFX here has **both** → over threshold. This is the only model
found so far that fits all four observations simultaneously.

**Supporting evidence the user supplied by accident:** on a rare boot where the
backlight failed to come on, they reported *zero* phantom touches — with
LovyanGFX DMA still running. Backlight not switching ⇒ no phantom touches.

### What 2.46 adds, and the test that is pending

Debug → **Backlight PWM** (640 Hz / 1 kHz / 5 kHz / 25 kHz), applied live via
`ledcChangeFrequency()` so frequencies can be A/B'd without rebooting between
them. Default deliberately left at 25 kHz so the baseline is unchanged.

Test protocol given to the user, display left on LovyanGFX 40 MHz throughout:

1. **25 kHz** — confirm phantom touches still occur (baseline).
2. **640 Hz** — OMOTE's value. Expect them to stop.
3. **5 kHz** — if 640 Hz is clean, find the best compromise.

**Expected tradeoff:** 640 Hz may reintroduce audible backlight whine at partial
brightness — exactly what 1.05 was solving. 1 kHz / 5 kHz exist to find the
sweet spot between silence and clean touch.

**Free confirmation, no flash required:** at exactly **100 % brightness**
`currentLcdOnDuty()` returns max, so `ledcWrite()` gets duty 0, which holds the
pin static and stops switching entirely regardless of the dropdown. If 100 % is
always clean at any frequency, that independently confirms the backlight.

### If the theory is confirmed

Change the default `BACKLIGHT_PWM_HZ`, keep the dropdown for tuning, and
consider adopting OMOTE's `ledc_stop()`-at-full-brightness behaviour. If it is
**not** confirmed, the next unexplored angle is the shared 3V3 rail / bulk
decoupling during DMA bursts, which drive-strength changes would not have
addressed — that negative result is consistent with a rail-transient cause
rather than a pad-EMI cause.

---

## 5. Other open items

- **OMOTE menu style rollout** — implemented for Settings home + Display only.
  Extend to Wi-Fi, Bluetooth, Clock, Buttons, Debug, Backup, About once the
  sample look is confirmed. Helpers already exist: `makeOmoteCard()`,
  `makeOmoteDivider()`, `makeOmoteRow()`; `makeDisplaySlider()` takes optional
  `parent`/`x`/`width` for reuse inside cards.
- **Archive 2.46** to `BIN/` + `Release Archives/Versions/` when the user says
  to publish.
- **`tft.init()` return value is discarded** (`setup()`, LovyanGFX branch), and
  `lcdControllerReady = true` is set unconditionally. The Arduino_GFX branch
  right beside it *does* check. A rare dark-screen-on-boot report may be this;
  worth adding a check + log + retry. Low priority, not yet acted on.
- **Deep-sleep wake latency** — a few seconds. User has said they accept it.

---

## 6. Recent version history

**Firmware** (full detail in the `.ino` changelog):

| Ver | Summary |
|---|---|
| 2.46 | Selectable Backlight PWM frequency (phantom-touch experiment) |
| 2.45 | OMOTE Display page: slider scroll-in-card + knob/divider overlap fixes |
| 2.44 | OMOTE-style Menu Style option (Settings home + Display sample) |
| 2.43 | Selectable touch driver (Adafruit / FT5x06) + double-read validation |
| 2.42 | GPIO drive-strength option (ghost-touch experiment — no effect) |
| 2.41 | Low-risk battery + deep-sleep wake-latency improvements |
| 2.39 | **REJECTED** — hesitant-swipe change, reverted in 2.40 |

**WebConfig:**

| Ver | Summary |
|---|---|
| 2.24 | Loading-screen pill text shortened to "Connecting" |
| 2.23 | Inline-SVG loading screen on the LCD preview (replaces stale first paint) |
| 2.22 | Row position driven by the theme's own split/rows setting again |
| 2.21 | (superseded) bottom-anchored rows — wrong approach, see 2.22 |
| 2.20 | Reclaimed preview screenBody height so configured rows fit |
| 2.18 | Fixed Midnight Penthouse losing its image; preview top-gap fix |

### WebConfig gotchas worth knowing

- The file is ~1.5 MB, single self-contained HTML with base64 theme images.
- CSS has **many** later `!important` blocks that override earlier rules. Two
  fixes were wasted editing dead rules. Search for *all* occurrences of a
  property before changing one — e.g. `.lcdGridCanvas` row sizing is controlled
  by the block commented "LCD grid is 3 across x 10 down", not the earlier
  percentage rule.
- Verify layout changes by rendering headless rather than reading CSS:
  ```bash
  "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome" \
    --headless=new --disable-gpu --virtual-time-budget=3000 \
    --window-size=1400,1600 --screenshot=out.png "file:///abs/path.html"
  ```
  Injecting a small script that dumps `getBoundingClientRect()` values catches
  overlap/clipping that eyeballing the CSS does not.
- Theme "rows" (1–5) maps to a split value via `THEME_ROW_SPLITS`
  `{1:246, 2:195, 3:144, 4:93, 5:42}`. That split doubles as the button grid's
  start position — it is not purely decorative.

---

## 7. Working style the user prefers

- Investigate before implementing; say so when a requested approach looks
  unlikely to work, and propose the better one rather than silently building it.
- Verify with real evidence — serial captures, headless renders, measured
  values — not by reasoning over source alone.
- Explain *why* in changelogs and code comments, including dead ends, so the
  next session does not repeat them.
- Build → upload → let the user test on hardware → iterate.
- Only archive to `BIN/` on explicit instruction.

# OpenRemote Handoff

**Date:** 2026-08-17 (supersedes `OPENREMOTE_AI_HANDOFF_2026-08-04b.md`)
**Reason for handoff:** User asked to stop the boot-speed investigation here. Nothing is mid-edit; everything below is committed.

## Current Versions

| Component | Version | Location |
|---|---|---|
| Firmware | **3.04** | `SOFTWARE/FIRMWARE/BIN/OpenRemote_3.04.bin` |
| WebConfig | 2.38 | `SOFTWARE/WebConfig/WebConfig 2.38.html` |
| Studio (Mac) | 2.65 | `SOFTWARE/OpenRemote Studio/Mac/OpenRemote Studio 2.65.app` |
| Studio (Win) | 2.65 | `SOFTWARE/OpenRemote Studio/Windows/OpenRemote Studio 2.65.exe` |
| Sensor Test | 2026-08-04 (rot 0) | `SOFTWARE/Sensor_Test/BIN/OpenRemote_Sensor_Test_factory_2026-08-04_rot0.bin` |

The previous handoff's table was already current for WebConfig/Studio/Sensor Test — only firmware moved, 2.92 → 3.04.

## NEW RULE this session: git commit on every version bump

The user explicitly asked for this. **Every firmware version bump must be `git commit`ed on `main`** in
`Platformio:Arduino/OpenRemote/OpenRemote_1.0` (the only component that's a git repo), in addition to the
changelog entry and the archived `.bin`. This had been skipped for versions 2.49 through 2.94 (all
accumulated uncommitted); that backlog was committed in one catch-up commit (`df3011b`) before resuming
per-version commits. Saved to the assistant's persistent memory as a standing rule — do not let this lapse
again.

## Boot speed: investigated and improved, methodically

The user reported firmware boots in ~4s vs. official OMOTE-Community stock firmware's near-instant boot,
and asked to speed it up. This was **not** a blind-fix session — every change below was made only after a
real serial log confirmed the specific cost, per this project's hard-learned "no blind fixes" rule (from
the USB transfer saga earlier this month). Boot time went **4038ms → 3162ms (876ms, ~22% faster)** across
five real hardware-measured rounds. The user chose to stop here rather than continue into riskier territory.

### Comparison research (informational, no code changes)
`SOFTWARE/Platformio:Arduino/OpenRemote/OpenRemote_2.0/` is a **separate, unrelated codebase** — a July
fork of the stock OMOTE-Community firmware (modular, multi-file, ~100 files), explicitly marked read-only
in its own handoff (`OPENREMOTE_2.0_HANDOFF_2026-08-02.md`): *"not a continuation of OpenRemote_1.0... stay
read-only, useful as a reference only."* OMOTE is fast because it has **no SD card at all on Rev 5** (an
explicit `// SD card is currently not used, so save some time on startup` in its `main.cpp`) and **no
runtime JSON parse** — every device/activity/scene is compiled into the binary as static C++
registrations. That's the entire speed difference: OMOTE trades runtime configurability (which this
firmware's whole WebConfig/Studio/backup ecosystem depends on) for compile-time fixed content. This is an
architectural trade-off already made, not a bug — confirmed but not chased further.

### Diagnostics added (keep these — they're load-bearing for future investigation)
- `Serial.printf("Setup finished in %lu ms.\n", ...)` at the very end of `setup()` (2.99), matching OMOTE's
  own line at the same point.
- `delay(500)` after `Serial.begin()` cut to `delay(10)` (2.99) — was pure dead time for a serial monitor
  to catch the boot banner.
- `"Boot stages (ms from start): panel_black=... config=... calibrated=... ui_built=... first_frame=..."`
  (3.00, renamed in 3.02) — five cumulative checkpoints across `setup()`.
- `"Config sub-stages (ms): sensors=... sd_mount=... webconfig_check=... irdb_load=... runtime_config=..."`
  (3.01) — splits what used to be one "config" number into TCA8418/LIS3DH probing, SD mount, and the three
  SD-reading calls.
- `"Runtime config timing: read=... parse=... model=..."` (3.01, inside `loadRuntimeConfig()`) — splits the
  SD file read, the `deserializeJson()` parse, and `loadRuntimeModel()`'s own struct-building work.
- `"Runtime model: N devices, M activities (.ir file load: Xms)"` (3.03) — times
  `loadIrDeviceFilesIntoRuntime()` specifically, isolated from the rest of `loadRuntimeModel()`.
- `"Display calibration: gamma X, saturation Y% (Nms)"` (3.04) — `rebuildDisplayColourLut()` now times and
  prints its own cost regardless of which of its two call sites triggered it.

### Fixes shipped, each confirmed on real hardware before moving to the next
1. **3.02 — panel painted black immediately, not after SD/config load.** User reported black→white→black
   flicker before the UI painted. The panel driver bring-up (`agfx->begin()`/`fillScreen(black)`) used to
   run *after* the ~2.5s SD/config block; moved to run immediately after `lcdPowerOn()`/`initBacklightPwm()`,
   since every display setting it needs (`displayDriverChoice` etc.) comes from NVS via `loadSettings()`,
   which already runs at the very top of `setup()`. Confirmed: white flash eliminated. (One residual: user
   reported two brief backlight blinks during power-on, most likely from `lcdPowerOn()`'s plain
   `digitalWrite` handoff followed by `initBacklightPwm()`'s `ledcAttachChannel()` re-taking the same GPIO
   as a PWM channel — **not yet investigated further**, cosmetic, low priority.)
2. **3.03 — stopped rebuilding the display colour LUT twice per boot.** `rebuildDisplayColourLut()` (65536
   entries × `powf()` gamma/saturation math) was called once inside `loadRuntimeModel()` (via
   `applySettingsJson()`, which ends with it unconditionally) and again explicitly in `setup()`, with
   nothing between the two calls able to change `displayGamma`/`displaySaturation`. `loadRuntimeConfig()`'s
   return value is now captured; the `setup()`-level call only fires when it's `false` (no SD card, no
   `runtime.json`, or a parse failure) — the one path where the config-load call never ran. Confirmed:
   "calibrated" stage delta went from **520ms → 0ms**, total boot 4038ms → 3531ms.
   - Also checked and **ruled out** `rebuildPages()` as a second duplicate-work source (also called at boot
     before LVGL exists) — it only writes a small fixed array, no LVGL objects, trivially cheap regardless
     of call count. Not touched.
3. **3.04 — cut the surviving LUT rebuild's own cost, not just its call count.** The one remaining call
   did `powf()` 3× per RGB565 entry (196,608 calls). The saturation mix blends in luminance from all three
   channels, so `pow()`'s input can't be reduced to one lookup per channel level — but it's always a float
   in [0,1], and the output is rounded to 5-6 bits regardless of `pow()`'s precision, so quantising the
   input to 8 bits (256 levels) before lookup discards nothing the final rounding wasn't already discarding
   — the same margin a standard sRGB gamma LUT relies on. Precomputed `pow()` once at that resolution:
   196,608 calls → 256. Confirmed: **556ms → 150ms** for that one call, total boot 3531ms → **3162ms**.

### Investigated, deliberately NOT touched (documented reasoning, not oversights)
- **`loadIrDeviceFilesIntoRuntime()` (173ms measured in 3.03).** Real per-file SD I/O using `String`-based
  `readStringUntil()` line parsing, buried inside what looks like pure struct population. Same inefficient
  pattern already fixed for JSON parsing in 2.91 (`BufferedSdJsonStream`) — a known-good fix exists. **Not
  done** because this parses the user's actual devices; a rewrite done carelessly risks silently breaking a
  device rather than just being slow. This is the one clear remaining lever if speed work resumes.
- **`deserializeJson()` parse cost (290ms).** ArduinoJson's `psramJsonAllocator` does one `ps_malloc()` per
  small allocation, and `loadRuntimeModel()` needs nearly the whole parsed tree (devices, activities,
  macros, themes, settings), so a filtered/streaming parse (the trick used for backup validation in 2.91)
  wouldn't help much here — most of the document actually gets used. Not attempted; no clear safe lever
  found.
- **SD mount handshake (~792ms of the 983ms `sd_mount` figure).** Broke this down fully: ~140ms is
  deliberate power-rail settle delay (`delay(120)` + `delay(20)` in `initSdStorage()`) that should **not**
  be shrunk without physical bench testing across multiple cards/temperatures — reducing it blind risks
  intermittent mount failures, a far worse regression than a slow boot, especially given how much of this
  project's history is SD-reliability bugs already fixed. The remaining ~792ms is the actual `SD.begin()`
  card negotiation (CMD0/CMD8/ACMD41/CSD/CID) and is **inherent to SPI-mode communication with this card**.
  Confirmed native 4-bit SDMMC (which would be genuinely much faster) is **not available on this hardware**
  — checked the wiring directly: only `MISO`/`SCK`/`MOSI`/`CS` are connected (`SPIClass sdSpi(FSPI)`), no
  `DAT1`-`DAT3` lines; those same SPI pins are even time-shared with the I2S microphone
  (`PIN_MIC_BCLK`/`WS`/`DATA` alias `PIN_SD_SCK`/`MOSI`/`MISO`). This is a firm hardware constraint, not a
  software choice — do not re-investigate without new hardware.
- **`createSdFolderIfMissing()` (~51ms folder bootstrap).** Checked: already guards with `SD.exists()`
  before any `SD.mkdir()` call. No wasted writes, nothing to trim.
- **Placeholder/skeleton UI (paint instantly, hydrate from SD after).** User proposed this mid-session,
  architecturally sound (`setupLvgl()`+`setupUiRoot()`+`rebuildPages()` together cost only ~23ms, so
  painting *something* before the SD/config block is entirely feasible), but explicitly withdrawn by the
  user in favour of finishing the LUT/SD investigation instead. If revisited: the real open question is
  product/UX, not engineering — what should the placeholder actually show (branded splash vs. an empty
  Activities page that fills in), and accept that this trades one long wait for two shorter, visible
  transitions (placeholder → real UI). Also touches `currentPage`/`activeDevice` restoration ordering,
  which currently depends on config having already loaded.

## BLE / battery life (separate thread, also resolved this session)

- **2.93/2.94: tried and reverted releasing the BLE link during idle to allow deep sleep.** Saved real
  power but the Chromecast paused playback ~0.5s on every disconnect and deep-sleep wake is an unavoidable
  ~3s cold boot (CPU/RAM fully power down, no way to make that faster in software). **Do not re-attempt** —
  documented at length in the 2.94 changelog entry specifically so this isn't tried again.
- **2.95: shipped instead — request the idle BLE connection profile without disconnecting.** The idle
  profile constants (120-150ms interval, slave latency 3) and `requestBluetoothConnectionProfile()` had
  existed for many versions but nothing ever called it with `idle=true` — the link ran permanently at the
  responsive profile (15-30ms interval, latency 0) for as long as any BLE activity was selected. Now
  requested 2.5s after entering BLE-connected idle, queued through `serviceBluetoothConnectionProfile()`
  and applied from `loop()`, never from inside a sleep/wake transition (per the existing Bluedroid GAP-task
  warning). Cuts radio duty ~20× with the link fully up throughout — no disconnect, no pause, no cold boot.
  **Whether the Chromecast host actually accepts these parameters is not yet confirmed on hardware** —
  watch for `"BLE HID: params interval=... latency=... timeout=..."` from the existing
  `onConnParamsUpdate()` callback after the screen sleeps. If the host refuses, `interval` just stays low
  and the change has no effect — it cannot break the link by being refused.

## Split-line / scroll fixes (2.96-2.98, all confirmed on hardware)

- **2.96:** the 12px bottom content padding counted toward LVGL's scrollable extent, so every remote page
  scrolled a few pixels even when its tiles already fit. Fixed: pad only applied when content genuinely
  overflows. Separately, the Row 5 split-line calibration dropdown was dead for its entire lower half — the
  grid-start formula floored at `max(5, split-42+4)` and 42 *is* the canonical 5-row split, so values 32-43
  all produced the identical result. Floor lowered to 0.
- **2.97:** the overflow-scroll fix from 2.96 let a page scroll 12px *past* flush, clipping row 1.
- **2.98:** scrolling now stops at the row count's own *calibrated* split value (read from
  `debugRowPixels[]`), not wherever the raw arithmetic happens to land flush — so scrolling a 4-row theme
  down to reveal a 5th row lands exactly where a native 5-row theme would put it. User confirmed working.

## USB backup restore (SOLVED — see prior handoff for full detail, unchanged since)

Firmware 2.91/2.92 fixed the long-running "backup copies but won't apply" / "rejected as truncated" fault.
Root cause was ArduinoJson defaulting to `uint16_t` string-length/slot-id types on this 32-bit target
(65535-character ceiling per string; a backup's base64 theme wallpaper is ~204800 chars). Both sizes are
now forced to 4 bytes in `platformio.ini`, guarded by `static_assert`s in the `.ino` so this can't silently
regress. **User confirmed a real restore worked on hardware.** Nothing new to add here this session.

## IRDB / IR code database research (informational, no code changes)

User asked about adding more IR code sources after an air conditioner wasn't found. Findings:
- `probonopd/irdb` (already scraped, one of 4 sources in Studio's `REPOS[]`) is itself the large aggregated
  database — there isn't a bigger *open* equivalent sitting unadded. Global Cache/Harmony's actual database
  is commercial and not redistributable.
- One genuine gap: `irplus-remote/irplus-codes.github.io` (the IRplus Android app's code database) is
  independently-sourced with different long-tail coverage, XML format — would need its own parser. Modest
  incremental gain, not chased further (not requested).
- AC-specific: generic databases store one fixed code per button, but AC remotes transmit full state
  (temp/mode/fan) per code — no static database can represent the combinatorial space well. The real fix
  for AC support would be a protocol encoder (à la `IRremoteESP8266`'s per-brand implementations), not more
  scraped databases — significantly bigger scope, not attempted.

## Workspace tooling

Created `OpenRemote.code-workspace` in the firmware folder — a multi-root VS Code workspace covering both
`OpenRemote_1.0` (firmware) and `OpenRemote Studio` (Mac/Windows builds + shared `app/` source). Not
committed to git (IDE convenience, not firmware). Open via File → Open Workspace from File in VS Code.

## Working Rules (the user's conventions — follow these)

1. **Every change bumps the version by 0.01**, adds a changelog entry at the top of `OpenRemote_1.0.ino`
   (newest first, verbose, root-cause-first), **and now also gets a `git commit`** (new rule this session,
   see above — do not skip this).
2. **Save a compiled `.bin` to `SOFTWARE/FIRMWARE/BIN/OpenRemote_<version>.bin`** after every firmware
   change.
3. **COPY to the new version, never rename/`mv`.** Full running history on disk is intentional.
4. **All handoff files live in `SOFTWARE/AI Handoff/`**, named `OPENREMOTE_AI_HANDOFF_<YYYY-MM-DD>.md`
   (append a letter suffix if a handoff already exists for that date, as this repo's history shows).
5. **Do not blind-fix.** Every fix in this session's boot-speed work was made only after a real serial log
   confirmed the specific cost. This discipline is what got 876ms out safely with zero regressions; abandon
   it and you're back to the four-blind-attempts USB saga from earlier this month.

## Build and Install

```bash
cd "/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/Platformio:Arduino/OpenRemote/OpenRemote_1.0"
~/.platformio/penv/bin/platformio run
~/.platformio/penv/bin/platformio run --target upload --upload-port /dev/cu.usbserial-1330
~/.platformio/penv/bin/platformio device monitor -p /dev/cu.usbserial-1330   # 460800 baud
```

Only `Platformio:Arduino/OpenRemote/OpenRemote_1.0` is a git repo. `SOFTWARE/WebConfig`,
`SD Card Structure`, `Sensor_Test`, `OpenRemote_2.0` (OMOTE reference fork), and `OpenRemote Studio` are
**not** — they rely on the copy-per-version convention.

## Open Items / Not Verified

1. **Two brief backlight blinks on power-on**, reported after 3.02. Not investigated. Likely cause: two
   separate GPIO reconfiguration events on `PIN_LCD_BL` — `lcdPowerOn()`'s plain `digitalWrite` followed by
   `initBacklightPwm()`'s `ledcAttachChannel()` re-taking the same pin as a PWM channel, which can output a
   brief default duty before the very next `ledcWrite()` corrects it. Cosmetic, low priority, user has not
   asked for a fix.
2. **`.ir` file parsing (173ms) is the one clear remaining boot-speed lever** if the user wants to resume
   that investigation — same `String`-based line-reading inefficiency already fixed for JSON in 2.91, but
   touches correctness-critical device data and needs the same careful, confirmed-not-guessed treatment the
   rest of this session used.
3. **BLE idle connection profile (2.95) is not confirmed accepted by the Chromecast host** — watch serial
   for `onConnParamsUpdate()`'s output after the screen sleeps on next real-world use.
4. **No end-to-end restore test of every backup category** since the 2.92 fix — a full backup restores
   (confirmed), but backup → wipe → restore → sync → verify icons/themes/devices as one full cycle has not
   been done.
5. Everything else carried over unverified from the prior handoff (WebConfig's stale `FIRMWARE_CHANGELOG`,
   `themes.json`'s `rgb565Revision: 3` vs current 5, `_source.png` excluded from SD prep, fixed 9s firmware
   reboot wait, the QR-page-opens-itself mitigation, a possibly-mislabelled `OpenRemote_2.78.bin`) — none of
   these were touched this session.

## Constraints

- Preserve the user's SD card data: devices, activities, themes, icons, Wi-Fi credentials, backups.
- Do not erase flash or format the SD card unless explicitly asked.
- The user tests on real hardware. A successful build proves nothing about runtime behaviour — say so
  rather than implying verification that didn't happen. This entire session's boot-speed work is a model
  of the alternative: real serial logs before and after every change, no exceptions.

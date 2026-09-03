# OpenRemote Firmware Handoff

**Date:** 2026-08-01  
**Reason for handoff:** Firmware 2.37 still has the same severe display and touch failures after several unsuccessful attempts to adopt a persistent native LVGL page strip. The user has explicitly asked the current agent to stop making firmware changes.

## Immediate First Action

1. Do not modify or upload the active 2.37 source until it has been copied or archived.
2. Treat 2.37 as a failed experimental build, not a release.
3. Reproduce the failure while capturing the complete serial log from physical reset through first touch and one light-sleep/wake cycle.
4. Investigate display object visibility separately from touch/I2C corruption. The evidence strongly suggests two faults rather than one.
5. Use firmware 2.09 and 2.10 as known-good comparison points. Do not discard their working display, touch, BLE, and Wi-Fi behavior.

## User Priorities

The user wants:

- Stability above all else.
- Smooth, direct finger-following horizontal page movement like official OMOTE.
- Native LVGL page handling if it can be made reliable.
- Double-buffered display output, clear text, good color, and responsive vertical scrolling.
- No phantom touches, dead touch after wake, random page activations, or crashes.
- BLE Chromecast control and Wi-Fi to coexist reliably.
- All current OpenRemote features and visual design retained.

The user does **not** want another blind patch cycle. Diagnose the current failure before changing architecture again.

## Current Failure: Firmware 2.37

The remote currently has firmware 2.37 installed.

Observed result after boot or physical reset:

- Only a narrow band of the selected wallpaper appears at the top of the LCD.
- A narrow wallpaper band can also appear at the bottom.
- The center of the 240x320 panel is blank, dark gray, or black.
- Page titles, cards, buttons, activity sliders, and other page controls are absent.
- The Debug touch overlay can draw red/orange dots across the **entire** LCD, including the blank center.
- This proves LVGL can flush pixels across the full panel and the touch overlay is above the broken page content.
- Crazy phantom touches still occur.
- After light sleep, real touch/debug dots may stop working until the physical reset button is pressed.
- Firmware 2.37 produced exactly the same visible result as the immediately preceding attempts.

The last user report was:

> No change. Crazy touches and still the same background bit up the top and bottom.

## Strongest Current Diagnosis

### Display

The LCD transport is probably not the primary reason that page controls are missing:

- Full-screen Debug touch objects render in the blank region.
- The current DMA staging buffers report internal, DMA-capable memory.
- The wallpaper's top and bottom fragments are plausible remnants of objects or bands that remain visible while the page object hierarchy is clipped, hidden, positioned off-screen, or incompletely invalidated.
- Logs from earlier builds reported that page content objects had children, yet those children did not appear.

The highest-value display investigation is therefore the native LVGL object tree:

- Active tile selection and tile coordinates.
- Tile and root hidden/visible flags.
- Parent clipping and scroll coordinates.
- Object coordinates after layout.
- Invalidated regions for the selected tile.
- Whether the active page is rendered into the tile LVGL considers visible.
- Whether `lv_tileview` is being used with assumptions that only hold for its internal tile layout.

Do not infer that the narrow bands mean the physical LCD only receives those rows. The global touch overlay contradicts that.

### Touch

Touch is likely a separate shared-I2C lifecycle/concurrency problem:

- Touch controller is FT5x06-compatible at address `0x38`.
- It shares SDA/SCL with the TCA8418 keypad and LIS3DH accelerometer.
- Earlier serial logs repeatedly showed:
  - `i2c_master_transmit_receive failed: [259] ESP_ERR_INVALID_STATE`
  - `Wire requestFrom Error 259`
- Phantom contacts became especially common after wake or during busy rendering.
- Firmware 2.09 used a passive touch address probe and had no phantom-touch problem according to the user.
- Firmware 2.37 restored that passive startup probe, but phantom touches remained. Startup register writes therefore were not the only cause.

Audit every `Wire` transaction and every task/core that can access the bus. A single mutex around all touch, keypad, accelerometer, and fuel-gauge I2C operations is a strong next diagnostic. Also verify bus teardown/reinitialization across light sleep and deep sleep.

## Active Workspace

Active PlatformIO workspace:

`/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/Platformio:Arduino/OpenRemote/OpenRemote_1.0`

Main firmware source:

`/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/Platformio:Arduino/OpenRemote/OpenRemote_1.0/OpenRemote_1.0.ino`

PlatformIO configuration:

`/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/Platformio:Arduino/OpenRemote/OpenRemote_1.0/platformio.ini`

Current source constants:

```cpp
static constexpr float OPENREMOTE_VERSION = 2.37f;
static constexpr char OPENREMOTE_VERSION_TEXT[] = "2.37";
```

Current compiled image:

`/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/Platformio:Arduino/OpenRemote/OpenRemote_1.0/.pio/build/openremote_rev5/firmware.bin`

This 2.37 image has not been promoted into the normal release folder and should not be promoted.

## Build and Upload

```bash
cd "/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/Platformio:Arduino/OpenRemote/OpenRemote_1.0"
~/.platformio/penv/bin/platformio run
~/.platformio/penv/bin/platformio run --target upload --upload-port /dev/cu.usbserial-1330
```

Typical USB port:

`/dev/cu.usbserial-1330`

Configured upload and monitor speed is 460800 baud. Some previous troubleshooting sessions also obtained readable startup output at 115200, so check both if the configured rate does not produce usable logs.

Latest verified 2.37 build size:

- RAM: approximately 107,396 / 327,680 bytes (32.8%).
- Flash: approximately 2,278,199 / 3,342,336 bytes (68.2%).

## Hardware and Display Configuration

- Board: OMOTE Rev 5 / ESP32-S3.
- Flash: 16 MB.
- PSRAM: 8 MB OPI.
- LCD: 240x320, RGB565.
- Display driver: LovyanGFX, 8-bit parallel bus with DMA.
- LVGL: 8.3.11.
- LovyanGFX: 1.2.26.
- Framework: Arduino via PlatformIO.

LCD pins:

| Signal | GPIO |
|---|---:|
| LCD_EN | 38, active low |
| LCD_BL | 9, active low |
| LCD_CS | 39 |
| LCD_DC | 40 |
| LCD_WR | 41 |
| LCD_RD | 42 |
| D0 | 48 |
| D1 | 47 |
| D2 | 21 |
| D3 | 14 |
| D4 | 13 |
| D5 | 12 |
| D6 | 11 |
| D7 | 10 |

Shared I2C:

| Signal | GPIO |
|---|---:|
| SDA | 20 |
| SCL | 19 |

Hardware schematic:

`/Users/phillipcarlson/Documents/Arduino/OpenRemote/HARDWARE/OMOTE-Hardware-main/PCB/OMOTE Rev5.pdf`

## Current Display Architecture

Firmware 2.37 uses:

- Two static internal DMA-capable LVGL draw buffers.
- Each buffer is `240 x 32` RGB565 pixels.
- `lvFlush()` waits for prior DMA, applies the color LUT if enabled, and sends the requested band with `pushPixelsDMA()`.
- A persistent `lv_tileview` page strip with four reusable page slots.
- Each page slot contains a 240x320 tile/root, wallpaper, content container, top bar, and page dots.
- Debug diagnostics are children of the global `uiRoot`, above the page strip.

That final point is important: the global Debug overlay can work while every page slot remains visually broken.

Relevant source areas, approximate current line numbers:

| Area | Function | Approx. line |
|---|---|---:|
| Touch sampling | `readTouchSample` | 2766 |
| LVGL touch callback | `lvTouchRead` | 9580 |
| Display buffers | `ensureDisplayFlushBuffers` | 9247 |
| LVGL flush | `lvFlush` | 9327 |
| Theme application | `applyRuntimeTheme` | 9455 |
| Content layout | `configureContent` | 9877 |
| Page rendering | `renderCurrentPage` | 12259 |
| Page-slot rendering | `renderAllPageSlots` | 12347 |
| LVGL setup | `setupLvgl` | 13092 |
| Root/page-strip setup | `setupUiRoot` | 13136 |

Re-run `rg -n` before relying on those line numbers because they will move after edits.

## What Firmware 2.37 Tried

Firmware 2.37 added these changes over the preceding experiment:

- Forced an LVGL layout pass before selecting the active tile.
- Selected the active tile after layout.
- Invalidated and synchronously refreshed the full screen after rendering.
- Restored the passive 2.09-style touch-controller address probe instead of configuring touch registers at startup.

None of those changes altered the visible failure or phantom-touch behavior.

## Relevant Version History

### 2.07

- Switched to LovyanGFX 40 MHz parallel DMA.
- Added double draw buffers.
- Improved text clarity and overall responsiveness.

### 2.09 - best known baseline

- User reported good graphics, good touch, good BLE, and good Wi-Fi.
- User specifically reported zero phantom touches.
- Used smooth finger-following navigation based on captured previous/current/next page images rather than the current persistent native LVGL tile strip.
- Used a passive touch probe and direct touch path.
- Source archive and binary are available.

### 2.10

- Fixed crashes around temporary device-page transitions.
- User later described its page movement as smooth and stable.
- Source archive and binary are available.

### 2.15

- Known working BLE/Wi-Fi behavior and Chromecast connection handling.
- Source archive and binary are available.

### 2.23

- Display and normal UI were usable and bright.
- Touch was substantially improved at one point, though the user later still observed phantom contacts.
- Temporarily designated current after 2.24 was rejected.

### 2.24 - rejected

- After software reboot, touch could be completely dead until physical reset.
- Quarantined as `OpenRemote_2.24_BAD_DO_NOT_USE.bin`.

### 2.27 through 2.30 - rejected

- BLE discoverability, BLE connection, and Wi-Fi coexistence regressions.
- 2.30 was rebuilt from 2.09 with newer features but remained unstable.

### 2.31

- Incorporated 2.10-style transition fixes.
- Still crashed during horizontal page movement.

### 2.32

- Introduced the persistent native LVGL tile strip inspired by official OMOTE.
- Intended to eliminate screenshot-based page transitions.
- This architecture is the direction the user asked to retain, but severe display and touch failures followed.

### 2.33

- Replaced an unsupported full-frame DMA transfer with two 32-row DMA buffers.

### 2.34

- Serialized the shared calibrated staging buffer with `waitDMA()`.
- Serial logs around this period showed repeated I2C invalid-state errors.

### 2.35

- Made the active tile visible before rendering.
- Restored a direct one-frame touch path.

### 2.36

- Moved color-LUT staging to internal DMA-capable memory.
- Set the FT5x06 controller to active mode.
- Still displayed only wallpaper bands and had touch problems.

### 2.37 - current failed experiment

- Added layout, active-tile selection, full invalidation, synchronous refresh, and passive touch initialization.
- User reports no improvement at all.

## Archived Releases

Normal firmware binaries:

`/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/FIRMWARE/BIN`

Source archives:

`/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/FIRMWARE/Other/Release Archives/Versions`

Rejected releases:

`/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/FIRMWARE/Other/Rejected Releases`

Especially useful comparison archives:

- `OpenRemote_2.09_source.zip`
- `OpenRemote_2.10_source.zip`
- `OpenRemote_2.15_source.zip`
- `OpenRemote_2.31_source.zip`

Useful binaries include:

- `OpenRemote_2.09.bin`
- `OpenRemote_2.10.bin`
- `OpenRemote_2.15.bin`
- `OpenRemote_2.23.bin`
- `OpenRemote_2.31.bin`
- `OpenRemote_2.32.bin`
- `OpenRemote_2.33.bin`

## Other Project Locations

WebConfig:

`/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/WebConfig`

SD card template:

`/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/SD Card Structure`

OpenRemote Studio:

`/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/OpenRemote Studio`

Do not modify WebConfig or OpenRemote Studio while solving this firmware-only failure unless a concrete protocol mismatch is proven.

## Suggested Diagnostic Sequence

1. Archive the active 2.37 workspace.
2. Capture 2.37 startup logs from physical reset.
3. Print coordinates, hidden flags, parent pointers, child counts, scroll positions, and invalidation bounds for every page-strip layer after the final layout pass.
4. Temporarily render unmistakable solid colors into the tile, page root, wallpaper, content, and global root to identify which object layers are actually visible.
5. Build a minimal one-file LVGL tileview test using the exact current `lvFlush()` and buffer setup. Do not include SD, BLE, Wi-Fi, themes, or runtime JSON.
6. If the minimal tileview works, add the current page hierarchy one layer at a time.
7. Separately instrument all I2C callers with one shared mutex and owner/timestamp logging.
8. Run untouched wake-cycle tests before re-enabling normal touch events.
9. Compare each changed touch and display block directly against 2.09 and official OMOTE.
10. Only after both minimal tests pass should the full runtime be rebuilt around the native strip.

## Important Constraints

- This folder is not currently managed as a Git repository. Make explicit source archives before risky changes.
- Preserve the user's SD card configuration, devices, activities, themes, icons, Wi-Fi credentials, and backups.
- Do not erase flash or format the SD card unless the user explicitly requests it.
- Do not call 2.37 a release or place it in the normal BIN folder.
- Do not return to the old screenshot/pixel-push navigation without explaining the tradeoff and obtaining user approval. The user explicitly asked to keep pursuing the native LVGL strip.
- Do not assume a successful build proves the display or touch lifecycle works on hardware.

## Final State at Handoff

- Remote connected over USB during the last tests, usually at `/dev/cu.usbserial-1330`.
- Firmware 2.37 installed.
- No serial-monitor process intentionally left running.
- Build and upload completed successfully, but runtime remains unusable.
- No further changes should be attributed to the previous agent after this handoff.

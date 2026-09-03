# OpenRemote Codex Handoff

> **Current handoff (2026-08-01):** Stop using the stale status below as the
> starting point. Firmware 2.37 is installed but unusable: only narrow wallpaper
> bands render at the top/bottom, page controls are absent, and phantom touches
> persist. Read the complete current diagnostic handoff at:
> `/Users/phillipcarlson/Documents/Arduino/OpenRemote/OPENREMOTE_AI_HANDOFF_2026-08-01.md`

## 2026-07-31: Firmware 2.24 rejected; 2.23 restored as current

- Firmware 2.24 restored a 25 ms LCD/touch rail pulse. Although the display
  initially became bright again, every software reboot then left touch fully
  unavailable until the physical reset button was pressed.
- The user restored 2.23 and confirmed that its screen remained bright while
  touch worked correctly. The earlier darkness did not recur after a physical
  reset, so 2.23 is the current release.
- The active source is restored to version 2.23 and its no-pulse startup path.
  The 2.24 binary is quarantined under `FIRMWARE/Other/Rejected Releases` and
  removed from the normal firmware and SD recovery folders.
- Firmware 2.24 SHA-256:
  `b05a241cc4e9c24291864806309c29ebfb051b598f0b6275454c5324107232c7`

## 2026-07-31: Firmware 2.23 stable touch-frame validation

- Diagnosed firmware 2.22 on the connected Rev 5 remote at 460800 baud. With
  the screen untouched, it emitted false contacts clustered near X 125-140,
  repeated `ESP_ERR_INVALID_STATE` I2C failures and incomplete FT5x06 frames.
- Restored the stable 2.09 LCD/touch rail startup sequence and removed writes
  to undocumented touch-controller registers. Recovery now performs a bounded
  I2C bus restart only when the controller probe fails.
- Adopted the official OMOTE/LovyanGFX validation strategy: non-zero contacts
  are accepted only when two complete consecutive five-byte frames match,
  with at most five attempts. Unstable or incomplete frames never reach LVGL.
- Live verification after installation: 15 seconds untouched produced zero
  false contacts and zero I2C errors. A slow top-to-bottom swipe was captured
  as one continuous 827 ms contact (`151,64` to `102,246`).
- PlatformIO build and USB installation passed. The packaged release is
  `SOFTWARE/FIRMWARE/BIN/OpenRemote_2.23.bin`, with an identical SD recovery
  copy at `SOFTWARE/SD Card Structure/firmware/OpenRemote_2.23.bin`.
- Firmware 2.23 SHA-256:
  `72df90a9ea9657b7ffe9bdc25e596dd1211fa8e3a428f06c11e11084033a6f53`

## 2026-07-31: Firmware 2.22 touch recovery and Debug controls

- Removed the exact byte-for-byte frame match and second nearby LVGL sample
  that could reject genuine moving-finger data indefinitely. The 2.09 direct
  path is restored with strict touch-count, event and coordinate validation.
- Touch discovery now retries during startup, after display wake and following
  repeated I2C failures. A single missed controller response no longer leaves
  touch disabled until the physical reset button is pressed.
- Touch Debug shows a 44 px red scope reticle around the finger, red live X/Y,
  an orange trail fading over three seconds and orange released X/Y retained
  for five seconds.
- Row 1-5 calibration uses dropdowns containing only each saved value +/-10 px.
  Microphone is a dropdown with `I2S Mic` and `Test Only` sources.
- PlatformIO build and USB installation passed. The installed release is
  `SOFTWARE/FIRMWARE/BIN/OpenRemote_2.22.bin`; startup touch input was active
  immediately after PlatformIO's software reset.
- Firmware 2.22 SHA-256:
  `ed2fad76121171576adf7a8acf02948cd2bf435775fdb72d20c06432b8014284`

## 2026-07-31: Firmware 2.21 persistent Debug tools and live I2S microphone

- Added **Settings > Debug** with persistent Split Line, Touch, CPU/RAM,
  Accelerometer and FPS switches. Diagnostics use separate fixed screen slots,
  the existing 10 px font and distinct colours so their labels do not overlap.
- Row 1 through Row 5 calibration values are editable from 0 to 319 px and are
  saved in ESP32 Preferences as well as the runtime settings document.
- **Microphone Test** is persistent and defaults off. Off selects a real I2S
  microphone; on retains the embedded `Hello this is a test` audio stream.
- Real microphone mode unmounts and powers off SD, holds CS high, powers the
  microphone from GPIO45, captures 16 kHz/32-bit mono I2S on GPIO15/17/7,
  downsamples to 8 kHz, encodes IMA ADPCM and restores SD on every exit path.
- PlatformIO compilation passed. The release is
  `SOFTWARE/FIRMWARE/BIN/OpenRemote_2.21.bin`. No USB serial device was visible,
  so firmware 2.21 was packaged but not installed on the remote.
- Firmware 2.21 SHA-256:
  `5d54b7e51391a106c80ba42c6e13d2445e6b4bf23b539e2d398e851b5787a916`

## 2026-07-31: Firmware 2.20 and WebConfig 2.15 row mapping

- The temporary green LCD row-position diagnostic now reports the same live
  coordinate as firmware 2.19 minus 4 px, matching the value requested for
  final WebConfig row tuning.
- Replaced the free-position theme Split line slider with a five-step **Rows**
  selector. Rows 1 through 5 save exact split positions 246, 195, 144, 93 and
  42 px respectively.
- Existing themes remain compatible: the editor selects the nearest row for an
  older split position, while saving writes one of the five exact pixel values
  used by the firmware runtime.
- PlatformIO build, USB installation and WebConfig JavaScript validation
  passed. Releases are `SOFTWARE/FIRMWARE/BIN/OpenRemote_2.20.bin` and
  `SOFTWARE/WebConfig/WebConfig 2.15.html`.

## 2026-07-31: Firmware 2.19 split-position diagnostic

- Added a temporary small green Y-coordinate readout at the lower-left of
  every LCD page. On Activities, activity and device pages it measures the
  actual first slider/tile row rather than echoing the stored theme split.
- The number is read from the row's absolute LVGL coordinates and updates live
  while vertical scrolling moves that row, giving a direct value for tuning
  WebConfig's future Split line slider limits.
- The overlay is outside the scrollable content, has no clickable area and is
  reacquired whenever a page is rebuilt so it cannot retain a deleted object.
- PlatformIO build and USB installation passed. The installed and packaged
  release is `SOFTWARE/FIRMWARE/BIN/OpenRemote_2.19.bin`; the SD recovery copy
  is `SOFTWARE/SD Card Structure/firmware/OpenRemote_2.19.bin`.

## 2026-07-31: Firmware 2.18 and WebConfig 2.14

- Firmware now power-cycles the shared LCD/touch rail on cold reset, puts the
  FT5x06-compatible controller into polling mode and accepts only stable,
  validated touch frames. A clean USB boot produced no phantom press/release
  events before normal display sleep.
- Editing a Default theme no longer loses its pending asset upload merely
  because the previous files at that SD path are valid. Mode changes such as
  Combined to Image now rebuild and transfer the RGB565, preview and source.
- Installing a synchronized runtime model invalidates the decoded wallpaper
  held in PSRAM, so a replaced theme is reloaded even when its SD path is
  unchanged.
- PlatformIO and WebConfig JavaScript validation passed. Firmware 2.18 was
  installed over USB. WebConfig was intentionally not installed. Release files
  are `SOFTWARE/FIRMWARE/BIN/OpenRemote_2.18.bin` and
  `SOFTWARE/WebConfig/WebConfig 2.14.html`.

## 2026-07-31: Firmware 2.17 and WebConfig 2.13

- Restored firmware 2.09's direct LVGL touchscreen press/release path. The
  touch quarantine and synthetic release handling added after 2.09 have been
  removed because they could retain coordinates and present as phantom taps.
- Page-strip state is still cleaned immediately before display sleep, but the
  touch driver is no longer reset or quarantined during cold boot or movement
  wake.
- Combined Default themes now render their source image and saved gradient into
  one flattened wallpaper before PNG preview and dithered RGB565 generation.
  Image-only and gradient-only themes keep their existing paths.
- Default-theme RGB565 revision 5 forces older image-only combined assets to be
  replaced on synchronisation or **Reinstall Default themes**.
- PlatformIO and WebConfig JavaScript validation passed. Firmware 2.17 was
  installed over USB. Release files are
  `SOFTWARE/FIRMWARE/BIN/OpenRemote_2.17.bin` and
  `SOFTWARE/WebConfig/WebConfig 2.13.html`.

## 2026-07-31: Firmware 2.16 and WebConfig 2.12

- Cold boot now quarantines the touch controller for two seconds and requires
  ten consecutive released samples before LVGL receives input. Ordinary wake
  keeps the shorter quarantine so raise-to-wake remains responsive.
- WebConfig starts in a real **Loading** state. Remote configuration, status and
  the active LCD-theme preview must finish before the pill becomes
  **Synchronised**, so placeholder IP, battery and firmware details are never
  presented as ready data.
- Loading and Synchronise use a clockwise pill-perimeter progress stroke with
  live KB/MB transferred and total counters. Runtime and theme transfers report
  real XHR byte progress rather than a decorative timer.
- Default gradient themes are rebuilt from their saved colour recipe before PNG
  preview and dithered RGB565 generation. RGB565 revision 4 forces stale black
  or image-only Default assets to be replaced on the next synchronisation or
  **Reinstall Default themes** operation.
- PlatformIO, WebConfig JavaScript and a 390x844 headless-browser render passed.
  Release files are `SOFTWARE/FIRMWARE/BIN/OpenRemote_2.16.bin` and
  `SOFTWARE/WebConfig/WebConfig 2.12.html`.

## 2026-07-31: Firmware 2.15 and WebConfig 2.11

- Firmware now checks the ESP32-S3's real Wi-Fi mode whenever station mode is
  restarted. A stale `networkStackActive` flag after activity sleep/BLE use can
  no longer prevent the radio from being initialised.
- WebConfig QR sessions temporarily suspend BLE, cold-recover Wi-Fi with checked
  retries, and restore the active activity's BLE connection after the user
  leaves the QR page. Setup AP startup also retries from a clean radio state.
- LCD and WebConfig network scans cold-recover a failed/off station interface
  before retrying, avoiding the permanent "No networks found" state.
- WebConfig 2.11 hides Delete for built-in themes and edits them with their real
  embedded/SD source image instead of treating them as blank custom themes.
  Built-in edits survive configuration reloads and changed previews use a fresh
  cache URL after upload.
- PlatformIO and JavaScript checks passed. Release files are
  `SOFTWARE/FIRMWARE/BIN/OpenRemote_2.15.bin` and
  `SOFTWARE/WebConfig/WebConfig 2.11.html`.

## 2026-07-31: Firmware 2.14 and WebConfig 2.10

- QR startup now makes one 10-second attempt to use the selected saved Wi-Fi,
  then remains committed to retrying the open setup AP until it is running.
  The generic Settings reconnect path can no longer restart station mode while
  QR fallback is pending.
- LCD and WebConfig Wi-Fi discovery no longer start a saved-network connection
  immediately before scanning. Discovery uses three asynchronous active scans,
  includes hidden SSIDs and keeps the setup AP alive when WebConfig requested it.
- Cold boot now uses the same touch quarantine as wake: LVGL sees no press until
  three clean release samples arrive, preventing stale-looking touches after an
  upgrade or reset.
- WebConfig 2.10 fixes Default themes syncing an empty runtime wallpaper path.
  Default and Custom themes now both use their SD RGB565 paths.
- Reinstall Default themes now succeeds only after the firmware verifies exact
  RGB565 size, visible changing pixel data and a valid PNG signature.
- WebConfig sync preserves the LCD's normal Wi-Fi off state instead of silently
  enabling Wi-Fi again.
- PlatformIO and JavaScript checks passed. Release files are
  `SOFTWARE/FIRMWARE/BIN/OpenRemote_2.14.bin` and
  `SOFTWARE/WebConfig/WebConfig 2.10.html`.

## 2026-07-31: Firmware 2.13 and WebConfig 2.09

- Firmware 2.13 restores firmware 2.10's fixed 150 ms page settle. The live
  three-page strip still follows the finger, while the variable 12-60 ms snap
  introduced in 2.12 has been removed.
- Setup-AP startup, shutdown and every Wi-Fi mode transition now wait for the
  core 0 HTTP worker to stop before changing the interface. This closes the
  stop/relisten and teardown races that could leave an active phone request on
  a destroyed interface, freeze touch or reset the remote.
- The WebConfig worker stack is 24 KB, giving phone sessions and large theme or
  runtime transfers more headroom without moving HTTP work back onto the LVGL
  core.
- WebConfig 2.09 adds **Reinstall Default themes**. It regenerates and uploads
  all seven dithered 240x320 RGB565 wallpapers and PNG previews sequentially,
  with a real progress bar and transferred/total MB counter.
- Theme selection again presents separate Default themes and Custom themes
  sections. The Themes page retains its separate Default and Custom libraries.
- PlatformIO and WebConfig JavaScript validation passed. Release files are
  `SOFTWARE/FIRMWARE/BIN/OpenRemote_2.13.bin` and
  `SOFTWARE/WebConfig/WebConfig 2.09.html`. No USB serial device was present for
  a hardware upload during this build.

## 2026-07-31: Firmware 2.12 and WebConfig 2.08

- Firmware 2.12 removes the long final pause from finger-tracked page changes,
  keeps decoded SD-card icons cached between neighboring pages, and adds a
  right-edge-originating Back gesture throughout nested Settings pages.
- Display and Button sliders use the official OMOTE grey-track, blue-fill and
  white-knob treatment. Vertical movement over a slider now scrolls the page
  without changing its value.
- QR setup starts the open recovery AP whenever normal Wi-Fi is disabled or no
  saved profile is available. Connected station mode is reused without a
  disruptive second `WiFi.begin()`, and nearby-network discovery is now an
  asynchronous retried scan rather than a blocking UI operation.
- WebConfig 2.08 embeds usable previews for all seven factory themes and checks
  `/api/themes/status` before sync. Missing PNG and RGB565 assets are installed
  instead of leaving black preview/LCD backgrounds.
- Smooth Blue was rebuilt as a continuous dithered blend without the former
  radial contour rings. Theme RGB565 revision 3 forces existing SD cards to
  replace the older Smooth Blue bitmap; all seven assets were regenerated and
  visually checked.
- PlatformIO and WebConfig JavaScript validation passed. Release files are
  `SOFTWARE/FIRMWARE/BIN/OpenRemote_2.12.bin` and
  `SOFTWARE/WebConfig/WebConfig 2.08.html`.

## 2026-07-30: Firmware 2.11 and WebConfig 2.07

- Firmware 2.11 adds a post-wake touch quarantine. LVGL receives no pointer
  input until the touch controller has settled and reported three consecutive
  released samples, preventing random taps, stuck swipes and wake-time crashes.
- Any active page-strip animation and pointer state are cancelled before sleep.
- The selected activity is persisted only when it changes and is restored after
  an unexpected reset.
- WebConfig 2.07 includes seven built-in dithered Default themes: Smooth Blue,
  Obsidian Silk, Aurora Glass, Champagne Noir, Grand Cinema, Alpine Ember and
  Midnight Penthouse.
- LCD-ready theme assets are in `SOFTWARE/SD Card Structure/themes/Default`;
  the reproducible builder is `tools/build_default_themes.py`.
- PlatformIO build passed. The remote was not reachable at 192.168.1.168 or
  192.168.1.122 and no USB serial port was present, so device upload remains.

This file transfers the working context from the original Codex chat into a new
Codex project rooted at:

`/Users/phillipcarlson/Documents/Arduino/OpenRemote`

The original referenced ChatGPT conversation is:

`chatgpt-conversation://6a4f663b-1a80-83ec-b787-c49908562040`

## Project Layout

- Firmware workspace: `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/Platformio:Arduino/OpenRemote`
- Active Arduino sketch: `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/Platformio:Arduino/OpenRemote/OpenRemote_1.0/OpenRemote_1.0.ino`
- Firmware release binaries: `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/FIRMWARE/BIN`
- Firmware release archives: `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/FIRMWARE/Other/Release Archives`
- Web Config destination: `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/WebConfig`
- Active Web Config: `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/WebConfig/WebConfig 2.13.html`
- OpenRemote Studio Mac app: `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/OpenRemote Studio/Mac/OpenRemote Studio 2.44.app`
- OpenRemote Studio Windows package: `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/OpenRemote Studio/Windows`
- OpenRemote Studio source package: `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/OpenRemote Studio/Other/Legacy Releases and Source/OpenRemoteStudio_v2_44_cross_platform_source`
- Active Studio IRDB: `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/OpenRemote Studio/Other/IRDB/OpenRemote.irdb v2026.07.17.2319/OpenRemote.irdb`
- Clean SD-card template: `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/SD Card Structure`
- Icons: `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/SD Card Structure/icons`
- Hardware files: `/Users/phillipcarlson/Documents/Arduino/OpenRemote/HARDWARE/OMOTE-Hardware-main`

Keep editing the same `OpenRemote_1.0` Arduino sketch folder so Arduino IDE
retains the selected ESP32 board, port, and board settings. Archive completed
versions separately rather than creating a new live sketch folder each time.

## WebConfig 2.06 RGB565 wallpaper quality

- WebConfig now converts the 24-bit theme composite to the LCD's raw RGB565
  wallpaper with serpentine Floyd-Steinberg error diffusion. This removes the
  large contour bands previously produced by direct per-pixel rounding.
- Theme records carry `rgb565Revision: 2`. Existing themes without that marker
  have only their 150 KB runtime wallpaper regenerated on the next sync; their
  preview and original source files are not needlessly uploaded again.
- Firmware remains unchanged: LVGL and the stored wallpaper are RGB565. The
  RGB666 panel-transfer option cannot restore shades already lost at export.

## OpenRemote Studio standalone macOS and Windows release

- OpenRemote Studio 2.44 is the current cross-platform release. Remote Config
  includes a guided **Set Up a New Remote** workflow for a blank ESP32-S3.
- After a successful board detection, **Install Sensor Test** installs an
  embedded Rev 5 diagnostic factory image without changing the SD card. The
  user can then restore whichever OpenRemote firmware was selected in the
  firmware chooser.
- Studio 2.44 fixes the embedded esptool 4.8.1 argument spelling used by its
  Python API. The former CLI-style `--flash-mode`, `--flash-freq`, and
  `--flash-size` options caused `dio` to be parsed as an address and prevented
  firmware installation; the API-compatible underscore forms are now used.
- Studio detects the USB bootloader, flashes a checksum-protected complete
  16 MB factory image at address `0x0`, and verifies the image before writing.
  It does not silently select bundled firmware or WebConfig versions.
- **Choose Firmware** and **Choose WebConfig** are always visible, including
  before USB detection. Studio validates explicitly selected files and reads
  their embedded versions; steps 2 and 3 show only those detected versions.
- OMOTE Rev 5 uses its DTR/RTS automatic programming circuit and has only a
  Reset button. Studio never instructs the user to hold a nonexistent BOOT
  button; a failed automatic connection asks for one Reset tap and a retry.
- The FAT32 / MS-DOS (FAT), Master Boot Record SD card stays inserted in the
  remote. Firmware 2.03 prepares it over USB and Studio installs the chosen
  WebConfig as `/www/index.html`, application recovery binary, version metadata,
  required folders, and the current Default icon library. It never formats it.
- Preparing an existing SD card preserves `/config/runtime.json`, devices,
  activities, macros, custom icons/themes, backups, and IRDB files while
  updating the factory-managed WebConfig, firmware, and Default assets.
- The Python 3.9-compatible ESP32 flashing engine 4.8.1 and pyserial 3.5 are
  bundled. This fixes the Click/esptool syntax failure when the unpackaged Mac
  source app uses Apple's Python 3.9. The macOS standalone app is universal2
  and embeds Python 3.14; the Windows 10/11 x64 portable app embeds Python
  3.12. Neither needs Python, Arduino IDE, PlatformIO, developer tools, or
  internet access on the destination computer.
- The macOS app was launch-tested as a packaged universal executable. Its
  `/app/ping`, v2.44 UI, nested Python framework, ad-hoc signature, OpenRemote
  image, Sensor Test image, and setup manifest passed verification.
- The real `/dev/cu.usbserial-1330` board passed Studio 2.44 detection as an
  ESP32-S3. Studio installed and verified the 456,496-byte Sensor Test factory
  image at address `0x0`, then restored the selected OpenRemote 2.03 image. The
  live serial boot confirmed firmware 2.03, the 29,664 MB SD card, six devices,
  and three activities.
- The Windows package is a native PE32+ x86-64 GUI launcher. Its embedded
  runtime, flashing modules, factory image checksum, application resources,
  and Windows serial backend were structurally verified. It still needs a
  final launch/flash test on a real Windows 10/11 computer.
- This locally built Mac app is not Apple-notarized, so a destination Mac may
  require right-clicking the app and choosing Open on first launch.
- Distribution ZIP:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/OpenRemote Studio/Other/Legacy Releases and Source/OpenRemote_Studio_v2_44_macOS_Windows_Standalone.zip`
- Distribution SHA-256:
  `1e9486ae266c05130f10368dc9b929ee1b481ccf0fa1192ada9ef3366924295f`.
- Individual platform archives:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/OpenRemote Studio/Other/Legacy Releases and Source/OpenRemote_Studio_macOS_v2_44_Standalone.zip`
  and
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/OpenRemote Studio/Other/Legacy Releases and Source/OpenRemote_Studio_Windows_v2_44_Portable.zip`.
- macOS archive SHA-256:
  `d5fa62691448fd77fdfa350a698d13c13136432d1c33a0d37e594d79172e75db`.
- Windows archive SHA-256:
  `74e0bcbc6254d9e9cd660de917f77cf522f186d9cc2ec80a5f8b1275289ea794`.
- Source archive:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/OpenRemote Studio/Other/Legacy Releases and Source/OpenRemote_Studio_v2_44_Source.zip`
  with SHA-256
  `e9999e1896ec949e1c7061d402f54a6e3b5dec8b35ca8598e45465cdee2a31d4`.

## PlatformIO migration

### Firmware 2.10 temporary device-page stability

- Firmware 2.10 fixes intermittent crashes when opening or leaving a device
  page from either Activities Home or an active activity page.
- Device-picker clicks now queue the selected device and return immediately.
  The main loop opens the device only after LVGL has finished dispatching the
  picker event, so `renderCurrentPage()` can no longer delete the modal object
  tree that LVGL is still traversing.
- A device page records the exact source page index before it opens. Closing it
  removes the temporary device page first, then returns to that saved Home or
  activity page and redraws a consistent page-dot model. It no longer guesses
  the destination from the existence of `activeActivity`.
- Temporary device pages are excluded from the 2.09 previous/current/next
  snapshot builder. They retain a lightweight release swipe to exit, avoiding
  the former activity + large device + device render cycle. Normal activity
  pages retain the finger-tracked direct-manipulation strip.
- Firmware 2.10 builds successfully at 2,255,887 bytes flash (67.5%) and
  165,244 bytes RAM (50.4%).
- Release binary:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/FIRMWARE/BIN/OpenRemote_2.10.bin`
  with SHA-256
  `055a66f4e00337185a58502513c481f0751175182b5e5f7b73947df7079abd93`.
- Compact source archive:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/FIRMWARE/Other/Release Archives/Versions/OpenRemote_2.10_source.zip`
  with SHA-256
  `3d97cf12995991add0afdb318bd4dea145292c7ed707017c6bb65e6c9b73f04a`.
- No USB port or recent Wi-Fi endpoint was reachable during this release, so
  installation and repeated physical device-page entry/exit testing remain
  pending on the real remote.

### Firmware 2.09 finger-tracked page navigation

- Firmware 2.09 replaces release-triggered horizontal page changes with a
  direct-manipulation strip that follows the user's finger throughout the
  swipe. Reversing direction before release moves the page back with the
  finger; releasing short of the threshold snaps back, while a committed drag
  settles onto the adjacent page.
- Previous, current, and next full-screen previews are cached in reusable
  PSRAM-backed LVGL snapshots. This keeps the existing single live page and
  dynamic runtime architecture while providing OMOTE-style adjacent pages
  without consuming scarce internal heap or keeping three full interactive
  page trees alive.
- Once a horizontal drag is claimed, the original tile/button press is reset
  so changing pages cannot accidentally transmit a command. Vertical menu and
  device-page scrolling continues to use the responsive LVGL path introduced
  in 2.07/2.08.
- Firmware 2.09 builds successfully at 2,255,775 bytes flash (67.5%) and
  165,244 bytes RAM (50.4%). The three 240x320 RGB565 page previews use about
  460 KB of the available 8 MB PSRAM and retain their buffers for reuse.
- Release binary:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/FIRMWARE/BIN/OpenRemote_2.09.bin`
  with SHA-256
  `7657a9e9f460b36661fac249804c088db334df4d04e09aeeb7cb349e3259ceb2`.
- Compact source archive:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/FIRMWARE/Other/Release Archives/Versions/OpenRemote_2.09_source.zip`
  with SHA-256
  `337b83d571f151ee2d4b145d609b78b5ea474bb013def91ac0a14d5c8ce0aa24`.
- No USB serial port or recent Wi-Fi endpoint was reachable during the 2.09
  release, so installation and the final physical finger-tracking test remain
  pending on the real remote.

- PlatformIO Core 6.1.19 is installed at
  `/Users/phillipcarlson/.platformio/penv/bin/pio`.
- The active sketch folder is now also a working PlatformIO project. Open
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/Platformio:Arduino/OpenRemote/OpenRemote_1.0`
  in Visual Studio Code; `OpenRemote_1.0.ino` remains authoritative.
- PlatformIO uses PIOArduino ESP32 platform 55.03.311, Arduino core 3.3.11,
  ESP-IDF libraries 5.5.5, 16 MB flash, `qio_opi` OPI PSRAM, and the current
  dual-OTA 8 MB partition map.
- Firmware 2.10 is the active PlatformIO release. It builds successfully at
  2,255,887 bytes flash (67.5%) and 165,244 bytes RAM (50.4%).
- `openremote_rev5` is the only release environment. The unsafe experimental
  SDK power environment was removed; the pinned standard framework already
  supplies FreeRTOS tickless scheduling and Bluetooth controller modem sleep.
- The build output is
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/FIRMWARE/BIN/Build Artifacts/OpenRemote_1.0/.pio/build/openremote_rev5/firmware.bin`.
- Firmware 2.03 was restored successfully at 460800 baud to
  `/dev/cu.usbserial-1330` after the Studio 2.44 Sensor Test installation test.
- The live startup log confirmed 8 MB PSRAM allocation, TCA8418 keypad, mounted
  29,664 MB SD card, six devices, three activities, touch at `0x38`, LIS3DH,
  LCD transfer, Wi-Fi/NTP startup and display sleep. PlatformIO serial monitor
  speed is 460800 baud.

## Versioning

- Firmware save point: v1.00
- Latest firmware continuation: v2.08 forgotten Wi-Fi setup AP fix
- Firmware versions now increment by 0.01: v1.01, v1.02, and so on.
- Firmware v1.00 snapshot:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/FIRMWARE/Other/Release Archives/Versions/OpenRemote_v1.00_savepoint.zip`
- Snapshot SHA-256:
  `5055b4348b34be33db46b74a9a43b0a8c1443fc4d9d8cdc38d336684a0751261`
- Latest Web Config: v2.05 direct Clock and Wi-Fi management

### Firmware 2.08

- Forgetting a Wi-Fi network now removes it as the preferred automatic station
  target instead of leaving its SSID selected with an empty password.
- Automatic connection and Internet-time paths now require an actual saved
  Wi-Fi profile; an SSID retained temporarily for password entry is ignored.
- Opening the QR page with no saved profile immediately starts the open
  OpenRemote setup AP. Forgetting the active network from WebConfig disables
  auto-reconnect and changes the QR transport back to the setup AP.
- Unsaved password-entry targets are no longer persisted through the legacy
  SSID preference, preventing a forgotten network from returning after reboot.
- Firmware binary:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/FIRMWARE/BIN/OpenRemote_2.08.bin`
- Firmware SHA-256:
  `e7772c83b612b0897629e5893068963959833087131427c009d8d4ceda8ce644`.
- PlatformIO source archive:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/FIRMWARE/Other/Release Archives/Versions/OpenRemote_2.08_source.zip`
- Source archive SHA-256:
  `5c3fa3e18796bf82d7acce8c84ee9d91179a8a181c9cb2e3c95a81c0a2262d15`.

### Firmware 2.07

- The LCD driver now follows the official OMOTE Rev 5 implementation: a
  LovyanGFX 40 MHz eight-bit parallel bus with DMA-backed LVGL flushes and two
  partial draw buffers. This removes the previous synchronous Arduino_GFX
  transfer bottleneck without changing the Rev 5 pin map.
- The interface now uses LVGL's anti-aliased Montserrat fonts instead of the
  thresholded one-bit font copies, producing substantially cleaner text on the
  physical ILI9341 display.
- Awake LVGL touch/render servicing runs at the beginning of each loop, before
  radio, storage and command work. LVGL's standard scroll threshold and
  momentum values replace the previous unusually low custom values.
- RGB565/RGB666 selection, colour calibration, inversion, display sleep, deep
  sleep, activities, IR, BLE and WebConfig behavior remain available.
- Two clean PlatformIO builds passed. No USB serial port was present for a
  physical upload or on-device visual check during this release.
- Firmware binary:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/FIRMWARE/BIN/OpenRemote_2.07.bin`
- Firmware SHA-256:
  `a97e0014e24c8f4a64fc2fd036bc28e36ef5db13fc17c20d66d5e8a76d0af59a`.
- PlatformIO source archive:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/FIRMWARE/Other/Release Archives/Versions/OpenRemote_2.07_source.zip`
- Source archive SHA-256:
  `4dc8a8e86fb208aae256b37bead2fff64a98fb7179e22246bb9273d35d2255d0`.

### Firmware 2.06

- The five-minute light-sleep timer is no longer interpreted as movement when
  the transition into deep sleep is briefly deferred. The LCD remains dark and
  retries at a low-power five-second cadence; only genuine movement or a keypad
  event calls the display wake path.
- Deep-sleep LIS3DH setup now establishes a stationary high-pass reference
  before routing its active-low interrupt. This prevents gravity from holding
  the wake line active and blocking deep sleep while the remote is resting.
- Live Rev 5 serial verification confirmed the original timer wake and busy
  accelerometer condition. The corrected build entered real deep sleep at
  318 seconds with both wake lines idle and without relighting the LCD.
- Firmware binary:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/FIRMWARE/BIN/OpenRemote_2.06.bin`
- Firmware SHA-256:
  `b7796850fe82284f27239d2f9c5c03dff26b213e73cf687df363a68abdceff85`.
- PlatformIO source archive:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/FIRMWARE/Other/Release Archives/Versions/OpenRemote_2.06_source.zip`
- Source archive SHA-256:
  `4b8f9ee73776e883fc830553ba20263099fd033128a6d08ad3ae891a55dc9b42`.

### Firmware 2.05 / WebConfig 2.05

- WebConfig Settings now mirrors the remote LCD Clock menu. Clock visibility,
  Internet/manual mode, city/UTC offset and manual date/time are read from and
  written directly to the remote without requiring a full runtime sync.
- Internet time cannot be enabled until the remote has an active Wi-Fi
  connection. Manual date/time controls appear only while Internet time is off.
- WebConfig can scan nearby Wi-Fi networks, join open or password-protected
  networks, reconnect with one of the remote's eight saved passwords, or forget
  a saved password and request a replacement. There is deliberately no Wi-Fi
  off switch in WebConfig.
- Wi-Fi scans and joins preserve the `OpenRemote-CBA4` setup AP for as long as
  the LCD remains on the QR page, so changing networks does not intentionally
  close the setup connection.
- The captive splash links directly to WebConfig and explicitly tells the user
  to open `http://192.168.4.1` in Safari, Chrome or Edge if Apple's captive
  window cannot launch the normal browser.
- Firmware binary:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/FIRMWARE/BIN/OpenRemote_2.05.bin`
- Firmware SHA-256:
  `bcd692dfc59b29f524841ac21c3fe969c697496c7674c2b95d6d904f9d840cec`.
- PlatformIO source archive:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/FIRMWARE/Other/Release Archives/Versions/OpenRemote_2.05_source.zip`
- Source archive SHA-256:
  `188cbca163607322eb2c5b291561463e0897b89c074f2ac8d055604eea521aa2`.
- WebConfig SHA-256:
  `8a614b590f3ca7a3facff7b1569fe1bd91d12bdd68fd9e7f531ade51b6e0b485`.

### Latest packaged release

- Firmware 2.03 adds the USB `PREPARESD` command and approved chunked writes for
  WebConfig, recovery firmware, version metadata and Default icons. It creates
  the required SD folders on the card inside the remote and rejects traversal,
  unsupported paths, malformed HTML, and invalid ESP application images.
- Studio uploads in one serial session with 1,024-byte acknowledged windows and
  visible aggregate progress. Existing runtime/user data is preserved.
- Firmware 2.03 was compiled successfully with PlatformIO and restored to the
  connected Rev 5 remote after the Studio 2.44 factory-installer test.
- Firmware binary:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/FIRMWARE/BIN/OpenRemote_2.03.bin`
- Firmware SHA-256:
  `9962a05d78081bc4567a4368d20cfbef0fb91c6bd458c846f654cc9a2e96ae37`.
- PlatformIO source archive:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/FIRMWARE/Other/Release Archives/Versions/OpenRemote_2.03_source.zip`
- Source archive SHA-256:
  `4252e104082a4b82528abf6be2c761ec2441fdaa043560fffb662ed7b1b9c99a`.

- Firmware 2.02 selects Estimated until full or Estimated until empty from the
  charger state. Connecting or removing the charger begins a new Last Hour
  measurement window without deleting the independent 24-hour history.
- Once the new mode has one hour of data, the estimate is calculated from the
  displayed two-decimal Last Hour change. For example, 92.60% / 0.16% per hour
  is 578.75 hours and is rendered as `24 days 3 hours`.
- Rev 5 only exposes the active-low TP4056 `CRG_STAT` signal, not a separate
  VBUS-present signal. Firmware retains the connected state after charge
  completion and identifies removal from the fuel-gauge discharge direction.
- Firmware 2.02 was compiled successfully with PlatformIO. It was not uploaded
  during packaging because the remote did not expose a USB serial port.
- Firmware binary:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/FIRMWARE/BIN/OpenRemote_2.02.bin`
- Firmware SHA-256:
  `7f30925e830e2c98247d4445c159b9d9b6834a7d7d9a5fb6dbaf3d83352b4d74`
- PlatformIO source archive:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/FIRMWARE/Other/Release Archives/Versions/OpenRemote_2.02_source.zip`
- Source archive SHA-256:
  `27bd6c40dc62ee8932a16664bd0a8330bae259547c5941608cbbd57438404f79`
- The full ready-to-copy SD package remains the v2.00/WebConfig 2.04 package;
  replace its firmware binary with v2.02 when preparing another card.

- Firmware 2.00 matches the official OMOTE Rev 5 deep-sleep electrical state:
  LCD/touch and SD rails are off, LCD/SD/I2C buses are parked to prevent
  back-feeding, and all gate plus wake pins are held while GPIO power is down.
- IR-only activities retain light sleep followed by the user's 5-30 minute
  deep-sleep setting. Activities requiring Chromecast keep BLE HID alive and
  use controller modem sleep plus a stable 80 MHz idle CPU; they deliberately
  do not enter deep sleep because that would disconnect Chromecast.
- PlatformIO serial monitor is timestamped and automatically decodes ESP32
  crash reports. The release target uses PIOArduino 55.03.311 / Arduino 3.3.11.
- Firmware 2.00 was installed over USB on `/dev/cu.usbserial-1330`; its initial
  unattended trace completed hardware/runtime load, NTP shutdown, display
  sleep and LIS3DH motion-wake setup without a reset.
- The full five-minute unattended test then logged a clean timer wake from
  light sleep and entered deep sleep with EXT1 mask `0x104`, accelerometer high
  and keypad high. No panic, watchdog reset or immediate wake occurred.
- Firmware binary:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/FIRMWARE/BIN/OpenRemote_2.00.bin`
- Firmware SHA-256:
  `0bcefb6ac8ff8b1d6479bd16effb7211358bbe7140e6caf2324afcae74ab7f72`
- PlatformIO source archive:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/FIRMWARE/Other/Release Archives/Versions/OpenRemote_2.00_source.zip`
- Source archive SHA-256:
  `d780777e279fb8c0f0ec73b5e62b958825ecb3ade1b08ae475bc2720c9ab568c`
- Ready-to-copy SD archive:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/Archive/Legacy SD Card Packages/OpenRemote_SD_CARD_v2.00_WebConfig_v2.04_PLATFORMIO_DEEP_SLEEP.zip`
- SD archive SHA-256:
  `f771453c0f8805700e9848447cd559f8901d025bc92261bb5989a3c5d00109d3`

- Firmware 1.98 changes the physical Voice Search microphone overlay to
  OpenRemote blue and prepares its entire LVGL object tree once during boot.
- Revealing the overlay is deferred 180 ms beyond the physical key edge, so no
  heap allocation or full object construction occurs inside Chromecast's
  initial MIC_OPEN handshake window. The ATVV/HID protocol path remains exactly
  the proven v1.96 sequence used by Google Home and YouTube search.
- Firmware 1.98 compiled and was installed successfully over USB at 460800 baud
  on `/dev/cu.usbserial-1330`; every flashed section passed hash verification.
- Firmware 1.98 compiled size: 2,256,982 bytes (67%); globals 163,144 bytes (49%).
- Firmware 1.98 SHA-256:
  `0e1960390af406746716bc5cc39164aade6f5bf4c29b87c541a092329438c971`
- Current release archive:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/Archive/Legacy SD Card Packages/OpenRemote_SD_CARD_v1.98_WebConfig_v2.04_BLUE_VOICE_IN_APP_FIX.zip`
- Release archive SHA-256:
  `60912cc0c28c3064f9bf6ec1053b3d707df4731d7df0fc707e6255f7a1928953`

- WebConfig 2.04 removes every reference to a deleted device from activity
  startup steps, conditional delays, saved macros, activity/device LCD tiles,
  physical-button assignments and the open activity draft. It also prunes old
  orphan references when loading the remote and immediately before sync.
- Firmware 1.97 shows a large pulsing microphone overlay only while Voice
  Search is held from a physical key. The top-layer overlay leaves the active
  page untouched and disappears immediately on release; LCD tiles do not show it.
- Firmware 1.97 compiled on 2026-07-24 with the ESP32-S3 N16R8 profile:
  2,256,850 bytes (67%); globals 163,136 bytes (49%).
- Firmware 1.97 was not installed during packaging because no ESP32 USB serial
  port was present and the remote did not answer at its recent Wi-Fi addresses.
- Firmware 1.97 SHA-256:
  `e2ccfbce38feec6ed1ed0de0dfd673b2522c976d6f6e6336405ff5db06e2f4a2`
- Current release archive:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/Archive/Legacy SD Card Packages/OpenRemote_SD_CARD_v1.97_WebConfig_v2.04_DEVICE_CLEANUP_VOICE_OVERLAY.zip`
- Release archive SHA-256:
  `84a3c23da8fb9b789ed1fdcae9c77627bb097e76ae4217d19ac05b474ee9ec4e`

- Firmware 1.96 converts the supplied `Test audio.aifc` recording to 8 kHz mono
  IMA-ADPCM and streams the audible 3.14-second "Hello this is a test" phrase
  through 628 Chromecast-negotiated 20-byte ATVV audio frames.
- It sends the required `AUDIO_SYNC` predictor/index state before the first
  frame, then re-synchronizes to zero-state silence when the phrase completes.
  Physical and LCD Hold-to-Talk behavior remains release-controlled.
- The generated source asset is
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/Platformio:Arduino/OpenRemote/OpenRemote_1.0/atvv_test_audio.h`.
  This validates BLE audio transport only; the reserved SD/I2S pin-switching
  path is still not enabled until a microphone is installed.
- Firmware 1.96 compiled on 2026-07-24 with the ESP32-S3 N16R8 profile:
  2,255,898 bytes (67%); globals 163,128 bytes (49%).
- Firmware 1.96 was not installed during packaging because no ESP32 USB serial
  port was present and the remote did not answer at its recent Wi-Fi address.
- Firmware 1.96 SHA-256:
  `76213dc36478f3b5c448b34ff55a4ebd51527f23ce785da83a7838a03200c684`
- Current release archive:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/Archive/Legacy SD Card Packages/OpenRemote_SD_CARD_v1.96_WebConfig_v2.03_AUDIBLE_ATVV_TEST.zip`
- Release archive SHA-256:
  `f287475ff5c827c717bc66aa7698b9f8b58c02c63bdb2590bbce1ba61ffc8563`

- Firmware 1.95 fixes the physical-key-only ATVV timestamp underflow. A keypad
  press can start audio after the loop timestamp was captured, so the previous
  unsigned elapsed-time check interpreted a sub-millisecond negative delta as
  billions of milliseconds and sent timeout reason `0x08` immediately.
- ATVV service now refreshes its timestamp after Bluetooth work and uses signed
  elapsed-time comparisons. A held physical key or LCD tile remains live until
  release, with a separate two-minute ceiling only for a genuinely lost edge.
- ATVV audio notifications run in a serialized core 0 worker while core 1 owns
  LVGL and TCA8418 scanning. The physical key-up path also polls the keypad FIFO
  throughout a hold, independent of page binding and interrupt-line state.
- Firmware 1.95 compiled on 2026-07-24 with the ESP32-S3 N16R8 profile:
  2,242,794 bytes (67%); globals 163,136 bytes (49%).
- Firmware 1.95 was installed successfully over USB at 460800 baud on
  `/dev/cu.usbserial-1330`; every flashed section passed hash verification.
- Live tests on TCA8418 raw keys 41 and 21 each logged press, immediate release,
  and `AUDIO_STOP` reason `0x02`; no timeout reason `0x08` occurred.
- Firmware 1.95 SHA-256:
  `78eb29415493031198687641ec013c4d3dfa8d8f15c0d46b8335596f6dd2a782`
- Current release archive:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/Archive/Legacy SD Card Packages/OpenRemote_SD_CARD_v1.95_WebConfig_v2.03_PHYSICAL_VOICE_HOLD_FIX.zip`
- Release archive SHA-256:
  `85c5c54a1d7dfe3651366d30a6cd10de4e90d62339e973b2453d885f3b9050e4`

- Firmware 1.92 replaces the incomplete one-byte voice control messages with
  full ATVV 1.0 payloads. It responds to `GET_CAPS`, selects Hold-to-Talk when
  Chromecast advertises support, starts a numbered 8 kHz ADPCM stream on press
  and stops it with release reason `0x02` on button-up. No Assistant HID click
  is sent in negotiated HTT mode.
- Until the reserved I2S microphone is installed, firmware 1.92 supplies valid
  20-byte silent ADPCM frames. This keeps the ATVV audio session structurally
  complete and allows Chromecast to close its microphone UI cleanly.
- Firmware 1.92 compiled on 2026-07-23 with the ESP32-S3 N16R8 profile:
  2,241,694 bytes (67%); globals 163,128 bytes (49%).
- Firmware 1.92 was installed successfully over USB at 460800 baud on
  `/dev/cu.usbserial-1330`; all flashed sections passed hash verification.
- Firmware 1.92 SHA-256:
  `364d1f5b8d1b2c6e989d3f21c98dec61324d2f79c3f4ee691f936f9394f8dec1`
- Current release archive:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/Archive/Legacy SD Card Packages/OpenRemote_SD_CARD_v1.92_WebConfig_v2.03_COMPLETE_ATVV_HTT.zip`
- Release archive SHA-256:
  `581e5c53057c07bbe940b666c0922ace8c558e440fa6d895172244022e27f5ea`

- Firmware 1.91 fixes the live Chromecast sequence where `MIC_OPEN` arrived
  after the firmware's earlier Voice Search state had been cleared. Every valid
  late open request now receives `AUDIO_START`; if button-up already occurred,
  a deferred `AUDIO_STOP` follows after a short settling interval.
- Firmware 1.91 compiled on 2026-07-23 with the ESP32-S3 N16R8 profile:
  2,240,026 bytes (67%); globals 163,080 bytes (49%).
- Firmware 1.91 was installed successfully over USB at 460800 baud on
  `/dev/cu.usbserial-1330`; all flashed sections passed hash verification.
- Firmware 1.91 SHA-256:
  `af0d8d34317fda2828e7494328f60f260e13c3466c5857978e3720b14b872192`
- Current release archive:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/Archive/Legacy SD Card Packages/OpenRemote_SD_CARD_v1.91_WebConfig_v2.03_LATE_MIC_OPEN_FIX.zip`
- Release archive SHA-256:
  `ad43ea04fcd9becc82679707332f26ae91b84dc35a2d3546e5e50dff5e23a3fa`

- Firmware 1.90 sends a complete Assistant HID click immediately when Voice
  Search is pressed, allowing Chromecast to return `MIC_OPEN` while the user's
  physical or LCD button remains held. The firmware acknowledges with
  `AUDIO_START` and defers `AUDIO_STOP` until release and microphone open are
  correctly ordered, preventing Android TV's green microphone dot from being
  left latched by an early stop.
- Firmware 1.90 compiled on 2026-07-23 with the ESP32-S3 N16R8 profile:
  2,239,894 bytes (67%); globals 163,080 bytes (49%).
- Firmware 1.90 was not installed during packaging because no ESP32 USB serial
  port was present and `192.168.1.168` did not answer ping or HTTP.
- Firmware 1.90 SHA-256:
  `820a0b06f6ad63f489d5be988f01a6478f397197761d0341dbdaef4c4e9f8813`
- Current release archive:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/Archive/Legacy SD Card Packages/OpenRemote_SD_CARD_v1.90_WebConfig_v2.03_ORDERED_VOICE_HANDSHAKE.zip`
- Release archive SHA-256:
  `ac8a915b7686a9fd4d48f20c2ebfd04c39716c693aab9cf160fe8f308c40c66f`

- Firmware 1.89 makes remembered smart-power state exclusively activity-owned.
  Activity startup may skip an already-on device and records successful ON
  transitions; Activity All Off records OFF transitions. Manual LCD,
  physical-button, WebConfig-test and standalone macro power commands transmit
  normally without changing remembered state.
- Firmware 1.89 compiled on 2026-07-23 with the ESP32-S3 N16R8 profile:
  2,239,086 bytes (66%); globals 163,064 bytes (49%).
- Firmware 1.89 was not installed during packaging because no ESP32 USB serial
  port was present and `192.168.1.168` did not answer ping or HTTP.
- Firmware 1.89 SHA-256:
  `0599dcd2f56dd7506151f526b78a331cd95dd7f64598517ff02bdcb7eb6fa36f`
- Current release archive:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/Archive/Legacy SD Card Packages/OpenRemote_SD_CARD_v1.89_WebConfig_v2.03_ACTIVITY_POWER_MEMORY.zip`
- Release archive SHA-256:
  `166b558300f6066156076d8eab8aa3d1d48ef08813c49e765e72693d8e70460d`

- Firmware 1.88 makes Voice Search stateful for physical assignments and LCD
  tiles. Button-down sends exactly one ATVV `START_SEARCH` and holds Assistant
  HID usage `0x0221`; button-up sends HID release followed by ATVV
  `AUDIO_STOP`, allowing Chromecast to switch from listening to processing.
- Voice Search bypasses the physical repeat engine regardless of the global
  repeat setting. Existing IR and other command-repeat behavior is unchanged.
- Firmware 1.88 compiled on 2026-07-22 with the ESP32-S3 N16R8 profile:
  2,239,126 bytes (66%); globals 163,064 bytes (49%).
- Firmware 1.88 SHA-256:
  `f387524ad12ed97645d100072578b470573532fce23b2d9c4a269627220a519e`
- Current release archive:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/Archive/Legacy SD Card Packages/OpenRemote_SD_CARD_v1.88_WebConfig_v2.03_HELD_VOICE_SEARCH.zip`
- Release archive SHA-256:
  `d5037062fd9ccb5ca91c1cafb82219f8e578b91298c4421b47f0d2833bb46a5e`

- Firmware 1.87 moves all synchronous `WebServer` request handling to a
  low-priority FreeRTOS task pinned to ESP32-S3 core 0. Core 1 remains the sole
  owner of LVGL, touch, keypad, activity rendering and sleep, so slow phone
  transfers can no longer freeze the LCD user interface.
- The HTTP worker exclusively owns listener begin/stop/rebind and browser
  cancellation. Runtime sync writes cooperatively to an SD temporary file and
  queues the model reload on core 1 only after the response completes.
- The worker blocks indefinitely when WebConfig is not listening, avoiding a
  periodic wake or power penalty outside setup sessions.
- Firmware 1.87 compiled on 2026-07-22 with the ESP32-S3 N16R8 profile:
  2,238,534 bytes (66%); globals 163,056 bytes (49%). It was not installed
  because no USB serial device was present and `192.168.1.168` was unreachable.
- Firmware 1.87 SHA-256:
  `0a1aa083849a3b73c1236c6266763775d5f8ab4d4f083cac9e1c4755ec01c86c`
- Current release archive:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/Archive/Legacy SD Card Packages/OpenRemote_SD_CARD_v1.87_WebConfig_v2.03_DUAL_CORE_HTTP.zip`
- Release archive SHA-256:
  `29b53d258f13263dfdff8995b07151911ea0eb9ccbbcf78283000f7cc0e4e3b3`

- Firmware 1.86 keeps station Wi-Fi alive while the Wi-Fi or QR page is visible,
  preventing the QR page from reporting a connection that the Wi-Fi page has
  already shut down. Station modem sleep is disabled during WebConfig sessions.
- Up to eight Wi-Fi profiles are saved in Preferences. Existing credentials
  migrate automatically, and selecting a saved scan result offers `Use saved
  password` or `Forget & enter password`.
- Scan results are cached, deduplicated by SSID and stripped of blank hidden
  networks. A connected station remains associated during a scan.
- Firmware 1.86 compiled on 2026-07-22 with the ESP32-S3 N16R8 profile:
  2,237,630 bytes (66%); globals 163,048 bytes (49%).
- Firmware 1.86 SHA-256:
  `176717bc51d5b25847f1de0a2c1edc5f70e7534e2e3a969c105c387178c52f64`
- Current release archive:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/Archive/Legacy SD Card Packages/OpenRemote_SD_CARD_v1.86_WebConfig_v2.03_WIFI_PROFILES.zip`
- Release archive SHA-256:
  `19e4b35011f3afebc9253667999350683c84f0eb0f2d0aaac6b8bddd5f15b3aa`

- Firmware 1.85 adds the Android TV Voice-over-GATT service and its TX, RX and
  control characteristics beside the working combined HID service. It logs
  characteristic subscriptions and every Chromecast TX opcode so a fresh
  pairing can prove the real protocol before microphone capture is enabled.
- WebConfig 2.03 adds an assignable `Voice Search` command. Firmware sends ATVV
  `START_SEARCH`, then the Android Assistant HID usage `0x0221`.
- Firmware 1.85 reserves a Rev 5 digital microphone plan that shares the SD SPI
  pins only while SD is unmounted: power GPIO45, BCLK GPIO15/TP19, WS
  GPIO17/TP18, data GPIO7/TP20 and ground TP13. The microphone stays unpowered
  in this discovery build.
- Firmware 1.85 compiled on 2026-07-22 with the ESP32-S3 N16R8 profile:
  2,234,358 bytes (66%); globals 162,496 bytes (49%). The remote was not visible
  on USB, so this build is packaged but not installed or physically paired.
- Firmware 1.85 SHA-256:
  `22d8f879d35f9e6870baa915a057fb5e79377b0f13d0315206086fcc22dc0fae`
- Current release archive:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/Archive/Legacy SD Card Packages/OpenRemote_SD_CARD_v1.85_WebConfig_v2.03_ATVV_DISCOVERY.zip`
- Release archive SHA-256:
  `8ea6fe63b0944fad22e1b0b4cd2a4844abc441c4b4cd691f49a87ed8cca9f807`

- Firmware 1.84 fixes the 1.83 regression where a BLE-connected remote could
  remain paired to Chromecast but fail to wake from movement or physical keys.
  BLE idle still blanks both backlights, stops unused Wi-Fi, reduces CPU speed
  and requests the relaxed HID connection interval, but LCD/touch and SD rails
  now remain powered so the wake path stays dependable. Full CPU speed and the
  responsive BLE profile are restored before LCD or IR control resumes.
- Firmware 1.84 compiled on 2026-07-21 with the ESP32-S3 N16R8 profile:
  2,231,074 bytes (66%); globals 162,464 bytes (49%). The remote was not visible
  on USB, so this build is packaged but not installed or physically wake-tested.
- Firmware 1.84 SHA-256:
  `df01af1a069416b06956a41a492648789a8cfe6d08bf287341b058e0dc3a3a23`
- Current release archive:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/Archive/Legacy SD Card Packages/OpenRemote_SD_CARD_v1.84_WebConfig_v2.02_BLE_WAKE_FIX.zip`
- Release archive SHA-256:
  `5769340580403f9a48bda7532ad951dbc533187ffbcdf5858102df8759836ef1`

- Firmware 1.83 keeps Chromecast HID continuously connected but power-gates the
  complete LCD/touch and SD-card rails after the normal screen timeout. It also
  requests a relaxed 120-150 ms BLE connection interval with slave latency 3
  while idle, then requests the responsive 15-30 ms HID profile before controls
  resume. The host may accept or ignore either standards-compliant request.
- Movement and physical-button wake restore the CPU, remount SD and fully
  initialise the LCD behind its off backlight before LVGL reveals a frame. IR is
  restored on wake, so mixed BLE/IR activities retain normal control.
- Investigation of OMOTE Community's reference firmware found that its long-life
  standby explicitly ends BLE, shuts Wi-Fi down and enters true ESP32 deep sleep;
  it does not retain an Android TV HID connection. Arduino ESP32 3.3.10's shipped
  ESP32-S3 libraries have Bluetooth modem sleep, dynamic power management and
  tickless idle compiled out. Reaching connection-preserving automatic light
  sleep therefore requires a future custom ESP-IDF/Arduino-component build, not
  another sketch-level call. Rev5 also has no external 32.768 kHz crystal.
- Firmware 1.83 compiled on 2026-07-21 with the ESP32-S3 N16R8 profile:
  2,231,422 bytes (66%); globals 162,472 bytes (49%). The remote was not visible
  on USB or `192.168.1.122`, so this build is packaged but not installed or
  physically power-tested yet.
- Firmware 1.83 SHA-256:
  `33ada5f6f7b41a87e81c6d808556b0cc0ed957ff0e44872e1581587804e696e9`
- Current release archive:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/Archive/Legacy SD Card Packages/OpenRemote_SD_CARD_v1.83_WebConfig_v2.02_BLE_LOW_POWER_IDLE.zip`
- Release archive SHA-256:
  `a105b5747b64f098743218507a32cd08ba701f5fe32102b62efbc7653765e394`

- Firmware 1.82 adds a connected-idle path for activities containing BLE. After
  the normal screen timeout it retains the Chromecast HID connection, turns off
  unused Wi-Fi and reduces the application CPU from 240 MHz to 80 MHz. Movement
  or a physical key restores full speed before the LCD and IR path resumes.
  Mixed BLE/IR activities keep normal controls; IR-only activities continue to
  use the existing light-sleep and deep-sleep path.
- Firmware 1.82 also prevents the display from sleeping midway through an
  activity or macro sequence, so a delayed IR command is never attempted while
  the IR rail is asleep.
- Firmware 1.82 compiled on 2026-07-21 with the ESP32-S3 N16R8 profile:
  2,235,638 bytes (66%); globals 162,656 bytes (49%). The remote was not visible
  on USB or its previous WebConfig address, so this build is packaged but not
  yet installed on the physical remote.
- Firmware 1.82 SHA-256:
  `5f19841cd79b44d45f05a4b26607a2528bfe96bc3588610a72dc1d7829d8c35e`
- Current release archive:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/Archive/Legacy SD Card Packages/OpenRemote_SD_CARD_v1.82_WebConfig_v2.02_BLE_CONNECTED_IDLE.zip`
- Release archive SHA-256:
  `cc2cf389b5f8b00144b4dd194a07269fa9a68b6c58d865ae25159b68ca62cb75`

- Firmware 1.81 keeps the bonded BLE HID connection active while the selected
  activity or open device page uses a Bluetooth device. The LCD may turn off,
  but ESP32 light/deep sleep are deferred until a non-BLE activity is selected
  or All Off clears the active activity. This prevents Chromecast playback
  glitches caused by the remote deliberately disconnecting during sleep.
- Firmware 1.81 compiled on 2026-07-21 with the ESP32-S3 N16R8 profile:
  2,234,986 bytes (66%); globals 162,632 bytes (49%). The remote was not visible
  on USB or its previous WebConfig address, so this build is packaged but not
  yet installed on the physical remote.
- Firmware 1.81 SHA-256:
  `eb031a54b3f2da752caac62f8932ec672068b822f5ee33d1d2d92b234fe51627`
- Current release archive:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/Archive/Legacy SD Card Packages/OpenRemote_SD_CARD_v1.81_WebConfig_v2.02_BLE_KEEPALIVE.zip`
- Release archive SHA-256:
  `8a6747f66a673b88896436c5bd483ea6a6a7ac97be8f4b2f2b7b524d68371e1f`

- Firmware 1.80 is installed on the USB-connected remote. Leaving the QR page
  now cancels any in-flight browser response, closes that socket immediately
  and defers Wi-Fi shutdown until the HTTP handler has returned. LVGL touch,
  keypad and power-hold servicing continue during both the main HTML transfer
  and runtime configuration downloads.
- Firmware 1.80 compiled on 2026-07-21 with the ESP32-S3 N16R8 profile:
  2,234,854 bytes (66%); globals 162,632 bytes (49%). Upload at 460800 baud
  completed and the written flash hash was verified.
- Firmware 1.80 SHA-256:
  `2e2417d129c801321b85de19c7a4caf9ed6113d71e579a74a9c983b6cdca9b99`
- Firmware 1.80 release archive:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/Archive/Legacy SD Card Packages/OpenRemote_SD_CARD_v1.80_WebConfig_v2.02_WEB_EXIT_FIX.zip`
- Release archive SHA-256:
  `39939eaa726a0f28a9439775e28e27fa6bd6728e84051f1f56af5b64ed98dab4`

- Firmware 1.79 fixed activity pages so they can
  once again swipe to Activities home or Settings even after a Settings child
  page was previously opened. Returning to Settings now always opens the
  Settings home menu.
- QR sessions remain fully awake for 15 minutes. The saved screen timeout starts
  after that grace period; once the screen turns off, the configured deep-sleep
  countdown starts. This applies to both station-Wi-Fi and setup-AP sessions.
- Firmware 1.79 compiled on 2026-07-21 with the ESP32-S3 N16R8 profile:
  2,234,482 bytes (66%); globals 162,624 bytes (49%). Upload at 460800 baud
  completed and the written flash hash was verified.
- Firmware 1.79 SHA-256:
  `0ffdad97ab73f3895abb84a43f472d1dacac7dd17a6fc8e6a20abaac0d38b318`
- Firmware 1.79 release archive:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/Archive/Legacy SD Card Packages/OpenRemote_SD_CARD_v1.79_WebConfig_v2.02_NAV_QR_SLEEP.zip`
- Release archive SHA-256:
  `7140ee819db168a1976da4b1350cb7047816669e533c4d68849a985a64649e47`

- Firmware 1.78 made the main Activities page and nested activity-page sliders
  share one geometry calculation for
  icon scale, card height, text placement and 52 px row spacing. This removes
  the firmware-only oversized main-page appearance and matches Screen Designer.
- Firmware 1.78 compiled on 2026-07-21 with the ESP32-S3 N16R8 profile:
  2,234,314 bytes (66%); globals 162,616 bytes (49%). Upload at 460800 baud
  completed and the written flash hash was verified.
- Firmware 1.78 SHA-256:
  `b645a35bbbcad5e7e95bf0d3bd7169a2da555864ca41bb77c6e64cc8c622c30d`
- Firmware 1.78 release archive:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/Archive/Legacy SD Card Packages/OpenRemote_SD_CARD_v1.78_WebConfig_v2.02_ACTIVITY_SLIDER_PARITY.zip`
- Release archive SHA-256:
  `383ab40e731636d984608b69905110a225966da6f328fab9ad77fb2e34b37270`

- Firmware 1.77 added a Buttons page directly beneath Display with a persistent Repeat switch,
  100-1500 ms first-repeat delay and 1-20 repeats-per-second speed. The Test
  Button mode intercepts all 23 matrix keys plus the red power key without
  transmitting, then pulses the key name and status pill red using that timing.
- Settings sliders now distinguish horizontal adjustment from vertical page
  scrolling. SD-card mount capacity appears under Settings > About.
- Firmware 1.77 compiled on 2026-07-21 with the ESP32-S3 N16R8 profile:
  2,234,362 bytes (66%); globals 162,616 bytes (49%). Upload at 460800 baud
  completed and the written flash hash was verified.
- Firmware 1.77 SHA-256:
  `4949326734d3c0092b4d4d46ae2fa0132dfcf07ebe1700ccab6ad902af59e0dc`

- Firmware 1.75 introduced real
  standard-keyboard `0` through `9` commands to the existing Chromecast /
  Google TV profile without changing any of the first 17 working command or
  physical-button mappings.
- Keyboard and Consumer Control fields use separate HID application collections
  inside one combined ten-byte Report ID 1 characteristic. This preserves
  Android media/navigation recognition while avoiding the ESP32 BLE 3.3.10
  duplicate-Report crash fixed in firmware 1.74.
- Existing bonded Android hosts must forget `OpenRemote HID`, the remote must
  forget its pairing, and the pair must be created once more so Android reads
  the new HID report map.
- Firmware 1.75 compiled on 2026-07-21 with the ESP32-S3 N16R8 profile:
  2,230,726 bytes (66%); globals 162,440 bytes (49%). Upload at 460800 baud
  completed and the written flash hash was verified.
- WebConfig 2.02 creates the same ten keyboard commands for new Bluetooth
  streamer devices. Existing devices are upgraded by firmware at boot; all ten
  commands can be assigned to LCD controls, physical keys, activities or macros.
- Firmware 1.75 release archive:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/Archive/Legacy SD Card Packages/OpenRemote_SD_CARD_v1.75_WebConfig_v2.02_KEYBOARD_DIGITS.zip`
- Firmware binary:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/FIRMWARE/BIN/OpenRemote_1.75.bin`
- Firmware SHA-256:
  `25fbf049557f7ffd63b860269e2f2f17fbb40e0360504b46324efa2d4b19b4d4`

- Firmware 1.52 loads WebConfig macros into the runtime and executes their real
  command/delay sequence when a macro tile is tapped on an activity page.
  Macro execution is non-blocking and supports nested macros.
- Firmware 1.52 compiled with the ESP32-S3 N16R8 profile on 2026-07-19:
  2,208,986 bytes (66%); globals 161,800 bytes (49%).
- Firmware 1.51 preserves `type: "activity"` page items and renders them as
  full-width draggable activity sliders on activity pages. Completing a nested
  slide runs the referenced activity's real sequence and opens its page.
- Firmware 1.51 compiled with the ESP32-S3 N16R8 profile on 2026-07-19:
  2,207,934 bytes (66%); globals 161,600 bytes (49%).
- Nested sliders use their Screen Designer icon, label, activity text/icon size
  and activity boundary setting. The live runtime contains six valid nested
  slider references across Fetch TV, Chromecast and Steamdeck.
- Firmware 1.52 and WebConfig 2.00 are installed on the live remote at
  `192.168.1.122`. Status confirms SD mounted, ORBIMESH connected, BLE HID
  connected, seven devices, three activities and two saved macros.
- Firmware 1.50 compiled with the ESP32-S3 N16R8 profile on 2026-07-19:
  2,206,818 bytes (66%); globals 161,384 bytes (49%).
- Firmware 1.50 replaces the old RAM-heavy sync handler with a multipart upload
  streamed directly to `/tmp/runtime.upload.json`. The upload is validated in
  PSRAM, atomically swapped into place, and keeps the prior config as a rollback.
- WebConfig 2.00 skips unchanged theme assets and sends ordinary syncs as one
  streamed runtime JSON upload. A real browser sync returned to `Synchronised`
  with no console errors; an 87,915-byte API round trip also completed in about
  three seconds without losing touch, Wi-Fi, devices or activities.
- Firmware 1.49 clamps MAX17048 fuel-gauge percentages to 100% before LCD,
  WebConfig status, backup/status JSON or BLE HID battery reporting.
- ESP32 light sleep is removed. Display timeout now only fades both backlights,
  hides LVGL and paints the framebuffer black. The CPU, ILI9341 controller,
  Wi-Fi, Bluetooth, IR receiver rail, keypad and accelerometer remain active.
- WebConfig 1.99 normalises restored theme `sourcePath` and `previewPath`
  values, strips stale query tokens and rebuilds URLs for the current session.
  Embedded data-image themes are preserved instead of being replaced by blank
  URLs.
- Firmware 1.50/WebConfig 2.00 was the previous live safe-sync baseline.
- LIS3DHTR light-sleep setup now settles the high-pass filter and clears stale
  INT1 state before enabling GPIO wake. Both accelerometer and keypad lines must
  remain idle before sleep, and are checked again after the backlight fade.
- Settings changed on the LCD are mirrored into `/config/runtime.json` as well
  as Preferences/NVS. Runtime loading can no longer replace a newer brightness,
  timeout, sensitivity, clock or connectivity choice with an older saved value.
- WebConfig 1.96 adds `Always` and `Only when this activity turns on a device`
  delay modes. The firmware records which smart-power ON commands were actually
  sent in the current activation and skips conditional startup waits otherwise.
- The live Fetch TV, Chromecast and Steamdeck 5000 ms delays are tied to the
  Sony amplifier. The wait therefore runs after a cold amplifier start and is
  skipped when changing activities while the amplifier is already remembered on.
- Firmware 1.47/WebConfig 1.98 validation below is retained as release history;
  the authoritative live versions are now 1.50/2.00.
- WebConfig `/www/index.html` is now streamed cooperatively from SD. A live pull
  of the 1,287,213-byte HTML file completed in about 15 seconds and `/api/status`
  responded immediately afterward, confirming the old blocking `streamFile`
  crash path is gone.
- Display sleep no longer aborts before fading when the accelerometer or keypad
  wake line is already asserted. The LCD/backlight sleeps first; ESP32 light
  sleep is skipped only for that noisy/stuck-wake cycle. Live status after the
  5-second timeout reported `displaySleeping:true`, `touchDown:false`, QR
  inactive, and Activities page active.
- Live WebConfig browser verification loaded all five devices and all three
  activities, showed the Sony conditional delay in Fetch TV, and logged no
  JavaScript warnings or errors.
- WebConfig 1.97 accepts the exact user backup
  `OpenRemote_Backup_2026-07-19_17-14-10.json`. The old validator rejected
  SD-backed custom icon URLs even though WebConfig exported them. The installed
  page now stages 5 devices, 1 learned device, 3 activities, 4 custom icons and
  3 themes, with Restore enabled.
- WebConfig 1.98 refreshes the Themes library after restore. The missing Steam
  Deck theme from that backup was converted to a real 240x320 PNG and
  153,600-byte RGB565 asset, uploaded to `/themes/Custom`, and added to the live
  runtime. Live verification now reports Smooth Blue, Luxe and Steam Deck while
  retaining 5 devices and 3 activities.
- Authoritative live snapshots are
  `OpenRemote/versions/OpenRemote_Live_1.52_runtime.json` and
  `OpenRemote/versions/OpenRemote_Live_1.52_status.json`. Runtime SHA-256 is
  `9e3c676fe828d2778be280df13035c3b2a3feb9356a083f23413dc91ec53b0fb`.
- Ready-to-copy SD release:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/Archive/Legacy SD Card Packages/OpenRemote_SD_CARD_v1.50_WebConfig_v2.00_SAFE_STREAMING_SYNC`
- Source snapshot:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/FIRMWARE/Other/Release Archives/Active Sketch/OpenRemote_1.52_source.zip`
- Combined firmware/WebConfig snapshot:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/FIRMWARE/Other/Release Archives/Versions/OpenRemote_v1.52_webconfig_v2.00_executable_macro_tiles.zip`
- Ready-to-copy SD ZIP:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/Archive/Legacy SD Card Packages/OpenRemote_SD_CARD_v1.50_WebConfig_v2.00_SAFE_STREAMING_SYNC.zip`
- Firmware SHA-256: `5511b82835232a6eee2278bc5a404cea71e6d00ecfe122c08f7347c3ace588d5`
- WebConfig SHA-256: `69ab8a33c92a031bb4053c0023d987d9f01f206aa0654168f82f3ca2ed74b9e5`
- Source snapshot SHA-256: `d8b7c0a6a0efe114a2aaf422aad42f126e9e7e66e7774b935ea0de57a1c198dd`
- Combined snapshot SHA-256: `5edebfc17eeea0200f70a3870d4b68061bf8babfaa1ed73d879433a25772ef20`
- SD ZIP SHA-256: `f8bcf80c4f6029b746000cce7e1a6ecd8490783720a3c38357a7f61586b214ad`
- Active source, combined release and SD archives passed `unzip -t`.

## Immediate First Action

1. On a spare/blank ESP32-S3, open Studio 2.43 and confirm **Choose Firmware**
   and **Choose WebConfig** are immediately visible with no versions selected.
2. Choose a normal PlatformIO application `.bin` plus a WebConfig `.html`, then
   use **Check the board**. Confirm automatic USB programming detects the S3
   without a BOOT button and the selected versions appear in steps 2 and 3.
3. Insert a FAT32/MBR SD card in the remote and run **Prepare Remote SD**. Confirm
   `/www/index.html`, recovery firmware, version metadata and Default icons are
   present while any pre-existing runtime/devices/custom assets remain intact.

## Hardware

- Board: OMOTE Rev 5 / OpenRemote
- MCU: ESP32-S3-WROOM-1-N16R8
- Flash: 16 MB
- PSRAM: 8 MB OPI
- LCD: ILI9341, 240 x 320, 8-bit parallel
- Touch: CST026 at I2C address `0x38`
- I2C: SDA GPIO20, SCL GPIO19
- LIS3DH accelerometer: `0x19`
- LIS3DH INT1 / motion wake: GPIO2, active high
- TCA8418 keypad controller: `0x34`
- TCA8418 keypad IRQ / key wake: GPIO8, active low
- MAX17048 fuel gauge: `0x36`
- LCD enable: GPIO38, active low
- LCD backlight: GPIO9, active-low P-channel driver
- Keypad blue LEDs: GPIO46, active high
- IR LED: GPIO5, active low
- Charge status: GPIO1, active low
- SD CS: GPIO18
- SD MOSI: GPIO17
- SD MISO: GPIO7
- SD clock: GPIO15
- SD enable: GPIO16, active low

Note: the earlier handoff pin list for SD CS/MOSI/MISO overlapped the 8-bit LCD
data bus. The corrected values above come from the Rev 5 KiCad netlist:
`SD_CS=GPIO18`, `SD_MOSI=GPIO17`, `SD_MISO=GPIO7`, `SD_SCK=GPIO15`,
`SD_EN=GPIO16`.

### Verified Rev 5 physical keys

The v1.36 runtime table comes directly from the completed LCD diagnostic:

| Logical button | Key | Switch | Row | Column |
|---|---:|---:|---:|---:|
| Stop | 5 | S25 | 0 | 4 |
| Rewind | 4 | S24 | 0 | 3 |
| Play | 2 | S22 | 0 | 1 |
| Forward | 11 | S16 | 1 | 0 |
| Menu | 3 | S23 | 0 | 2 |
| Info | 31 | S6 | 3 | 0 |
| D-pad Up | 14 | S19 | 1 | 3 |
| D-pad Down | 35 | S10 | 3 | 4 |
| D-pad Left | 15 | S20 | 1 | 4 |
| D-pad Right | 32 | S7 | 3 | 1 |
| OK | 34 | S9 | 3 | 3 |
| Back | 13 | S18 | 1 | 2 |
| Return | 41 | S1 | 4 | 0 |
| Volume Up | 33 | S8 | 3 | 2 |
| Volume Down | 43 | S3 | 4 | 2 |
| Mute | 44 | S4 | 4 | 3 |
| Channel Up | 42 | S2 | 4 | 1 |
| Channel Down | 22 | S12 | 2 | 1 |
| Record | 45 | S5 | 4 | 4 |
| Red | 23 | S13 | 2 | 2 |
| Green | 25 | S15 | 2 | 4 |
| Yellow | 24 | S14 | 2 | 3 |
| Blue | 21 | S11 | 2 | 0 |

The separate hardware power key is S17/TCA event 102. A short press remains
global All Off and a seven-second hold reboots from any screen.

Display rotation is currently `0`, matching the permanently installed screen.
Touch coordinates use:

```cpp
x = (LCD_W - 1) - rawX;
y = (LCD_H - 1) - rawY;
```

## Firmware Runtime State

- LVGL 8.3.11 runtime with Arduino GFX 1.6.6.
- Cinematic wallpaper and glass-style controls.
- Thresholded Montserrat fonts for sharper text.
- Activities, device pages, Settings, page dots, title bar, clock, battery pill,
  brightness control, touch gestures, and vertical device-page scrolling.
- Left/right page swiping and up/down scrolling are implemented.
- Device picker grows down to the page dots and only scrolls vertically when
  required.
- Settings title is `Settings`.
- Time uses 12-hour format with AM/PM.
- Battery uses a light-green fill and a smooth empty-to-full charging animation.
- LCD and keypad LEDs fade out together over 0.5 seconds at timeout.
- LCD wakes using LIS3DH movement detection.
- Keypad LEDs remain on while the LCD backlight is on.
- Brightness popup closes after inactivity and when tapping outside it.
- Periodic IR heartbeat is disabled. Bound command tiles now transmit real
  supported parsed or raw IRDB payloads through active-low GPIO5.

## Arduino Build

Compile command used successfully:

```bash
'/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli' compile \
  --fqbn 'esp32:esp32:esp32s3:FlashSize=16M,PartitionScheme=default_8MB,PSRAM=opi' \
  --libraries '/Users/phillipcarlson/Documents/Arduino/SLS/Test/libraries' \
  '/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/Platformio:Arduino/OpenRemote/OpenRemote_1.0'
```

Known successful build before this handoff:

- Sketch: 761,997 bytes (58%)
- Global variables: 121,956 bytes (37%)

Known successful build after v1.01 SD bootstrap:

- Sketch: 815,709 bytes (62%)
- Global variables: 122,228 bytes (37%)

v1.01 changes:

- Added SD/SPI firmware initialization using the corrected Rev 5 SD pins.
- Powers the microSD rail with active-low `SD_EN`.
- Mounts FAT storage on boot and creates `/config`, `/themes`, `/icons`,
  `/devices`, `/irdb`, `/backups`, `/logs`, and `/tmp` when missing.
- Adds a Settings-page `SD Card` row showing mounted/folder-ready state.
- Rebuilt active ZIP:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/FIRMWARE/Other/Release Archives/Active Sketch/OpenRemote_1.0.zip`
- Saved snapshot:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/FIRMWARE/Other/Release Archives/Versions/OpenRemote_v1.01_sd_bootstrap.zip`

Known successful build after v1.02 real-device settings:

- Sketch: 1,756,546 bytes (52% of the 3 MB application partition)
- Global variables: 164,632 bytes (50%)
- Required partition: `default_8MB` (`3 MB APP / 1.5 MB SPIFFS`) so OTA has room

v1.02 changes:

- Added the required running change log and numeric/string `1.02` version at the top of the INO.
- Added LCD About page with the active firmware version.
- Added persistent Preferences-backed Wi-Fi, BLE, Clock, brightness, sleep,
  raise-to-wake and slide-unlock settings.
- Added real Wi-Fi scanning, on-screen password keyboard, station connection,
  password-protected setup AP and captive WebConfig server.
- Added real LVGL QR output for the setup network or connected WebConfig URL.
- Added ESP32-S3 BLE advertising controlled by the Bluetooth switch.
- Added real SD WebConfig serving, runtime JSON sync, compiled `.bin` OTA,
  replacement `.html` upload and authenticated SD data-rebuild endpoints.
- Removed active demo devices, activities, macros and IR records. Runtime data
  now loads from `/config/runtime.json`.
- Added WebConfig v1.70; historical card images are now retained under:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/Archive/Legacy SD Card Packages`
- Saved firmware snapshot:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/FIRMWARE/Other/Release Archives/Versions/OpenRemote_v1.02_real_settings_webconfig.zip`
- Saved ready-to-copy SD archive:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/Archive/Legacy SD Card Packages/OpenRemote_SD_CARD_v1.02.zip`

Known successful build after v1.03 Studio IRDB integration:

- Sketch: 1,767,498 bytes (52% of the 3 MB application partition)
- Global variables: 189,488 bytes (57%); raw timings and runtime JSON use PSRAM
- IRremote 4.7.1 is installed under the Arduino user libraries folder

v1.03 changes:

- Added `IRremote` active-low GPIO5 transmission for NEC, NECext/NEC1,
  Samsung32, RC5/RC5X, RC6, SIRC/SIRC15/SIRC20 and raw Flipper timings.
- Added command IDs and real device/activity tile bindings.
- Added PSRAM-backed raw timing storage, up to 1,024 timings per command.
- Added authenticated `GET/POST /api/irdb` for
  `/irdb/OpenRemote.irdb`, including replacement-file backup.
- Added WebConfig v1.71, which fetches and parses Studio's untouched SQLite
  database in the phone browser and syncs only selected transmit-ready command
  payloads to `/config/runtime.json`.
- The real Studio database was validated at 14,551 devices: 8,694 Flipper,
  3,141 probonopd compact and 2,716 LIRC records.
- LIRC records remain browse-only because Studio v2.29 stores their button
  codes without the complete LIRC timing/protocol definition.
- Saved firmware snapshot:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/FIRMWARE/Other/Release Archives/Versions/OpenRemote_v1.03_real_irdb_runtime.zip`
- Saved ready-to-copy SD archive:
  `/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/Archive/Legacy SD Card Packages/OpenRemote_SD_CARD_v1.03.zip`
- Firmware ZIP SHA-256: `00794bb3b50ff1c0f3a88db1925226ca7fa337b4e94a65b5f7009027dcc2d2fb`
- SD archive SHA-256: `04817e934f6d1f0b9f7618c955f812bc9abdab98dba206a3bcbd461a4d803664`
- Native IRDB SHA-256: `cb4bd0aac95212c524ecfb0f4c85893ef6ff518ef86cd8d2cf0ca03d3be7b132`
- Physical IR output remains to be verified on a connected Rev 5 remote; no
  board or IR receiver was connected during this pass.

## SD Card Direction

The 32 GB microSD card should be formatted as FAT32 with an MBR partition map
and named `OPENREMOTE`.

Active SD structure:

```text
/www/index.html
/config
/config/runtime.json
/config/version.json
/themes
/icons
/devices
/activities
/macros
/irdb
/firmware
/backups
/logs
/tmp
```

Copy the contents of
`/Users/phillipcarlson/Documents/Arduino/OpenRemote/SOFTWARE/Archive/Legacy SD Card Packages/OpenRemote_SD_CARD_v1.98_WebConfig_v2.04_BLUE_VOICE_IN_APP_FIX`
to the FAT32 card root. WebConfig 2.04 calls the ESP32 directly. Factory reset
deletes only user configuration and recreates required folders; it does not
format the card and preserves WebConfig, firmware, SD backups and Default asset
libraries. The full IRDB stays on the computer; OpenRemote Studio copies only
selected `.ir` devices into `/devices`.

## Working Preferences

- Do not reintroduce fake devices, activities, macros or IRDB content.
- Make closely scoped edits and compile firmware after changes.
- Rebuild the active ZIP after verified firmware changes.
- Keep the live Arduino folder stable for Arduino IDE settings.
- Save future Web Config HTML versions directly in the `WebConfig` folder.
- Continue version increments in steps of 0.01.

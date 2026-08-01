# OpenRemote 2.36 - DMA Display Bands and Active Touch

Firmware 2.36 keeps the persistent, finger-following LVGL page strip and its
two 240x32 DMA buffers. It makes the destination tile visible before building
its controls, preventing LVGL from clipping every invalidation generated for
an off-screen tile and leaving only the wallpaper bands visible. Touch uses the
proven firmware 2.09 single-frame FT5x06 read again, while retaining the clean
release quarantine after wake and UI rebuilds.

# OpenRemote 2.34 - Persistent LVGL Page Strip

Firmware 2.34 keeps the 2.32 replacement of screenshot-driven swipe transitions
with persistent
native LVGL tiles. Page content is never deleted or rebuilt while a touch or
scroll animation is active. The display uses two proven 240x32 RGB565 DMA
buffers, so it remains double-buffered without asking the Rev 5 parallel bus to
accept one unsupported full-screen transfer. Calibrated bands finish their DMA
transfer before the shared colour buffer is reused, so every row reaches the
LCD reliably. Touch input requires matching
valid FT5x06 frames, resets LVGL's input state before every page-tree mutation,
and quarantines stale samples after wake or navigation. Ordinary display sleep
keeps the FT5x06 awake instead of using its unreliable I2C hibernate sequence.
WebConfig also pauses BLE and performs a cold, retryable Wi-Fi controller
recovery, preventing Chromecast use from leaving Wi-Fi dead until a hardware
reset.

The UI object tree is allocated in PSRAM so the persistent page strip does not
consume the internal DMA heap.

Firmware 2.31 applies the focused page-lifecycle fix from firmware 2.10 to the
2.30 branch. Device selection is deferred until LVGL finishes its picker event,
temporary device pages are excluded from adjacent-page snapshot generation,
and closing a device returns to the exact page that opened it.

# OpenRemote 2.30 - Stable 2.09 Foundation

Firmware 2.30 is deliberately rebuilt from firmware 2.09. It preserves the
2.09 display, raw touch, BLE, Wi-Fi and page-navigation implementation instead
of carrying those subsystems forward from rejected firmware 2.29.

Only the requested later features have been added:

- Complete LCD Debug menu with saved split-line calibration, touch diagnostics,
  CPU/RAM, accelerometer and FPS overlays
- Real I2S microphone or embedded test-audio selection for Chromecast ATVV
  Voice Search, including the blue physical-button microphone overlay
- One-minute deep-sleep selection in addition to the existing 5-30 minute range
- Current charger-aware battery history and time-to-full/time-to-empty metrics
- Runtime and API compatibility required by WebConfig 2.16, including default
  theme-file status checks
- Firmware version and running INO change log updated to 2.30

Firmware 2.29 is rejected because BLE discovery/connection and Wi-Fi operation
could fail after starting a Bluetooth activity. Do not use it as a base.

# OpenRemote 2.06 - Reliable Deep Sleep

Firmware 2.06 fixes the five-minute power transition on Rev 5. Timer wakeups
remain dark while deep sleep is prepared, and the LIS3DH now establishes a
stationary high-pass reference so gravity cannot hold its wake line active.
Only real movement or a physical key wakes the LCD.

# OpenRemote 2.05 - WebConfig Clock and Wi-Fi Management

WebConfig 2.05 mirrors the LCD Clock controls and saves Clock visibility,
Internet/manual mode, city offset and manual date/time directly to the remote.
Its Settings page can scan and join nearby Wi-Fi networks, reuse or forget saved
passwords, and keep the setup AP available while changing networks. The captive
splash also provides a direct `http://192.168.4.1` fallback.

# OpenRemote 1.98 - Blue Voice Overlay and In-App Search

Physical Chromecast Voice Search displays a large blue animated microphone over
the active LCD page for the duration of the held hardware button. Its complete
LVGL object tree is now allocated at boot and merely revealed after the opening
Bluetooth handshake window, preserving the v1.96 Google Home and in-app YouTube
Voice Search behavior. WebConfig 2.04 removes deleted-device commands from every
runtime location before the next sync.

Open `OpenRemote_1.0.ino` from this folder in Arduino IDE. Keep `lv_conf.h`,
`cinema_wallpaper_rgb565.h`, `atvv_test_audio.h`, and the four
`lv_font_openremote_*.c` files beside the INO file.

Required libraries:

- Arduino_GFX_Library
- LVGL 8.x
- ArduinoJson 7.x
- IRremote 4.7.1 or newer

Suggested ESP32 board settings:

- Board: ESP32S3 Dev Module
- Flash size: 16 MB
- Partition scheme: 8M with spiffs (3 MB APP / 1.5 MB SPIFFS)
- PSRAM: OPI PSRAM
- USB CDC On Boot: Enabled
- USB Mode: Hardware CDC and JTAG
- Upload Mode: UART0 / Hardware CDC
- Upload speed: 460800

This build includes:

- Boot-preallocated blue Voice Search overlay that does not construct LVGL
  objects during Chromecast's time-sensitive in-app microphone handshake
- Physical-button-only animated Voice Search microphone overlay
- Complete deleted-device reference cleanup across activity startup sequences,
  conditional delays, macros, LCD page items and physical-button assignments
- Audible 3.14-second ATVV test phrase encoded into 628 native 20-byte frames
- Standards-compliant `AUDIO_SYNC` state before speech and trailing silence
- Physical and LCD Hold-to-Talk remain live until real button release, with a
  two-minute emergency ceiling only for a genuinely lost release event
- Signed ATVV timeout arithmetic and a fresh post-Bluetooth service timestamp
- Raw-key physical Voice Search release that cannot be lost when the active
  activity/device page or its resolved binding changes during a hold
- Serial diagnostics for the exact physical Voice Search press/release edges
- Late Chromecast `MIC_OPEN` requests are always acknowledged with
  `AUDIO_START`, even when button-up was already queued
- ATVV host events survive session cleanup so release reliably follows with
  `AUDIO_STOP` instead of leaving Android TV's green microphone dot latched
- Chromecast Voice Search sends a complete Assistant HID click on physical/LCD
  press so the TV opens its microphone UI while the button remains held
- Ordered ATVV `MIC_OPEN`/`AUDIO_START`/`AUDIO_STOP` handling prevents release
  from racing ahead of microphone open and leaving Android TV's green mic dot on
- Smart-power memory owned exclusively by activity transitions: activity
  startup records ON and Activity All Off records OFF
- Manual LCD, physical-button, WebConfig-test and standalone macro power
  commands transmit without changing remembered device state
- Stateful Chromecast Voice Search for physical assignments and LCD tiles:
  one ATVV start/HID key-down on press and HID key-up/AUDIO_STOP on release
- Voice Search is never repeated by the physical-button repeat engine, while
  ordinary IR hold-repeat behavior remains unchanged
- Dedicated low-priority WebConfig HTTP worker on ESP32-S3 core 0, isolated
  from the core 1 LVGL, touchscreen, keypad and sleep loop
- Responsive LCD controls during phone page loads, runtime sync uploads and
  slow or abandoned browser connections
- Worker-owned HTTP listener lifecycle with clean cancellation and interface
  rebinding when changing between station Wi-Fi and the setup access point
- Consistent live Wi-Fi state across the Wi-Fi and WebConfig QR pages
- Up to eight saved Wi-Fi networks with `Use saved password` and
  `Forget & enter password` actions on the LCD
- Connection-preserving Wi-Fi scans that remove blank hidden entries and
  duplicate mesh access points while keeping the strongest result
- Automatic station reconnect and setup-AP fallback from the QR page, with
  Wi-Fi modem sleep disabled while WebConfig is actively available
- Bonded BLE HID remains active while the selected activity or open device page
  contains a Bluetooth device, including after the LCD backlight turns off
- BLE-connected activity idle turns off unused Wi-Fi, blanks both backlights,
  reduces CPU speed and requests a relaxed BLE connection schedule without
  breaking the Chromecast HID connection
- Movement or a physical key restores full speed before waking the LCD or
  transmitting IR, so mixed Bluetooth/IR activities retain normal controls
- LCD/touch and SD rails remain powered during BLE idle so movement and keypad
  wake cannot be lost
- Full light sleep and deep sleep resume automatically after switching to an
  IR-only activity or using All Off
- QR-page Back immediately cancels an abandoned browser response, closes the
  active socket and defers Wi-Fi shutdown until the HTTP handler has returned
- Runtime configuration uploads stream to SD on the HTTP core and reload only
  after the response completes, avoiding network/UI/SD work in one loop
- Reliable left/right navigation from activity pages back to Activities home
  or Settings, independent of any previously opened Settings subpage
- Settings navigation always reopens the Settings home menu instead of leaving
  a stale child page active behind an activity or device page
- QR-page sleep sequence with a 15-minute awake grace period, followed by the
  saved screen timeout and then the configured deep-sleep delay
- Matching activity-slider dimensions on the main Activities page and nested
  activity pages, using the same Screen Designer grid geometry
- Normal ILI9341 rotation with matching 180-degree touch-coordinate correction
- Working left/right page swipes
- Perceptual 0-100% active-low 25 kHz 10-bit PWM brightness control on GPIO9
- Non-black minimum brightness: 0% remains dimly lit instead of turning the panel off
- Blue button backlights on GPIO46 follow the LCD awake/sleep state
- Synchronized 500 ms LCD and blue keypad LED fade before display sleep
- Unfiltered full-opacity RGB565 wallpaper rendering
- Tap-outside dismissal and four-second timeout for the brightness panel
- Custom glass slide-to-activate controls
- Clock page with status-bar switch, Wi-Fi-gated Internet time city selection, immediate Internet-time/manual refresh and manual date/time rollers
- Raw touch-distance page swipes that work over cards and tiles
- True-white 1-bit Montserrat fonts with uniform solid pixels and no antialiasing haze
- Settings rows hand vertical swipes to the page instead of selecting row text
- Vertically scrollable device command pages with a fixed title bar and up to 50 commands per imported device
- Dynamic vertical-only device picker that grows down to the page-dot boundary
- Compact 12-hour clock with AM/PM and tighter battery spacing
- Light-green MAX17048 battery display with a debounced 2.2-second empty-to-full charging loop
- Shortened `Settings` page title
- Saved brightness, sleep timer, 1-100% motion sensitivity and post-sleep slide unlock
- Real Wi-Fi scanning, compact full-alphabet password keypad and station connection
- QR-only open `OpenRemote-XXXX` setup AP that remains active until the QR page is closed, with BLE paused during setup and live client-count refresh
- Fifteen-minute QR-page awake grace period before the saved screen timeout begins
- Minimal captive splash page that opens the full WebConfig in the device's normal browser
- Real QR screen: station URL when connected, setup-network QR before provisioning
- Bonded `OpenRemote HID` keyboard/media remote for Chromecast, Google TV and
  Android TV, with real LCD/WebConfig Pair and Forget controls
- Automatic persistent Chromecast / Google TV device creation after BLE pairing,
  with a complete 27-command LCD page and useful default physical-key bindings
- Standard keyboard 0-9 commands for Android Button Mapper, assignable to LCD
  controls, physical keys, activities and macros without changing existing mappings
- Assignable `Voice Search` command that sends Android TV Voice-over-GATT
  `START_SEARCH`, followed by the Android Assistant HID usage
- Android TV Voice-over-GATT discovery service with TX, RX and control
  characteristics, subscription diagnostics and complete TX opcode logging
- Paired-host persistence across restarts so the BLE HID service advertises for
  reconnect even when the ESP32 bond-count query is temporarily empty
- Smart physical-button defaults for every device type, using ranked aliases for
  navigation, transport, volume, channel, menu/info, record and colour commands
- Real local Homebridge integration through the ESP32: authenticated accessory
  discovery, live connection checks and command execution without browser CORS
  or `.local` DNS limitations
- Homebridge switch, dimmer, fan, volume, mute, blind, door, lock and thermostat
  characteristics become real LCD, activity, WebConfig-test and physical-button
  commands; read-only sensor services are omitted
- Homebridge credentials are saved in ESP32 Preferences/NVS only and never
  written to `/www/index.html`, `/config/runtime.json`, backups or release files
- Per-device smart-mapping initialization that fills only unassigned buttons and
  preserves every later WebConfig override or deliberately cleared assignment
- BLE command routing for LCD tiles, activities, physical assignments and
  WebConfig command tests; microphone audio remains disabled until a fresh
  Chromecast pairing confirms the expected ATVV capability handshake
- SD-served `/www/index.html` WebConfig and `/config/runtime.json` real-data model
- WebConfig 2.00 streams runtime sync directly to an SD temporary file, validates
  it in PSRAM, atomically swaps it into place and retains the previous config for rollback
- OpenRemote Studio `.ir` device imports stored under `/devices`; WebConfig does not transfer or search the full IRDB database
- Real active-low GPIO5 IR transmission for Flipper parsed/raw commands and supported compact records
- Real nonblocking 38 kHz IR learning through the Rev 5 receiver on GPIO4, with receiver power on GPIO6
- Rev 5 TCA8418 physical-key scanner with WebConfig command assignments
- Adjustable physical-button hold repeat with independent first-repeat delay
  and repeat speed under LCD Settings > Buttons
- Non-transmitting physical-button test mode with red key-name and status-pill
  pulses that follow the selected repeat timing
- Nonblocking OpenRemote Studio USB transfers on both native CDC and UART0
- Studio `.ir` imports saved directly under `/devices` with interrupted-upload cleanup
- Automatic `/devices/*.ir` discovery for the LCD runtime and WebConfig device list without copying imported devices into `runtime.json`
- Default icon discovery from `/icons/Default` and custom icon upload, rename and removal under `/icons/Custom`
- Cooperative `/www/index.html` WebConfig streaming from SD that keeps LVGL,
  touch/keypad servicing and the watchdog alive while the 1.2 MB HTML loads
- WebConfig QR sessions stay fully awake for 15 minutes, then run the saved
  screen timeout; the deep-sleep countdown begins only after the display turns off
- Display timeout fades the LCD and keypad backlights and blanks the screen;
  BLE activity idle then powers off LCD/touch, SD, Wi-Fi and IR while retaining
  the HID link and movement/keypad wake
- `/api/status` includes diagnostic `displaySleeping`, `touchDown`,
  `awakeForMs`, page and setup-AP state fields for live sleep debugging
- PSRAM-backed raw timing storage for selected commands
- PSRAM-backed device/activity runtime tables that preserve internal heap for Wi-Fi and BLE
- Command-ID bindings for device and activity tiles
- SD-backed PNG icons, RGB565 theme wallpapers, boundary settings and LVGL-matched font sizes from WebConfig
- Station-Wi-Fi WebConfig QR URLs that preserve the existing network, with setup-AP fallback when disconnected
- Brightness-panel battery percentage and fixed top-bar/content boundaries
- Versioned About menu showing firmware `1.96`, WebConfig metadata and SD-card status
- Scroll-safe Display and Buttons sliders that distinguish horizontal changes
  from vertical page scrolling
- MAX17048 fuel-gauge values are clamped to 100% before display, WebConfig status or BLE HID battery reporting
- Unchanged theme bitmaps are skipped during normal sync, avoiding repeated large SD/Wi-Fi uploads
- LCD `Backup / Restore` menu for creating full SD backups and restoring from a scrollable date/time list
- Remote-created full backups preserve runtime configuration, file-backed devices, activities, macros, themes and custom icons
- Compatible WebConfig full-backup JSON files can also be restored from the LCD menu
- Display settings omit the deprecated inversion control while retaining compatibility with previously saved state
- WebConfig 1.88 exact spacer-slot layout sync, stable post-sync status and Computer/SD backup choices
- Three-column runtime geometry that places slot 6 at LCD y=151, matching the designer's 150 px split line
- Vertically scrollable activity and device canvases with held-repeat preserved while LVGL evaluates a scroll gesture
- Cached RGB565 themes and asynchronous unlock completion for an immediate transition without a white redraw flash
- Authenticated command testing, reboot and SD-backed device removal from WebConfig
- Red command feedback in the LCD clock/battery pill for every transmit and held-button repeat
- Global and per-item activity/button/macro boundary visibility, with activity boundaries limited to the thin outer slider outline
- Full-range icon/text sizing, title-based device selection, responsive long-page scrolling and short page transitions
- Real nonblocking activity startup sequences, including nested macro commands and delays
- Activity-page macro tiles execute their complete saved command/delay sequence
  without leaving the current LCD page; nested macros are expanded at load time
- Firmware-side smart power memory for activity changes: toggle-only and
  discrete-power devices already remembered as on skip redundant startup power
  commands while input/source commands still run
- PSRAM-buffered, length-delimited runtime downloads plus WebConfig 1.94's
  runtime-first startup queue and automatic retries, preventing status/icon
  requests from hiding saved devices and activities in Screen Designer
- Stable screen-off behavior with no ESP32 light sleep, radio suspension,
  controller sleep commands or peripheral power cycling
- Continuous LIS3DHTR motion and TCA8418 keypad servicing for immediate wake
  while the display is blank
- PSRAM page-icon caching, slower battery/status refresh intervals, and
  lightweight translation-only page transitions for smoother multi-activity
  themes
- WebConfig 1.95 per-activity `Clear physical buttons` action, which removes all
  hardware-button assignments from the selected activity without changing its
  LCD commands, devices, macros, or other activities
- Stable LIS3DHTR light-sleep arming that settles the high-pass filter, clears
  stale interrupts, and refuses to sleep while a wake input is still active
- LCD brightness, sleep timeout, motion sensitivity, clock, Bluetooth and other
  on-device settings are mirrored into `/config/runtime.json`, preventing an
  older runtime model from replacing newer LCD choices after a restart
- WebConfig 1.96 conditional activity delays: choose `Always`, or wait only when
  the selected smart-power device was actually turned on by that activation
- Smart power state is shared by activity steps, LCD/physical commands,
  WebConfig command tests and All Off, and survives runtime synchronisation
- All Off sends power only to devices remembered as on; repeated All Off presses
  remain harmless after the first successful shutdown
- Temporary device pages that are removed after swiping away instead of remaining as stale page dots
- Immediate LCD wake and redraw after a successful runtime sync
- Verified Rev 5 physical-key mapping without the completed `Buttons` diagnostic page in the device picker
- Seven-second hardware power hold to reboot from any screen, while short press remains global All Off
- Hardware power short presses are ignored on the Activities home page after All Off has already cleared the active activity/device state
- Authenticated SD backup list/upload/download APIs and a non-formatting factory reset that preserves WebConfig, firmware, backups and Default assets
- No bundled sample devices, activities or macros
- Rev 5 microSD mount on boot with automatic `/www`, `/config`, `/themes`, `/icons`, `/devices`, `/activities`, `/macros`, `/irdb`, `/firmware`, `/backups`, `/logs`, and `/tmp` folder bootstrap

Rev 5 SD pin map used by the firmware:

- CS: GPIO18
- MOSI: GPIO17
- MISO: GPIO7
- SCK: GPIO15
- Enable: GPIO16, active low

Reserved Rev 5 digital I2S microphone wiring for the next ATVV milestone:

- Microphone VCC: GPIO45 / onboard D3 anode net, controlled by firmware
- Microphone GND: GND / TP13
- Microphone SCK/BCLK: GPIO15 / TP19
- Microphone WS/LRCLK: GPIO17 / TP18
- Microphone SD/data: GPIO7 / TP20
- Microphone L/R select: GND

GPIO15, GPIO17 and GPIO7 are shared with the microSD bus. Firmware 1.85 keeps
GPIO45 low so the microphone is unpowered during normal SD operation. A later
audio-capture build must unmount SD, hold SD CS GPIO18 high, power the
microphone, capture audio, then power it off before remounting SD. Do not power
the microphone permanently from 3.3 V while its data output is connected to
GPIO7.

Touch coordinates and Studio USB commands use Serial at 460800 baud.

The setup AP is open and only runs while the WebConfig QR page is visible.

WebConfig firmware updates require the compiled ESP32-S3 application `.bin`; an `.ino` is source code and cannot become active until it is compiled. Firmware is validated and staged on the SD card before the user presses Install. The active WebConfig can be any complete `.html` file: it is always installed as `/www/index.html`, with the previous version backed up.

Verified compiling with ESP32 core 3.3.10, ArduinoJson 7.4.2, IRremote 4.7.1, GFX Library for Arduino 1.6.6, and LVGL 8.3.11.

/*
  OpenRemote firmware change log (newest first)

  2.38 - 2026-08-01
    - In progress.

  2.37 - 2026-08-01
    - Fixes persistent tile selection before the first LVGL layout pass. The
      visible tile is now positioned, selected and laid out before its controls
      are built, followed by one complete screen invalidation after mutation.
    - Restores firmware 2.09's passive touch-controller startup exactly: the
      FT5x06 is detected but no operating-mode or threshold registers are
      rewritten during boot.
    - Actually fixes the blank-center display: lv_obj_remove_style_all() on
      each page-strip tile was wiping the x-position lv_tileview_add_tile()
      had just assigned, leaving every tile stacked at (0,0) so the strip
      never really scrolled regardless of which page was "active." Tile
      position is now restored immediately after the style reset.
    - Fixes touch going permanently dead after light/deep sleep: the
      touch-quarantine latch had no timeout and could wait forever for a
      clean release that a noisy post-wake I2C bus might never deliver. It
      now force-clears after 2s and runs the existing (previously unused)
      touch-rail recovery routine.
    - Fixes activity sliders being hijacked into a page swipe after 2-3px of
      drag: their card containers are now scrollable (chaining disabled) so
      LVGL picks the card, not the page strip, as the drag target, letting
      the thumb's own drag handler keep the whole gesture.
    - Reduces random phantom touches, especially right after sleep/wake: a
      new touch-down now requires two consecutive matching samples instead
      of accepting a single sample, filtering out single-frame electrical/
      capacitive noise from the shared LCD/touch rail settling.

  2.36 - 2026-08-01
    - Restores the colour-corrected LCD staging band to internal DMA-capable
      memory. The accidental PSRAM staging introduced with the persistent page
      strip could transfer only the first and final invalidated bands reliably.
    - Keeps the FT5x06-compatible touch controller in active polling mode.
      Firmware 2.35 incorrectly selected monitor mode during initialisation,
      causing delayed, stale and phantom coordinates on this Rev 5 panel.

  2.35 - 2026-08-01
    - Keeps the persistent native LVGL page strip, but makes the destination
      tile visible before rendering its controls so LVGL does not discard the
      off-screen invalidation and leave only the slot wallpaper shell visible.
    - Restores firmware 2.09's proven single-frame FT5x06 sampling while keeping
      the clean-release quarantine used after UI rebuilds and display wake.

  2.34 - 2026-08-01
    - Keeps the persistent native LVGL page strip and its finger-following
      navigation introduced in 2.32.
    - Serialises each calibrated 32-row DMA transfer before reusing the shared
      colour-correction buffer, preventing all but the first LCD band from
      being overwritten while the parallel display bus is still transmitting.

  2.33 - 2026-08-01
    - Keeps the new persistent, finger-following LVGL tile strip from 2.32.
    - Replaces the unsupported single 153600-byte LCD DMA transfer with the
      proven pair of 32-row LVGL DMA buffers; rendering remains double-buffered
      and native LVGL, with no screenshot or pixel-push navigation fallback.
    - Keeps the FT5x06 awake during ordinary display/light sleep because its
      deep-hibernate register cannot reliably be released over I2C on Rev 5.
      LVGL gesture state is reset and touch is quarantined until a clean release.

  2.32 - 2026-08-01
    - Replaced PSRAM screenshots and delete/rebuild swipe transitions with a
      persistent native LVGL tile strip modelled on OMOTE's stable navigation.
    - Parks Wi-Fi safely during Chromecast BLE idle, then pauses BLE and fully
      recovers the Wi-Fi controller with retries whenever WebConfig is opened.
    - Hardened FT5x06 input with matching-frame validation, invalid-coordinate
      rejection, controller sleep/wake commands and a clean-release quarantine.
    - Moved LVGL object allocation to PSRAM and added OMOTE-style full-frame
      double buffering, with a safe partial-buffer fallback if PSRAM is absent.
    - Resets and quarantines LVGL input whenever a page tree is replaced so a
      stale swipe or release can never be delivered to newly-created controls.
    - Restored the proven 2.15 BLE/Wi-Fi handoff: Wi-Fi is fully stopped during
      BLE idle and cold-recovered before station scans or WebConfig startup.

  2.31 - 2026-08-01
    - Ported the focused device-page stability fix from firmware 2.10 onto the
      2.30 branch without changing its 2.09 BLE, Wi-Fi, touch or display core.
    - Defers device opening until the LVGL picker callback has returned, avoids
      unsafe snapshot rebuilding for large transient device pages, and reliably
      returns to the exact Home or activity page that opened the device.

  2.30 - 2026-08-01
    - Rebuilt from the proven 2.09 firmware base so its responsive DMA display,
      direct touch path and known-good Chromecast BLE/Wi-Fi coexistence remain
      unchanged.
    - Added the complete persistent Debug menu: split-line calibration, touch
      reticle/trail, CPU/RAM, accelerometer and FPS diagnostics, microphone
      source selection, and confirmed soft/full-system reboot controls.
    - Added live I2S microphone capture for Chromecast Voice Search while
      retaining the embedded Test Only phrase and blue hold-to-talk overlay.
    - Added the one-minute deep-sleep choice and current WebConfig theme-asset
      status/configuration fields. Retained 2.09's charger-aware battery history,
      two-decimal statistics and time-to-full/time-to-empty calculations.

  2.29 - 2026-08-01 - REJECTED / DO NOT USE
    - BLE reconnection and Wi-Fi availability could fail after starting a BLE
      activity. Firmware 2.30 replaces it with the 2.09 radio foundation.

  2.09 - 2026-07-30
    - Replaced release-triggered horizontal page changes with a direct-manipulation
      page strip that follows the user's finger throughout the swipe.
    - Added PSRAM-backed previous/current/next page previews with natural snap-back
      and snap-forward behavior, including reversing direction before release.
    - Cancels the underlying tile or button press as soon as page dragging begins,
      preventing accidental commands while changing pages.

  2.08 - 2026-07-30
    - Fixed forgotten Wi-Fi networks remaining selected as automatic connection
      targets and delaying the WebConfig setup AP with an empty password.
    - QR setup now attempts station mode only for a real saved profile and
      immediately starts the open OpenRemote setup AP when none is available.
    - Forgetting the active network now disables station auto-reconnect and
      cleanly changes the live QR/WebConfig transport back to setup AP mode.

  2.07 - 2026-07-30
    - Replaced the synchronous Arduino_GFX LCD transfer path with the official
      OMOTE Rev 5 LovyanGFX 40 MHz parallel DMA driver and double buffering.
    - Restored anti-aliased LVGL Montserrat text for substantially cleaner
      labels while retaining all existing sizes, layouts and symbols.
    - Moved awake LVGL input/render servicing to the start of every loop and
      restored OMOTE's standard scroll threshold and momentum for smoother,
      more immediate page movement.
    - Preserved RGB565/RGB666 output, gamma/saturation calibration, inversion,
      display sleep, deep sleep and the existing OpenRemote feature set.

  2.06 - 2026-07-29
    - Fixed the deep-sleep deadline timer being treated as movement whenever a
      Rev 5 wake input briefly deferred entry into deep sleep.
    - Restored the LIS3DH high-pass reference sequence for deep sleep so the
      stationary gravity vector cannot continuously assert its wake interrupt.
    - Keeps the display, LCD controller and backlights off while retrying deep
      sleep, so only real accelerometer or keypad input can wake the screen.
    - Added explicit serial diagnostics for every condition that can postpone
      deep sleep, making power-state failures visible without waking the LCD.

  2.05 - 2026-07-28
    - Added authenticated WebConfig Clock controls that mirror the LCD Clock
      page: status-bar visibility, Internet time, city/UTC offset and manual
      date/time now read from and write directly to the remote.
    - Added WebConfig Wi-Fi discovery, saved-password reconnect, forget and
      password-based connection APIs without exposing stored passwords.
    - Preserved the open setup AP while WebConfig scans for or joins a home
      network, so a new remote can be configured from a phone or computer.
    - Made the captive splash link directly to 192.168.4.1 and added explicit
      fallback text for Apple captive windows that cannot open the full browser.
    - Increased the USB SD-file idle allowance so large WebConfig transfers do
      not fail while the remote services its display and serial diagnostics.
    - Matched the USB parser budget to Studio's 1 KB transfer window so every
      chunk is acknowledged before display or sleep work can interrupt it.
    - Suspended light/deep sleep while a Studio USB SD-file transfer is active,
      then restored the user's normal sleep behavior after completion.

  2.04 - 2026-07-28
    - Fixed the BLE-connected Chromecast idle loop that immediately woke the
      display after every fade by replacing stale raw-delta checks with a
      settled, sustained orientation-angle measurement.
    - Recalibrated Motion sensitivity from a 60-degree pickup threshold at the
      least-sensitive end to the previous 3-degree threshold at the most-
      sensitive end, and applied the same range to normal sleep wake interrupts.
    - Removed BLE-idle wake-source cleanup for GPIO sleep sources that were not
      armed, eliminating the repeated ESP-IDF "Incorrect wakeup source" error.

  2.03 - 2026-07-28
    - Added OpenRemote Studio USB provisioning for an SD card installed inside
      the remote, including card remount/folder bootstrap and protected writes
      for WebConfig, recovery firmware, version metadata and Default icons.
    - Increased the cooperative USB transfer window and file-size limit so
      multi-megabyte WebConfig and firmware files install at practical speed.
    - Added an embedded firmware-version marker so Studio can identify selected
      PlatformIO application binaries without relying on their filenames.

  2.02 - 2026-07-28
    - Made charger state select Estimated until full or Estimated until empty,
      with completed-charge retention because Rev 5 exposes CRG_STAT but no
      separate VBUS-present signal to the ESP32-S3.
    - Reset the Last Hour measurement whenever the charger is connected or
      removed, while preserving the independent Last 24 Hours history.
    - Based the runtime estimate directly on the measured one-hour percentage
      change and rounded estimates over 24 hours to whole day/hour text.

  2.01 - 2026-07-28
    - Increased the LCD About-page precision to two decimal places for Battery
      Level, Rate, Last Hour and Last 24 Hours, including live refreshes.

  2.00 - 2026-07-28
    - Matched the official OMOTE Rev 5 deep-sleep pin isolation sequence:
      powers off LCD/touch and SD, parks their buses, and closes the shared
      I2C bus only after the accelerometer and keypad wake sources are armed.
    - Holds every power-gate, wake and active-low output at its safe sleep
      level to prevent peripheral back-feeding during long standby periods.
    - Keeps the proven activity-aware behavior: IR-only activities may enter
      light/deep sleep, while Chromecast activities retain BLE HID at 80 MHz.
    - Consolidated PlatformIO on one pinned, stable Rev 5 build target with
      timestamped serial output and automatic ESP32 crash decoding.

  1.99 - 2026-07-27
    - Added a pinned PlatformIO build whose ESP32-S3 framework provides
      tickless scheduling and Bluetooth controller modem sleep.
    - BLE-connected idle now keeps HID, IR and instant motion/button wake
      available while dropping idle CPU speed to a UART-safe 80 MHz.
    - USB serial remains available during BLE idle for Studio and diagnostics.
    - Stops both backlight PWM channels after the fade. Live Rev 5 testing
      keeps the LCD/touch rail powered until deep sleep so its shared I2C bus
      cannot interfere with accelerometer wake; the SD card remains mounted.

  1.98 - 2026-07-24
    - Changed the physical Voice Search overlay from red to OpenRemote blue.
    - Pre-creates the overlay at boot and defers its first redraw beyond the
      time-critical Chromecast MIC_OPEN window, restoring in-app voice search.

  1.97 - 2026-07-24
    - Added a large animated microphone overlay while Voice Search is held from
      a physical button; release removes it without disturbing the active page.
    - Paired with WebConfig 2.04, which removes every reference to a deleted
      device from activities, macros, LCD tiles, delays and physical bindings.

  1.96 - 2026-07-24
    - Replaced placeholder silent Voice Search data with a real 8 kHz mono
      IMA-ADPCM test phrase generated from the supplied "Hello this is a test"
      recording, using the negotiated 20-byte ATVV audio frame size.
    - Added standards-compliant AUDIO_SYNC decoder state before the first frame
      and before continuing with silence after the phrase finishes.

  1.95 - 2026-07-24
    - Fixed an unsigned timestamp underflow that could send ATVV AUDIO_STOP in
      the same millisecond a physical key started Hold-to-Talk.
    - Refreshes time before ATVV service and keeps a genuinely held voice key
      live until release, retaining a two-minute lost-release safety ceiling.

  1.94 - 2026-07-23
    - Moved ATVV audio-frame notifications to a dedicated core 0 worker so the
      core 1 keypad loop can observe physical Voice Search release immediately.
    - Polls the TCA8418 event FIFO throughout a physical voice hold even if its
      interrupt line is not asserted, with serialized audio/control BLE writes.

  1.93 - 2026-07-23
    - Made physical Voice Search release follow the raw TCA8418 matrix key that
      started the hold instead of re-resolving a possibly changed page binding.
    - Added physical voice key-edge diagnostics and guaranteed cleanup on BLE
      disconnect, preventing valid key-up events from falling into the timeout.

  1.92 - 2026-07-23
    - Implemented complete ATVV control packets: AUDIO_START now includes the
      trigger, codec and stream ID; AUDIO_STOP now includes its stop reason.
    - Added GET_CAPS/CAPS_RESP negotiation and real Hold-to-Talk behavior when
      supported, with valid silent ADPCM frames until the I2S mic is installed.

  1.91 - 2026-07-23
    - Fixed a late-MIC_OPEN race shown by live Chromecast logs: every valid
      microphone-open request is now acknowledged even if button-up arrived first.
    - Preserves host events across session cleanup and guarantees a deferred
      release sends AUDIO_START followed by AUDIO_STOP instead of latching the mic.

  1.90 - 2026-07-23
    - Corrected Chromecast Voice Search timing so physical/LCD press sends a
      complete Assistant HID click immediately, allowing MIC_OPEN while held.
    - Added an ordered ATVV session: acknowledge MIC_OPEN with AUDIO_START,
      defer release until the microphone is open, then send AUDIO_STOP without
      leaving Android TV's microphone indicator latched on.

  1.89 - 2026-07-23
    - Restricted smart-power memory updates to activity-controlled transitions:
      activity startup records devices on and Activity All Off records them off.
    - Manual LCD, physical-button, WebConfig-test and standalone macro power
      commands still transmit normally without changing remembered power state.

  1.88 - 2026-07-22
    - Changed Voice Search from a repeatable one-shot command into a real held
      ATVV microphone button for both physical assignments and LCD tiles.
    - Sends one START_SEARCH plus Assistant HID key-down on press, suppresses
      all hold repeats, then sends HID key-up and AUDIO_STOP on release so the
      Chromecast leaves listening mode and begins processing.

  1.87 - 2026-07-22
    - Moved the synchronous WebConfig HTTP server onto a low-priority task on
      ESP32-S3 core 0 so phone page loads and sync uploads cannot starve the
      core 1 LVGL, touchscreen, keypad and sleep loop.
    - Gave the HTTP worker sole ownership of server begin, stop, client and
      request handling, including clean cancellation when leaving WebConfig.
    - Made runtime sync uploads cooperative and deferred model reload until the
      response has completed, keeping the remote responsive during SD writes.

  1.86 - 2026-07-22
    - Kept station Wi-Fi alive while the Wi-Fi and WebConfig pages are open so
      both pages report the same live connection instead of contradictory state.
    - Added up to eight saved Wi-Fi profiles with Use saved password and
      Forget & enter password actions on the LCD.
    - Made Wi-Fi scans preserve an existing connection, remove hidden/blank and
      duplicate mesh entries, and reconnect automatically when required.
    - Disabled station modem sleep during an active WebConfig session and added
      automatic reconnect/AP fallback if the station connection drops.

  1.85 - 2026-07-22
    - Added the Android TV Voice-over-GATT (ATVV) service beside the existing
      Chromecast HID service, with TX/RX/CTL characteristics and diagnostics.
    - Added an assignable Voice Search command using ATVV START_SEARCH followed
      by the Android TV Assistant consumer usage 0x0221.
    - Reserved a Rev 5 microphone wiring plan that powers the I2S microphone
      from GPIO45 and reuses the deselected SD bus only during future capture.

  1.84 - 2026-07-21
    - Fixed the BLE-connected idle wake regression that could leave the remote
      paired to Chromecast with a permanently dark, unresponsive display.
    - Keeps the LCD/touch and SD rails powered in BLE idle, while retaining the
      dark backlights, stopped IR rail, disabled Wi-Fi and relaxed HID interval.
    - Restores the responsive BLE profile and full CPU speed before waking the
      display so movement and physical keys remain dependable.

  1.83 - 2026-07-21
    - Power-gated the complete LCD/touch and SD-card rails while a BLE activity
      is idle, instead of leaving those peripherals powered behind a dark
      backlight while the Chromecast HID connection remains active.
    - Restores CPU speed, remounts SD and fully initialises the LCD behind an
      off backlight before showing the first frame, preserving movement wake,
      physical-button wake and mixed BLE/IR activity control.
    - Requests a relaxed BLE connection interval and slave latency only while
      idle, then restores the responsive HID connection profile on wake.

  1.82 - 2026-07-21
    - Added a BLE-connected activity idle mode that keeps Chromecast HID
      continuously connected after the normal LCD timeout without entering
      ESP32 light sleep or deep sleep.
    - Turns off unused Wi-Fi, keeps the existing dark LCD/backlight state and
      reduces CPU speed only while idle, then restores full speed before the
      screen, touch, IR and physical-button path resumes on movement or input.
    - Mixed BLE and IR activities retain normal IR control; IR-only activities
      continue to use the full light-sleep and deep-sleep power-saving path.

  1.81 - 2026-07-21
    - Kept the bonded BLE HID link active for the full lifetime of any activity
      or device page that uses Bluetooth, including while the LCD is asleep.
    - Deferred light sleep and deep sleep until the BLE activity is changed or
      powered off, preventing Chromecast playback glitches from disconnects.

  1.80 - 2026-07-21
    - Made QR-page Back cancel any in-flight WebConfig response before shutting
      down Wi-Fi, preventing a disconnected browser from trapping the LCD UI.
    - Kept LVGL touch, keypad and power-hold handling alive during runtime JSON
      transfers as well as the main WebConfig HTML transfer.

  1.79 - 2026-07-21
    - Fixed activity-page swipes being blocked by a stale Settings subpage and
      made navigation back to Settings always open the Settings home menu.
    - Changed QR-page sleep to a 15-minute awake grace period followed by the
      saved screen timeout, then starts deep-sleep timing after screen-off.

  1.78 - 2026-07-21
    - Unified main Activities-page sliders with nested activity-page sliders so
      both use the same icon scale, card height, text position and row spacing.

  1.77 - 2026-07-21
    - Added Settings > Buttons beneath Display with a global physical-key Repeat
      switch, adjustable first-repeat delay and 1-20 repeats-per-second speed.
    - Added a non-transmitting Test Button mode that pulses the pressed key name
      and status pill bright red using the configured delay and repeat timing.
    - Moved SD-card mount capacity to About and protected settings sliders from
      accidental value changes while vertically scrolling their pages.

  1.76 - 2026-07-21
    - Fixed temporary device pages becoming trapped when a stale Settings view
      or activity-slider drag flag suppressed their raw horizontal exit swipe.
    - Device pages now always accept either horizontal exit direction while
      retaining the brightness, modal, lock and vertical-scroll safeguards.

  1.75 - 2026-07-21
    - Added real standard-keyboard 0-9 BLE commands for Android Button Mapper,
      available to LCD tiles and physical-button assignments in WebConfig.
    - Kept the stable single BLE Report characteristic by combining its existing
      Consumer Control field with a six-key keyboard field in one HID report.

  1.74 - 2026-07-21
    - Fixed the root Chromecast command panic in ESP32 BLE 3.3.10: its HID
      helper cannot safely register two same-UUID Report characteristics and
      left the Consumer Control report pointing at an uninitialised service.
    - Uses the single Consumer Control report required by the complete Android
      TV profile and translates any saved legacy navigation key usages to it.

  1.73 - 2026-07-21
    - Fixed the confirmed BLE HID crash by retaining one valid server and HID
      characteristic graph for the whole boot instead of deleting it whenever
      Bluetooth becomes temporarily unnecessary, during sleep or for setup Wi-Fi.
    - Added a post-pair connection grace period and clean suspend/resume path so
      Chromecast cannot be disconnected in the loop immediately after pairing.

  1.72 - 2026-07-21
    - Fixed the post-pair Chromecast crash by removing the unsafe live teardown
      and rebuild of every runtime object while BLE and LVGL still referenced it.
    - Applies Android TV HID navigation mappings during normal runtime loading
      and patches connected command objects in place after saving any migration.

  1.71 - 2026-07-21
    - Changed Chromecast/Google TV direction, OK, Home and Back commands from
      ordinary keyboard keys to 16-bit Consumer Control menu-navigation usages
      so physical controls also work inside Android TV on-screen keyboards.
    - Migrates already-saved Chromecast BLE commands in runtime.json and sends
      every Consumer Control press for 20 ms followed by a zero-value release.

  1.70 - 2026-07-21
    - Changed battery time-to-full and time-to-empty estimates of 24 hours or
      longer to friendly day/hour text, for example "14 days 12 hours".

  1.69 - 2026-07-20
    - Ported OMOTE Community's proven Rev 5 deep-sleep shutdown sequence:
      neutralise the complete LCD parallel bus before removing panel power,
      leave the LCD power controls high-impedance, and fully isolate SD/IR.
    - Changed deep-sleep motion wake to the Rev 5 active-low latched LIS3DH
      configuration and one EXT1 mask shared by motion and keypad interrupts.
    - Kept the responsive existing light-sleep lift detector before the selected
      deep-sleep timeout, then uses OMOTE's conservative wake threshold to stop
      tiny idle vibrations repeatedly rebooting the remote overnight.

  1.68 - 2026-07-20
    - Added a persistent LCD Deep Sleep slider in five-minute steps from 5 to
      30 minutes, directly beneath the normal display sleep timer.
    - Kept both backlights dark during boot and deep-sleep restoration until
      LVGL has drawn the first complete frame, removing the white wake flash.
    - Refreshed visible About/Battery statistics in place every second without
      changing scroll position, with a five-sample smoothed runtime estimate.

  1.67 - 2026-07-20
    - Used the MAX17048 live CRATE register for immediate charging/discharging
      estimates, with dynamic time-to-full/time-to-empty labels and h/min text.
    - Renamed the LCD battery percentage row from Charge to Battery Level.

  1.66 - 2026-07-20
    - Adopted ST's documented high-pass wake recipe, enabling only X/Y/Z high
      threshold events so negative comparator flags cannot hold INT1 active.

  1.65 - 2026-07-20
    - Re-armed light sleep internally after the LIS3DH's identifiable all-axis
      startup event, keeping the display off until real movement or a keypress.

  1.64 - 2026-07-20
    - Let the LIS3DH collect fresh stationary samples before establishing its
      high-pass reference, eliminating the all-axis startup wake transient.

  1.63 - 2026-07-20
    - Captured LIS3DH and TCA8418 interrupt sources immediately after GPIO
      wake so motion and keypad wake events can be distinguished reliably.

  1.62 - 2026-07-20
    - Kept both light-sleep wake inputs on the ESP32-S3 per-pin GPIO wake
      controller, avoiding false motion wakes caused by RTC-muxing ACC_INT.

  1.61 - 2026-07-20
    - Changed LIS3DH lift wake to a direct non-latched interrupt so a transient
      before CPU sleep cannot be preserved and cause an immediate false wake.

  1.60 - 2026-07-20
    - Kept the LIS3DH wake high-pass filter in stable normal mode instead of
      auto-reset mode, preventing the filter from retriggering after sleep.

  1.59 - 2026-07-20
    - Added a short sustained-motion requirement and a calmer LIS3DH threshold
      curve so an ordinary lift wakes promptly without idle false-wake loops.

  1.58 - 2026-07-20
    - Split opposite-polarity light-sleep wake inputs across the ESP32-S3 RTC
      EXT1 controller for LIS3DH motion and GPIO wake for the TCA8418 keypad.

  1.57 - 2026-07-20
    - Added the TCA8418-required GPIO interrupt-status reads before clearing
      INT_STAT, allowing the dedicated power-button GPI event to release INT.

  1.56 - 2026-07-20
    - Cleared every TCA8418 interrupt-status flag after draining its key FIFO,
      preventing a stale active-low keypad interrupt from blocking real sleep.

  1.55 - 2026-07-20
    - Replaced the conflicting generic GPIO light-sleep wake source with the
      ESP32-S3 RTC EXT1 controller, independently waking high from LIS3DH INT1
      or low from the TCA8418 keypad interrupt.

  1.54 - 2026-07-20
    - Moved live voltage, charge rate, hourly/daily change and runtime estimate
      into a Battery section on the LCD About page.
    - Retuned the LIS3DH sleep interrupt for gentle, slow lifting using its
      low-cutoff high-pass filter, a lower threshold and immediate detection.

  1.53 - 2026-07-20
    - Added real staged power saving: LCD/controller sleep followed by ESP32-S3
      light sleep with instant LIS3DH motion or keypad wake, then motion-wake
      deep sleep after ten minutes without movement.
    - Changed Wi-Fi and BLE to on-demand radios. WebConfig is served only while
      its QR page is open, NTP syncs at boot and once daily, Homebridge wakes
      Wi-Fi only for commands, and BLE runs only for pairing or BLE activities.
    - Added persistent battery history, voltage, hourly/daily usage and runtime
      estimates to the LCD Battery page and WebConfig status API.
    - Added the configured remote name and installed firmware version to the
      live WebConfig status area.
    - Preserved active pages and smart-power memory through deep sleep, and
      made the scheduled 3am clock refresh return directly to deep sleep.

  1.52 - 2026-07-19
    - Made macro tiles on activity pages execute their saved command and delay
      sequence instead of behaving as unbound command buttons.
    - Added non-blocking runtime support for nested macros so touch, Wi-Fi and
      display updates remain responsive while a macro runs.

  1.51 - 2026-07-19
    - Preserved nested activity items in the LCD runtime model instead of
      flattening them into unbound command buttons.
    - Rendered activity-page activity items as full-width slide controls using
      their selected icons, boundary settings and real activation sequences.

  1.50 - 2026-07-19
    - Replaced the RAM-heavy WebConfig JSON sync path with a streamed SD upload,
      PSRAM validation and atomic runtime-config swap with automatic rollback.
    - Delayed runtime reload until the sync response has cleared and reset stale
      activity/device page indexes before rebuilding the live LCD model.

  1.49 - 2026-07-19
    - Clamped MAX17048 fuel-gauge percentages to 100% so charger or gauge
      overshoot can never display or report battery levels above full.

  1.48 - 2026-07-19
    - Removed ESP32 light sleep and all display-timeout radio, IR-rail and LCD
      controller power transitions to eliminate sleep and WebConfig crashes.
    - Kept the user display timeout, backlight fade and instant motion/key wake;
      screen-off now leaves the processor and every peripheral fully running.

  1.47 - 2026-07-19
    - Streamed the SD-card WebConfig HTML cooperatively so loading WebConfig no
      longer starves touch, keypad, sleep timers or the watchdog.
    - Let the WebConfig QR page use the saved display sleep timeout when the
      remote is already on home Wi-Fi, while keeping setup-AP sessions alive.

  1.46 - 2026-07-19
    - Stabilised the LIS3DH high-pass filter and required both wake interrupt
      lines to be idle before entering light sleep, preventing instant wake.
    - Persisted LCD setting changes to runtime.json as well as Preferences so a
      restart cannot restore older WebConfig brightness or timeout values.
    - Added activity delays that run only when a selected smart-power device
      was actually turned on by the current activity sequence.

  1.45 - 2026-07-19
    - Added LIS3DH INT1 and keypad GPIO wake sources for true ESP32-S3 light
      sleep, while retaining instant movement and physical-button wake.
    - Put the ILI9341 into controller sleep and suspended Wi-Fi and BLE during
      normal display sleep, restoring them after the LCD is visibly awake.
    - Cached current-page PNG icons in PSRAM and removed full-page opacity
      animation to improve Smooth Blue performance with several activities.
    - Reduced redundant clock, battery and status-pill redraw and I2C work.

  1.44 - 2026-07-19
    - Streamed WebConfig runtime downloads from a PSRAM buffer instead of making
      a second large internal-heap String copy before each HTTP response.
    - Added an explicit response length and bounded network writes so browsers
      reliably receive the complete device, activity and Screen Designer model.

  1.43 - 2026-07-19
    - Added firmware-side smart power memory for toggle-only and discrete-power
      devices so shared equipment is not toggled again when changing activities.
    - Preserved remembered power state across WebConfig runtime reloads and made
      direct LCD, physical-key and WebConfig command tests update that state.
    - Changed hardware All Off to transmit only for devices remembered as on,
      then clear their state so repeated All Off presses remain harmless.

  1.42 - 2026-07-19
    - Replaced the simulated Homebridge picker with authenticated local discovery
      through Homebridge Config UI X and real writable-characteristic commands.
    - Added persistent NVS-backed Homebridge connection details, automatic token
      renewal and direct execution from LCD tiles, activities, macros, physical
      buttons and WebConfig command-test pills.
    - Added a filtered discovery proxy so browsers do not need cross-origin
      access to Homebridge and passwords are never written to runtime.json.
    - Added real parsed NEC payloads to Apple TV Learn Remote profiles so every
      generated navigation command can be taught to an Apple TV.

  1.41 - 2026-07-19
    - Added alias-aware smart physical-button defaults for every device type,
      including Studio IRDB, learned IR, Bluetooth and WebConfig devices.
    - Mapped common navigation, transport, volume, channel, menu/info, record
      and colour commands only where the matching hardware key is unassigned.
    - Preserved user overrides with a per-device initialization marker so a
      deliberately cleared or changed WebConfig binding is never recreated.
    - Treated every legacy device page as already initialized, preserving old
      blank assignments while new Studio devices without a page still map once.

  1.40 - 2026-07-19
    - Automatically adds a persistent Chromecast / Google TV BLE HID device,
      complete command page and useful physical-button defaults after pairing.
    - Preserved a known paired host across restarts and advertised for automatic
      BLE HID reconnection even when the ESP32 bond query is temporarily empty.
    - Removed the completed Buttons keypad diagnostic from the LCD device picker
      and runtime page model.

  1.39 - 2026-07-19
    - Disabled the hardware power button's short-press All Off action when no
      activity or device is active, preventing duplicate power commands after
      All Off has already returned the remote to the Activities home page.
    - Kept the seven-second hardware power hold available globally for reboot.

  1.38 - 2026-07-19
    - Added a simple LCD Backup / Restore menu that creates native full backups
      and lists compatible full backups by their creation date and time only.
    - Preserved runtime configuration, file-backed devices, activities, macros,
      custom/default themes and custom icons in remote-created SD backups.
    - Added direct restore support for both native remote backups and WebConfig
      full-backup JSON files, followed by an immediate runtime reload.
    - Removed the Invert display row from the LCD Display settings page while
      retaining compatibility with already-saved display state and API data.

  1.37 - 2026-07-19
    - Added persistent LCD gamma, saturation, RGB565/RGB666 transfer and display
      inversion controls to the scrollable Display settings page.
    - Applied gamma and saturation calibration to every LVGL display flush and
      added a real 18-bit ILI9341 panel-transfer path for RGB666 mode.
    - Replaced unfiltered SD backup listings with full-backup summaries and a
      protected delete endpoint for WebConfig's SD Card backups manager.

  1.36 - 2026-07-19
    - Corrected all Rev 5 keypad bindings from the photographed 23-key raw
      diagnostic map, including the swapped D-pad Up and OK assignments.
    - Limited the activity boundary option to the thin outer slider outline;
      activity icons no longer receive a coloured thumb box.

  1.35 - 2026-07-19
    - Replaced the placeholder BLE service with a bonded keyboard and consumer
      control HID remote compatible with Chromecast, Google TV and Android TV.
    - Added real Bluetooth pairing, reconnect and forget controls to the LCD
      Settings menu and authenticated WebConfig API.
    - Routed saved Bluetooth device commands through real HID key reports so
      LCD tiles, activities, hardware bindings and WebConfig tests all work.

  1.34 - 2026-07-19
    - Executed real command and delay steps when an activity slider completes.
    - Made device pages temporary, removable with either horizontal exit swipe.
    - Added a firmware-only Buttons diagnostic page for raw Rev 5 key mapping.
    - Added a global seven-second hardware power hold to reboot the ESP32.
    - Redrew sleeping LCDs immediately after a successful WebConfig sync.
    - Added SD backup-file APIs and a non-destructive user factory reset.

  1.33 - 2026-07-19
    - Honoured global and per-item boundary settings on activity sliders,
      command buttons and macros, including the activity icon thumb.
    - Removed the TV device chip and made the top-left page title open Devices.
    - Improved long-page scrolling and added a short directional page animation.
    - Applied the complete WebConfig icon and text sizing ranges on the LCD.

  1.32 - 2026-07-18
    - Added authenticated WebConfig command testing, reboot and SD-backed
      device removal, plus live runtime device/activity counts.
    - Preserved learned-device provenance and file-backed device aliases in
      the synced runtime model.
    - Flashed the LCD clock/battery pill red for every real command transmit,
      including each held-button IR repeat.

  1.31 - 2026-07-18
    - Fixed SD PNG white boxes and corrupted wallpaper strips by returning the
      LVGL image cache to one entry and invalidating wallpaper buffers safely.
    - Enabled the Rev 5 S17/TCA8418 ROW5 power switch as a global All Off key
      that powers down the current activity's devices and closes the activity.
    - Made every assigned hardware IR button repeat while physically held;
      LCD command tiles remain single-send.

  1.30 - 2026-07-18
    - Reworked LCD IR hold handling around LVGL long-press events and preserved
      repeat choices in the synced screen-designer model.
    - Replaced the unlock page rebuild with an overlay that disappears
      immediately at the end of the slide without a white redraw.
    - Applied synced theme split positions, glass colour/transparency and
      boundary visibility, and enabled SD-backed PNG activity/button icons.
    - Separated theme assets into /themes/Default and /themes/Custom.

  1.29 - 2026-07-18
    - Synced designer spacer slots to the LCD, retained vertical page scrolling,
      fixed held IR repeat across LVGL scroll handoff and cached active themes
      for an immediate, flash-free slide unlock.
    - Simplified About to firmware/WebConfig versions and read the WebConfig
      version directly from /www/index.html metadata.
    - Added validated SD staging and explicit installation for firmware updates,
      while preserving progress-capable WebConfig replacement as index.html.

  1.28 - 2026-07-18
    - Moved the large runtime device, activity, tile and physical-binding tables
      into PSRAM, restoring the internal heap required by the ESP32 Wi-Fi driver
      and fixing the white-screen reboot loop introduced in 1.27.

  1.27 - 2026-07-18
    - Fixed real IR learning timing, added the Rev 5 TCA8418 physical-key
      scanner and shared held-repeat handling for LCD and hardware buttons.
    - Preserved WebConfig designer pages and synced page themes, icons,
      boundary boxes, LVGL-matched font sizes and physical assignments.
    - Served WebConfig on an existing Wi-Fi connection without replacing it
      with the setup AP, and added the connected SSID/IP beneath the QR code.
    - Added the live battery percentage to the brightness control.

  1.26 - 2026-07-18
    - Kept the setup AP alive through WebConfig sync and deferred runtime reload
      until after the HTTP response, preventing 0.0.0.0 and lost-sync errors.
    - Added real GPIO4 IR learning through the Rev 5 IRM-V838M3 receiver,
      powered by GPIO6, with nonblocking WebConfig capture endpoints.
    - Raised imported device capacity to 50 commands and rendered every command
      on each vertically scrollable LCD device page.
    - Changed QR-page sleep to ten minutes, added a lightweight captive splash,
      SD-backed default/custom icon APIs and live battery status support.

  1.25 - 2026-07-18
    - Loaded Studio-imported /devices/*.ir files directly into the LCD runtime
      and merged file-backed device summaries into GET /api/config so WebConfig
      displays them without storing them in /config/runtime.json.

  1.24 - 2026-07-18
    - Changed Studio USB uploads to a 192-byte acknowledged flow-control
      protocol so 460800-baud IR-device transfers cannot overrun UART or SD.
    - Redirected direct station-IP WebConfig visits to the authenticated token
      URL so Studio can return to fast IP-based Remote Config connections.

  1.23 - 2026-07-18
    - Raised the OpenRemote Studio runtime USB/UART protocol from 115200 to
      460800 baud to match the proven Arduino upload connection and reduce
      WebConfig and IR-device transfer time.

  1.22 - 2026-07-18
    - Added USB status and runtime-config read/write commands so WebConfig
      loaded by Studio can display live remote state and synchronise real
      settings, devices, activities and macros back to /config/runtime.json.

  1.21 - 2026-07-18
    - Replaced blocking Studio USB payload reads and SD-file streaming with
      incremental transfers so touch, display sleep, Wi-Fi and LVGL continue
      running while Studio sends a device or loads WebConfig.
    - Added framed WebConfig download chunks and interrupted-transfer cleanup
      so debug output and disconnected USB clients cannot corrupt or wedge a
      transfer.

  1.20 - 2026-07-18
    - Added /devices/index.txt maintenance for Studio USB .ir imports so the
      remote reports saved IR device-file counts reliably even when SD directory
      iteration does not expose a newly written file immediately.

  1.19 - 2026-07-18
    - Listened for OpenRemote Studio USB commands on both native USB CDC and
      UART0 when USB CDC On Boot is enabled, so /dev/cu.usbserial bridge ports
      and /dev/cu.usbmodem native CDC ports both work.

  1.18 - 2026-07-18
    - Changed OpenRemote Studio USB imports to save selected IRDB devices as
      real .ir files in /devices on the SD card instead of editing
      /config/runtime.json.
    - Added USB SD-file streaming so Studio can load /www/index.html from the
      remote over USB.

  1.17 - 2026-07-18
    - Normalised OpenRemote Studio USB-imported IRDB devices before saving them
      so WebConfig and the runtime both see the same device schema.
    - Added the saved runtime device count to USB import replies for clearer
      Studio verification after sending a device.

  1.16 - 2026-07-18
    - Added USB serial device import for OpenRemote Studio. Studio can now send
      one selected IRDB device over USB CDC and the remote merges it into
      /config/runtime.json without using captive Wi-Fi for IRDB search.

  1.15 - 2026-07-18
    - Closed chunked /api/irdb/search responses with the required terminating
      chunk so WebConfig stops waiting at "Searching..." and renders results.

  1.14 - 2026-07-18
    - Added lightweight /api/irdb/search and /api/irdb/detail endpoints backed
      by an SD-card JSONL search index and per-device detail files so WebConfig
      sends only the search term and fetches full IR payloads only for the
      selected device.
    - Kept the legacy /api/irdb file endpoint for backups and database updates.

  1.13 - 2026-07-18
    - Streamed /api/irdb in small yielded chunks with a Content-Length header
      so WebConfig can show download progress without starving the ESP32 loop.
    - Refreshed the QR page setup AP client count once per second while visible.

  1.12 - 2026-07-18
    - Simplified setup Wi-Fi AP startup to the stock Arduino ESP32 open SoftAP
      path so iPhone/Mac clients can associate before captive portal handling.
    - Temporarily pauses BLE advertising while the setup AP is visible to reduce
      2.4 GHz coexistence during phone provisioning.

  1.11 - 2026-07-18
    - Refreshed the Clock settings page immediately when Internet time is
      switched on or off so city/manual controls appear without leaving page.
    - Renamed Raise to wake to Motion sensitivity and clamped it to 1-100%.
    - Made setup AP startup more conservative with explicit open-network,
      DHCP and captive-portal configuration plus a fallback open AP start.

  1.10 - 2026-07-18
    - Made WebConfig setup use exclusive AP-only radio mode on fixed channel 1
      so phones can reliably join the open network and trigger captive portal.
    - Made Wi-Fi discovery use an exclusive STA-only synchronous scan with
      station reconnect paused, then restore the saved connection on exit.
    - Recalibrated the Rev 5 MOSFET backlight curve with a lower visible floor
      and much wider adjustment through the middle and upper slider range.

  1.09 - 2026-07-18
    - Made the OpenRemote setup access point password-free and active only
      while the WebConfig QR page is open.
    - Disabled display sleep on the QR page and restarted the user's saved
      sleep countdown when leaving it.
    - Recalibrated LCD brightness so zero remains visibly lit and the useful
      adjustment range is spread across the full slider without PWM whine.
    - Isolated Wi-Fi scanning from setup-AP mode and added radio settling,
      timeout and automatic retry handling for real network discovery.

  1.08 - 2026-07-18
    - Restored the Rev 5 LCD backlight to active-low drive after hardware
      testing showed v1.07 left the panel dark.
    - Kept explicit LEDC channels but relaxed PWM to 25 kHz / 10-bit for
      better ESP32-S3 timer margin while staying above audible range.
    - Made the backlight fallback fail visibly on instead of dark if PWM attach
      fails.

  1.07 - 2026-07-18
    - Tried active-high LCD backlight PWM after the first brightness report;
      hardware testing showed this was wrong and v1.08 reverts it.
    - Re-linked the blue button LEDs to LCD wake/sleep state.
    - Updated Clock settings so Internet time is Wi-Fi gated and manual wheels
      are shown only when Internet time is off.
    - Added an Internet-time city entry page placeholder gated behind Wi-Fi.
    - Added SD card card-detect diagnostics on the settings screen and Serial.

  1.06 - 2026-07-18
    - Replaced the LVGL stock Wi-Fi password keyboard with a compact
      full-alphabet keypad that fits above the lower bezel on the real LCD.
    - Added captive-portal routes for iPhone setup-network joins and clearer
      WebConfig QR instructions.
    - Added a Clock settings page with status-bar switch, Internet time mode
      and manual date/time rollers.
    - Added IRDB build date and device-count metadata to About and /api/status.
    - Added lower-speed SD mount retries for cards formatted on macOS.

  1.05 - 2026-07-18
    - Raised the LCD and button backlight PWM frequency from 5 kHz to 30 kHz
      to eliminate audible switching noise at brightness levels below 100%.

  1.04 - 2026-07-18
    - Reset retained page scrolling before each view change so the Wi-Fi
      password keyboard cannot inherit the network list's scroll offset.
    - Moved the password keyboard upward and added a bottom touch margin so
      every letter row remains visible and reachable on the physical LCD.

  1.03 - 2026-07-18
    - Added real IR transmission for SD-synchronised Flipper raw and supported
      parsed commands using the Rev 5 active-low IR output.
    - Added native /irdb/OpenRemote.irdb download and upload APIs so WebConfig
      can search the real OpenRemote Studio database stored on the SD card.
    - Bound device and activity tiles to command IDs instead of visual-only
      labels.

  1.02 - 2026-07-17
    - Added persistent hardware-backed Settings, Display, About and lock screens.
    - Added real Wi-Fi scanning/connection, on-screen password keyboard, setup AP,
      captive WebConfig server and a scannable LCD QR code.
    - Added real ESP32-S3 BLE advertising controlled by the Bluetooth switch.
    - Added SD-served WebConfig, configuration sync, firmware OTA and WebConfig
      upload endpoints.
    - Replaced the fixed one-minute sleep and wake threshold with saved controls.
    - Added a Clock switch which resizes the status pill around the battery only.

  1.01 - 2026-07-17
    - Added Rev 5 microSD mounting and automatic OpenRemote folder bootstrap.

  1.00 - Initial LVGL cinematic runtime prototype.
*/

static constexpr float OPENREMOTE_VERSION = 2.38f;
static constexpr char OPENREMOTE_VERSION_TEXT[] = "2.38";
static constexpr char OPENREMOTE_FIRMWARE_MARKER[] =
  "OPENREMOTE_FIRMWARE_VERSION=2.37";

/*
  OpenRemote / OMOTE Rev 5 - LVGL cinematic runtime prototype

  This is the non-flickering version of the remote-side runtime. LVGL owns
  the UI objects and only flushes changed regions to the ILI9341. The static
  cinema wallpaper is stored in flash as RGB565 data.

  Hardware used:
    - ESP32-S3 OMOTE Rev 5 pin map
    - ILI9341 240x320 display, 8-bit parallel bus
    - FT6206/FT6236/CST026-compatible touch controller at 0x38
    - LIS3DH accelerometer at 0x19 for movement wake
    - MAX17048 fuel gauge at 0x36 for battery percentage

  Libraries:
    - LovyanGFX by lovyan03
    - lvgl, preferably v8.x for this sketch
*/

#define LV_CONF_INCLUDE_SIMPLE

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <Update.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <BLEHIDDevice.h>
#include <BLESecurity.h>
#if defined(CONFIG_BLUEDROID_ENABLED)
#include <esp_gap_ble_api.h>
#endif
#include <ctype.h>
#include <strings.h>
#include <sys/time.h>
#include <driver/gpio.h>
#include <driver/i2s_std.h>
#include <driver/uart.h>
#include <esp_pm.h>
#include <esp_rom_sys.h>
#include <esp_sleep.h>
#include <esp_sntp.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <ArduinoJson.h>
#define USE_ACTIVE_LOW_OUTPUT_FOR_SEND_PIN
#define NO_LED_SEND_FEEDBACK_CODE
#define NO_LED_RECEIVE_FEEDBACK_CODE
#define IR_SEND_PIN 5
#define IR_RECEIVE_PIN 4
#define RAW_BUFFER_LENGTH 750
#include <IRremote.hpp>
#include <LovyanGFX.hpp>
#include <lvgl.h>
#include "cinema_wallpaper_rgb565.h"
#include "atvv_test_audio.h"

class PsramJsonAllocator : public ArduinoJson::Allocator {
 public:
  void *allocate(size_t size) override {
    void *memory = psramFound() ? ps_malloc(size) : nullptr;
    return memory ? memory : malloc(size);
  }
  void deallocate(void *pointer) override { free(pointer); }
  void *reallocate(void *pointer, size_t size) override {
    void *memory = psramFound() ? ps_realloc(pointer, size) : nullptr;
    return memory ? memory : realloc(pointer, size);
  }
};

PsramJsonAllocator psramJsonAllocator;

// Retained for compatibility with older saved layouts. The live UI now uses
// LVGL's anti-aliased Montserrat fonts, matching the official OMOTE firmware.
LV_FONT_DECLARE(lv_font_montserrat_10);
LV_FONT_DECLARE(lv_font_montserrat_12);
LV_FONT_DECLARE(lv_font_montserrat_16);
LV_FONT_DECLARE(lv_font_montserrat_20);

// ---------------------------------------------------------------------------
// Rev 5 pin map
// ---------------------------------------------------------------------------

static const int PIN_LCD_EN = 38;   // active-low
static const int PIN_LCD_BL = 9;    // active-low
static const int PIN_BUTTON_BL = 46; // SW_BL, active-high blue button LEDs
static const int PIN_IR_LED = 5;    // active-low
static const int PIN_IR_RX = 4;     // IRM-V838M3 demodulated output
static const int PIN_IR_VCC = 6;    // receiver power, active-high
static const int PIN_CHARGE_STATUS = 1; // TP4056 CRG_STAT, active-low
static const int PIN_ACC_INT = 2;   // LIS3DHTR INT1, polarity set per sleep stage
static const int PIN_TCA_INT = 8;   // TCA8418 keypad interrupt, active-low
static const uint64_t DEEP_SLEEP_WAKE_MASK =
  (1ULL << PIN_ACC_INT) | (1ULL << PIN_TCA_INT);

// Rev 5 schematic nets. The SD card has its own SPI pins; do not reuse the
// handoff's early pin list, which overlaps the 8-bit LCD data bus.
static const int PIN_SD_MISO = 7;
static const int PIN_SD_SCK = 15;
static const int PIN_SD_EN = 16;   // active-low card power switch
static const int PIN_SD_MOSI = 17;
static const int PIN_SD_CS = 18;

// Rev 5 has no three independent spare GPIOs. Future I2S capture powers the
// mic only while SD is unmounted, then temporarily reuses the deselected SD
// test pads. GPIO45 already drives D3, which becomes a visible mic indicator.
static const int PIN_MIC_POWER = 45;
static const int PIN_MIC_BCLK = PIN_SD_SCK;
static const int PIN_MIC_WS = PIN_SD_MOSI;
static const int PIN_MIC_DATA = PIN_SD_MISO;

static const int PIN_I2C_SCL = 19;
static const int PIN_I2C_SDA = 20;

static const int PIN_LCD_CS = 39;
static const int PIN_LCD_DC = 40;
static const int PIN_LCD_WR = 41;
static const int PIN_LCD_RD = 42;
static const int PIN_LCD_RST = -1;

static const int PIN_LCD_D0 = 48;
static const int PIN_LCD_D1 = 47;
static const int PIN_LCD_D2 = 21;
static const int PIN_LCD_D3 = 14;
static const int PIN_LCD_D4 = 13;
static const int PIN_LCD_D5 = 12;
static const int PIN_LCD_D6 = 11;
static const int PIN_LCD_D7 = 10;

static const uint8_t ADDR_TOUCH = 0x38;
static const uint8_t ADDR_LIS3DH = 0x19;
static const uint8_t ADDR_MAX17048 = 0x36;
static const uint8_t ADDR_TCA8418 = 0x34;

static const int LCD_W = 240;
static const int LCD_H = 320;
static const uint32_t BLE_CONNECTED_IDLE_MAX_CPU_MHZ = 80;
// Keep APB at 80 MHz so the Rev 5 UART remains exactly 460800 baud. A 40 MHz
// CPU floor changes the UART peripheral clock in this board/core combination.
static const uint32_t BLE_CONNECTED_IDLE_MIN_CPU_MHZ = 80;
static const uint32_t BLE_CONNECTED_IDLE_POLL_MS = 100;
static const uint32_t BLE_CONNECTED_IDLE_MOTION_CONFIRM_MS = 80;
static const uint8_t MOTION_WAKE_LEAST_SENSITIVE_DEGREES = 60;
static const uint8_t MOTION_WAKE_MOST_SENSITIVE_DEGREES = 3;
static const uint8_t MOTION_WAKE_LEAST_SENSITIVE_THRESHOLD = 54; // 864 mg.
static const uint8_t MOTION_WAKE_MOST_SENSITIVE_THRESHOLD = 3;   // 48 mg.
static const uint16_t BLE_CONN_ACTIVE_MIN_INTERVAL = 12;  // 15 ms.
static const uint16_t BLE_CONN_ACTIVE_MAX_INTERVAL = 24;  // 30 ms.
static const uint16_t BLE_CONN_IDLE_MIN_INTERVAL = 96;    // 120 ms.
static const uint16_t BLE_CONN_IDLE_MAX_INTERVAL = 120;   // 150 ms.
static const uint16_t BLE_CONN_IDLE_LATENCY = 3;
static const uint16_t BLE_CONN_SUPERVISION_TIMEOUT = 600; // 6 seconds.
static const uint32_t BACKLIGHT_PWM_HZ = 25000;
static const uint8_t BACKLIGHT_PWM_BITS = 10;
static const uint32_t BACKLIGHT_PWM_MAX = (1UL << BACKLIGHT_PWM_BITS) - 1UL;
static const uint8_t LCD_BACKLIGHT_PWM_CHANNEL = 0;
static const uint8_t BUTTON_BACKLIGHT_PWM_CHANNEL = 1;
// Keep the lowest slider position above the Rev 5 backlight driver's visible
// threshold. The remaining duty range follows a perceptual curve.
static const uint32_t BACKLIGHT_MIN_VISIBLE_DUTY = 8;
static const uint16_t BACKLIGHT_FADE_MS = 500;
static const uint8_t BACKLIGHT_FADE_STEPS = 25;
static const uint32_t BRIGHTNESS_PANEL_TIMEOUT_MS = 4000;
static const uint16_t WIFI_SCAN_SETTLE_MS = 300;
static const uint16_t WIFI_SCAN_RETRY_DELAY_MS = 450;
static const uint8_t WIFI_SCAN_MAX_ATTEMPTS = 2;
static const uint8_t MAX_WIFI_PROFILES = 8;
static const uint8_t MAX_WIFI_SCAN_RESULTS = 12;
static const size_t IRDB_STREAM_CHUNK_BYTES = 1024;
static const size_t USB_IMPORT_MAX_BYTES = 4UL * 1024UL * 1024UL;
static const size_t USB_IO_CHUNK_BYTES = 192;
static const size_t USB_UPLOAD_WINDOW_BYTES = 1024;
static const uint16_t USB_IO_BUDGET_BYTES = USB_UPLOAD_WINDOW_BYTES;
static const uint32_t USB_UPLOAD_IDLE_TIMEOUT_MS = 30000;
static const uint32_t USB_STUDIO_BAUD = 460800;
static const uint32_t QR_PAGE_AWAKE_GRACE_MS = 15UL * 60UL * 1000UL;
static const uint32_t NTP_SYNC_TIMEOUT_MS = 60000UL;
static const uint32_t NETWORK_IDLE_SHUTDOWN_MS = 1200UL;
static const uint32_t BATTERY_SAMPLE_INTERVAL_SEC = 30UL * 60UL;
static const uint32_t IR_LEARN_TIMEOUT_MS = 12000;
static const uint16_t IR_LEARN_MAX_TIMINGS = 749;
static const uint16_t IR_REPEAT_DELAY_MS = 420;
static const uint16_t IR_REPEAT_INTERVAL_MS = 115;
static const uint16_t BUTTON_REPEAT_DELAY_MIN_MS = 100;
static const uint16_t BUTTON_REPEAT_DELAY_MAX_MS = 1500;
static const uint8_t BUTTON_REPEAT_RATE_MIN_HZ = 1;
static const uint8_t BUTTON_REPEAT_RATE_MAX_HZ = 20;
static const uint16_t STATUS_REFRESH_MS = 200;
static const uint16_t BATTERY_REFRESH_MS = 5000;
static const uint16_t CHARGE_STATE_DEBOUNCE_MS = 300;
static const uint16_t CHARGE_ANIMATION_FILL_MS = 2200;
static const uint8_t MAX_PAGE_ICON_CACHE = 12;
static const int16_t PAGE_SWIPE_THRESHOLD = 42;
static const int16_t PAGE_DRAG_START_PIXELS = 9;
static const int16_t PAGE_DRAG_COMMIT_PIXELS = 60;
static const uint16_t PAGE_DRAG_SETTLE_MS = 150;
static const uint16_t DNS_PORT = 53;
static const char *PREFERENCES_NAMESPACE = "openremote";
static const char *WEB_CONFIG_PATH = "/www/index.html";
static const char *FIRMWARE_STAGE_PATH = "/firmware/pending.bin";
static const char *RUNTIME_CONFIG_PATH = "/config/runtime.json";
static const char *RUNTIME_CONFIG_UPLOAD_PATH = "/tmp/runtime.upload.json";
static const char *RUNTIME_CONFIG_BACKUP_PATH = "/config/runtime.previous.json";
static const char *DEVICE_INDEX_PATH = "/devices/index.txt";
static const char *IRDB_PATH = "/irdb/OpenRemote.irdb";
static const char *IRDB_SEARCH_INDEX_PATH = "/irdb/search.jsonl";
static const char *IRDB_DETAIL_DIR = "/irdb/details";
static const char *IRDB_MANIFEST_PATH = "/irdb/manifest.json";
static const char *IRDB_ALT_MANIFEST_PATH = "/irdb/Database Manifest.json";
static const char *OPENREMOTE_TZ = "AEST-10AEDT,M10.1.0,M4.1.0/3";
static const char *BLE_HID_NAME = "OpenRemote HID";
static const char *ATVV_SERVICE_UUID = "AB5E0001-5A21-4F05-BC7D-AF01F617B664";
static const char *ATVV_TX_UUID = "AB5E0002-5A21-4F05-BC7D-AF01F617B664";
static const char *ATVV_RX_UUID = "AB5E0003-5A21-4F05-BC7D-AF01F617B664";
static const char *ATVV_CTL_UUID = "AB5E0004-5A21-4F05-BC7D-AF01F617B664";
static const uint8_t ATVV_AUDIO_STOP = 0x00;
static const uint8_t ATVV_AUDIO_START = 0x04;
static const uint8_t ATVV_START_SEARCH = 0x08;
static const uint8_t ATVV_AUDIO_SYNC = 0x0A;
static const uint8_t ATVV_GET_CAPS = 0x0A;
static const uint8_t ATVV_CAPS_RESP = 0x0B;
static const uint8_t ATVV_MIC_OPEN = 0x0C;
static const uint8_t ATVV_MIC_CLOSE = 0x0D;
#define OPENREMOTE_ATVV_DEBUG 1
static const uint32_t ATVV_SEARCH_TIMEOUT_MS = 5000UL;
static const uint16_t ATVV_RELEASE_SETTLE_MS = 40;
static const uint32_t ATVV_AUDIO_TIMEOUT_MS = 15000UL;
static const uint32_t ATVV_HELD_AUDIO_TIMEOUT_MS = 120000UL;
static const uint8_t ATVV_CODEC_ADPCM_8KHZ = 0x01;
static const uint8_t ATVV_INTERACTION_ON_REQUEST = 0x00;
static const uint8_t ATVV_INTERACTION_HOLD_TO_TALK = 0x03;
static const uint8_t ATVV_AUDIO_START_MIC_OPEN = 0x00;
static const uint8_t ATVV_AUDIO_START_HTT = 0x03;
static const uint8_t ATVV_AUDIO_STOP_MIC_CLOSE = 0x00;
static const uint8_t ATVV_AUDIO_STOP_BUTTON_RELEASE = 0x02;
static const uint8_t ATVV_AUDIO_STOP_NEW_STREAM = 0x04;
static const uint8_t ATVV_AUDIO_STOP_TIMEOUT = 0x08;
static const uint8_t ATVV_AUDIO_STOP_OTHER = 0x80;
static const size_t ATVV_AUDIO_FRAME_BYTES = 20;
static const uint16_t ATVV_AUDIO_FRAME_INTERVAL_MS = 5;
static const uint32_t BLE_PAIRING_WINDOW_MS = 3UL * 60UL * 1000UL;
static const uint32_t BLE_POST_CONNECT_GRACE_MS = 60UL * 1000UL;

struct BluetoothPresetCommand {
  const char *id;
  const char *name;
  const char *report;
  uint16_t usage;
  const char *iconPath;
};

static const BluetoothPresetCommand CHROMECAST_COMMANDS[] = {
  {"ble_home", "Home", "consumer", 0x0223, "/icons/Default/home.png"},
  {"ble_back", "Back", "consumer", 0x0224, "/icons/Default/back.png"},
  {"ble_up", "Up", "consumer", 0x0042, "/icons/Default/up.png"},
  {"ble_down", "Down", "consumer", 0x0043, "/icons/Default/down.png"},
  {"ble_left", "Left", "consumer", 0x0044, "/icons/Default/left.png"},
  {"ble_right", "Right", "consumer", 0x0045, "/icons/Default/right.png"},
  {"ble_ok", "OK", "consumer", 0x0041, "/icons/Default/ok_select.png"},
  {"ble_play_pause", "Play / Pause", "consumer", 0x00CD, "/icons/Default/play.png"},
  {"ble_stop", "Stop", "consumer", 0x00B7, "/icons/Default/stop.png"},
  {"ble_rewind", "Rewind", "consumer", 0x00B4, "/icons/Default/rewind.png"},
  {"ble_fast_forward", "Fast Forward", "consumer", 0x00B3, "/icons/Default/fast_forward.png"},
  {"ble_previous", "Previous", "consumer", 0x00B6, "/icons/Default/skip_previous.png"},
  {"ble_next", "Next", "consumer", 0x00B5, "/icons/Default/skip_next.png"},
  {"ble_volume_up", "Volume Up", "consumer", 0x00E9, "/icons/Default/volume_up.png"},
  {"ble_volume_down", "Volume Down", "consumer", 0x00EA, "/icons/Default/volume_down.png"},
  {"ble_mute", "Mute", "consumer", 0x00E2, "/icons/Default/mute.png"},
  {"ble_power", "Power", "consumer", 0x0030, "/icons/Default/power.png"},
  {"ble_key_0", "Keyboard 0", "keyboard", 0x0027, "/icons/Default/number_0.png"},
  {"ble_key_1", "Keyboard 1", "keyboard", 0x001E, "/icons/Default/number_1.png"},
  {"ble_key_2", "Keyboard 2", "keyboard", 0x001F, "/icons/Default/number_2.png"},
  {"ble_key_3", "Keyboard 3", "keyboard", 0x0020, "/icons/Default/number_3.png"},
  {"ble_key_4", "Keyboard 4", "keyboard", 0x0021, "/icons/Default/number_4.png"},
  {"ble_key_5", "Keyboard 5", "keyboard", 0x0022, "/icons/Default/number_5.png"},
  {"ble_key_6", "Keyboard 6", "keyboard", 0x0023, "/icons/Default/number_6.png"},
  {"ble_key_7", "Keyboard 7", "keyboard", 0x0024, "/icons/Default/number_7.png"},
  {"ble_key_8", "Keyboard 8", "keyboard", 0x0025, "/icons/Default/number_8.png"},
  {"ble_key_9", "Keyboard 9", "keyboard", 0x0026, "/icons/Default/number_9.png"},
  {"ble_voice_search", "Voice Search", "consumer", 0x0221,
   "/icons/Default/microphone_voice.png"}
};
static const uint8_t CHROMECAST_COMMAND_COUNT =
  sizeof(CHROMECAST_COMMANDS) / sizeof(CHROMECAST_COMMANDS[0]);

static const uint8_t BLE_HID_REPORT_MAP[] = {
  0x05, 0x01, 0x09, 0x06, 0xA1, 0x01, 0x85, 0x01,
  0x05, 0x07, 0x19, 0xE0, 0x29, 0xE7, 0x15, 0x00,
  0x25, 0x01, 0x75, 0x01, 0x95, 0x08, 0x81, 0x02,
  0x95, 0x01, 0x75, 0x08, 0x81, 0x01,
  0x95, 0x06, 0x75, 0x08, 0x15, 0x00, 0x25, 0x65,
  0x05, 0x07, 0x19, 0x00, 0x29, 0x65, 0x81, 0x00,
  0xC0,
  0x05, 0x0C, 0x09, 0x01, 0xA1, 0x01, 0x85, 0x01,
  0x15, 0x00, 0x26, 0xFF, 0x03, 0x19, 0x00, 0x2A,
  0xFF, 0x03, 0x75, 0x10, 0x95, 0x01, 0x81, 0x00,
  0xC0
};
static const uint8_t BLE_HID_INPUT_BYTES = 10;

class OpenRemoteDisplay : public lgfx::LGFX_Device {
 private:
  lgfx::Panel_ILI9341 panel;
  lgfx::Bus_Parallel8 bus;

 public:
  OpenRemoteDisplay() {
    auto busConfig = bus.config();
    busConfig.freq_write = 40000000;
    busConfig.pin_wr = PIN_LCD_WR;
    busConfig.pin_rd = PIN_LCD_RD;
    busConfig.pin_rs = PIN_LCD_DC;
    busConfig.pin_d0 = PIN_LCD_D0;
    busConfig.pin_d1 = PIN_LCD_D1;
    busConfig.pin_d2 = PIN_LCD_D2;
    busConfig.pin_d3 = PIN_LCD_D3;
    busConfig.pin_d4 = PIN_LCD_D4;
    busConfig.pin_d5 = PIN_LCD_D5;
    busConfig.pin_d6 = PIN_LCD_D6;
    busConfig.pin_d7 = PIN_LCD_D7;
    bus.config(busConfig);
    panel.setBus(&bus);

    auto panelConfig = panel.config();
    panelConfig.pin_cs = PIN_LCD_CS;
    panelConfig.pin_rst = PIN_LCD_RST;
    panelConfig.pin_busy = -1;
    panelConfig.memory_width = LCD_W;
    panelConfig.memory_height = LCD_H;
    panelConfig.panel_width = LCD_W;
    panelConfig.panel_height = LCD_H;
    panelConfig.offset_rotation = 2;
    panel.config(panelConfig);
    setPanel(&panel);
  }
};

OpenRemoteDisplay tft;

// Persistent native LVGL tiles are transferred through two DMA-safe 32-row
// buffers. This remains true double buffering, while keeping each parallel-bus
// transaction within the proven transfer size used by firmware 2.09/2.15.
static lv_disp_draw_buf_t drawBuf;
static lv_color_t lvFallbackBuf1[LCD_W * 32];
static lv_color_t lvFallbackBuf2[LCD_W * 32];
static lv_color_t *lvBuf1 = lvFallbackBuf1;
static lv_color_t *lvBuf2 = lvFallbackBuf2;
static uint32_t lvDrawBufferPixels = LCD_W * 32;
static bool lvFullFrameDoubleBuffer = false;
static lv_disp_drv_t dispDrv;
static lv_indev_drv_t touchDrv;

// ---------------------------------------------------------------------------
// SD-backed runtime data model
// ---------------------------------------------------------------------------

static const uint8_t MAX_RUNTIME_DEVICES = 12;
static const uint8_t MAX_DEVICE_COMMANDS = 50;
static const uint8_t MAX_RUNTIME_ACTIVITIES = 12;
static const uint8_t MAX_RUNTIME_MACROS = 24;
static const uint8_t MAX_ACTIVITY_TILES = 30;
static const uint8_t MAX_ACTIVITY_STEPS = 48;
static const uint8_t MAX_RUNTIME_THEMES = 24;
static const uint8_t PHYSICAL_BUTTON_COUNT = 23;

struct RuntimeThemeStyle {
  char path[72];
  uint32_t glassColour;
  uint16_t split;
  uint8_t rowCount;
  uint8_t glassOpacity;
  bool glassEnabled;
};

struct DeviceCommand {
  char label[28];
  char id[48];
  char *iconPath;
  char protocol[16];
  uint32_t address;
  uint32_t command;
  uint16_t frequencyKhz;
  uint16_t *rawTimings;
  uint16_t rawCount;
  uint8_t sonyBits;
  uint16_t hidUsage;
  char homebridgeAccessoryId[72];
  char homebridgeCharacteristic[40];
  char homebridgeStringValue[28];
  float homebridgeValue;
  float homebridgeStep;
  float homebridgeMin;
  float homebridgeMax;
  uint8_t hidReport;
  uint8_t homebridgeOperation;
  uint8_t homebridgeValueType;
  uint8_t slot;
  uint8_t boxMode;
  bool showText;
  bool repeatDefault;
  enum Kind : uint8_t { NONE, PARSED, RAW, BLE_HID, HOMEBRIDGE } kind;
};

struct PhysicalBinding {
  uint8_t deviceIndex;
  uint8_t commandIndex;
  bool repeat;
};

struct ActivityStep {
  enum Kind : uint8_t { COMMAND, DELAY } kind;
  uint8_t deviceIndex;
  uint8_t commandIndex;
  uint32_t delayMs;
  bool delayWhenDevicePoweredOn;
};

struct Device {
  char name[32];
  char id[48];
  char transport[12];
  char themePath[72];
  DeviceCommand commands[MAX_DEVICE_COMMANDS];
  PhysicalBinding physicalBindings[PHYSICAL_BUTTON_COUNT];
  uint8_t commandCount;
  int8_t powerOnCommandIndex;
  int8_t powerOffCommandIndex;
  bool powerTrackingConfigured;
  bool powerTrackingEnabled;
};

struct Activity {
  char id[48];
  char name[32];
  char *iconPath;
  char themePath[72];
  uint16_t accent;
  uint16_t deviceMask;
  uint8_t boxMode;
  ActivityStep steps[MAX_ACTIVITY_STEPS];
  uint8_t stepCount;
  PhysicalBinding physicalBindings[PHYSICAL_BUTTON_COUNT];
};

struct Macro {
  char id[48];
  char name[32];
  ActivityStep steps[MAX_ACTIVITY_STEPS];
  uint8_t stepCount;
};

struct Tile {
  enum Kind : uint8_t { COMMAND, ACTIVITY, MACRO } kind;
  char label[28];
  char targetActivityId[48];
  char targetMacroId[48];
  char *iconPath;
  uint8_t deviceIndex;
  uint8_t commandIndex;
  uint8_t slot;
  uint8_t boxMode;
  bool showText;
  bool repeat;
};

struct UiCommandBinding {
  DeviceCommand *command;
  Macro *macro;
  bool repeat;
};

struct ActivitySliderUi {
  lv_obj_t *card;
  lv_obj_t *thumb;
  uint8_t activityIndex;
};

// OpenRemote ships with no demonstration devices or activities. WebConfig
// writes the user's real model to /config/runtime.json on the SD card.
Device *devices = nullptr;
Activity *activities = nullptr;
Macro *macros = nullptr;
Tile (*activityTiles)[MAX_ACTIVITY_TILES] = nullptr;
uint8_t *activityTileCounts = nullptr;
uint8_t DEVICE_COUNT = 0;
uint8_t ACTIVITY_COUNT = 0;
uint8_t MACRO_COUNT = 0;
UiCommandBinding *uiCommandBindings = nullptr;
uint8_t uiCommandBindingCount = 0;
DeviceCommand *heldRepeatCommand = nullptr;
const DeviceCommand *heldVoiceSearchCommand = nullptr;
uint8_t heldVoiceSearchPhysicalKey = 0;
unsigned long nextIrRepeatMs = 0;
uint16_t heldRepeatIntervalMs = IR_REPEAT_INTERVAL_MS;
bool heldRepeatFromTouch = false;
bool heldVoiceSearchFromTouch = false;
bool tca8418Ready = false;
bool activitySequenceActive = false;
uint8_t activitySequenceActivity = 0;
int8_t activitySequenceMacro = -1;
uint8_t activitySequenceStep = 0;
unsigned long activitySequenceNextMs = 0;
uint16_t activitySequencePoweredOnMask = 0;
bool hardwarePowerHeld = false;
bool hardwarePowerLongTriggered = false;
unsigned long hardwarePowerPressedMs = 0;
bool buttonDiagnosticActive = false;
uint8_t buttonDiagnosticRequested = 0;
uint8_t buttonDiagnosticKeys[PHYSICAL_BUTTON_COUNT] = {};
uint8_t buttonDiagnosticSwitches[PHYSICAL_BUTTON_COUNT] = {};
uint8_t buttonDiagnosticRows[PHYSICAL_BUTTON_COUNT] = {};
uint8_t buttonDiagnosticColumns[PHYSICAL_BUTTON_COUNT] = {};

struct PowerMemoryEntry {
  char deviceId[48];
  bool on;
};

PowerMemoryEntry powerMemory[MAX_RUNTIME_DEVICES] = {};
uint8_t powerMemoryCount = 0;

struct DeepSleepRuntimeState {
  uint32_t magic;
  char activityId[48];
  char deviceId[48];
  uint8_t powerMemoryCount;
  PowerMemoryEntry powerMemory[MAX_RUNTIME_DEVICES];
};

static const uint32_t DEEP_SLEEP_RUNTIME_MAGIC = 0x4F525443UL;
RTC_DATA_ATTR DeepSleepRuntimeState deepSleepRuntimeState;

uint8_t buttonIconSize = 52;
uint8_t buttonTextSize = 16;
uint8_t activityIconSize = 52;
uint8_t activityTextSize = 20;
bool buttonBoxesEnabled = true;
bool activityBoxesEnabled = true;
char activitiesThemePath[72] = "";
RuntimeThemeStyle runtimeThemes[MAX_RUNTIME_THEMES] = {};
uint8_t runtimeThemeCount = 0;
RuntimeThemeStyle *activeRuntimeThemeStyle = nullptr;

struct UsbSerialSession {
  String commandLine;
  File uploadFile;
  String uploadTempPath;
  String uploadFinalPath;
  size_t uploadExpected = 0;
  size_t uploadReceived = 0;
  size_t uploadNextAck = 0;
  uint32_t uploadLastDataMs = 0;
  bool uploadIsIrFile = false;
  bool uploadIsRuntimeConfig = false;
  File downloadFile;
  size_t downloadTotal = 0;
  size_t downloadSent = 0;
  uint32_t downloadLastProgressMs = 0;
};

UsbSerialSession usbCdcSession;
UsbSerialSession uart0Session;

bool usbSdTransferActive() {
  return usbCdcSession.uploadExpected > 0 || uart0Session.uploadExpected > 0 ||
    usbCdcSession.downloadTotal > 0 || uart0Session.downloadTotal > 0;
}

// ---------------------------------------------------------------------------
// Runtime page model
// ---------------------------------------------------------------------------

enum PageKind {
  PAGE_REMOTE_SETTINGS,
  PAGE_ACTIVITIES,
  PAGE_ACTIVITY,
  PAGE_DEVICE
};

struct RuntimePage {
  PageKind kind;
  const char *title;
};

RuntimePage pages[4];
uint8_t pageCount = 2;
uint8_t currentPage = 1;
uint8_t deviceReturnPage = 1;
int8_t pendingPageTransition = 0;
int activeActivity = -1;
int activeDevice = -1;
int8_t pendingDeviceOpen = -1;
int8_t pendingActivityOpen = -1;

bool wifiOn = true;
bool bluetoothOn = true;
bool clockEnabled = true;
bool clockUseInternetTime = true;
bool slideToUnlock = true;
bool raiseToWake = true;
bool physicalRepeatEnabled = true;
uint16_t physicalRepeatDelayMs = 400;
uint8_t physicalRepeatRateHz = 9;
bool debugSplitEnabled = true;
bool debugTouchEnabled = false;
bool debugCpuRamEnabled = false;
bool debugAccelerometerEnabled = false;
bool debugFpsEnabled = false;
bool microphoneTestAudioEnabled = false;
uint16_t debugRowPixels[5] = {246, 195, 144, 93, 42};
bool sdReady = false;
uint8_t brightness = 72;  // User-facing percentage: 0 to 100.
uint8_t timeoutSeconds = 25;
uint8_t deepSleepMinutes = 10;
uint8_t wakeSensitivity = 58;
uint16_t displayGamma = 100;
uint16_t displaySaturation = 100;
bool displayRgb666 = false;
bool displayInverted = false;
bool lcdControllerReady = false;
uint64_t manualClockEpoch = 0;
int16_t clockUtcOffsetMinutes = 600;
char irdbBuildDate[28] = "Not installed";
uint32_t irdbDeviceCount = 0;
String clockCityName = "Canberra";
String homebridgeAddress;
String homebridgeUsername;
String homebridgePassword;
String homebridgeToken;
String remoteName = "OpenRemote";
char sdStatusText[64] = "Not checked";

enum SettingsView {
  SETTINGS_HOME,
  SETTINGS_WIFI,
  SETTINGS_WIFI_PASSWORD,
  SETTINGS_WIFI_QR,
  SETTINGS_BLUETOOTH,
  SETTINGS_CLOCK,
  SETTINGS_CLOCK_CITY,
  SETTINGS_DISPLAY,
  SETTINGS_BUTTONS,
  SETTINGS_DEBUG,
  SETTINGS_BATTERY,
  SETTINGS_BACKUP,
  SETTINGS_ABOUT,
  SETTINGS_UNLOCK
};

SettingsView settingsView = SETTINGS_HOME;
Preferences preferences;
WebServer webServer(80);
DNSServer dnsServer;
bool webServerConfigured = false;
volatile bool webServerStarted = false;
volatile bool webServerListenRequested = false;
volatile bool webServerRebindRequested = false;
volatile bool webServerStopRequested = false;
TaskHandle_t webServerTaskHandle = nullptr;
TaskHandle_t atvvAudioTaskHandle = nullptr;
SemaphoreHandle_t atvvNotifyMutex = nullptr;
SemaphoreHandle_t microphoneMutex = nullptr;
i2s_chan_handle_t microphoneRxChannel = nullptr;
bool realMicrophoneActive = false;
bool atvvStreamUsesTestAudio = false;
volatile bool microphoneStopPending = false;
int16_t microphoneAdpcmPredictor = 0;
int8_t microphoneAdpcmStepIndex = 0;
bool dnsServerStarted = false;
bool networkStackActive = false;
bool setupApActive = false;
volatile bool webConfigTransferActive = false;
volatile bool webConfigTransferCancelRequested = false;
bool bleReady = false;
bool bleShutdownInProgress = false;
volatile bool bleSuspended = false;
bool scheduledNtpWake = false;
bool setupApPausedBle = false;
bool webConfigPausedBle = false;
volatile bool restartPending = false;
volatile bool hardRestartPending = false;
bool firmwareUploadOk = false;
bool firmwareStageOk = false;
bool webConfigUploadOk = false;
bool irdbUploadOk = false;
bool wifiScanPending = false;
bool wifiScanStartPending = false;
bool wifiConnectPending = false;
bool wifiScanKeepSetupAp = false;
volatile bool pendingUiRefresh = false;
bool pendingNetworkApply = false;
bool pendingBluetoothApply = false;
bool ntpSyncPending = false;
bool ntpConfigured = false;
bool stationFallbackToSetupAp = false;
unsigned long ntpSyncStartedMs = 0;
unsigned long networkShutdownAtMs = 0;
int16_t lastNtpSyncYDay = -1;
bool runtimeSettingsSavePending = false;
unsigned long runtimeSettingsSaveAtMs = 0;
bool lockActive = false;
bool customKeyboardCaps = false;
bool customKeyboardSymbols = false;
int wifiScanCount = -2;
uint8_t wifiScanAttempt = 0;
unsigned long wifiScanStartAtMs = 0;
unsigned long wifiScanStartedMs = 0;
unsigned long wifiConnectStartedMs = 0;
String selectedWifiSsid;
String setupApSsid;
String setupToken;

enum WebWifiAction : uint8_t {
  WEB_WIFI_NONE,
  WEB_WIFI_CONNECT,
  WEB_WIFI_FORGET
};

struct WebWifiRequest {
  WebWifiAction action = WEB_WIFI_NONE;
  bool useSavedPassword = false;
  char ssid[33] = {};
  char password[65] = {};
};

struct WebClockRequest {
  bool pending = false;
  bool enabled = true;
  bool useInternetTime = true;
  bool hasManualEpoch = false;
  uint64_t manualEpoch = 0;
  int16_t utcOffsetMinutes = 600;
  char city[64] = {};
};

portMUX_TYPE webControlMux = portMUX_INITIALIZER_UNLOCKED;
volatile bool webWifiScanRequested = false;
WebWifiRequest webWifiRequest;
WebClockRequest webClockRequest;
char webWifiStatusText[96] = "Ready";
unsigned long webWifiActionNotBeforeMs = 0;

struct WifiProfile {
  String ssid;
  String password;
};

struct WifiScanEntry {
  String ssid;
  int32_t rssi = -127;
  wifi_auth_mode_t encryption = WIFI_AUTH_OPEN;
};

WifiProfile wifiProfiles[MAX_WIFI_PROFILES];
uint8_t wifiProfileCount = 0;
WifiScanEntry wifiScanResults[MAX_WIFI_SCAN_RESULTS];
uint8_t wifiScanResultCount = 0;
File webConfigUploadFile;
File firmwareStageFile;
size_t firmwareStageBytes = 0;
File irdbUploadFile;
File customIconUploadFile;
String customIconUploadPath;
bool customIconUploadOk = false;
File themeUploadFile;
String themeUploadPath;
bool themeUploadOk = false;
File runtimeConfigUploadFile;
bool runtimeConfigUploadStarted = false;
bool runtimeConfigUploadOk = false;
size_t runtimeConfigUploadBytes = 0;
String runtimeConfigUploadError;
File backupUploadFile;
String backupUploadPath;
bool backupUploadOk = false;
static const uint8_t MAX_LCD_BACKUPS = 16;
struct LcdBackupEntry {
  char name[64];
  char exportedAt[32];
  char displayDate[40];
};
LcdBackupEntry lcdBackupEntries[MAX_LCD_BACKUPS] = {};
uint8_t lcdBackupCount = 0;
char lcdBackupStatus[88] = "";
char lcdPendingBackupName[64] = "";
lv_obj_t *lcdBackupStatusLabel = nullptr;
lv_obj_t *lcdBackupConfirmBox = nullptr;
volatile bool pendingRuntimeReload = false;
volatile bool runtimeReloadCanRollback = false;
volatile unsigned long runtimeReloadAfterMs = 0;
bool irLearningActive = false;
unsigned long irLearningStartedMs = 0;
String irLearningResult;
String irLearningError;
BLEServer *bleServer = nullptr;
BLEHIDDevice *bleHid = nullptr;
BLECharacteristic *bleKeyboardInput = nullptr;
BLECharacteristic *bleConsumerInput = nullptr;
BLEService *atvvService = nullptr;
BLECharacteristic *atvvTx = nullptr;
BLECharacteristic *atvvRx = nullptr;
BLECharacteristic *atvvCtl = nullptr;
BLE2902 *atvvRxCccd = nullptr;
BLE2902 *atvvCtlCccd = nullptr;
BLESecurity *bleSecurity = nullptr;
volatile bool bleConnected = false;
volatile bool bleBonded = false;
#if defined(CONFIG_BLUEDROID_ENABLED)
esp_bd_addr_t blePeerAddress = {};
#endif
bool blePeerAddressValid = false;
bool bleIdleConnectionProfileRequested = false;
bool blePairingMode = false;
unsigned long blePairingUntilMs = 0;
volatile unsigned long bleKeepAliveUntilMs = 0;
volatile bool bleBondStateSavePending = false;
volatile bool bleDeviceProvisionPending = false;
bool atvvRxSubscribed = false;
bool atvvCtlSubscribed = false;
unsigned long nextAtvvDebugMs = 0;
enum AtvvVoiceState : uint8_t {
  ATVV_VOICE_IDLE,
  ATVV_VOICE_SEARCH_REQUESTED,
  ATVV_VOICE_STREAMING
};
AtvvVoiceState atvvVoiceState = ATVV_VOICE_IDLE;
volatile bool atvvMicOpenPending = false;
volatile bool atvvMicClosePending = false;
volatile uint8_t atvvMicCloseStreamId = 0xFF;
volatile bool atvvCapsRequestPending = false;
volatile uint16_t atvvHostSpecVersion = 0x0004;
volatile uint8_t atvvHostInteractionModels = ATVV_INTERACTION_ON_REQUEST;
volatile bool atvvAudioStarted = false;
bool atvvVoiceReleasePending = false;
uint8_t atvvInteractionModel = ATVV_INTERACTION_ON_REQUEST;
uint8_t atvvStreamId = 0;
uint8_t atvvNextStreamId = 1;
volatile uint32_t atvvAudioFrameNumber = 0;
volatile size_t atvvTestAudioOffset = 0;
volatile bool atvvTestAudioFinished = false;
unsigned long atvvSearchRequestedMs = 0;
unsigned long atvvStopAfterMs = 0;
unsigned long atvvAudioStartedMs = 0;
volatile unsigned long atvvNextAudioFrameMs = 0;

bool touchFound = false;
bool lis3dhReady = false;
bool displaySleeping = false;
bool lightSleepArmed = false;
bool bleConnectedIdleActive = false;
bool bleIdleAccelerometerWake = false;
bool bleIdleKeypadWake = false;
bool bleIdleUartWake = false;
bool powerManagementReady = false;
bool sleepBacklightPwmSuspended = false;
esp_pm_lock_handle_t awakeCpuFrequencyLock = nullptr;
esp_pm_lock_handle_t awakeNoLightSleepLock = nullptr;
bool awakePowerLocksHeld = false;
uint32_t bleConnectedIdleRestoreCpuMhz = 240;
unsigned long nextBleConnectedIdleMotionMs = 0;
unsigned long bleIdleMotionAboveSinceMs = 0;
bool backlightPwmReady = false;
bool buttonBacklightPwmReady = false;
unsigned long lastWakeMs = 0;
unsigned long displaySleepStartedMs = 0;
unsigned long nextDeepSleepAttemptMs = 0;
unsigned long lastTickMs = 0;
unsigned long nextIrBlinkMs = 0;
unsigned long irOffAtMs = 0;
int16_t sleepBaseX = 0;
int16_t sleepBaseY = 0;
int16_t sleepBaseZ = 0;

struct BatteryHistorySample {
  uint32_t epoch;
  float percent;
};

struct BatteryHistoryStore {
  uint32_t magic;
  uint8_t count;
  uint8_t next;
  uint16_t reserved;
  BatteryHistorySample samples[49];
};

static const uint32_t BATTERY_HISTORY_MAGIC = 0x4F524248UL;
BatteryHistoryStore batteryHistory = {};
unsigned long nextBatteryHistoryCheckMs = 0;
bool batteryPowerModeKnown = false;
bool batteryPowerModeCharging = false;
uint32_t batteryPowerModeChangedEpoch = 0;
float batteryPowerModeStartPercent = -1.0f;

lv_obj_t *screenRoot = nullptr;
lv_obj_t *wallpaper = nullptr;
lv_obj_t *topBar = nullptr;
lv_obj_t *content = nullptr;
lv_obj_t *dots = nullptr;
lv_obj_t *splitDiagnosticLabel = nullptr;
lv_obj_t *touchDiagnosticLabel = nullptr;
lv_obj_t *cpuRamDiagnosticLabel = nullptr;
lv_obj_t *accelerometerDiagnosticLabel = nullptr;
lv_obj_t *fpsDiagnosticLabel = nullptr;
lv_obj_t *splitDiagnosticAnchor = nullptr;
int16_t splitDiagnosticAnchorY = INT16_MAX;
int16_t splitDiagnosticLastY = INT16_MIN;
lv_obj_t *deviceModal = nullptr;
lv_obj_t *brightnessOverlay = nullptr;
lv_obj_t *lockOverlay = nullptr;
lv_obj_t *physicalVoiceOverlay = nullptr;
lv_obj_t *physicalVoicePulse = nullptr;
bool physicalVoiceOverlayVisible = false;
unsigned long physicalVoiceOverlayShowAfterMs = 0;
lv_obj_t *brightnessPanel = nullptr;
lv_obj_t *touchDot = nullptr;
lv_obj_t *touchReticleTicks[4] = {nullptr, nullptr, nullptr, nullptr};
static const uint8_t TOUCH_TRAIL_POINT_COUNT = 8;
struct TouchTrailPoint {
  lv_obj_t *dot;
  unsigned long createdMs;
  bool active;
};
TouchTrailPoint touchTrail[TOUCH_TRAIL_POINT_COUNT] = {};
uint8_t nextTouchTrailPoint = 0;
unsigned long lastTouchTrailPointMs = 0;
unsigned long touchDiagnosticHoldUntilMs = 0;
lv_obj_t *clockLabel = nullptr;
lv_obj_t *batteryFill = nullptr;
lv_obj_t *statusPill = nullptr;
lv_obj_t *statusBattery = nullptr;
lv_obj_t *statusBatteryTerminal = nullptr;
lv_obj_t *brightnessBatteryLabel = nullptr;
lv_obj_t *wifiPasswordArea = nullptr;
lv_obj_t *wifiKeyboard = nullptr;
lv_obj_t *setupApStatusLabel = nullptr;
lv_obj_t *buttonTestPanel = nullptr;
lv_obj_t *buttonTestLabel = nullptr;
lv_obj_t *lcdRebootConfirmBox = nullptr;
bool lcdRebootConfirmHard = false;
lv_obj_t *clockRollers[5] = {nullptr, nullptr, nullptr, nullptr, nullptr};
lv_obj_t *displayValueLabels[6] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
lv_obj_t *buttonValueLabels[2] = {nullptr, nullptr};
lv_obj_t *debugRowDropdowns[5] = {nullptr, nullptr, nullptr, nullptr, nullptr};
int16_t debugRowDropdownMinimums[5] = {0, 0, 0, 0, 0};
lv_obj_t *batteryMetricNameLabels[6] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
lv_obj_t *batteryMetricValueLabels[6] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
unsigned long nextStatusRefreshMs = 0;
unsigned long nextDebugCpuRamRefreshMs = 0;
unsigned long nextDebugAccelerometerRefreshMs = 0;
unsigned long nextDebugFpsRefreshMs = 0;
volatile uint32_t debugDisplayFrameCount = 0;
uint32_t debugLastDisplayFrameCount = 0;
unsigned long debugLastFpsSampleMs = 0;
unsigned long nextBatteryPageRefreshMs = 0;
unsigned long nextSetupApStatusRefreshMs = 0;
unsigned long brightnessLastActivityMs = 0;
unsigned long commandFeedbackUntilMs = 0;
bool commandFeedbackActive = false;
unsigned long buttonTestFeedbackUntilMs = 0;
bool buttonTestFeedbackActive = false;
bool buttonTestActive = false;
int8_t buttonTestHeldIndex = -1;
unsigned long buttonTestPulseUntilMs = 0;
unsigned long nextButtonTestRepeatMs = 0;
char buttonTestHeldName[24] = "";
bool chargingState = false;
bool chargingCandidate = false;
unsigned long chargingCandidateSinceMs = 0;
unsigned long chargingAnimationStartMs = 0;
bool touchWasDown = false;
bool lvTouchDown = false;
lv_indev_t *touchInputDevice = nullptr;
bool activityDragActive = false;
uint16_t touchStartX = 0;
uint16_t touchStartY = 0;
uint16_t touchLastX = 0;
uint16_t touchLastY = 0;
uint8_t touchPendingConfirmCount = 0;
uint16_t touchPendingX = 0;
uint16_t touchPendingY = 0;
ActivitySliderUi *activitySliderUi = nullptr;
SPIClass sdSpi(FSPI);
static lv_fs_drv_t sdLvglFsDriver;
struct CachedPageIcon {
  char path[96];
  uint8_t *pixels;
  lv_img_dsc_t descriptor;
};

struct PageUi {
  lv_obj_t *tile = nullptr;
  lv_obj_t *root = nullptr;
  lv_obj_t *wallpaper = nullptr;
  lv_obj_t *topBar = nullptr;
  lv_obj_t *content = nullptr;
  lv_obj_t *dots = nullptr;
  lv_obj_t *clockLabel = nullptr;
  lv_obj_t *statusPill = nullptr;
  lv_obj_t *statusBattery = nullptr;
  lv_obj_t *statusBatteryTerminal = nullptr;
  lv_obj_t *batteryFill = nullptr;
  uint16_t *wallpaperPixels = nullptr;
  lv_img_dsc_t wallpaperDescriptor = {};
  char themePath[72] = "";
  CachedPageIcon iconCache[MAX_PAGE_ICON_CACHE] = {};
  uint8_t iconCacheCount = 0;
  UiCommandBinding commandBindings[MAX_DEVICE_COMMANDS] = {};
  ActivitySliderUi sliderUi[MAX_ACTIVITY_TILES] = {};
};

static const uint8_t PAGE_SLOT_COUNT = 4;
PageUi pageUi[PAGE_SLOT_COUNT];
PageUi *boundPageUi = nullptr;
lv_obj_t *uiRoot = nullptr;
lv_obj_t *pageStrip = nullptr;
bool pageStripChangePending = false;
uint8_t pageStripPendingPage = 1;
bool pageStripRebuildPending = false;
bool pageStripRendering = false;
uint8_t uiMutationDepth = 0;
unsigned long touchAcceptAfterMs = 0;
unsigned long touchReleasedSinceMs = 0;
unsigned long touchQuarantineStartedMs = 0;
bool touchQuarantineActive = true;
uint16_t *displayColourLut = nullptr;
uint16_t *displayFlush565 = nullptr;
size_t displayFlushPixelCapacity = 0;
bool displayColourLutActive = false;

struct ScrollSafeSliderState {
  int32_t committedValue;
  lv_point_t startPoint;
  bool tracking;
  bool horizontal;
  bool vertical;
  bool restoring;
};

static const uint8_t MAX_SCROLL_SAFE_SLIDERS = 10;
ScrollSafeSliderState scrollSafeSliderStates[MAX_SCROLL_SAFE_SLIDERS] = {};
uint8_t scrollSafeSliderCount = 0;

const char *sdFolders[] = {
  "/www",
  "/config",
  "/themes",
  "/themes/Default",
  "/themes/Custom",
  "/icons",
  "/icons/Default",
  "/icons/Custom",
  "/devices",
  "/activities",
  "/macros",
  "/irdb",
  "/firmware",
  "/backups",
  "/logs",
  "/tmp"
};
static const uint8_t SD_FOLDER_COUNT = sizeof(sdFolders) / sizeof(sdFolders[0]);

// ---------------------------------------------------------------------------
// Forward declarations used by LVGL callbacks
// ---------------------------------------------------------------------------

void renderCurrentPage();
void rebuildPages();
void changePage(int delta);
void renderAllPageSlots();
void configurePageStripDirections();
void bindPageUi(uint8_t index);
void requestPageStripRebuild();
void createPhysicalVoiceOverlay();
void servicePhysicalVoiceOverlay();
void showPhysicalVoiceOverlay();
void hidePhysicalVoiceOverlay();
void activateActivity(uint8_t index);
void openDevice(uint8_t index);
void showDevicePicker();
void toggleBrightnessPanel();
void closeBrightnessPanel();
void enterDisplaySleep();
void wakeDisplay();
void enterBleConnectedIdle();
void leaveBleConnectedIdle();
bool configureApplicationPowerMode(bool connectedIdle);
void neutraliseRev5DisplayBusForDeepSleep();
void suspendBacklightPwmForSleep();
void restoreBacklightPwmAfterSleep();
void saveSettings();
void scheduleRuntimeSettingsSave();
bool persistSettingsToRuntimeConfig();
void rebuildDisplayColourLut();
void applyDisplayControllerSettings();
bool bluetoothRuntimeRequired();
bool bluetoothActivitySessionRequired();
void applyBluetoothState();
void startBluetoothPairing();
void forgetBluetoothPairing();
void serviceBluetooth(unsigned long now);
bool ensureBluetoothRuntimeDevice();
void applyClockMode();
void startNetworkStack();
void stopNetworkStack();
void parkNetworkStackForBle();
void startSetupAccessPoint();
void stopSetupAccessPoint(bool resumeStation = true);
void configureWebServer();
void requestWebServerListen(bool rebind = false);
void requestWebServerStop();
bool requestWebServerStopAndWait(uint32_t timeoutMs = 1800UL);
bool recoverWifiRadio(wifi_mode_t targetMode, const char *reason);
void renderSettingsHome();
void renderWifiPage();
void renderWifiPasswordPage();
void renderWifiQrPage();
void renderBluetoothPage();
void renderClockPage();
void renderClockCityPage();
void renderDisplayPage();
void renderButtonsPage();
void renderDebugPage();
void renderBatteryPage();
void renderBackupRestorePage();
void renderAboutPage();
void renderUnlockPage();
uint16_t countSavedIrDeviceFiles();
bool i2cDevicePresent(uint8_t address);
bool transmitIrCommand(const DeviceCommand &command);
bool isVoiceSearchCommand(const DeviceCommand *command);
bool beginVoiceSearchHold(const DeviceCommand *command, bool fromTouch = false);
void endVoiceSearchHold(const DeviceCommand *command = nullptr);
void serviceHeldIrRepeat(unsigned long now);
void serviceButtonTest(unsigned long now);
bool buttonTestModeActive();
void beginButtonTest(int8_t index, const char *name);
void endButtonTest(int8_t index);
void serviceActivitySequence(unsigned long now);
void serviceHardwarePowerHold(unsigned long now);
void serviceKeypad(unsigned long now);
void handleHardwareAllOff();
String sanitizeBackupFileName(String name);
void clearPageIconCache();
const void *cachedPageIconSource(const char *path);
void requestInternetTimeSync();
void serviceInternetTime(unsigned long now);
void serviceWebControlRequests(unsigned long now);
void serviceAtvvVoice(unsigned long now);
void ensureAtvvAudioTask();
void refreshDebugOverlayVisibility();
void serviceDebugOverlay(unsigned long now);
bool startRealMicrophoneCapture();
void stopRealMicrophoneCapture();
void serviceNetworkPower(unsigned long now);
bool enterDeepPowerSleep(bool allowQrPage = false);
void releaseDeepSleepPinHolds();
void loadBatteryHistory();
void serviceBatteryHistory(unsigned long now, bool force = false);
bool updateChargingState();
void initialiseChargingState();

// ---------------------------------------------------------------------------
// Hardware helpers
// ---------------------------------------------------------------------------

void buttonBacklight(bool on) {
  if (buttonBacklightPwmReady) {
    ledcWrite(PIN_BUTTON_BL, on ? BACKLIGHT_PWM_MAX : 0);
  } else {
    digitalWrite(PIN_BUTTON_BL, on ? HIGH : LOW);
  }
}

void lcdPowerOn() {
  pinMode(PIN_LCD_EN, OUTPUT);
  pinMode(PIN_LCD_BL, OUTPUT);
  pinMode(PIN_BUTTON_BL, OUTPUT);
  pinMode(PIN_IR_LED, OUTPUT);
  pinMode(PIN_IR_VCC, OUTPUT);
  pinMode(PIN_IR_RX, INPUT);
  pinMode(PIN_CHARGE_STATUS, INPUT_PULLUP);
  pinMode(PIN_TCA_INT, INPUT_PULLUP);
  pinMode(PIN_SD_EN, OUTPUT);
  pinMode(PIN_MIC_POWER, OUTPUT);
  digitalWrite(PIN_IR_LED, HIGH);
  digitalWrite(PIN_IR_VCC, HIGH);
  digitalWrite(PIN_LCD_BL, HIGH);
  buttonBacklight(false);
  digitalWrite(PIN_MIC_POWER, LOW);
  digitalWrite(PIN_SD_EN, HIGH);
  // Match OMOTE's Rev 5 startup sequence: place the LCD/touch rail in its
  // inactive state before releasing any retained deep-sleep pin holds, then
  // give it a clean off-to-on edge before the I2C master is started. Merely
  // asserting ON here can leave the FT5x06 hibernating across an ESP_EN reset.
  digitalWrite(PIN_LCD_EN, HIGH);
  // Deep sleep holds these pins at their off levels. Program every output
  // before releasing the holds so no pin can briefly float or reveal the
  // uninitialised white panel during the ESP32 restart.
  releaseDeepSleepPinHolds();
  delay(40);
  digitalWrite(PIN_LCD_EN, LOW);
  delay(120);
}

bool tcaWriteRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(ADDR_TCA8418);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool tcaReadRegister(uint8_t reg, uint8_t &value) {
  Wire.beginTransmission(ADDR_TCA8418);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(ADDR_TCA8418, (uint8_t)1) != 1) return false;
  value = Wire.read();
  return true;
}

void clearTca8418Interrupts() {
  uint8_t ignored = 0;
  // Reading these registers clears the underlying GPI status. INT_STAT cannot
  // release its active-low output while any enabled GPI status remains set.
  tcaReadRegister(0x11, ignored);
  tcaReadRegister(0x12, ignored);
  tcaReadRegister(0x13, ignored);
  tcaWriteRegister(0x02, 0x1F);
}

void initTca8418() {
  tca8418Ready = i2cDevicePresent(ADDR_TCA8418);
  if (!tca8418Ready) {
    Serial.println("TCA8418 keypad: not found");
    return;
  }
  // Rows 0-4 and columns 0-4 form the 5x5 Rev 5 key matrix. S17 is
  // separately wired to ROW5 as an active-low GPI and enters the same FIFO
  // as key events with the TCA8418-defined event number 102.
  tca8418Ready = tcaWriteRegister(0x1D, 0x1F) &&
                  tcaWriteRegister(0x1E, 0x1F) &&
                  tcaWriteRegister(0x1F, 0x00) &&
                  tcaWriteRegister(0x20, 0x20) &&
                  tcaWriteRegister(0x23, 0x00) &&
                  tcaWriteRegister(0x26, 0x00) &&
                  tcaWriteRegister(0x2C, 0x00) &&
                  tcaWriteRegister(0x1A, 0x20) &&
                  tcaWriteRegister(0x01, 0x03);
  if (tca8418Ready) clearTca8418Interrupts();
  Serial.printf("TCA8418 keypad: %s\n", tca8418Ready ? "ready" : "init failed");
}

static const char *PHYSICAL_BUTTON_NAMES[PHYSICAL_BUTTON_COUNT] = {
  "Stop", "Rewind", "Play", "Forward", "Menu", "Info",
  "D-pad Up", "D-pad Down", "D-pad Left", "D-pad Right", "OK",
  "Back", "Return", "Volume Up", "Volume Down", "Mute",
  "Channel Up", "Channel Down", "Record", "Red", "Green", "Yellow", "Blue"
};

static const uint8_t PHYSICAL_SWITCH_NUMBERS[PHYSICAL_BUTTON_COUNT] = {
  25, 24, 22, 16, 23, 6, 19, 10, 20, 7, 9, 18, 1, 8, 3, 4, 2, 12, 5, 13, 15, 14, 11
};

int8_t physicalButtonIndex(const char *name) {
  if (!name) return -1;
  for (uint8_t i = 0; i < PHYSICAL_BUTTON_COUNT; i++) {
    if (strcmp(name, PHYSICAL_BUTTON_NAMES[i]) == 0) return i;
  }
  return -1;
}

int8_t physicalButtonIndexForSwitch(uint8_t switchNumber) {
  for (uint8_t i = 0; i < PHYSICAL_BUTTON_COUNT; i++) {
    if (PHYSICAL_SWITCH_NUMBERS[i] == switchNumber) return i;
  }
  return -1;
}

struct SmartBindingAlias {
  uint8_t buttonIndex;
  const char *alias;
  uint8_t score;
};

static const SmartBindingAlias SMART_BINDING_ALIASES[] = {
  {0, "stop", 100}, {0, "mediastop", 90},
  {1, "rewind", 100}, {1, "fastback", 95}, {1, "fastbackward", 94},
  {1, "fastreverse", 93}, {1, "reverse", 88}, {1, "skipback", 84},
  {1, "skipprevious", 83}, {1, "previous", 75}, {1, "prevtrack", 74},
  {2, "playpause", 100}, {2, "mediaplaypause", 98}, {2, "play", 92},
  {2, "pause", 88},
  {3, "fastforward", 100}, {3, "fastfo", 96}, {3, "forward", 90},
  {3, "skipforward", 86}, {3, "skipnext", 85}, {3, "next", 75},
  {3, "nexttrack", 74},
  {4, "menu", 100}, {4, "home", 92}, {4, "homemenu", 90},
  {4, "options", 85}, {4, "option", 84}, {4, "settings", 80},
  {5, "info", 100}, {5, "display", 90}, {5, "details", 85},
  {6, "up", 100}, {6, "dpadup", 98}, {6, "arrowup", 96},
  {6, "cursorup", 94}, {6, "navigationup", 92}, {6, "navup", 90},
  {7, "down", 100}, {7, "dpaddown", 98}, {7, "arrowdown", 96},
  {7, "cursordown", 94}, {7, "navigationdown", 92}, {7, "navdown", 90},
  {8, "left", 100}, {8, "dpadleft", 98}, {8, "arrowleft", 96},
  {8, "cursorleft", 94}, {8, "navigationleft", 92}, {8, "navleft", 90},
  {9, "right", 100}, {9, "dpadright", 98}, {9, "arrowright", 96},
  {9, "cursorright", 94}, {9, "navigationright", 92}, {9, "navright", 90},
  {10, "ok", 100}, {10, "okay", 99}, {10, "select", 96},
  {10, "enter", 92}, {10, "confirm", 90},
  {11, "back", 100}, {11, "menuback", 95}, {11, "goback", 92},
  {12, "return", 100}, {12, "exit", 92},
  {13, "volumeup", 100}, {13, "volup", 98}, {13, "volumeplus", 96},
  {13, "volplus", 94}, {13, "audioup", 85},
  {14, "volumedown", 100}, {14, "voldown", 98}, {14, "volumedn", 97},
  {14, "voldn", 96}, {14, "volumeminus", 94}, {14, "volminus", 92},
  {14, "audiodown", 85},
  {15, "mute", 100}, {15, "audiomute", 95}, {15, "volumemute", 94},
  {16, "channelup", 100}, {16, "chup", 98}, {16, "channelnext", 96},
  {16, "chnext", 95}, {16, "channelplus", 92}, {16, "chanup", 90},
  {17, "channeldown", 100}, {17, "chdown", 98}, {17, "channelprev", 96},
  {17, "chprev", 95}, {17, "channelminus", 92}, {17, "chandown", 90},
  {18, "record", 100}, {18, "rec", 92},
  {19, "red", 100}, {19, "redbutton", 95},
  {20, "green", 100}, {20, "greenbutton", 95},
  {21, "yellow", 100}, {21, "yellowbutton", 95},
  {22, "blue", 100}, {22, "bluebutton", 95}
};

String normaliseSmartCommandName(const char *label) {
  String source = label ? String(label) : String();
  source.toLowerCase();
  String output;
  output.reserve(source.length() + 8);
  for (size_t i = 0; i < source.length(); i++) {
    char c = source[i];
    if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) output += c;
    else if (c == '+') output += "plus";
    else if (c == '-') output += "minus";
  }
  return output;
}

bool smartBindingRepeats(uint8_t buttonIndex) {
  return buttonIndex == 1 || buttonIndex == 3 ||
         (buttonIndex >= 6 && buttonIndex <= 9) ||
         (buttonIndex >= 13 && buttonIndex <= 14) ||
         (buttonIndex >= 16 && buttonIndex <= 17);
}

void applySmartRuntimeBindings(Device &device) {
  for (uint8_t buttonIndex = 0; buttonIndex < PHYSICAL_BUTTON_COUNT; buttonIndex++) {
    PhysicalBinding &binding = device.physicalBindings[buttonIndex];
    if (binding.commandIndex != 0xFF) continue;
    int16_t bestScore = -1;
    int8_t bestCommand = -1;
    for (uint8_t commandIndex = 0; commandIndex < device.commandCount; commandIndex++) {
      String commandName = normaliseSmartCommandName(device.commands[commandIndex].label);
      for (const SmartBindingAlias &candidate : SMART_BINDING_ALIASES) {
        if (candidate.buttonIndex != buttonIndex || commandName != candidate.alias) continue;
        if (candidate.score > bestScore) {
          bestScore = candidate.score;
          bestCommand = commandIndex;
        }
      }
    }
    if (bestCommand < 0) continue;
    binding.deviceIndex = (uint8_t)(&device - devices);
    binding.commandIndex = (uint8_t)bestCommand;
    binding.repeat = smartBindingRepeats(buttonIndex);
  }
}

enum RuntimePowerRole : uint8_t {
  RUNTIME_POWER_NONE,
  RUNTIME_POWER_ON,
  RUNTIME_POWER_OFF,
  RUNTIME_POWER_TOGGLE
};

enum RuntimeCommandResult : uint8_t {
  RUNTIME_COMMAND_FAILED,
  RUNTIME_COMMAND_SENT,
  RUNTIME_COMMAND_SKIPPED
};

int8_t powerMemoryIndex(const char *deviceId) {
  if (!deviceId || !deviceId[0]) return -1;
  for (uint8_t i = 0; i < powerMemoryCount; i++) {
    if (strcmp(powerMemory[i].deviceId, deviceId) == 0) return (int8_t)i;
  }
  return -1;
}

bool rememberedDeviceOn(const Device &device) {
  int8_t index = powerMemoryIndex(device.id);
  return index >= 0 && powerMemory[index].on;
}

void rememberDevicePower(Device &device, bool on) {
  int8_t index = powerMemoryIndex(device.id);
  if (index < 0) {
    if (powerMemoryCount >= MAX_RUNTIME_DEVICES) return;
    index = (int8_t)powerMemoryCount++;
    strlcpy(powerMemory[index].deviceId, device.id,
            sizeof(powerMemory[index].deviceId));
  }
  powerMemory[index].on = on;
  Serial.printf("Power memory: %s is %s\n", device.name, on ? "on" : "off");
}

bool anyRememberedDeviceOn() {
  for (uint8_t i = 0; i < powerMemoryCount; i++) {
    if (powerMemory[i].on) return true;
  }
  return false;
}

bool locateRuntimeCommand(DeviceCommand *command, uint8_t &deviceIndex,
                          uint8_t &commandIndex) {
  if (!command) return false;
  for (uint8_t i = 0; i < DEVICE_COUNT; i++) {
    for (uint8_t j = 0; j < devices[i].commandCount; j++) {
      if (command != &devices[i].commands[j]) continue;
      deviceIndex = i;
      commandIndex = j;
      return true;
    }
  }
  return false;
}

RuntimePowerRole runtimePowerRole(const Device &device, uint8_t commandIndex) {
  if (!device.powerTrackingEnabled) return RUNTIME_POWER_NONE;
  bool isOn = device.powerOnCommandIndex >= 0 &&
              commandIndex == (uint8_t)device.powerOnCommandIndex;
  bool isOff = device.powerOffCommandIndex >= 0 &&
               commandIndex == (uint8_t)device.powerOffCommandIndex;
  if (isOn && isOff) return RUNTIME_POWER_TOGGLE;
  if (isOn) return RUNTIME_POWER_ON;
  if (isOff) return RUNTIME_POWER_OFF;
  return RUNTIME_POWER_NONE;
}

int8_t activityPowerTarget(const Device &device, uint8_t commandIndex) {
  RuntimePowerRole role = runtimePowerRole(device, commandIndex);
  if (role == RUNTIME_POWER_NONE) return -1;
  return role == RUNTIME_POWER_OFF ? 0 : 1;
}

RuntimeCommandResult transmitRuntimeCommand(DeviceCommand *command,
                                            int8_t desiredPower = -1) {
  uint8_t deviceIndex = 0;
  uint8_t commandIndex = 0;
  if (!locateRuntimeCommand(command, deviceIndex, commandIndex)) {
    return command && transmitIrCommand(*command)
      ? RUNTIME_COMMAND_SENT : RUNTIME_COMMAND_FAILED;
  }

  Device &device = devices[deviceIndex];
  RuntimePowerRole role = runtimePowerRole(device, commandIndex);
  if (desiredPower >= 0 && role != RUNTIME_POWER_NONE &&
      rememberedDeviceOn(device) == (desiredPower != 0)) {
    Serial.printf("Smart power skipped: %s already %s\n", device.name,
                  desiredPower ? "on" : "off");
    return RUNTIME_COMMAND_SKIPPED;
  }
  if (!transmitIrCommand(*command)) return RUNTIME_COMMAND_FAILED;

  if (desiredPower >= 0 && role != RUNTIME_POWER_NONE) {
    rememberDevicePower(device, desiredPower != 0);
  }
  return RUNTIME_COMMAND_SENT;
}

void beginHeldIrCommand(DeviceCommand *command, bool repeat, bool fromTouch = false) {
  heldRepeatCommand = nullptr;
  heldRepeatFromTouch = false;
  if (!command) return;
  if (isVoiceSearchCommand(command)) {
    beginVoiceSearchHold(command, fromTouch);
    lastWakeMs = millis();
    return;
  }
  RuntimeCommandResult result = transmitRuntimeCommand(command);
  uint8_t deviceIndex = 0;
  uint8_t commandIndex = 0;
  bool trackedPower = locateRuntimeCommand(command, deviceIndex, commandIndex) &&
    runtimePowerRole(devices[deviceIndex], commandIndex) != RUNTIME_POWER_NONE;
  if (result == RUNTIME_COMMAND_SENT && repeat && !trackedPower) {
    heldRepeatCommand = command;
    heldRepeatFromTouch = fromTouch;
    heldRepeatIntervalMs = fromTouch ? IR_REPEAT_INTERVAL_MS :
      (uint16_t)max(50, 1000 / (int)physicalRepeatRateHz);
    nextIrRepeatMs = millis() +
      (fromTouch ? IR_REPEAT_DELAY_MS : physicalRepeatDelayMs);
  }
  lastWakeMs = millis();
}

void endHeldIrCommand(DeviceCommand *command = nullptr) {
  if (!command || heldVoiceSearchCommand == command) {
    endVoiceSearchHold(command);
  }
  if (!command || heldRepeatCommand == command) {
    heldRepeatCommand = nullptr;
    heldRepeatFromTouch = false;
  }
}

void serviceHeldIrRepeat(unsigned long now) {
  if (heldVoiceSearchCommand) {
    lastWakeMs = now;
    if (heldVoiceSearchFromTouch && !lvTouchDown) endVoiceSearchHold();
  }
  if (heldRepeatCommand && heldRepeatFromTouch && !lvTouchDown) {
    endHeldIrCommand();
    return;
  }
  if (!heldRepeatCommand || (int32_t)(now - nextIrRepeatMs) < 0) return;
  transmitRuntimeCommand(heldRepeatCommand);
  nextIrRepeatMs = now + heldRepeatIntervalMs;
}

void serviceActivitySequence(unsigned long now) {
  if (!activitySequenceActive || (int32_t)(now - activitySequenceNextMs) < 0) return;
  ActivityStep *steps = nullptr;
  uint8_t stepCount = 0;
  const char *sequenceName = "Sequence";
  if (activitySequenceMacro >= 0) {
    if (activitySequenceMacro >= MACRO_COUNT) {
      activitySequenceActive = false;
      return;
    }
    Macro &macro = macros[activitySequenceMacro];
    steps = macro.steps;
    stepCount = macro.stepCount;
    sequenceName = macro.name;
  } else {
    if (activitySequenceActivity >= ACTIVITY_COUNT) {
      activitySequenceActive = false;
      return;
    }
    Activity &activity = activities[activitySequenceActivity];
    steps = activity.steps;
    stepCount = activity.stepCount;
    sequenceName = activity.name;
  }
  if (activitySequenceStep >= stepCount) {
    activitySequenceActive = false;
    Serial.printf("Runtime sequence complete: %s\n", sequenceName);
    return;
  }

  ActivityStep &step = steps[activitySequenceStep++];
  if (step.kind == ActivityStep::DELAY) {
    bool shouldWait = !step.delayWhenDevicePoweredOn ||
      (step.deviceIndex < DEVICE_COUNT && step.deviceIndex < 16 &&
       (activitySequencePoweredOnMask & (uint16_t)(1U << step.deviceIndex)) != 0);
    if (shouldWait) {
      Serial.printf("Activity delay: %u ms%s\n", step.delayMs,
                    step.delayWhenDevicePoweredOn ? " after device power-on" : "");
      activitySequenceNextMs = now + step.delayMs;
    } else {
      Serial.println("Activity delay skipped: device was already on");
      activitySequenceNextMs = now;
    }
    return;
  }
  if (step.deviceIndex < DEVICE_COUNT &&
      step.commandIndex < devices[step.deviceIndex].commandCount) {
    Device &device = devices[step.deviceIndex];
    // Standalone macros transmit power commands without owning smart-power state.
    int8_t desiredPower = activitySequenceMacro < 0
      ? activityPowerTarget(device, step.commandIndex) : -1;
    RuntimeCommandResult result =
      transmitRuntimeCommand(&device.commands[step.commandIndex], desiredPower);
    if (result == RUNTIME_COMMAND_SENT && desiredPower == 1 && step.deviceIndex < 16) {
      activitySequencePoweredOnMask |= (uint16_t)(1U << step.deviceIndex);
    }
  }
  activitySequenceNextMs = millis() + 120UL;
}

void serviceHardwarePowerHold(unsigned long now) {
  if (!hardwarePowerHeld || hardwarePowerLongTriggered ||
      (uint32_t)(now - hardwarePowerPressedMs) < 7000UL) return;
  hardwarePowerLongTriggered = true;
  Serial.println("Hardware power held for seven seconds: rebooting");
  Serial.flush();
  delay(40);
  ESP.restart();
}

void recordButtonDiagnostic(uint8_t key, uint8_t row, uint8_t col,
                            uint8_t switchNumber, bool pressed) {
  if (!buttonDiagnosticActive || !pressed ||
      buttonDiagnosticRequested >= PHYSICAL_BUTTON_COUNT) return;
  uint8_t target = buttonDiagnosticRequested++;
  buttonDiagnosticKeys[target] = key;
  buttonDiagnosticRows[target] = row;
  buttonDiagnosticColumns[target] = col;
  buttonDiagnosticSwitches[target] = switchNumber;
  Serial.printf("Button diagnostic %s: key=%u row=%u col=%u switch=S%u INT=GPIO%d\n",
                PHYSICAL_BUTTON_NAMES[target], key, row, col, switchNumber, PIN_TCA_INT);
  pendingPageTransition = 0;
  pendingUiRefresh = true;
  lastWakeMs = millis();
}

void serviceKeypad(unsigned long now) {
  if (!tca8418Ready) return;
  bool pollPhysicalVoice = heldVoiceSearchCommand &&
                           !heldVoiceSearchFromTouch &&
                           heldVoiceSearchPhysicalKey;
  if (digitalRead(PIN_TCA_INT) != LOW && !pollPhysicalVoice) return;
  uint8_t eventCount = 0;
  if (!tcaReadRegister(0x03, eventCount)) return;
  eventCount &= 0x0F;
  if (!eventCount) {
    if (digitalRead(PIN_TCA_INT) == LOW) clearTca8418Interrupts();
    return;
  }
  for (uint8_t i = 0; i < eventCount; i++) {
    uint8_t event = 0;
    if (!tcaReadRegister(0x04, event)) break;
    bool pressed = (event & 0x80) != 0;
    uint8_t key = event & 0x7F;
    if (key == 0) continue;
    if (key == 102) {
      if (buttonTestModeActive()) {
        if (displaySleeping && pressed) wakeDisplay();
        if (pressed) beginButtonTest(-2, "Power");
        else endButtonTest(-2);
        continue;
      }
      if (pressed) {
        hardwarePowerHeld = true;
        hardwarePowerLongTriggered = false;
        hardwarePowerPressedMs = now;
        recordButtonDiagnostic(key, 5, 1, 17, true);
      } else {
        bool shortPress = hardwarePowerHeld && !hardwarePowerLongTriggered &&
                          (uint32_t)(now - hardwarePowerPressedMs) < 7000UL;
        hardwarePowerHeld = false;
        if (shortPress && !buttonDiagnosticActive) handleHardwareAllOff();
      }
      continue;
    }
    uint8_t row = (key - 1) / 10;
    uint8_t col = (key - 1) % 10;
    if (row >= 5 || col >= 5) continue;
    uint8_t switchNumber = (4 - row) * 5 + col + 1;
    if (buttonDiagnosticActive) {
      recordButtonDiagnostic(key, row, col, switchNumber, pressed);
      continue;
    }
    int8_t buttonIndex = physicalButtonIndexForSwitch(switchNumber);
    if (buttonIndex < 0) continue;
    if (buttonTestModeActive()) {
      if (displaySleeping && pressed) wakeDisplay();
      if (pressed) beginButtonTest(buttonIndex, PHYSICAL_BUTTON_NAMES[buttonIndex]);
      else endButtonTest(buttonIndex);
      continue;
    }

    PhysicalBinding *binding = nullptr;
    if (activeDevice >= 0 && activeDevice < DEVICE_COUNT) {
      binding = &devices[activeDevice].physicalBindings[buttonIndex];
    } else if (activeActivity >= 0 && activeActivity < ACTIVITY_COUNT) {
      binding = &activities[activeActivity].physicalBindings[buttonIndex];
    }
    DeviceCommand *command = nullptr;
    if (binding && binding->deviceIndex < DEVICE_COUNT &&
        binding->commandIndex < devices[binding->deviceIndex].commandCount) {
      command = &devices[binding->deviceIndex].commands[binding->commandIndex];
    }
    if (pressed) {
      if (displaySleeping) wakeDisplay();
      if (heldVoiceSearchCommand && !heldVoiceSearchFromTouch &&
          heldVoiceSearchPhysicalKey && heldVoiceSearchPhysicalKey != key) {
        Serial.printf("Voice Search: physical key %u replaced by key %u\n",
                      heldVoiceSearchPhysicalKey, key);
        endVoiceSearchHold();
      }
      beginHeldIrCommand(command, physicalRepeatEnabled);
      if (heldVoiceSearchCommand == command && !heldVoiceSearchFromTouch) {
        heldVoiceSearchPhysicalKey = key;
        physicalVoiceOverlayShowAfterMs = now + 180UL;
        Serial.printf("Voice Search: physical key %u pressed\n", key);
      }
    } else {
      if (heldVoiceSearchCommand && !heldVoiceSearchFromTouch &&
          heldVoiceSearchPhysicalKey == key) {
        Serial.printf("Voice Search: physical key %u released\n", key);
        endVoiceSearchHold();
      } else {
        endHeldIrCommand(command);
      }
    }
  }
  clearTca8418Interrupts();
}

void handleHardwareAllOff() {
  if (activeActivity < 0 && activeDevice < 0 && !anyRememberedDeviceOn()) {
    Serial.println("Hardware All Off ignored: all devices are already off");
    return;
  }
  if (displaySleeping) wakeDisplay();
  endHeldIrCommand();
  activitySequenceActive = false;

  uint16_t deviceMask = 0;
  if (activeActivity >= 0 && activeActivity < ACTIVITY_COUNT) {
    deviceMask = activities[activeActivity].deviceMask;
  }
  if (activeDevice >= 0 && activeDevice < DEVICE_COUNT) {
    deviceMask |= (uint16_t)(1U << activeDevice);
  }
  for (uint8_t i = 0; i < DEVICE_COUNT; i++) {
    if (rememberedDeviceOn(devices[i])) deviceMask |= (uint16_t)(1U << i);
  }

  uint8_t sentCount = 0;
  for (uint8_t i = 0; i < DEVICE_COUNT; i++) {
    if (!(deviceMask & (uint16_t)(1U << i))) continue;
    if (devices[i].powerTrackingEnabled && !rememberedDeviceOn(devices[i])) continue;
    int8_t commandIndex = devices[i].powerOffCommandIndex;
    if (commandIndex < 0 || commandIndex >= devices[i].commandCount) continue;
    if (sentCount) delay(100);
    RuntimeCommandResult result = transmitRuntimeCommand(
      &devices[i].commands[commandIndex], 0);
    if (result == RUNTIME_COMMAND_SENT) sentCount++;
  }
  Serial.printf("Hardware All Off: %u device command(s) sent\n", sentCount);

  activeActivity = -1;
  activeDevice = -1;
  pendingDeviceOpen = -1;
  deviceReturnPage = 1;
  buttonDiagnosticActive = false;
  if (deviceModal) {
    lv_obj_del(deviceModal);
    deviceModal = nullptr;
  }
  rebuildPages();
  currentPage = pageCount > 1 ? 1 : 0;
  pendingPageTransition = -1;
  renderAllPageSlots();
  applyBluetoothState();
  lastWakeMs = millis();
}

uint32_t currentLcdOnDuty() {
  // The Rev 5 P-channel MOSFET and LED chain become visually bright at very
  // low electrical duty. A steep curve keeps useful adjustment available
  // through the middle and top of the slider. Zero is dim, never off.
  float level = constrain(brightness, (uint8_t)0, (uint8_t)100) / 100.0f;
  if (brightness >= 100) return BACKLIGHT_PWM_MAX;
  const uint32_t usableDuty = BACKLIGHT_PWM_MAX - BACKLIGHT_MIN_VISIBLE_DUTY;
  return BACKLIGHT_MIN_VISIBLE_DUTY +
         (uint32_t)roundf(powf(level, 5.0f) * usableDuty);
}

void applyBrightness() {
  if (backlightPwmReady) {
    ledcWrite(PIN_LCD_BL, BACKLIGHT_PWM_MAX - currentLcdOnDuty());
  } else {
    // A missing PWM peripheral must fail visibly on, including at slider zero.
    digitalWrite(PIN_LCD_BL, LOW);
  }
}

void initBacklightPwm() {
  backlightPwmReady = ledcAttachChannel(PIN_LCD_BL, BACKLIGHT_PWM_HZ, BACKLIGHT_PWM_BITS,
                                        LCD_BACKLIGHT_PWM_CHANNEL);
  buttonBacklightPwmReady = ledcAttachChannel(PIN_BUTTON_BL, BACKLIGHT_PWM_HZ, BACKLIGHT_PWM_BITS,
                                              BUTTON_BACKLIGHT_PWM_CHANNEL);
  // setup() reveals both backlights only after LVGL has flushed its first
  // complete frame. This also keeps scheduled background NTP wakes invisible.
  if (backlightPwmReady) ledcWrite(PIN_LCD_BL, BACKLIGHT_PWM_MAX);
  else digitalWrite(PIN_LCD_BL, HIGH);
  buttonBacklight(false);
  Serial.printf("Backlight PWM: %s\n", backlightPwmReady ? "ready" : "failed");
  Serial.printf("Button LED PWM: %s\n", buttonBacklightPwmReady ? "ready" : "failed");
}

void suspendBacklightPwmForSleep() {
  if (sleepBacklightPwmSuspended) return;
  if (backlightPwmReady) {
    ledcWrite(PIN_LCD_BL, BACKLIGHT_PWM_MAX);
    ledcDetach(PIN_LCD_BL);
  }
  if (buttonBacklightPwmReady) {
    ledcWrite(PIN_BUTTON_BL, 0);
    ledcDetach(PIN_BUTTON_BL);
  }
  backlightPwmReady = false;
  buttonBacklightPwmReady = false;
  pinMode(PIN_LCD_BL, OUTPUT);
  digitalWrite(PIN_LCD_BL, HIGH);
  pinMode(PIN_BUTTON_BL, OUTPUT);
  digitalWrite(PIN_BUTTON_BL, LOW);
  sleepBacklightPwmSuspended = true;
}

void restoreBacklightPwmAfterSleep() {
  if (!sleepBacklightPwmSuspended) return;
  backlightPwmReady = ledcAttachChannel(
    PIN_LCD_BL, BACKLIGHT_PWM_HZ, BACKLIGHT_PWM_BITS,
    LCD_BACKLIGHT_PWM_CHANNEL);
  buttonBacklightPwmReady = ledcAttachChannel(
    PIN_BUTTON_BL, BACKLIGHT_PWM_HZ, BACKLIGHT_PWM_BITS,
    BUTTON_BACKLIGHT_PWM_CHANNEL);
  if (backlightPwmReady) ledcWrite(PIN_LCD_BL, BACKLIGHT_PWM_MAX);
  else {
    pinMode(PIN_LCD_BL, OUTPUT);
    digitalWrite(PIN_LCD_BL, HIGH);
  }
  buttonBacklight(false);
  sleepBacklightPwmSuspended = false;
}

void fadeBacklightsToOff() {
  const uint32_t lcdStartDuty = currentLcdOnDuty();
  const uint16_t stepDelay = BACKLIGHT_FADE_MS / BACKLIGHT_FADE_STEPS;

  for (uint8_t step = 0; step <= BACKLIGHT_FADE_STEPS; step++) {
    uint8_t remaining = BACKLIGHT_FADE_STEPS - step;
    uint32_t lcdDuty = (lcdStartDuty * remaining) / BACKLIGHT_FADE_STEPS;
    uint32_t buttonDuty = (BACKLIGHT_PWM_MAX * remaining) / BACKLIGHT_FADE_STEPS;

    if (backlightPwmReady) {
      ledcWrite(PIN_LCD_BL, BACKLIGHT_PWM_MAX - lcdDuty);
    } else if (step == BACKLIGHT_FADE_STEPS) {
      digitalWrite(PIN_LCD_BL, HIGH);
    }

    if (buttonBacklightPwmReady) {
      ledcWrite(PIN_BUTTON_BL, buttonDuty);
    } else if (step == BACKLIGHT_FADE_STEPS) {
      digitalWrite(PIN_BUTTON_BL, LOW);
    }

    if (step < BACKLIGHT_FADE_STEPS) delay(stepDelay);
  }
}

void lcdBacklight(bool on) {
  if (!on) {
    buttonBacklight(false);
    if (backlightPwmReady) ledcWrite(PIN_LCD_BL, BACKLIGHT_PWM_MAX);
    else digitalWrite(PIN_LCD_BL, HIGH);
    return;
  }
  buttonBacklight(true);
  applyBrightness();
}

void irLed(bool on) {
  digitalWrite(PIN_IR_LED, on ? LOW : HIGH);
}

bool i2cDevicePresent(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

bool readBytes(uint8_t address, uint8_t startReg, uint8_t *data, uint8_t length) {
  Wire.beginTransmission(address);
  Wire.write(startReg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)address, (int)length) != length) return false;
  for (uint8_t i = 0; i < length; i++) data[i] = Wire.read();
  return true;
}

bool readReg8(uint8_t address, uint8_t reg, uint8_t &value) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)address, 1) != 1) return false;
  value = Wire.read();
  return true;
}

bool writeReg8(uint8_t address, uint8_t reg, uint8_t value) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool readReg16BE(uint8_t address, uint8_t reg, uint16_t &value) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)address, 2) != 2) return false;
  uint8_t msb = Wire.read();
  uint8_t lsb = Wire.read();
  value = ((uint16_t)msb << 8) | lsb;
  return true;
}

enum TouchSampleStatus : int8_t {
  TOUCH_SAMPLE_INVALID = -1,
  TOUCH_SAMPLE_RELEASED = 0,
  TOUCH_SAMPLE_PRESSED = 1
};

TouchSampleStatus readTouchSample(uint16_t &x, uint16_t &y) {
  uint8_t data[5];
  if (!readBytes(ADDR_TOUCH, 0x02, data, 5)) return TOUCH_SAMPLE_INVALID;
  uint8_t pointCount = data[0] & 0x0F;
  if (pointCount == 0) return TOUCH_SAMPLE_RELEASED;
  if (pointCount != 1) return TOUCH_SAMPLE_INVALID;

  uint16_t rawX = ((uint16_t)(data[1] & 0x0F) << 8) | data[2];
  uint16_t rawY = ((uint16_t)(data[3] & 0x0F) << 8) | data[4];
  if (rawX >= LCD_W || rawY >= LCD_H) return TOUCH_SAMPLE_INVALID;

  // The LCD now uses its normal rotation (0). The touch controller's native
  // coordinates run in the opposite direction, so rotate both axes by 180
  // degrees to keep touches aligned with the displayed controls.
  x = (LCD_W - 1) - rawX;
  y = (LCD_H - 1) - rawY;
  return TOUCH_SAMPLE_PRESSED;
}

bool readTouch(uint16_t &x, uint16_t &y) {
  // Firmware 2.09's direct one-frame read was the proven stable path on this
  // Rev 5 board. Extra matching reads multiply traffic on the shared I2C bus
  // and can push the ESP32 Arduino driver into ESP_ERR_INVALID_STATE.
  return readTouchSample(x, y) == TOUCH_SAMPLE_PRESSED;
}

void sleepTouchController() {
  // Do not put the FT5x06 into register-level hibernate here. On Rev 5 the
  // touch controller shares the LCD rail and does not reliably accept the I2C
  // wake sequence afterwards. Keep the low-current controller awake and reset
  // only LVGL's gesture state before the display sleeps.
  if (touchInputDevice) lv_indev_reset(touchInputDevice, nullptr);
  lvTouchDown = false;
  touchWasDown = false;
  touchPendingConfirmCount = 0;
  touchQuarantineActive = true;
  touchQuarantineStartedMs = millis();
  touchAcceptAfterMs = millis() + 80UL;
  touchReleasedSinceMs = 0;
}

void wakeTouchController(uint32_t settleMs = 80) {
  if (touchInputDevice) lv_indev_reset(touchInputDevice, nullptr);
  lvTouchDown = false;
  touchWasDown = false;
  touchPendingConfirmCount = 0;
  touchQuarantineActive = true;
  touchQuarantineStartedMs = millis();
  touchAcceptAfterMs = millis() + settleMs;
  touchReleasedSinceMs = 0;
}

bool initialiseTouchController() {
  // lcdPowerOn() already gave the shared display/touch rail a clean reset
  // before Wire began. Wait for the controller's real address ACK without
  // dismantling or power-cycling an active I2C bus.
  bool addressReady = false;
  for (uint8_t retry = 0; retry < 12 && !addressReady; retry++) {
    addressReady = i2cDevicePresent(ADDR_TOUCH);
    if (!addressReady) delay(20);
  }
  if (!addressReady) {
    touchFound = false;
    return false;
  }

  for (uint8_t attempt = 0; attempt < 3; attempt++) {
    writeReg8(ADDR_TOUCH, 0x00, 0x00);
    // PWR_MODE 0 is the stable continuously-scanned mode used by 2.09.
    // Monitor mode (1) can defer scans and replay stale coordinates.
    writeReg8(ADDR_TOUCH, 0xA5, 0x00);
    delay(12);
    uint8_t chipId = 0;
    if (readReg8(ADDR_TOUCH, 0xA3, chipId) && chipId != 0x00 && chipId != 0xFF) {
      writeReg8(ADDR_TOUCH, 0xA4, 0x00);
      touchFound = true;
      wakeTouchController(120);
      Serial.printf("Touch: controller ready, id=0x%02X attempt=%u\n",
                    chipId, (unsigned)(attempt + 1));
      return true;
    }
    delay(25);
  }
  touchFound = false;
  return false;
}

bool recoverTouchControllerPower() {
  Serial.println("Touch: no ACK, retrying a full display/touch rail cycle");
  Wire.end();
  pinMode(PIN_I2C_SDA, INPUT_PULLUP);
  pinMode(PIN_I2C_SCL, INPUT_PULLUP);
  digitalWrite(PIN_LCD_EN, HIGH);
  delay(180);
  digitalWrite(PIN_LCD_EN, LOW);
  delay(240);
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(400000);
  return initialiseTouchController();
}

uint8_t normaliseDeepSleepMinutes(int minutes) {
  if (minutes <= 1) return 1;
  minutes = constrain(minutes, 5, 30);
  return (uint8_t)(((minutes + 2) / 5) * 5);
}

uint8_t deepSleepSliderIndex(uint8_t minutes) {
  if (minutes <= 1) return 0;
  return (uint8_t)constrain((minutes / 5), 1, 6);
}

uint8_t deepSleepMinutesForIndex(int index) {
  return index <= 0 ? 1 : (uint8_t)(constrain(index, 1, 6) * 5);
}

bool configureLis3dhAwake() {
  if (!lis3dhReady) return false;
  // 100 Hz, all axes, high-resolution +/-2g while the UI is awake.
  return writeReg8(ADDR_LIS3DH, 0x22, 0x00) &&
         writeReg8(ADDR_LIS3DH, 0x30, 0x00) &&
         writeReg8(ADDR_LIS3DH, 0x20, 0x57) &&
         writeReg8(ADDR_LIS3DH, 0x21, 0x00) &&
         writeReg8(ADDR_LIS3DH, 0x23, 0x88) &&
         writeReg8(ADDR_LIS3DH, 0x24, 0x00) &&
         writeReg8(ADDR_LIS3DH, 0x25, 0x00);
}

uint8_t motionWakeAngleDegrees() {
  return (uint8_t)map(constrain(wakeSensitivity, (uint8_t)1, (uint8_t)100),
                      1, 100, MOTION_WAKE_LEAST_SENSITIVE_DEGREES,
                      MOTION_WAKE_MOST_SENSITIVE_DEGREES);
}

uint8_t motionWakeInterruptThreshold() {
  return (uint8_t)map(constrain(wakeSensitivity, (uint8_t)1, (uint8_t)100),
                      1, 100, MOTION_WAKE_LEAST_SENSITIVE_THRESHOLD,
                      MOTION_WAKE_MOST_SENSITIVE_THRESHOLD);
}

bool configureLis3dhBleIdleOrientation() {
  if (!lis3dhReady || !raiseToWake) return false;
  // BLE idle keeps the CPU available for HID, so use low-rate raw orientation
  // samples rather than a high-pass interrupt. This measures a real pickup
  // angle and cannot retrigger from the accelerometer mode-change transient.
  return writeReg8(ADDR_LIS3DH, 0x22, 0x00) &&
         writeReg8(ADDR_LIS3DH, 0x30, 0x00) &&
         writeReg8(ADDR_LIS3DH, 0x20, 0x37) &&
         writeReg8(ADDR_LIS3DH, 0x21, 0x00) &&
         writeReg8(ADDR_LIS3DH, 0x23, 0x80) &&
         writeReg8(ADDR_LIS3DH, 0x24, 0x00) &&
         writeReg8(ADDR_LIS3DH, 0x25, 0x00);
}

bool configureLis3dhMotionWake() {
  if (!lis3dhReady || !raiseToWake) return false;
  // A 25 Hz normal-mode sample rate responds promptly to lifting while drawing
  // less current than the previous 50 Hz mode. The low-cutoff high-pass
  // filter retains slow movement while rejecting the stationary gravity
  // vector. INT1_THS is 16 mg/LSB at +/-2g.
  uint8_t threshold = motionWakeInterruptThreshold();
  bool configured = writeReg8(ADDR_LIS3DH, 0x22, 0x00) &&
                    writeReg8(ADDR_LIS3DH, 0x30, 0x00) &&
                    writeReg8(ADDR_LIS3DH, 0x20, 0x37) &&
                    writeReg8(ADDR_LIS3DH, 0x21, 0x09) &&
                    writeReg8(ADDR_LIS3DH, 0x23, 0x80) &&
                    writeReg8(ADDR_LIS3DH, 0x24, 0x08) &&
                    writeReg8(ADDR_LIS3DH, 0x32, threshold) &&
                    writeReg8(ADDR_LIS3DH, 0x33, 0x01);
  if (!configured) return false;

  // Let the high-pass filter settle before an interrupt can reach GPIO2. A
  // startup transient otherwise looks like movement and wakes immediately.
  uint8_t ignored = 0;
  delay(400);                            // Collect fresh stationary samples.
  readReg8(ADDR_LIS3DH, 0x26, ignored);  // Establish HPF reference from them.
  delay(80);
  readReg8(ADDR_LIS3DH, 0x31, ignored);  // Clear a stale latched event.
  configured = writeReg8(ADDR_LIS3DH, 0x30, 0x2A) &&
               writeReg8(ADDR_LIS3DH, 0x22, 0x40);
  Serial.printf("LIS3DH wake: sensitivity=%u threshold=%u (%u mg) pin=%d\n",
                wakeSensitivity, threshold, threshold * 16U,
                digitalRead(PIN_ACC_INT));
  return configured;
}

bool configureLis3dhDeepSleepWake() {
  if (!lis3dhReady || !raiseToWake) return false;

  // Match the OMOTE Community Rev 5 standby setup. Its deliberately high
  // threshold ignores table vibration while still detecting a real pickup.
  // INT1 is latched and active-low so GPIO2 can share an EXT1 ANY_LOW wake
  // mask with the active-low TCA8418 keypad interrupt on GPIO8.
  uint8_t ignored = 0;
  uint8_t threshold = motionWakeInterruptThreshold();
  bool configured = readReg8(ADDR_LIS3DH, 0x31, ignored) &&
                    writeReg8(ADDR_LIS3DH, 0x22, 0x00) &&
                    writeReg8(ADDR_LIS3DH, 0x30, 0x00) &&
                    writeReg8(ADDR_LIS3DH, 0x20, 0x47) &&
                    writeReg8(ADDR_LIS3DH, 0x21, 0x09) &&
                    writeReg8(ADDR_LIS3DH, 0x23, 0x80) &&
                    writeReg8(ADDR_LIS3DH, 0x24, 0x08) &&
                    writeReg8(ADDR_LIS3DH, 0x25, 0x02) &&
                    writeReg8(ADDR_LIS3DH, 0x32, threshold) &&
                    writeReg8(ADDR_LIS3DH, 0x33, 0x00);
  if (!configured) return false;

  // Gravity exceeds the configured movement threshold on at least one axis.
  // Establish a stationary high-pass reference before routing the interrupt;
  // otherwise INT1 remains asserted and prevents ESP32 deep-sleep entry.
  delay(400);
  readReg8(ADDR_LIS3DH, 0x26, ignored);
  delay(80);
  readReg8(ADDR_LIS3DH, 0x31, ignored);
  configured = writeReg8(ADDR_LIS3DH, 0x30, 0x2A) &&
               writeReg8(ADDR_LIS3DH, 0x22, 0x60);
  if (!configured) return false;
  delay(30);
  readReg8(ADDR_LIS3DH, 0x31, ignored);
  Serial.printf("LIS3DH deep wake: sensitivity=%u angle=%u deg threshold=%u (%u mg) pin=%d\n",
                wakeSensitivity, motionWakeAngleDegrees(), threshold,
                threshold * 16U, digitalRead(PIN_ACC_INT));
  return digitalRead(PIN_ACC_INT) == HIGH;
}

bool waitForDeepWakeInputsIdle(uint16_t timeoutMs = 500) {
  unsigned long started = millis();
  unsigned long idleSince = 0;
  while ((uint32_t)(millis() - started) < timeoutMs) {
    if (digitalRead(PIN_ACC_INT) == LOW) {
      uint8_t ignored = 0;
      readReg8(ADDR_LIS3DH, 0x31, ignored);
    }
    if (digitalRead(PIN_TCA_INT) == LOW) serviceKeypad(millis());

    bool idle = digitalRead(PIN_ACC_INT) == HIGH &&
                (!tca8418Ready || digitalRead(PIN_TCA_INT) == HIGH);
    if (!idle) idleSince = 0;
    else if (idleSince == 0) idleSince = millis();
    else if ((uint32_t)(millis() - idleSince) >= 80UL) return true;
    delay(10);
  }
  Serial.printf("Deep wake input busy: accelerometer=%d keypad=%d\n",
                digitalRead(PIN_ACC_INT), digitalRead(PIN_TCA_INT));
  return false;
}

bool waitForWakeInputsIdle(bool accelerometerWake, bool keypadWake,
                           uint16_t timeoutMs = 320) {
  unsigned long started = millis();
  unsigned long idleSince = 0;
  while ((uint32_t)(millis() - started) < timeoutMs) {
    if (keypadWake && digitalRead(PIN_TCA_INT) == LOW) serviceKeypad(millis());
    if (accelerometerWake && digitalRead(PIN_ACC_INT) == HIGH) {
      uint8_t ignored = 0;
      readReg8(ADDR_LIS3DH, 0x31, ignored);
    }
    bool idle = (!accelerometerWake || digitalRead(PIN_ACC_INT) == LOW) &&
                (!keypadWake || digitalRead(PIN_TCA_INT) == HIGH);
    if (!idle) idleSince = 0;
    else if (idleSince == 0) idleSince = millis();
    else if ((uint32_t)(millis() - idleSince) >= 80UL) return true;
    delay(10);
  }
  return false;
}

void initLIS3DH() {
  pinMode(PIN_ACC_INT, INPUT);
  uint8_t who = 0;
  lis3dhReady = readReg8(ADDR_LIS3DH, 0x0F, who) && who == 0x33;
  if (lis3dhReady) configureLis3dhAwake();
}

bool readLIS3DH(int16_t &x, int16_t &y, int16_t &z) {
  uint8_t data[6];
  if (!readBytes(ADDR_LIS3DH, 0x28 | 0x80, data, 6)) return false;
  x = ((int16_t)((uint16_t)data[1] << 8 | data[0])) >> 4;
  y = ((int16_t)((uint16_t)data[3] << 8 | data[2])) >> 4;
  z = ((int16_t)((uint16_t)data[5] << 8 | data[4])) >> 4;
  return true;
}

uint16_t movementDelta() {
  if (!lis3dhReady) return 0;
  int16_t x, y, z;
  if (!readLIS3DH(x, y, z)) return 0;
  return max((uint16_t)abs(x - sleepBaseX), max((uint16_t)abs(y - sleepBaseY), (uint16_t)abs(z - sleepBaseZ)));
}

float movementAngleDegrees() {
  if (!lis3dhReady) return 0.0f;
  int16_t x, y, z;
  if (!readLIS3DH(x, y, z)) return 0.0f;
  float baseMagnitude = sqrtf((float)sleepBaseX * sleepBaseX +
                              (float)sleepBaseY * sleepBaseY +
                              (float)sleepBaseZ * sleepBaseZ);
  float currentMagnitude = sqrtf((float)x * x + (float)y * y + (float)z * z);
  if (baseMagnitude < 100.0f || currentMagnitude < 100.0f) return 0.0f;
  float cosine = ((float)sleepBaseX * x + (float)sleepBaseY * y +
                  (float)sleepBaseZ * z) / (baseMagnitude * currentMagnitude);
  cosine = constrain(cosine, -1.0f, 1.0f);
  return acosf(cosine) * 57.2957795f;
}

float readBatteryPercent() {
  uint16_t soc = 0;
  if (!readReg16BE(ADDR_MAX17048, 0x04, soc)) return -1.0f;
  float percent = (float)(soc >> 8) + ((float)(soc & 0xFF) / 256.0f);
  return constrain(percent, 0.0f, 100.0f);
}

float readBatteryVoltage() {
  uint16_t raw = 0;
  if (!readReg16BE(ADDR_MAX17048, 0x02, raw)) return -1.0f;
  return (float)raw * 0.000078125f;
}

float readBatteryRatePerHour() {
  uint16_t raw = 0;
  if (!readReg16BE(ADDR_MAX17048, 0x16, raw)) return NAN;
  return (float)(int16_t)raw * 0.208f;
}

struct BatteryMetrics {
  float percent = -1.0f;
  float voltage = -1.0f;
  float ratePerHour = NAN;
  float change1h = NAN;
  float change24h = NAN;
  float estimatedHours = NAN;
  bool chargerConnected = false;
};

void saveBatteryPowerMode() {
  preferences.begin(PREFERENCES_NAMESPACE, false);
  preferences.putBool("batModeKnown", batteryPowerModeKnown);
  preferences.putBool("batModePlug", batteryPowerModeCharging);
  preferences.putULong("batModeAt", batteryPowerModeChangedEpoch);
  preferences.putFloat("batModePct", batteryPowerModeStartPercent);
  preferences.end();
}

void resetBatteryMeasurementWindow(bool chargerConnected) {
  time_t epoch = time(nullptr);
  batteryPowerModeKnown = true;
  batteryPowerModeCharging = chargerConnected;
  batteryPowerModeChangedEpoch = epoch >= 1700000000 ? (uint32_t)epoch : 0;
  batteryPowerModeStartPercent = readBatteryPercent();
  saveBatteryPowerMode();
  Serial.printf("Battery hour reset: charger=%s level=%.2f%% epoch=%lu\n",
                chargerConnected ? "connected" : "removed",
                batteryPowerModeStartPercent,
                (unsigned long)batteryPowerModeChangedEpoch);
}

bool batteryPercentAtEpoch(uint32_t targetEpoch, uint32_t minimumEpoch,
                           float &percent) {
  BatteryHistorySample before = {};
  BatteryHistorySample after = {};
  bool haveBefore = false;
  bool haveAfter = false;

  auto consider = [&](uint32_t epoch, float samplePercent) {
    if (!epoch || epoch < minimumEpoch) return;
    if (epoch <= targetEpoch && (!haveBefore || epoch > before.epoch)) {
      before = {epoch, samplePercent};
      haveBefore = true;
    }
    if (epoch >= targetEpoch && (!haveAfter || epoch < after.epoch)) {
      after = {epoch, samplePercent};
      haveAfter = true;
    }
  };

  if (batteryPowerModeChangedEpoch >= minimumEpoch &&
      batteryPowerModeStartPercent >= 0.0f) {
    consider(batteryPowerModeChangedEpoch, batteryPowerModeStartPercent);
  }
  for (uint8_t i = 0; i < batteryHistory.count && i < 49; i++) {
    const BatteryHistorySample &sample = batteryHistory.samples[i];
    consider(sample.epoch, sample.percent);
  }

  if (haveBefore && haveAfter) {
    if (before.epoch == after.epoch) {
      percent = before.percent;
    } else {
      float position = (float)(targetEpoch - before.epoch) /
                       (float)(after.epoch - before.epoch);
      percent = before.percent + (after.percent - before.percent) * position;
    }
    return true;
  }
  const uint32_t tolerance = BATTERY_SAMPLE_INTERVAL_SEC + 15UL * 60UL;
  if (haveBefore && targetEpoch - before.epoch <= tolerance) {
    percent = before.percent;
    return true;
  }
  if (haveAfter && after.epoch - targetEpoch <= tolerance) {
    percent = after.percent;
    return true;
  }
  return false;
}

bool batterySampleAtOrBefore(uint32_t epoch, BatteryHistorySample &found) {
  bool have = false;
  for (uint8_t i = 0; i < batteryHistory.count && i < 49; i++) {
    const BatteryHistorySample &sample = batteryHistory.samples[i];
    if (!sample.epoch || sample.epoch > epoch) continue;
    if (!have || sample.epoch > found.epoch) {
      found = sample;
      have = true;
    }
  }
  return have;
}

bool oldestBatterySample(BatteryHistorySample &found) {
  bool have = false;
  for (uint8_t i = 0; i < batteryHistory.count && i < 49; i++) {
    const BatteryHistorySample &sample = batteryHistory.samples[i];
    if (!sample.epoch) continue;
    if (!have || sample.epoch < found.epoch) {
      found = sample;
      have = true;
    }
  }
  return have;
}

BatteryMetrics currentBatteryMetrics() {
  BatteryMetrics metrics;
  metrics.chargerConnected = updateChargingState();
  metrics.percent = readBatteryPercent();
  metrics.voltage = readBatteryVoltage();
  metrics.ratePerHour = readBatteryRatePerHour();
  time_t nowTime = time(nullptr);
  if (metrics.percent < 0.0f || nowTime < 1700000000) return metrics;

  if (!batteryPowerModeKnown || !batteryPowerModeChangedEpoch) {
    resetBatteryMeasurementWindow(metrics.chargerConnected);
  }

  BatteryHistorySample day = {};
  uint32_t nowEpoch = (uint32_t)nowTime;
  if (batteryPowerModeChangedEpoch &&
      nowEpoch >= batteryPowerModeChangedEpoch + 3600UL) {
    float hourPercent = NAN;
    if (batteryPercentAtEpoch(nowEpoch - 3600UL,
                              batteryPowerModeChangedEpoch, hourPercent)) {
      metrics.change1h = metrics.percent - hourPercent;
      float displayedHourlyChange = roundf(metrics.change1h * 100.0f) / 100.0f;
      if (metrics.chargerConnected && displayedHourlyChange > 0.005f) {
        metrics.estimatedHours = (100.0f - metrics.percent) / displayedHourlyChange;
      } else if (!metrics.chargerConnected && displayedHourlyChange < -0.005f) {
        metrics.estimatedHours = metrics.percent / -displayedHourlyChange;
      }
    }
  }
  if (batterySampleAtOrBefore(nowEpoch - 86400UL, day)) {
    metrics.change24h = metrics.percent - day.percent;
  }
  return metrics;
}

void loadBatteryHistory() {
  memset(&batteryHistory, 0, sizeof(batteryHistory));
  preferences.begin(PREFERENCES_NAMESPACE, true);
  size_t length = preferences.getBytesLength("batHist");
  if (length == sizeof(batteryHistory)) {
    preferences.getBytes("batHist", &batteryHistory, sizeof(batteryHistory));
  }
  batteryPowerModeKnown = preferences.getBool("batModeKnown", false);
  batteryPowerModeCharging = preferences.getBool("batModePlug", false);
  batteryPowerModeChangedEpoch = preferences.getULong("batModeAt", 0);
  batteryPowerModeStartPercent = preferences.getFloat("batModePct", -1.0f);
  preferences.end();
  if (batteryHistory.magic != BATTERY_HISTORY_MAGIC ||
      batteryHistory.count > 49 || batteryHistory.next >= 49) {
    memset(&batteryHistory, 0, sizeof(batteryHistory));
    batteryHistory.magic = BATTERY_HISTORY_MAGIC;
  }
}

void saveBatteryHistory() {
  preferences.begin(PREFERENCES_NAMESPACE, false);
  preferences.putBytes("batHist", &batteryHistory, sizeof(batteryHistory));
  preferences.end();
}

void serviceBatteryHistory(unsigned long now, bool force) {
  if (!force && (int32_t)(now - nextBatteryHistoryCheckMs) < 0) return;
  nextBatteryHistoryCheckMs = now + 60000UL;
  time_t epoch = time(nullptr);
  float percent = readBatteryPercent();
  if (epoch < 1700000000 || percent < 0.0f) return;
  if (!batteryPowerModeKnown || !batteryPowerModeChangedEpoch) {
    resetBatteryMeasurementWindow(chargingState);
  }

  BatteryHistorySample newest = {};
  for (uint8_t i = 0; i < batteryHistory.count && i < 49; i++) {
    if (batteryHistory.samples[i].epoch > newest.epoch) newest = batteryHistory.samples[i];
  }
  const uint32_t minimumInterval = force ? 15UL * 60UL : BATTERY_SAMPLE_INTERVAL_SEC;
  if (newest.epoch && (uint32_t)epoch - newest.epoch < minimumInterval) return;

  batteryHistory.magic = BATTERY_HISTORY_MAGIC;
  batteryHistory.samples[batteryHistory.next] = {(uint32_t)epoch, percent};
  batteryHistory.next = (batteryHistory.next + 1) % 49;
  if (batteryHistory.count < 49) batteryHistory.count++;
  saveBatteryHistory();
  Serial.printf("Battery history: %.1f%% at %lu\n", percent, (unsigned long)epoch);
}

bool createSdFolderIfMissing(const char *path) {
  if (SD.exists(path)) return true;
  if (SD.mkdir(path)) {
    Serial.printf("SD folder created: %s\n", path);
    return true;
  }
  Serial.printf("SD folder failed: %s\n", path);
  return false;
}

bool initSdStorage() {
  strlcpy(sdStatusText, "Powering card", sizeof(sdStatusText));
  pinMode(PIN_SD_EN, OUTPUT);
  digitalWrite(PIN_SD_EN, LOW);
  delay(120);

  sdSpi.begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);
  const uint32_t mountSpeeds[] = {20000000, 10000000, 4000000, 1000000};
  bool mounted = false;
  for (uint8_t i = 0; i < sizeof(mountSpeeds) / sizeof(mountSpeeds[0]); i++) {
    SD.end();
    delay(20);
    if (SD.begin(PIN_SD_CS, sdSpi, mountSpeeds[i])) {
      Serial.printf("SD card: mounted at %lu Hz\n", (unsigned long)mountSpeeds[i]);
      snprintf(sdStatusText, sizeof(sdStatusText), "Mounted at %lu MHz",
               (unsigned long)(mountSpeeds[i] / 1000000UL));
      mounted = true;
      break;
    }
    Serial.printf("SD card: mount failed at %lu Hz\n", (unsigned long)mountSpeeds[i]);
  }
  if (!mounted) {
    Serial.println("SD card: not mounted (use FAT32/MS-DOS FAT, not exFAT)");
    strlcpy(sdStatusText, "Mount failed at all SPI speeds", sizeof(sdStatusText));
    digitalWrite(PIN_SD_EN, HIGH);
    return false;
  }

  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("SD card: no card detected");
    strlcpy(sdStatusText, "Mounted bus, no card detected", sizeof(sdStatusText));
    SD.end();
    digitalWrite(PIN_SD_EN, HIGH);
    return false;
  }

  uint64_t cardSizeMb = SD.cardSize() / (1024ULL * 1024ULL);
  Serial.printf("SD card: mounted, %llu MB\n", cardSizeMb);
  snprintf(sdStatusText, sizeof(sdStatusText), "%llu MB mounted", cardSizeMb);

  bool foldersReady = true;
  for (uint8_t i = 0; i < SD_FOLDER_COUNT; i++) {
    foldersReady = createSdFolderIfMissing(sdFolders[i]) && foldersReady;
  }

  Serial.printf("SD folder bootstrap: %s\n", foldersReady ? "ready" : "incomplete");
  if (!foldersReady) strlcpy(sdStatusText, "Mounted, folder setup failed", sizeof(sdStatusText));
  return foldersReady;
}

// ---------------------------------------------------------------------------
// Persistent settings, BLE and WebConfig transport
// ---------------------------------------------------------------------------

String wifiProfileKey(char field, uint8_t index) {
  char key[9];
  snprintf(key, sizeof(key), "wifi%c%u", field, index);
  return String(key);
}

int findWifiProfile(const String &ssid) {
  for (uint8_t i = 0; i < wifiProfileCount; i++) {
    if (wifiProfiles[i].ssid == ssid) return i;
  }
  return -1;
}

bool hasSelectedWifiProfile() {
  return selectedWifiSsid.length() && findWifiProfile(selectedWifiSsid) >= 0;
}

void saveWifiProfiles() {
  preferences.begin(PREFERENCES_NAMESPACE, false);
  preferences.putUChar("wifiCnt", wifiProfileCount);
  for (uint8_t i = 0; i < MAX_WIFI_PROFILES; i++) {
    String ssidKey = wifiProfileKey('S', i);
    String passKey = wifiProfileKey('P', i);
    if (i < wifiProfileCount) {
      preferences.putString(ssidKey.c_str(), wifiProfiles[i].ssid);
      preferences.putString(passKey.c_str(), wifiProfiles[i].password);
    } else {
      preferences.remove(ssidKey.c_str());
      preferences.remove(passKey.c_str());
    }
  }
  int selected = findWifiProfile(selectedWifiSsid);
  preferences.putString("ssid", selected >= 0 ? selectedWifiSsid : "");
  preferences.putString("pass", selected >= 0 ? wifiProfiles[selected].password : "");
  preferences.end();
}

void saveWifiCredentials(const String &ssid, const String &password) {
  if (!ssid.length()) return;
  int index = findWifiProfile(ssid);
  if (index < 0) {
    if (wifiProfileCount < MAX_WIFI_PROFILES) {
      index = wifiProfileCount++;
    } else {
      for (uint8_t i = 1; i < MAX_WIFI_PROFILES; i++) {
        wifiProfiles[i - 1] = wifiProfiles[i];
      }
      index = MAX_WIFI_PROFILES - 1;
    }
  }
  wifiProfiles[index].ssid = ssid;
  wifiProfiles[index].password = password;
  selectedWifiSsid = ssid;
  saveWifiProfiles();
}

void forgetWifiCredentials(const String &ssid) {
  int index = findWifiProfile(ssid);
  if (index < 0) return;
  bool wasSelected = selectedWifiSsid == ssid;
  for (uint8_t i = index + 1; i < wifiProfileCount; i++) {
    wifiProfiles[i - 1] = wifiProfiles[i];
  }
  if (wifiProfileCount) wifiProfileCount--;
  wifiProfiles[wifiProfileCount].ssid = "";
  wifiProfiles[wifiProfileCount].password = "";
  if (wasSelected) selectedWifiSsid = "";
  saveWifiProfiles();
}

String savedWifiPassword(const String &ssid) {
  int index = findWifiProfile(ssid);
  return index >= 0 ? wifiProfiles[index].password : String();
}

void loadSettings() {
  preferences.begin(PREFERENCES_NAMESPACE, true);
  wifiOn = preferences.getBool("wifi", true);
  bluetoothOn = preferences.getBool("ble", true);
  bleBonded = preferences.getBool("bleBonded", false);
  clockEnabled = preferences.getBool("clock", true);
  clockUseInternetTime = preferences.getBool("ntp", true);
  slideToUnlock = preferences.getBool("lock", true);
  brightness = preferences.getUChar("bright", 72);
  timeoutSeconds = preferences.getUChar("sleep", 25);
  deepSleepMinutes = normaliseDeepSleepMinutes(
    preferences.getUChar("deepMin", 10));
  wakeSensitivity = constrain((int)preferences.getUChar("wake", 58), 1, 100);
  displayGamma = constrain((int)preferences.getUShort("gamma", 100), 50, 250);
  displaySaturation = constrain((int)preferences.getUShort("saturation", 100), 0, 200);
  displayRgb666 = preferences.getBool("rgb666", false);
  displayInverted = preferences.getBool("invert", false);
  physicalRepeatEnabled = preferences.getBool("btnRpt", true);
  physicalRepeatDelayMs = constrain(
    (int)preferences.getUShort("btnDelay", 400),
    (int)BUTTON_REPEAT_DELAY_MIN_MS, (int)BUTTON_REPEAT_DELAY_MAX_MS);
  physicalRepeatDelayMs = ((physicalRepeatDelayMs + 25U) / 50U) * 50U;
  physicalRepeatRateHz = constrain(
    (int)preferences.getUChar("btnRate", 9),
    (int)BUTTON_REPEAT_RATE_MIN_HZ, (int)BUTTON_REPEAT_RATE_MAX_HZ);
  debugSplitEnabled = preferences.getBool("dbgSplit", true);
  debugTouchEnabled = preferences.getBool("dbgTouch", false);
  debugCpuRamEnabled = preferences.getBool("dbgCpu", false);
  debugAccelerometerEnabled = preferences.getBool("dbgAccel", false);
  debugFpsEnabled = preferences.getBool("dbgFps", false);
  microphoneTestAudioEnabled = preferences.getBool("micTest", false);
  static const uint16_t defaultRows[5] = {246, 195, 144, 93, 42};
  for (uint8_t i = 0; i < 5; i++) {
    char key[8];
    snprintf(key, sizeof(key), "dbgRow%u", i + 1);
    debugRowPixels[i] = constrain(
      (int)preferences.getUShort(key, defaultRows[i]), 0, LCD_H - 1);
  }
  manualClockEpoch = preferences.getULong64("manualTs", 0);
  clockCityName = preferences.getString("city", "Canberra");
  clockUtcOffsetMinutes = constrain(
    (int)preferences.getShort("utcOffset", 600), -12 * 60, 14 * 60);
  selectedWifiSsid = preferences.getString("ssid", "");
  String legacyWifiPassword = preferences.getString("pass", "");
  wifiProfileCount = min((uint8_t)preferences.getUChar("wifiCnt", 0),
                         MAX_WIFI_PROFILES);
  for (uint8_t i = 0; i < wifiProfileCount; i++) {
    String ssidKey = wifiProfileKey('S', i);
    String passKey = wifiProfileKey('P', i);
    wifiProfiles[i].ssid = preferences.getString(ssidKey.c_str(), "");
    wifiProfiles[i].password = preferences.getString(passKey.c_str(), "");
  }
  homebridgeAddress = preferences.getString("hbAddr", "");
  homebridgeUsername = preferences.getString("hbUser", "");
  homebridgePassword = preferences.getString("hbPass", "");
  remoteName = preferences.getString("remoteName", "OpenRemote");
  preferences.end();
  bool migratedWifiProfile = false;
  if (!wifiProfileCount && selectedWifiSsid.length()) {
    wifiProfiles[0].ssid = selectedWifiSsid;
    wifiProfiles[0].password = legacyWifiPassword;
    wifiProfileCount = 1;
    migratedWifiProfile = true;
  }
  for (int i = wifiProfileCount - 1; i >= 0; i--) {
    if (wifiProfiles[i].ssid.length()) continue;
    for (uint8_t j = i + 1; j < wifiProfileCount; j++) {
      wifiProfiles[j - 1] = wifiProfiles[j];
    }
    wifiProfileCount--;
    migratedWifiProfile = true;
  }
  if (!selectedWifiSsid.length() && wifiProfileCount) {
    selectedWifiSsid = wifiProfiles[0].ssid;
    migratedWifiProfile = true;
  }
  if (migratedWifiProfile) saveWifiProfiles();
  raiseToWake = true;
}

void saveHomebridgeCredentials(const String &address, const String &username,
                               const String &password) {
  homebridgeAddress = address;
  homebridgeUsername = username;
  homebridgePassword = password;
  preferences.begin(PREFERENCES_NAMESPACE, false);
  preferences.putString("hbAddr", homebridgeAddress);
  preferences.putString("hbUser", homebridgeUsername);
  preferences.putString("hbPass", homebridgePassword);
  preferences.end();
}

void saveSettings() {
  preferences.begin(PREFERENCES_NAMESPACE, false);
  preferences.putBool("wifi", wifiOn);
  preferences.putBool("ble", bluetoothOn);
  preferences.putBool("bleBonded", (bool)bleBonded);
  preferences.putBool("clock", clockEnabled);
  preferences.putBool("ntp", clockUseInternetTime);
  preferences.putBool("lock", slideToUnlock);
  preferences.putUChar("bright", brightness);
  preferences.putUChar("sleep", timeoutSeconds);
  preferences.putUChar("deepMin", deepSleepMinutes);
  preferences.putUChar("wake", wakeSensitivity);
  preferences.putUShort("gamma", displayGamma);
  preferences.putUShort("saturation", displaySaturation);
  preferences.putBool("rgb666", displayRgb666);
  preferences.putBool("invert", displayInverted);
  preferences.putBool("btnRpt", physicalRepeatEnabled);
  preferences.putUShort("btnDelay", physicalRepeatDelayMs);
  preferences.putUChar("btnRate", physicalRepeatRateHz);
  preferences.putBool("dbgSplit", debugSplitEnabled);
  preferences.putBool("dbgTouch", debugTouchEnabled);
  preferences.putBool("dbgCpu", debugCpuRamEnabled);
  preferences.putBool("dbgAccel", debugAccelerometerEnabled);
  preferences.putBool("dbgFps", debugFpsEnabled);
  preferences.putBool("micTest", microphoneTestAudioEnabled);
  for (uint8_t i = 0; i < 5; i++) {
    char key[8];
    snprintf(key, sizeof(key), "dbgRow%u", i + 1);
    preferences.putUShort(key, debugRowPixels[i]);
  }
  preferences.putULong64("manualTs", manualClockEpoch);
  preferences.putString("city", clockCityName);
  preferences.putShort("utcOffset", clockUtcOffsetMinutes);
  preferences.putString("remoteName", remoteName);
  preferences.end();
}

bool webConfigQrPageActive() {
  return pages[currentPage].kind == PAGE_REMOTE_SETTINGS &&
         settingsView == SETTINGS_WIFI_QR;
}

void scheduleNetworkShutdown(uint32_t delayMs = NETWORK_IDLE_SHUTDOWN_MS) {
  bool wifiUiActive = pages[currentPage].kind == PAGE_REMOTE_SETTINGS &&
    (settingsView == SETTINGS_WIFI || settingsView == SETTINGS_WIFI_PASSWORD ||
     settingsView == SETTINGS_WIFI_QR);
  if (wifiUiActive || ntpSyncPending || wifiScanPending || wifiConnectPending) return;
  networkShutdownAtMs = millis() + delayMs;
}

bool ensureStationConnected(uint32_t timeoutMs = 15000UL) {
  if (WiFi.status() == WL_CONNECTED) return true;
  if (!wifiOn || !hasSelectedWifiProfile()) return false;
  startNetworkStack();
  unsigned long started = millis();
  while (WiFi.status() != WL_CONNECTED &&
         (uint32_t)(millis() - started) < timeoutMs) {
    serviceKeypad(millis());
    serviceHardwarePowerHold(millis());
    delay(20);
  }
  return WiFi.status() == WL_CONNECTED;
}

String clockTimezoneRule() {
  if (clockUtcOffsetMinutes == 600 &&
      (clockCityName == "Canberra" || clockCityName == "Sydney" ||
       clockCityName == "Melbourne")) {
    return OPENREMOTE_TZ;
  }
  int offset = constrain((int)clockUtcOffsetMinutes, -12 * 60, 14 * 60);
  char rule[20];
  char sign = offset >= 0 ? '-' : '+';
  offset = abs(offset);
  snprintf(rule, sizeof(rule), "UTC%c%02d:%02d", sign, offset / 60, offset % 60);
  return String(rule);
}

void applyClockMode() {
  String timezone = clockTimezoneRule();
  setenv("TZ", timezone.c_str(), 1);
  tzset();
  if (clockUseInternetTime) return;
  if (manualClockEpoch > 0) {
    timeval now = {(time_t)manualClockEpoch, 0};
    settimeofday(&now, nullptr);
  }
}

void requestInternetTimeSync() {
  if (!clockUseInternetTime || !wifiOn || !hasSelectedWifiProfile()) return;
  ntpSyncPending = true;
  ntpConfigured = false;
  ntpSyncStartedMs = millis();
  networkShutdownAtMs = 0;
  if (!setupApActive || WiFi.status() != WL_CONNECTED) startNetworkStack();
  Serial.println("NTP: sync requested");
}

void serviceInternetTime(unsigned long now) {
  if (ntpSyncPending) {
    if ((uint32_t)(now - ntpSyncStartedMs) > NTP_SYNC_TIMEOUT_MS) {
      Serial.println("NTP: sync timed out");
      ntpSyncPending = false;
      scheduleNetworkShutdown();
      return;
    }
    if (WiFi.status() == WL_CONNECTED && !ntpConfigured) {
      String timezone = clockTimezoneRule();
      configTzTime(timezone.c_str(), "pool.ntp.org", "time.google.com");
      ntpConfigured = true;
    }
    time_t epoch = time(nullptr);
    if (ntpConfigured && epoch > 1700000000 &&
        esp_sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
      tm local = {};
      localtime_r(&epoch, &local);
      lastNtpSyncYDay = local.tm_yday;
      ntpSyncPending = false;
      serviceBatteryHistory(now, true);
      scheduleNetworkShutdown();
      Serial.println("NTP: time updated");
    }
    return;
  }

  if (!clockUseInternetTime) return;
  time_t epoch = time(nullptr);
  if (epoch < 1700000000) return;
  tm local = {};
  localtime_r(&epoch, &local);
  if (local.tm_hour == 3 && local.tm_yday != lastNtpSyncYDay) {
    requestInternetTimeSync();
  }
}

void serviceNetworkPower(unsigned long now) {
  if (!networkShutdownAtMs || (int32_t)(now - networkShutdownAtMs) < 0) return;
  networkShutdownAtMs = 0;
  bool wifiUiActive = pages[currentPage].kind == PAGE_REMOTE_SETTINGS &&
    (settingsView == SETTINGS_WIFI || settingsView == SETTINGS_WIFI_PASSWORD ||
     settingsView == SETTINGS_WIFI_QR);
  if (!wifiUiActive && !ntpSyncPending && !wifiScanPending && !wifiConnectPending) {
    if (bluetoothActivitySessionRequired()) parkNetworkStackForBle();
    else stopNetworkStack();
  }
}

void loadIrdbMetadata() {
  strlcpy(irdbBuildDate, sdReady && SD.exists(IRDB_PATH) ? "Unknown build" : "Not installed",
          sizeof(irdbBuildDate));
  irdbDeviceCount = 0;
  if (!sdReady) return;

  const char *manifestPath = SD.exists(IRDB_MANIFEST_PATH)
    ? IRDB_MANIFEST_PATH
    : (SD.exists(IRDB_ALT_MANIFEST_PATH) ? IRDB_ALT_MANIFEST_PATH : nullptr);
  if (!manifestPath) return;

  File file = SD.open(manifestPath, FILE_READ);
  if (!file) return;
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  if (error) {
    Serial.printf("IRDB manifest parse failed: %s\n", error.c_str());
    return;
  }

  const char *created = doc["created_date"] | doc["scrape_date_local"] | doc["database_version"] | "";
  if (created[0]) strlcpy(irdbBuildDate, created, sizeof(irdbBuildDate));
  irdbDeviceCount = doc["device_count"] | 0;
  Serial.printf("IRDB metadata: %s, %lu devices\n", irdbBuildDate, (unsigned long)irdbDeviceCount);
}

void refreshBluetoothBondState() {
#if defined(CONFIG_BLUEDROID_ENABLED)
  // Retain the persisted host-known flag. Some Android TV hosts reconnect as
  // HID clients even when Bluedroid's bond count is briefly unavailable.
  bleBonded = bleBonded || esp_ble_get_bond_device_num() > 0;
#endif
}

void advertiseBluetoothHid() {
  if (!bleReady || bleConnected || bleSuspended) return;
  BLEDevice::startAdvertising();
  Serial.printf("BLE HID: advertising %s%s\n", BLE_HID_NAME,
                blePairingMode ? " for pairing" : " for reconnect");
}

void requestBluetoothConnectionProfile(bool idle) {
#if defined(CONFIG_BLUEDROID_ENABLED)
  if (!bleConnected || !bleServer || !blePeerAddressValid) return;
  bleServer->updateConnParams(
    blePeerAddress,
    idle ? BLE_CONN_IDLE_MIN_INTERVAL : BLE_CONN_ACTIVE_MIN_INTERVAL,
    idle ? BLE_CONN_IDLE_MAX_INTERVAL : BLE_CONN_ACTIVE_MAX_INTERVAL,
    idle ? BLE_CONN_IDLE_LATENCY : 0,
    BLE_CONN_SUPERVISION_TIMEOUT);
  bleIdleConnectionProfileRequested = idle;
  Serial.printf("BLE HID: %s connection profile requested\n",
                idle ? "idle" : "responsive");
#else
  (void)idle;
#endif
}

const char *atvvOpcodeName(uint8_t opcode) {
  switch (opcode) {
    case ATVV_AUDIO_STOP: return "AUDIO_STOP";
    case ATVV_AUDIO_START: return "AUDIO_START";
    case ATVV_START_SEARCH: return "START_SEARCH";
    case ATVV_GET_CAPS: return "GET_CAPS";
    case ATVV_CAPS_RESP: return "CAPS_RESP";
    case ATVV_MIC_OPEN: return "MIC_OPEN";
    case ATVV_MIC_CLOSE: return "MIC_CLOSE";
    default: return "UNKNOWN";
  }
}

bool atvvNotifyControlPacket(const uint8_t *packet, size_t length) {
  if (!packet || !length) return false;
  if (!bleConnected || !atvvCtl || !atvvCtlCccd) return false;
  atvvCtlSubscribed = atvvCtlCccd->getNotifications();
  if (!atvvCtlSubscribed) {
#if OPENREMOTE_ATVV_DEBUG
    Serial.printf("ATVV CTL %s not sent: Chromecast has not subscribed\n",
                  atvvOpcodeName(packet[0]));
#endif
    return false;
  }
  if (atvvNotifyMutex &&
      xSemaphoreTake(atvvNotifyMutex, pdMS_TO_TICKS(100)) != pdTRUE) {
    Serial.println("ATVV CTL not sent: notification worker busy");
    return false;
  }
  atvvCtl->setValue((uint8_t *)packet, length);
  atvvCtl->notify();
  if (atvvNotifyMutex) xSemaphoreGive(atvvNotifyMutex);
#if OPENREMOTE_ATVV_DEBUG
  Serial.print("ATVV CTL notify:");
  for (size_t i = 0; i < length; i++) Serial.printf(" %02X", packet[i]);
  Serial.printf("  %s\n", atvvOpcodeName(packet[0]));
#endif
  return true;
}

bool atvvNotifyControl(uint8_t opcode) {
  return atvvNotifyControlPacket(&opcode, 1);
}

bool atvvSendAudioSync(uint16_t frameNumber, int16_t predictor,
                       uint8_t stepIndex) {
  uint16_t predictorBits = static_cast<uint16_t>(predictor);
  uint8_t packet[] = {
    ATVV_AUDIO_SYNC,
    ATVV_CODEC_ADPCM_8KHZ,
    static_cast<uint8_t>(frameNumber >> 8),
    static_cast<uint8_t>(frameNumber & 0xFF),
    static_cast<uint8_t>(predictorBits >> 8),
    static_cast<uint8_t>(predictorBits & 0xFF),
    stepIndex
  };
  return atvvNotifyControlPacket(packet, sizeof(packet));
}

bool atvvSendCapabilities() {
  uint16_t version = atvvHostSpecVersion >= 0x0100 ? 0x0100 : 0x0004;
  uint8_t model = atvvHostInteractionModels == ATVV_INTERACTION_HOLD_TO_TALK
    ? ATVV_INTERACTION_HOLD_TO_TALK
    : ATVV_INTERACTION_ON_REQUEST;
  uint8_t packet[] = {
    ATVV_CAPS_RESP,
    (uint8_t)(version >> 8), (uint8_t)(version & 0xFF),
    ATVV_CODEC_ADPCM_8KHZ,
    model,
    0x00, (uint8_t)ATVV_AUDIO_FRAME_BYTES,
    0x00,
    0x00
  };
  if (!atvvNotifyControlPacket(packet, sizeof(packet))) return false;
  atvvInteractionModel = model;
  Serial.printf("ATVV capabilities: version=%u.%u model=%s frame=%u\n",
                version >> 8, version & 0xFF,
                model == ATVV_INTERACTION_HOLD_TO_TALK ? "hold-to-talk" : "on-request",
                (unsigned)ATVV_AUDIO_FRAME_BYTES);
  return true;
}

static const int16_t MICROPHONE_ADPCM_STEP_TABLE[89] = {
  7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31,
  34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130,
  143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449,
  494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411,
  1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026,
  4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442,
  11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623,
  27086, 29794, 32767
};

static const int8_t MICROPHONE_ADPCM_INDEX_TABLE[16] = {
  -1, -1, -1, -1, 2, 4, 6, 8,
  -1, -1, -1, -1, 2, 4, 6, 8
};

uint8_t encodeMicrophoneAdpcmSample(int16_t sample) {
  int step = MICROPHONE_ADPCM_STEP_TABLE[microphoneAdpcmStepIndex];
  int difference = (int)sample - microphoneAdpcmPredictor;
  uint8_t code = 0;
  if (difference < 0) {
    code = 8;
    difference = -difference;
  }

  int delta = step >> 3;
  if (difference >= step) {
    code |= 4;
    difference -= step;
    delta += step;
  }
  step >>= 1;
  if (difference >= step) {
    code |= 2;
    difference -= step;
    delta += step;
  }
  step >>= 1;
  if (difference >= step) {
    code |= 1;
    delta += step;
  }

  if (code & 8) microphoneAdpcmPredictor -= delta;
  else microphoneAdpcmPredictor += delta;
  microphoneAdpcmPredictor = constrain((int)microphoneAdpcmPredictor, -32768, 32767);
  microphoneAdpcmStepIndex = constrain(
    (int)microphoneAdpcmStepIndex + MICROPHONE_ADPCM_INDEX_TABLE[code], 0, 88);
  return code;
}

bool startRealMicrophoneCapture() {
  if (realMicrophoneActive) return true;
  if (webConfigTransferActive || usbSdTransferActive() || setupApActive) {
    Serial.println("I2S microphone: shared SD pins are busy");
    return false;
  }
  if (!microphoneMutex) microphoneMutex = xSemaphoreCreateMutex();
  if (!microphoneMutex) return false;

  if (sdReady) {
    sdReady = false;
    SD.end();
    sdSpi.end();
  }
  pinMode(PIN_SD_CS, OUTPUT);
  digitalWrite(PIN_SD_CS, HIGH);
  digitalWrite(PIN_SD_EN, HIGH);
  pinMode(PIN_MIC_POWER, OUTPUT);
  digitalWrite(PIN_MIC_POWER, HIGH);
  delay(20);

  i2s_chan_config_t channelConfig = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
  channelConfig.dma_desc_num = 4;
  channelConfig.dma_frame_num = 80;
  esp_err_t error = i2s_new_channel(&channelConfig, nullptr, &microphoneRxChannel);
  if (error == ESP_OK) {
    i2s_std_config_t config = {
      .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000),
      .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
        I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO),
      .gpio_cfg = {
        .mclk = I2S_GPIO_UNUSED,
        .bclk = (gpio_num_t)PIN_MIC_BCLK,
        .ws = (gpio_num_t)PIN_MIC_WS,
        .dout = I2S_GPIO_UNUSED,
        .din = (gpio_num_t)PIN_MIC_DATA,
        .invert_flags = {
          .mclk_inv = false,
          .bclk_inv = false,
          .ws_inv = false,
        },
      },
    };
    config.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;
    error = i2s_channel_init_std_mode(microphoneRxChannel, &config);
    if (error == ESP_OK) error = i2s_channel_enable(microphoneRxChannel);
  }
  if (error != ESP_OK) {
    Serial.printf("I2S microphone: initialise failed (%d)\n", (int)error);
    if (microphoneRxChannel) {
      i2s_del_channel(microphoneRxChannel);
      microphoneRxChannel = nullptr;
    }
    digitalWrite(PIN_MIC_POWER, LOW);
    sdReady = initSdStorage();
    return false;
  }

  microphoneAdpcmPredictor = 0;
  microphoneAdpcmStepIndex = 0;
  realMicrophoneActive = true;
  Serial.println("I2S microphone: live capture ready");
  return true;
}

void stopRealMicrophoneCapture() {
  if (!realMicrophoneActive && !microphoneRxChannel) return;
  if (microphoneMutex) xSemaphoreTake(microphoneMutex, portMAX_DELAY);
  if (microphoneRxChannel) {
    i2s_channel_disable(microphoneRxChannel);
    i2s_del_channel(microphoneRxChannel);
    microphoneRxChannel = nullptr;
  }
  realMicrophoneActive = false;
  digitalWrite(PIN_MIC_POWER, LOW);
  if (microphoneMutex) xSemaphoreGive(microphoneMutex);
  delay(2);
  sdReady = initSdStorage();
  Serial.printf("I2S microphone: stopped; SD %s\n",
                sdReady ? "restored" : "unavailable");
}

bool readRealMicrophoneAdpcmFrame(uint8_t *frame) {
  if (!frame || !realMicrophoneActive || !microphoneRxChannel || !microphoneMutex) return false;
  int32_t samples[80] = {};
  size_t bytesRead = 0;
  if (xSemaphoreTake(microphoneMutex, pdMS_TO_TICKS(20)) != pdTRUE) return false;
  esp_err_t error = i2s_channel_read(microphoneRxChannel, samples, sizeof(samples),
                                     &bytesRead, pdMS_TO_TICKS(12));
  if (error == ESP_OK && bytesRead >= sizeof(samples)) {
    for (uint8_t i = 0; i < ATVV_AUDIO_FRAME_BYTES; i++) {
      int32_t first = samples[i * 4] >> 14;
      int32_t second = samples[i * 4 + 2] >> 14;
      uint8_t low = encodeMicrophoneAdpcmSample(
        (int16_t)constrain(first, (int32_t)-32768, (int32_t)32767));
      uint8_t high = encodeMicrophoneAdpcmSample(
        (int16_t)constrain(second, (int32_t)-32768, (int32_t)32767));
      frame[i] = low | (high << 4);
    }
  }
  xSemaphoreGive(microphoneMutex);
  return error == ESP_OK && bytesRead >= sizeof(samples);
}

bool atvvStartAudio(uint8_t reason, uint8_t streamId) {
  if (!atvvRx || !atvvRxCccd || !atvvRxCccd->getNotifications()) {
    Serial.println("ATVV AUDIO_START not sent: Chromecast audio notifications are off");
    return false;
  }
  atvvStreamUsesTestAudio = microphoneTestAudioEnabled;
  if (!atvvStreamUsesTestAudio && !startRealMicrophoneCapture()) {
    Serial.println("ATVV AUDIO_START not sent: real I2S microphone unavailable");
    return false;
  }
  uint8_t packet[] = {
    ATVV_AUDIO_START, reason, ATVV_CODEC_ADPCM_8KHZ, streamId
  };
  if (!atvvNotifyControlPacket(packet, sizeof(packet))) {
    if (!atvvStreamUsesTestAudio) stopRealMicrophoneCapture();
    return false;
  }
  atvvStreamId = streamId;
  atvvAudioFrameNumber = 0;
  atvvTestAudioOffset = 0;
  atvvTestAudioFinished = false;
  atvvAudioStartedMs = millis();
  atvvNextAudioFrameMs = atvvAudioStartedMs;
  atvvAudioStarted = true;
  atvvVoiceState = ATVV_VOICE_STREAMING;
  int16_t initialPredictor = atvvStreamUsesTestAudio
    ? OPENREMOTE_ATVV_TEST_INITIAL_PREDICTOR : microphoneAdpcmPredictor;
  uint8_t initialStep = atvvStreamUsesTestAudio
    ? OPENREMOTE_ATVV_TEST_INITIAL_STEP_INDEX : microphoneAdpcmStepIndex;
  if (!atvvSendAudioSync(0, initialPredictor, initialStep)) {
    Serial.println("ATVV audio: initial AUDIO_SYNC failed");
  }
  if (atvvStreamUsesTestAudio) {
    Serial.printf("ATVV test phrase: streaming %u bytes (%u frames, %.2f s)\n",
                  (unsigned)OPENREMOTE_ATVV_TEST_AUDIO_BYTES,
                  (unsigned)(OPENREMOTE_ATVV_TEST_AUDIO_BYTES /
                             ATVV_AUDIO_FRAME_BYTES),
                  OPENREMOTE_ATVV_TEST_PCM_SAMPLES / 8000.0f);
  } else {
    Serial.println("ATVV microphone: streaming live 8 kHz IMA ADPCM");
  }
  ensureAtvvAudioTask();
  if (atvvAudioTaskHandle) xTaskNotifyGive(atvvAudioTaskHandle);
  return true;
}

bool atvvStopAudio(uint8_t reason) {
  if (!atvvAudioStarted) return false;
  atvvAudioStarted = false;
  uint8_t packet[] = {ATVV_AUDIO_STOP, reason};
  bool sent = atvvNotifyControlPacket(packet, sizeof(packet));
  if (!atvvStreamUsesTestAudio) stopRealMicrophoneCapture();
  return sent;
}

void atvvSendAudioFrame() {
  if (!atvvAudioStarted || !atvvRx || !atvvRxCccd ||
      !atvvRxCccd->getNotifications()) return;
  uint8_t frame[ATVV_AUDIO_FRAME_BYTES] = {0};
  bool switchToSilence = false;
  if (atvvStreamUsesTestAudio) {
    if (!atvvTestAudioFinished &&
        atvvTestAudioOffset < OPENREMOTE_ATVV_TEST_AUDIO_BYTES) {
      memcpy_P(frame, OPENREMOTE_ATVV_TEST_AUDIO + atvvTestAudioOffset,
               sizeof(frame));
    } else if (!atvvTestAudioFinished) {
      switchToSilence = true;
    }
  } else if (!readRealMicrophoneAdpcmFrame(frame)) {
    for (uint8_t i = 0; i < sizeof(frame); i++) {
      uint8_t low = encodeMicrophoneAdpcmSample(0);
      uint8_t high = encodeMicrophoneAdpcmSample(0);
      frame[i] = low | (high << 4);
    }
  }
  if (switchToSilence) {
    if (!atvvSendAudioSync(static_cast<uint16_t>(atvvAudioFrameNumber), 0, 0)) {
      Serial.println("ATVV test phrase: silence AUDIO_SYNC failed");
    }
    atvvTestAudioFinished = true;
    Serial.println("ATVV test phrase: complete; sending silence until release");
  }
  if (atvvNotifyMutex &&
      xSemaphoreTake(atvvNotifyMutex, pdMS_TO_TICKS(100)) != pdTRUE) return;
  if (!atvvAudioStarted) {
    if (atvvNotifyMutex) xSemaphoreGive(atvvNotifyMutex);
    return;
  }
  atvvRx->setValue(frame, sizeof(frame));
  atvvRx->notify();
  if (atvvNotifyMutex) xSemaphoreGive(atvvNotifyMutex);
  if (atvvStreamUsesTestAudio && !atvvTestAudioFinished) {
    atvvTestAudioOffset += sizeof(frame);
  }
  atvvAudioFrameNumber++;
}

void atvvAudioTask(void *parameter) {
  Serial.printf("ATVV audio worker: core %d, priority %u\n",
                xPortGetCoreID(), (unsigned)uxTaskPriorityGet(nullptr));
  for (;;) {
    if (!atvvAudioStarted || !bleConnected || bleSuspended) {
      ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100));
      continue;
    }
    unsigned long now = millis();
    if ((int32_t)(now - atvvNextAudioFrameMs) >= 0) {
      atvvSendAudioFrame();
      atvvNextAudioFrameMs = millis() + ATVV_AUDIO_FRAME_INTERVAL_MS;
    }
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

void ensureAtvvAudioTask() {
  if (!atvvNotifyMutex) atvvNotifyMutex = xSemaphoreCreateMutex();
  if (atvvAudioTaskHandle) return;
  BaseType_t created = xTaskCreatePinnedToCore(
    atvvAudioTask, "openremote_atvv", 4096, nullptr, 2,
    &atvvAudioTaskHandle, 0);
  if (created != pdPASS) {
    atvvAudioTaskHandle = nullptr;
    Serial.println("ATVV audio worker: could not start");
  }
}

void resetAtvvVoiceSession(bool clearHostEvents = false) {
  atvvVoiceState = ATVV_VOICE_IDLE;
  if (clearHostEvents) {
    atvvMicOpenPending = false;
    atvvMicClosePending = false;
    atvvCapsRequestPending = false;
  }
  atvvAudioStarted = false;
  atvvVoiceReleasePending = false;
  atvvSearchRequestedMs = 0;
  atvvStopAfterMs = 0;
  atvvAudioStartedMs = 0;
  atvvNextAudioFrameMs = 0;
  atvvStreamId = 0;
}

class OpenRemoteAtvvTxCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *characteristic) override {
    String value = characteristic ? characteristic->getValue() : String();
    if (!value.length()) return;
    uint8_t opcode = (uint8_t)value[0];
    if (opcode == ATVV_MIC_OPEN) atvvMicOpenPending = true;
    else if (opcode == ATVV_MIC_CLOSE) {
      atvvMicCloseStreamId = value.length() > 1 ? (uint8_t)value[1] : 0xFF;
      atvvMicClosePending = true;
    } else if (opcode == ATVV_GET_CAPS) {
      if (value.length() >= 3) {
        atvvHostSpecVersion = ((uint16_t)(uint8_t)value[1] << 8) |
                              (uint8_t)value[2];
      }
      atvvHostInteractionModels = value.length() >= 6
        ? (uint8_t)value[5]
        : ATVV_INTERACTION_ON_REQUEST;
      atvvCapsRequestPending = true;
    }
#if OPENREMOTE_ATVV_DEBUG
    Serial.printf("ATVV TX write (%u):", (unsigned)value.length());
    for (size_t i = 0; i < value.length(); i++) {
      Serial.printf(" %02X", (uint8_t)value[i]);
    }
    Serial.printf("  %s\n", atvvOpcodeName(opcode));
    if (opcode == ATVV_GET_CAPS) {
      Serial.println("ATVV discovery confirmed: Chromecast requested capabilities; response queued");
    } else if (opcode == ATVV_MIC_OPEN) {
      Serial.println("ATVV microphone open observed; 8 kHz ADPCM stream queued");
    }
#endif
  }
};

static OpenRemoteAtvvTxCallbacks atvvTxCallbacks;

void serviceAtvvVoice(unsigned long now) {
  if (atvvCapsRequestPending) {
    atvvCapsRequestPending = false;
    if (!atvvSendCapabilities()) {
      Serial.println("ATVV capabilities response failed");
    }
  }

  if (atvvMicOpenPending) {
    atvvMicOpenPending = false;
    if (bleConnected && !bleSuspended) {
      if (atvvAudioStarted &&
          atvvInteractionModel == ATVV_INTERACTION_HOLD_TO_TALK) {
        const uint8_t errorPacket[] = {ATVV_MIC_OPEN, 0x0F, 0x80};
        atvvNotifyControlPacket(errorPacket, sizeof(errorPacket));
        Serial.println("Voice Search: MIC_OPEN rejected during active hold-to-talk stream");
      } else if (atvvStartAudio(ATVV_AUDIO_START_MIC_OPEN, 0x00)) {
        if (atvvVoiceReleasePending || !heldVoiceSearchCommand) {
          atvvVoiceReleasePending = true;
          atvvStopAfterMs = now + ATVV_RELEASE_SETTLE_MS;
        }
        Serial.println("Voice Search: MIC_OPEN acknowledged with complete AUDIO_START");
      } else {
        Serial.println("Voice Search: MIC_OPEN received but AUDIO_START failed");
      }
    }
  }

  if (atvvMicClosePending) {
    atvvMicClosePending = false;
    uint8_t closeStreamId = atvvMicCloseStreamId;
    if (atvvAudioStarted && bleConnected && !bleSuspended &&
        (closeStreamId == atvvStreamId || closeStreamId == 0xFF)) {
      atvvStopAudio(ATVV_AUDIO_STOP_MIC_CLOSE);
      Serial.println("Voice Search: MIC_CLOSE acknowledged with complete AUDIO_STOP");
      resetAtvvVoiceSession();
      return;
    }
    Serial.printf("Voice Search: ignored MIC_CLOSE for stream %u (active %u)\n",
                  closeStreamId, atvvStreamId);
  }

  if (atvvVoiceReleasePending && atvvAudioStarted &&
      (int32_t)(now - atvvStopAfterMs) >= 0) {
    uint8_t reason = atvvInteractionModel == ATVV_INTERACTION_HOLD_TO_TALK
      ? ATVV_AUDIO_STOP_BUTTON_RELEASE
      : ATVV_AUDIO_STOP_OTHER;
    atvvStopAudio(reason);
    Serial.println("Voice Search: physical release sent complete AUDIO_STOP");
    resetAtvvVoiceSession();
    return;
  }

  uint32_t audioTimeoutMs = heldVoiceSearchCommand
    ? ATVV_HELD_AUDIO_TIMEOUT_MS : ATVV_AUDIO_TIMEOUT_MS;
  if (atvvAudioStarted &&
      (int32_t)(now - atvvAudioStartedMs) >= (int32_t)audioTimeoutMs) {
    atvvStopAudio(ATVV_AUDIO_STOP_TIMEOUT);
    Serial.println("Voice Search: audio safety timeout sent AUDIO_STOP");
    resetAtvvVoiceSession();
    return;
  }

  if (atvvVoiceState == ATVV_VOICE_SEARCH_REQUESTED &&
      (uint32_t)(now - atvvSearchRequestedMs) >= ATVV_SEARCH_TIMEOUT_MS) {
    Serial.println("Voice Search: timed out waiting for MIC_OPEN");
    resetAtvvVoiceSession();
  }
}

bool setupAtvvService(BLEServer *server) {
  if (!server) return false;
  atvvService = server->createService(BLEUUID(String(ATVV_SERVICE_UUID)), 15);
  if (!atvvService) return false;
  atvvTx = atvvService->createCharacteristic(
    BLEUUID(String(ATVV_TX_UUID)),
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  atvvRx = atvvService->createCharacteristic(
    BLEUUID(String(ATVV_RX_UUID)), BLECharacteristic::PROPERTY_NOTIFY);
  atvvCtl = atvvService->createCharacteristic(
    BLEUUID(String(ATVV_CTL_UUID)), BLECharacteristic::PROPERTY_NOTIFY);
  if (!atvvTx || !atvvRx || !atvvCtl) return false;

  atvvTx->setCallbacks(&atvvTxCallbacks);
  atvvRxCccd = new BLE2902();
  atvvCtlCccd = new BLE2902();
  atvvRx->addDescriptor(atvvRxCccd);
  atvvCtl->addDescriptor(atvvCtlCccd);
  atvvService->start();
  atvvRxSubscribed = false;
  atvvCtlSubscribed = false;
  nextAtvvDebugMs = millis();
  Serial.println("ATVV: service and TX/RX/CTL characteristics ready");
  return true;
}

void serviceAtvvDebug(unsigned long now) {
#if OPENREMOTE_ATVV_DEBUG
  if (!atvvService || (int32_t)(now - nextAtvvDebugMs) < 0) return;
  nextAtvvDebugMs = now + 1000UL;
  bool rxSubscribed = atvvRxCccd && atvvRxCccd->getNotifications();
  bool ctlSubscribed = atvvCtlCccd && atvvCtlCccd->getNotifications();
  if (rxSubscribed != atvvRxSubscribed || ctlSubscribed != atvvCtlSubscribed) {
    atvvRxSubscribed = rxSubscribed;
    atvvCtlSubscribed = ctlSubscribed;
    Serial.printf("ATVV subscriptions: connected=%s RX=%s CTL=%s\n",
                  bleConnected ? "yes" : "no",
                  atvvRxSubscribed ? "notify" : "off",
                  atvvCtlSubscribed ? "notify" : "off");
  }
#else
  (void)now;
#endif
}

class OpenRemoteBleServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *server) override {
    bleConnected = true;
    bleBonded = true;
    bleSuspended = false;
    blePairingMode = false;
    bleKeepAliveUntilMs = millis() + BLE_POST_CONNECT_GRACE_MS;
    bleBondStateSavePending = true;
    bleDeviceProvisionPending = true;
    pendingUiRefresh = settingsView == SETTINGS_BLUETOOTH;
    atvvInteractionModel = ATVV_INTERACTION_ON_REQUEST;
    atvvCapsRequestPending = false;
    nextAtvvDebugMs = millis();
    Serial.println("BLE HID/ATVV: host connected; Chromecast controls queued");
  }

#if defined(CONFIG_BLUEDROID_ENABLED)
  void onConnect(BLEServer *server, esp_ble_gatts_cb_param_t *param) override {
    if (!param) return;
    memcpy(blePeerAddress, param->connect.remote_bda, sizeof(blePeerAddress));
    blePeerAddressValid = true;
    requestBluetoothConnectionProfile(false);
  }

  void onConnParamsUpdate(esp_bd_addr_t remoteBda, uint16_t interval,
                          uint16_t latency, uint16_t timeout,
                          esp_bt_status_t status) override {
    Serial.printf("BLE HID: params interval=%.1f ms latency=%u timeout=%u ms status=%d\n",
                  interval * 1.25f, latency, timeout * 10U, (int)status);
  }
#endif

  void onDisconnect(BLEServer *server) override {
    bleConnected = false;
    if (realMicrophoneActive) microphoneStopPending = true;
    heldVoiceSearchCommand = nullptr;
    heldVoiceSearchFromTouch = false;
    heldVoiceSearchPhysicalKey = 0;
    resetAtvvVoiceSession(true);
    atvvInteractionModel = ATVV_INTERACTION_ON_REQUEST;
    blePeerAddressValid = false;
    bleIdleConnectionProfileRequested = false;
    atvvRxSubscribed = false;
    atvvCtlSubscribed = false;
    pendingUiRefresh = settingsView == SETTINGS_BLUETOOTH;
    if (!bleShutdownInProgress && !bleSuspended &&
        (blePairingMode || bluetoothRuntimeRequired())) advertiseBluetoothHid();
    Serial.println("BLE HID/ATVV: host disconnected");
  }
};

class OpenRemoteBleSecurityCallbacks : public BLESecurityCallbacks {
  bool onSecurityRequest() override { return true; }
  uint32_t onPassKeyRequest() override { return 0; }
  void onPassKeyNotify(uint32_t passKey) override {}
  bool onConfirmPIN(uint32_t passKey) override { return true; }

#if defined(CONFIG_BLUEDROID_ENABLED)
  void onAuthenticationComplete(esp_ble_auth_cmpl_t result) override {
    bleBonded = result.success;
    if (result.success) {
      blePairingMode = false;
      bleKeepAliveUntilMs = millis() + BLE_POST_CONNECT_GRACE_MS;
      bleDeviceProvisionPending = true;
    }
    bleBondStateSavePending = true;
    pendingUiRefresh = settingsView == SETTINGS_BLUETOOTH;
    Serial.printf("BLE HID: pairing %s\n", result.success ? "complete" : "failed");
  }
#elif defined(CONFIG_NIMBLE_ENABLED)
  void onAuthenticationComplete(ble_gap_conn_desc *result) override {
    bleBonded = result && result->sec_state.encrypted;
    if (bleBonded) {
      blePairingMode = false;
      bleKeepAliveUntilMs = millis() + BLE_POST_CONNECT_GRACE_MS;
      bleDeviceProvisionPending = true;
    }
    bleBondStateSavePending = true;
    pendingUiRefresh = settingsView == SETTINGS_BLUETOOTH;
    Serial.printf("BLE HID: pairing %s\n", bleBonded ? "complete" : "failed");
  }
#endif
};

static OpenRemoteBleServerCallbacks bleServerCallbacks;
static OpenRemoteBleSecurityCallbacks bleSecurityCallbacks;

void stopBluetoothRadio(const char *reason) {
  if (!bleReady || bleSuspended) return;
  endVoiceSearchHold();
  bleShutdownInProgress = true;
  bleSuspended = true;
  BLEDevice::stopAdvertising();
  if (bleServer && bleConnected) bleServer->disconnect(bleServer->getConnId());
  bleShutdownInProgress = false;
  Serial.printf("BLE HID: suspended (%s)\n", reason);
}

bool runtimeDeviceNeedsBluetooth(uint8_t index) {
  if (index >= DEVICE_COUNT) return false;
  String transport = devices[index].transport;
  transport.toLowerCase();
  if (transport.indexOf("bluetooth") >= 0 || transport.indexOf("ble") >= 0) return true;
  for (uint8_t i = 0; i < devices[index].commandCount; i++) {
    if (devices[index].commands[i].kind == DeviceCommand::BLE_HID) return true;
  }
  return false;
}

bool bluetoothActivitySessionRequired() {
  if (activeDevice >= 0 && runtimeDeviceNeedsBluetooth((uint8_t)activeDevice)) {
    return true;
  }
  if (activeActivity >= 0 && activeActivity < ACTIVITY_COUNT) {
    uint16_t mask = activities[activeActivity].deviceMask;
    for (uint8_t i = 0; i < DEVICE_COUNT && i < 16; i++) {
      if ((mask & (uint16_t)(1U << i)) && runtimeDeviceNeedsBluetooth(i)) {
        return true;
      }
    }
  }
  return false;
}

bool bluetoothRuntimeRequired() {
  if (blePairingMode) return true;
  if (bleConnected && (int32_t)(bleKeepAliveUntilMs - millis()) > 0) return true;
  return bluetoothActivitySessionRequired();
}

void applyBluetoothState() {
  bool shouldRun = bluetoothOn && (blePairingMode || bleBonded) &&
                   bluetoothRuntimeRequired() && !scheduledNtpWake &&
                   !setupApActive && !setupApPausedBle &&
                   !webConfigPausedBle;
  if (shouldRun && !bleReady && !setupApActive && !setupApPausedBle) {
    if (!BLEDevice::init(BLE_HID_NAME)) {
      Serial.println("BLE HID: init failed");
      return;
    }

    bleSecurity = new BLESecurity();
    bleSecurity->setCapability(ESP_IO_CAP_NONE);
    bleSecurity->setAuthenticationMode(true, false, true);
    bleSecurity->setForceAuthentication(true);
    BLEDevice::setSecurityCallbacks(&bleSecurityCallbacks);

    bleServer = BLEDevice::createServer();
    bleServer->setCallbacks(&bleServerCallbacks);
    bleHid = new BLEHIDDevice(bleServer);
    bleHid->manufacturer()->setValue("OpenRemote");
    bleHid->pnp(0x02, 0x303A, 0x4001, 0x0135);
    bleHid->hidInfo(0x00, 0x01);
    bleHid->reportMap((uint8_t *)BLE_HID_REPORT_MAP, sizeof(BLE_HID_REPORT_MAP));
    // ESP32 BLE 3.3.10 mis-associates same-UUID HID Report characteristics.
    // One combined keyboard/consumer report provides both command families
    // without recreating the duplicate characteristic that caused the panic.
    bleKeyboardInput = bleHid->inputReport(1);
    bleConsumerInput = bleKeyboardInput;
    bool atvvReady = setupAtvvService(bleServer);
    float batteryPercent = readBatteryPercent();
    bleHid->setBatteryLevel(batteryPercent >= 0.0f
      ? (uint8_t)constrain((int)roundf(batteryPercent), 0, 100) : 100);
    bleHid->startServices();

    BLEAdvertising *advertising = BLEDevice::getAdvertising();
    advertising->setAppearance(HID_KEYBOARD);
    advertising->addServiceUUID(bleHid->hidService()->getUUID());
    if (atvvReady) advertising->addServiceUUID(BLEUUID(String(ATVV_SERVICE_UUID)));
    advertising->setScanResponse(true);
    advertising->setMinPreferred(0x06);
    advertising->setMaxPreferred(0x12);
    bleReady = true;
    bleSuspended = false;
    refreshBluetoothBondState();
    if (bleBonded) bleDeviceProvisionPending = true;
    if (blePairingMode || bleBonded) advertiseBluetoothHid();
    Serial.printf("BLE HID: ready, bonded=%s ATVV=%s\n",
                  bleBonded ? "yes" : "no", atvvReady ? "ready" : "failed");
  } else if (shouldRun && bleReady && !webConfigPausedBle) {
    if (bleSuspended) {
      bleSuspended = false;
      Serial.println("BLE HID: resumed");
    }
    if (!bleConnected) advertiseBluetoothHid();
  } else if (!shouldRun && bleReady) {
    if (!bluetoothOn) blePairingMode = false;
    stopBluetoothRadio(bluetoothOn ? "not required by active device" : "switch");
  }
}

void startBluetoothPairing() {
  bluetoothOn = true;
  blePairingMode = true;
  bleKeepAliveUntilMs = millis() + BLE_PAIRING_WINDOW_MS;
  blePairingUntilMs = millis() + BLE_PAIRING_WINDOW_MS;
  saveSettings();
  applyBluetoothState();
  advertiseBluetoothHid();
  pendingUiRefresh = settingsView == SETTINGS_BLUETOOTH;
}

void forgetBluetoothPairing() {
  blePairingMode = false;
  if (bleServer && bleConnected) bleServer->disconnect(bleServer->getConnId());
#if defined(CONFIG_BLUEDROID_ENABLED)
  int count = esp_ble_get_bond_device_num();
  if (count > 0) {
    esp_ble_bond_dev_t *bonds = new esp_ble_bond_dev_t[count];
    if (bonds && esp_ble_get_bond_device_list(&count, bonds) == ESP_OK) {
      for (int i = 0; i < count; i++) esp_ble_remove_bond_device(bonds[i].bd_addr);
    }
    delete[] bonds;
  }
#endif
  bleBonded = false;
  bleBondStateSavePending = true;
  BLEDevice::stopAdvertising();
  pendingUiRefresh = settingsView == SETTINGS_BLUETOOTH;
  Serial.println("BLE HID: pairing forgotten");
}

void serviceBluetooth(unsigned long now) {
  if (bleBondStateSavePending) {
    bleBondStateSavePending = false;
    saveSettings();
  }
  if (bleDeviceProvisionPending) {
    bleDeviceProvisionPending = false;
    if (!ensureBluetoothRuntimeDevice()) {
      Serial.println("BLE HID: could not provision Chromecast runtime device");
    }
  }
  if (!blePairingMode || bleConnected || (int32_t)(now - blePairingUntilMs) < 0) return;
  blePairingMode = false;
  if (!bleBonded && bleReady) BLEDevice::stopAdvertising();
  pendingUiRefresh = settingsView == SETTINGS_BLUETOOTH;
  Serial.println("BLE HID: pairing window expired");
}

bool requestFromSetupAp() {
  IPAddress remote = webServer.client().remoteIP();
  IPAddress ap = WiFi.softAPIP();
  return remote[0] == ap[0] && remote[1] == ap[1] && remote[2] == ap[2];
}

bool requestAuthorized() {
  if (requestFromSetupAp()) return true;
  if (webServer.hasHeader("X-OpenRemote-Token") &&
      webServer.header("X-OpenRemote-Token") == setupToken) return true;
  return webServer.hasArg("token") && webServer.arg("token") == setupToken;
}

void sendJson(int status, const String &body) {
  webServer.sendHeader("Cache-Control", "no-store");
  webServer.send(status, "application/json", body);
}

void serviceCommandFeedback(unsigned long now);
void serviceButtonTestFeedback(unsigned long now);
void refreshStatusPill();

String webConfigUrl() {
  IPAddress ip = WiFi.status() == WL_CONNECTED ? WiFi.localIP() : WiFi.softAPIP();
  return String("http://") + ip.toString() + "/?token=" + setupToken;
}

void serviceUiDuringLongHttpTransfer() {
  // HTTP runs on core 0. LVGL, touch and keypad belong exclusively to the
  // Arduino loop on core 1, so a long transfer only yields its worker here.
  vTaskDelay(1);
}

bool streamSdFileCooperatively(File &file, const char *mimeType) {
  if (!file) return false;
  const size_t total = file.size();
  webServer.sendHeader("Cache-Control", "no-store");
  webServer.setContentLength(total);
  webServer.send(200, mimeType, "");

  WiFiClient client = webServer.client();
  client.setNoDelay(true);
  uint8_t buffer[1024];
  size_t sent = 0;
  unsigned long lastProgressMs = millis();
  while (!webConfigTransferCancelRequested && client.connected() && sent < total) {
    int readCount = file.read(buffer, min(sizeof(buffer), total - sent));
    if (readCount <= 0) break;
    size_t offset = 0;
    while (!webConfigTransferCancelRequested && client.connected() &&
           offset < (size_t)readCount) {
      size_t written = client.write(buffer + offset, (size_t)readCount - offset);
      if (written) {
        offset += written;
        sent += written;
        lastProgressMs = millis();
      }
      if (millis() - lastProgressMs > 15000UL) {
        Serial.println("HTTP stream: client stalled");
        client.stop();
        return false;
      }
      serviceUiDuringLongHttpTransfer();
    }
  }
  if (webConfigTransferCancelRequested) {
    Serial.println("HTTP stream: cancelled by LCD navigation");
    client.stop();
    return false;
  }
  Serial.printf("HTTP stream: %u/%u bytes sent\n", (unsigned)sent, (unsigned)total);
  return sent == total;
}

String installedWebConfigVersion() {
  if (!sdReady || !SD.exists(WEB_CONFIG_PATH)) return "";
  File file = SD.open(WEB_CONFIG_PATH, FILE_READ);
  if (!file) return "";
  String version;
  for (uint8_t lineNumber = 0; lineNumber < 24 && file.available(); lineNumber++) {
    String line = file.readStringUntil('\n');
    int marker = line.indexOf("openremote-webconfig-version");
    if (marker >= 0) {
      int content = line.indexOf("content=\"", marker);
      if (content >= 0) {
        content += 9;
        int end = line.indexOf('"', content);
        if (end > content) version = line.substring(content, end);
      }
    }
    if (!version.length()) {
      int title = line.indexOf("OpenRemote Web Config v");
      if (title >= 0) {
        title += 23;
        int end = line.indexOf('<', title);
        version = line.substring(title, end > title ? end : line.length());
      }
    }
    if (version.length()) break;
  }
  file.close();
  version.trim();
  return version;
}

void serveWebConfig() {
  if (!sdReady || !SD.exists(WEB_CONFIG_PATH)) {
    webServer.send(503, "text/plain",
      "OpenRemote WebConfig is not installed. Copy WebConfig to /www/index.html on the SD card.");
    return;
  }
  if (!requestFromSetupAp() && !webServer.hasArg("token")) {
    webServer.sendHeader("Location", webConfigUrl(), true);
    webServer.send(302, "text/plain", "Opening authenticated OpenRemote WebConfig...");
    return;
  }
  File file = SD.open(WEB_CONFIG_PATH, FILE_READ);
  if (!file) {
    webServer.send(500, "text/plain", "Could not open WebConfig from SD card.");
    return;
  }
  lastWakeMs = millis();
  webConfigTransferCancelRequested = false;
  webConfigTransferActive = true;
  streamSdFileCooperatively(file, "text/html; charset=utf-8");
  webConfigTransferActive = false;
  file.close();
}

String buildStatusJson() {
  JsonDocument doc;
  doc["ok"] = true;
  doc["firmwareVersion"] = OPENREMOTE_VERSION_TEXT;
  doc["remoteName"] = remoteName;
  doc["webConfigInstalled"] = sdReady && SD.exists(WEB_CONFIG_PATH);
  doc["webConfigVersion"] = installedWebConfigVersion();
  doc["irdbInstalled"] = sdReady && SD.exists(IRDB_PATH);
  doc["irdbSearchIndexInstalled"] = sdReady && SD.exists(IRDB_SEARCH_INDEX_PATH);
  doc["irdbDetailIndexInstalled"] = sdReady && SD.exists(IRDB_DETAIL_DIR);
  doc["irdbBuildDate"] = irdbBuildDate;
  doc["irdbDeviceCount"] = irdbDeviceCount;
  doc["deviceFileCount"] = countSavedIrDeviceFiles();
  doc["deviceCount"] = DEVICE_COUNT;
  doc["activityCount"] = ACTIVITY_COUNT;
  if (sdReady && SD.exists(IRDB_PATH)) {
    File irdb = SD.open(IRDB_PATH, FILE_READ);
    doc["irdbSizeBytes"] = irdb ? irdb.size() : 0;
    if (irdb) irdb.close();
  }
  doc["sdReady"] = sdReady;
  doc["sdStatus"] = sdStatusText;
  doc["wifiEnabled"] = wifiOn;
  doc["wifiConnected"] = WiFi.status() == WL_CONNECTED;
  doc["ssid"] = WiFi.status() == WL_CONNECTED ? WiFi.SSID() : "";
  doc["stationIp"] = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "";
  doc["setupSsid"] = setupApSsid;
  doc["setupIp"] = WiFi.softAPIP().toString();
  doc["setupApActive"] = setupApActive;
  doc["bluetoothEnabled"] = bluetoothOn;
  doc["bluetoothActive"] = bleReady;
  doc["bluetoothHidReady"] = bleReady;
  doc["bluetoothConnected"] = (bool)bleConnected;
  doc["bluetoothPaired"] = (bool)bleBonded;
  doc["bluetoothPairing"] = blePairingMode;
  doc["bluetoothName"] = BLE_HID_NAME;
  doc["clockEnabled"] = clockEnabled;
  doc["clockUseInternetTime"] = clockUseInternetTime;
  doc["clockCity"] = clockCityName;
  doc["clockUtcOffsetMinutes"] = clockUtcOffsetMinutes;
  doc["manualClockEpoch"] = manualClockEpoch;
  doc["wifiScanning"] = wifiScanPending || webWifiScanRequested;
  doc["wifiConnecting"] = wifiConnectPending;
  doc["wifiStatus"] = webWifiStatusText;
  doc["brightness"] = brightness;
  doc["sleepSeconds"] = timeoutSeconds;
  doc["deepSleepMinutes"] = deepSleepMinutes;
  doc["wakeSensitivity"] = wakeSensitivity;
  doc["displayGamma"] = displayGamma;
  doc["displaySaturation"] = displaySaturation;
  doc["displayRgb666"] = displayRgb666;
  doc["displayInverted"] = displayInverted;
  doc["physicalRepeatEnabled"] = physicalRepeatEnabled;
  doc["physicalRepeatDelayMs"] = physicalRepeatDelayMs;
  doc["physicalRepeatRateHz"] = physicalRepeatRateHz;
  doc["slideToUnlock"] = slideToUnlock;
  doc["displaySleeping"] = displaySleeping;
  doc["touchDown"] = lvTouchDown;
  doc["awakeForMs"] = (uint32_t)(millis() - lastWakeMs);
  doc["currentPage"] = currentPage;
  doc["settingsView"] = (int)settingsView;
  doc["setupApActive"] = setupApActive;
  float batteryPercent = readBatteryPercent();
  if (batteryPercent >= 0.0f) doc["batteryPercent"] = roundf(batteryPercent * 10.0f) / 10.0f;
  else doc["batteryPercent"] = nullptr;
  BatteryMetrics battery = currentBatteryMetrics();
  doc["chargerConnected"] = battery.chargerConnected;
  if (battery.voltage >= 0.0f) doc["batteryVoltage"] = roundf(battery.voltage * 100.0f) / 100.0f;
  else doc["batteryVoltage"] = nullptr;
  if (!isnan(battery.ratePerHour)) doc["batteryRatePerHour"] = roundf(battery.ratePerHour * 100.0f) / 100.0f;
  else doc["batteryRatePerHour"] = nullptr;
  if (!isnan(battery.change1h)) doc["batteryChange1h"] = roundf(battery.change1h * 10.0f) / 10.0f;
  else doc["batteryChange1h"] = nullptr;
  if (!isnan(battery.change24h)) doc["batteryChange24h"] = roundf(battery.change24h * 10.0f) / 10.0f;
  else doc["batteryChange24h"] = nullptr;
  if (!isnan(battery.estimatedHours)) doc["batteryEstimatedHours"] = roundf(battery.estimatedHours * 10.0f) / 10.0f;
  else doc["batteryEstimatedHours"] = nullptr;
  doc["resetReason"] = (int)esp_reset_reason();
  String body;
  serializeJson(doc, body);
  return body;
}

void handleStatusApi() {
  sendJson(200, buildStatusJson());
}

void handleClockSettingsApi() {
  if (!requestAuthorized()) {
    sendJson(403, "{\"ok\":false,\"error\":\"Not authorized\"}");
    return;
  }
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, webServer.arg("plain"));
  if (error) {
    sendJson(400, "{\"ok\":false,\"error\":\"Invalid Clock settings\"}");
    return;
  }

  WebClockRequest request;
  request.pending = true;
  request.enabled = doc["clockEnabled"] | clockEnabled;
  request.useInternetTime = doc["clockUseInternetTime"] | clockUseInternetTime;
  if (request.useInternetTime && WiFi.status() != WL_CONNECTED) {
    sendJson(409, "{\"ok\":false,\"error\":\"Connect the remote to Wi-Fi before enabling Internet time\"}");
    return;
  }
  request.utcOffsetMinutes = constrain(
    (int)(doc["clockUtcOffsetMinutes"] | clockUtcOffsetMinutes),
    -12 * 60, 14 * 60);
  const char *city = doc["clockCity"] | clockCityName.c_str();
  strlcpy(request.city, city && city[0] ? city : "UTC", sizeof(request.city));
  uint64_t manualEpoch = doc["manualClockEpoch"] | (uint64_t)0;
  request.hasManualEpoch = manualEpoch > 0;
  request.manualEpoch = manualEpoch;

  portENTER_CRITICAL(&webControlMux);
  webClockRequest = request;
  portEXIT_CRITICAL(&webControlMux);
  sendJson(202, "{\"ok\":true,\"accepted\":true}");
}

void handleWifiNetworksApi() {
  if (!requestAuthorized()) {
    sendJson(403, "{\"ok\":false,\"error\":\"Not authorized\"}");
    return;
  }
  JsonDocument doc;
  doc["ok"] = true;
  doc["scanning"] = wifiScanPending || webWifiScanRequested;
  doc["connecting"] = wifiConnectPending;
  doc["connected"] = WiFi.status() == WL_CONNECTED;
  doc["ssid"] = WiFi.status() == WL_CONNECTED ? WiFi.SSID() : "";
  doc["ip"] = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "";
  doc["setupApActive"] = setupApActive;
  doc["setupIp"] = setupApActive ? WiFi.softAPIP().toString() : "";
  doc["message"] = webWifiStatusText;
  JsonArray networks = doc["networks"].to<JsonArray>();
  for (uint8_t i = 0; i < wifiScanResultCount; i++) {
    JsonObject network = networks.add<JsonObject>();
    network["ssid"] = wifiScanResults[i].ssid;
    network["rssi"] = wifiScanResults[i].rssi;
    network["secure"] = wifiScanResults[i].encryption != WIFI_AUTH_OPEN;
    network["saved"] = findWifiProfile(wifiScanResults[i].ssid) >= 0;
    network["connected"] = WiFi.status() == WL_CONNECTED &&
                           WiFi.SSID() == wifiScanResults[i].ssid;
  }
  String body;
  serializeJson(doc, body);
  sendJson(200, body);
}

bool validDefaultThemeWallpaper(const char *path) {
  if (!sdReady || !SD.exists(path)) return false;
  File file = SD.open(path, FILE_READ);
  const size_t expected = LCD_W * LCD_H * sizeof(uint16_t);
  if (!file || file.size() != expected) {
    if (file) file.close();
    return false;
  }
  uint8_t buffer[256];
  bool havePixel = false;
  uint16_t firstPixel = 0;
  uint32_t visiblePixels = 0;
  uint32_t changedPixels = 0;
  while (file.available()) {
    size_t count = file.read(buffer, sizeof(buffer));
    for (size_t i = 0; i + 1 < count; i += 2) {
      uint16_t pixel = (uint16_t)buffer[i] | ((uint16_t)buffer[i + 1] << 8);
      if (!havePixel) {
        firstPixel = pixel;
        havePixel = true;
      }
      if (pixel != 0) visiblePixels++;
      if (pixel != firstPixel) changedPixels++;
    }
  }
  file.close();
  return havePixel && visiblePixels > 256 && changedPixels > 256;
}

bool validDefaultThemePreview(const char *path) {
  if (!sdReady || !SD.exists(path)) return false;
  File file = SD.open(path, FILE_READ);
  if (!file || file.size() < 128) {
    if (file) file.close();
    return false;
  }
  uint8_t signature[8] = {};
  bool valid = file.read(signature, sizeof(signature)) == sizeof(signature) &&
    signature[0] == 0x89 && signature[1] == 'P' && signature[2] == 'N' &&
    signature[3] == 'G' && signature[4] == 0x0D && signature[5] == 0x0A &&
    signature[6] == 0x1A && signature[7] == 0x0A;
  file.close();
  return valid;
}

bool defaultThemeAssetsReady(const char *id) {
  String base = String("/themes/Default/") + id;
  return validDefaultThemeWallpaper((base + ".rgb565").c_str()) &&
         validDefaultThemePreview((base + ".png").c_str());
}

void handleThemeStatusApi() {
  if (!requestAuthorized()) {
    sendJson(403, "{\"ok\":false,\"error\":\"Not authorized\"}");
    return;
  }
  JsonDocument doc;
  doc["ok"] = true;
  JsonObject assets = doc["assets"].to<JsonObject>();
  assets["smooth_blue"] = defaultThemeAssetsReady("smooth_blue");
  assets["obsidian_silk"] = defaultThemeAssetsReady("obsidian_silk");
  assets["aurora_glass"] = defaultThemeAssetsReady("aurora_glass");
  assets["champagne_noir"] = defaultThemeAssetsReady("champagne_noir");
  assets["grand_cinema"] = defaultThemeAssetsReady("grand_cinema");
  assets["alpine_ember"] = defaultThemeAssetsReady("alpine_ember");
  assets["midnight_penthouse"] = defaultThemeAssetsReady("midnight_penthouse");
  String body;
  serializeJson(doc, body);
  sendJson(200, body);
}

void handleWifiScanApi() {
  if (!requestAuthorized()) {
    sendJson(403, "{\"ok\":false,\"error\":\"Not authorized\"}");
    return;
  }
  if (wifiScanPending || webWifiScanRequested) {
    sendJson(202, "{\"ok\":true,\"scanning\":true}");
    return;
  }
  webWifiScanRequested = true;
  strlcpy(webWifiStatusText, "Scanning for nearby networks...",
          sizeof(webWifiStatusText));
  sendJson(202, "{\"ok\":true,\"scanning\":true}");
}

void handleWifiConnectApi() {
  if (!requestAuthorized()) {
    sendJson(403, "{\"ok\":false,\"error\":\"Not authorized\"}");
    return;
  }
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, webServer.arg("plain"));
  const char *ssid = doc["ssid"] | "";
  const char *password = doc["password"] | "";
  bool useSavedPassword = doc["useSavedPassword"] | false;
  if (error || !ssid[0] || strlen(ssid) > 32 || strlen(password) > 64) {
    sendJson(400, "{\"ok\":false,\"error\":\"Choose a valid Wi-Fi network\"}");
    return;
  }
  if (useSavedPassword && findWifiProfile(String(ssid)) < 0) {
    sendJson(404, "{\"ok\":false,\"error\":\"No saved password exists for that network\"}");
    return;
  }

  webWifiActionNotBeforeMs = millis() + 350UL;
  portENTER_CRITICAL(&webControlMux);
  webWifiRequest.action = WEB_WIFI_CONNECT;
  webWifiRequest.useSavedPassword = useSavedPassword;
  strlcpy(webWifiRequest.ssid, ssid, sizeof(webWifiRequest.ssid));
  strlcpy(webWifiRequest.password, password, sizeof(webWifiRequest.password));
  portEXIT_CRITICAL(&webControlMux);
  strlcpy(webWifiStatusText, "Connection request accepted", sizeof(webWifiStatusText));
  sendJson(202, "{\"ok\":true,\"accepted\":true}");
}

void handleWifiForgetApi() {
  if (!requestAuthorized()) {
    sendJson(403, "{\"ok\":false,\"error\":\"Not authorized\"}");
    return;
  }
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, webServer.arg("plain"));
  const char *ssid = doc["ssid"] | "";
  if (error || !ssid[0] || strlen(ssid) > 32) {
    sendJson(400, "{\"ok\":false,\"error\":\"Choose a saved Wi-Fi network\"}");
    return;
  }
  webWifiActionNotBeforeMs = millis() + 350UL;
  portENTER_CRITICAL(&webControlMux);
  webWifiRequest.action = WEB_WIFI_FORGET;
  webWifiRequest.useSavedPassword = false;
  strlcpy(webWifiRequest.ssid, ssid, sizeof(webWifiRequest.ssid));
  webWifiRequest.password[0] = '\0';
  portEXIT_CRITICAL(&webControlMux);
  sendJson(202, "{\"ok\":true,\"accepted\":true}");
}

void applySettingsJson(JsonVariantConst settings) {
  if (settings.isNull()) return;
  bool previousWifiOn = wifiOn;
  const char *configuredName = settings["remoteName"] | nullptr;
  if (configuredName && configuredName[0]) remoteName = configuredName;
  wifiOn = settings["wifiEnabled"] | wifiOn;
  bluetoothOn = settings["bluetoothEnabled"] | bluetoothOn;
  clockEnabled = settings["clockEnabled"] | clockEnabled;
  clockUseInternetTime = settings["clockUseInternetTime"] | clockUseInternetTime;
  const char *city = settings["clockCity"] | nullptr;
  if (city && city[0]) clockCityName = city;
  clockUtcOffsetMinutes = constrain(
    (int)(settings["clockUtcOffsetMinutes"] | clockUtcOffsetMinutes),
    -12 * 60, 14 * 60);
  uint64_t configuredManualEpoch = settings["manualClockEpoch"] | (uint64_t)0;
  if (configuredManualEpoch > 0) manualClockEpoch = configuredManualEpoch;
  slideToUnlock = settings["slideToUnlock"] | slideToUnlock;
  brightness = constrain((int)(settings["brightness"] | brightness), 5, 100);
  timeoutSeconds = constrain((int)(settings["sleepSeconds"] | timeoutSeconds), 5, 120);
  deepSleepMinutes = normaliseDeepSleepMinutes(
    settings["deepSleepMinutes"] | deepSleepMinutes);
  wakeSensitivity = constrain((int)(settings["wakeSensitivity"] | wakeSensitivity), 1, 100);
  displayGamma = constrain((int)(settings["displayGamma"] | displayGamma), 50, 250);
  displaySaturation = constrain((int)(settings["displaySaturation"] | displaySaturation), 0, 200);
  displayRgb666 = settings["displayRgb666"] | displayRgb666;
  displayInverted = settings["displayInverted"] | displayInverted;
  physicalRepeatEnabled = settings["physicalRepeatEnabled"] | physicalRepeatEnabled;
  physicalRepeatDelayMs = constrain(
    (int)(settings["physicalRepeatDelayMs"] | physicalRepeatDelayMs),
    (int)BUTTON_REPEAT_DELAY_MIN_MS, (int)BUTTON_REPEAT_DELAY_MAX_MS);
  physicalRepeatDelayMs = ((physicalRepeatDelayMs + 25U) / 50U) * 50U;
  physicalRepeatRateHz = constrain(
    (int)(settings["physicalRepeatRateHz"] | physicalRepeatRateHz),
    (int)BUTTON_REPEAT_RATE_MIN_HZ, (int)BUTTON_REPEAT_RATE_MAX_HZ);
  debugSplitEnabled = settings["debugSplit"] | debugSplitEnabled;
  debugTouchEnabled = settings["debugTouch"] | debugTouchEnabled;
  debugCpuRamEnabled = settings["debugCpuRam"] | debugCpuRamEnabled;
  debugAccelerometerEnabled =
    settings["debugAccelerometer"] | debugAccelerometerEnabled;
  debugFpsEnabled = settings["debugFps"] | debugFpsEnabled;
  microphoneTestAudioEnabled =
    settings["microphoneTestAudio"] | microphoneTestAudioEnabled;
  JsonArrayConst debugRows = settings["debugRowPixels"].as<JsonArrayConst>();
  if (!debugRows.isNull()) {
    for (uint8_t i = 0; i < 5 && i < debugRows.size(); i++) {
      debugRowPixels[i] = constrain(
        (int)(debugRows[i] | debugRowPixels[i]), 0, LCD_H - 1);
    }
  }
  buttonIconSize = constrain((int)(settings["buttonIconSize"] | buttonIconSize), 20, 64);
  buttonTextSize = constrain((int)(settings["buttonTextSize"] | buttonTextSize), 10, 24);
  activityIconSize = constrain((int)(settings["activityIconSize"] | activityIconSize), 20, 64);
  activityTextSize = constrain((int)(settings["activityTextSize"] | activityTextSize), 10, 24);
  buttonBoxesEnabled = settings["buttonBoxesEnabled"] | buttonBoxesEnabled;
  activityBoxesEnabled = settings["activityBoxesEnabled"] | activityBoxesEnabled;
  raiseToWake = true;
  saveSettings();
  applyBrightness();
  rebuildDisplayColourLut();
  applyDisplayControllerSettings();
  applyBluetoothState();
  applyClockMode();
  // The QR page owns the AP lifecycle. Settings may be saved while clients are
  // connected, but radio changes are deferred until the user leaves that page.
  if (!setupApActive && previousWifiOn != wifiOn) pendingNetworkApply = true;
}

void clearRuntimeCommands() {
  if (!devices || !activities || !macros || !activityTiles) return;
  for (uint8_t deviceIndex = 0; deviceIndex < MAX_RUNTIME_DEVICES; deviceIndex++) {
    for (uint8_t commandIndex = 0; commandIndex < MAX_DEVICE_COMMANDS; commandIndex++) {
      if (devices[deviceIndex].commands[commandIndex].rawTimings) {
        free(devices[deviceIndex].commands[commandIndex].rawTimings);
        devices[deviceIndex].commands[commandIndex].rawTimings = nullptr;
      }
      if (devices[deviceIndex].commands[commandIndex].iconPath) {
        free(devices[deviceIndex].commands[commandIndex].iconPath);
        devices[deviceIndex].commands[commandIndex].iconPath = nullptr;
      }
    }
  }
  for (uint8_t activityIndex = 0; activityIndex < MAX_RUNTIME_ACTIVITIES; activityIndex++) {
    if (activities[activityIndex].iconPath) {
      free(activities[activityIndex].iconPath);
      activities[activityIndex].iconPath = nullptr;
    }
    for (uint8_t tileIndex = 0; tileIndex < MAX_ACTIVITY_TILES; tileIndex++) {
      if (activityTiles[activityIndex][tileIndex].iconPath) {
        free(activityTiles[activityIndex][tileIndex].iconPath);
        activityTiles[activityIndex][tileIndex].iconPath = nullptr;
      }
    }
  }
}

bool allocateRuntimeStorage() {
  if (devices && activities && macros && activityTiles && activityTileCounts) return true;
  if (!psramFound()) {
    Serial.println("Runtime storage: PSRAM unavailable; select OPI PSRAM in Arduino IDE");
    return false;
  }

  devices = static_cast<Device *>(ps_malloc(sizeof(Device) * MAX_RUNTIME_DEVICES));
  activities = static_cast<Activity *>(ps_malloc(sizeof(Activity) * MAX_RUNTIME_ACTIVITIES));
  macros = static_cast<Macro *>(ps_malloc(sizeof(Macro) * MAX_RUNTIME_MACROS));
  activityTiles = static_cast<Tile (*)[MAX_ACTIVITY_TILES]>(
    ps_malloc(sizeof(Tile) * MAX_RUNTIME_ACTIVITIES * MAX_ACTIVITY_TILES));
  activityTileCounts = static_cast<uint8_t *>(ps_malloc(MAX_RUNTIME_ACTIVITIES));
  if (!devices || !activities || !macros || !activityTiles || !activityTileCounts) {
    if (devices) free(devices);
    if (activities) free(activities);
    if (macros) free(macros);
    if (activityTiles) free(activityTiles);
    if (activityTileCounts) free(activityTileCounts);
    devices = nullptr;
    activities = nullptr;
    macros = nullptr;
    activityTiles = nullptr;
    activityTileCounts = nullptr;
    Serial.println("Runtime storage: PSRAM allocation failed");
    return false;
  }

  memset(devices, 0, sizeof(Device) * MAX_RUNTIME_DEVICES);
  memset(activities, 0, sizeof(Activity) * MAX_RUNTIME_ACTIVITIES);
  memset(macros, 0, sizeof(Macro) * MAX_RUNTIME_MACROS);
  memset(activityTiles, 0, sizeof(Tile) * MAX_RUNTIME_ACTIVITIES * MAX_ACTIVITY_TILES);
  memset(activityTileCounts, 0, MAX_RUNTIME_ACTIVITIES);
  Serial.printf("Runtime storage: %u bytes in PSRAM, %u bytes internal heap free\n",
                (unsigned)(sizeof(Device) * MAX_RUNTIME_DEVICES +
                           sizeof(Activity) * MAX_RUNTIME_ACTIVITIES +
                           sizeof(Macro) * MAX_RUNTIME_MACROS +
                           sizeof(Tile) * MAX_RUNTIME_ACTIVITIES * MAX_ACTIVITY_TILES +
                           MAX_RUNTIME_ACTIVITIES),
                (unsigned)ESP.getFreeHeap());
  return true;
}

char *duplicateRuntimeString(const String &value) {
  if (!value.length()) return nullptr;
  char *copy = static_cast<char *>(psramFound() ? ps_malloc(value.length() + 1) : malloc(value.length() + 1));
  if (!copy) return nullptr;
  memcpy(copy, value.c_str(), value.length() + 1);
  return copy;
}

uint32_t parseFlipperHex(const char *text) {
  if (!text) return 0;
  uint32_t value = 0;
  uint8_t shift = 0;
  while (*text && shift < 32) {
    while (*text == ' ') text++;
    if (!*text) break;
    char *end = nullptr;
    uint32_t byteValue = strtoul(text, &end, 16);
    if (end == text) break;
    value |= (byteValue & 0xFFU) << shift;
    shift += 8;
    text = end;
  }
  return value;
}

bool loadRawTimings(DeviceCommand &target, const char *text) {
  if (!text || !text[0]) return false;
  uint16_t count = 0;
  const char *cursor = text;
  while (*cursor && count < 1024) {
    while (*cursor == ' ') cursor++;
    if (!*cursor) break;
    char *end = nullptr;
    strtoul(cursor, &end, 10);
    if (end == cursor) return false;
    count++;
    cursor = end;
  }
  if (!count) return false;

  size_t bytes = count * sizeof(uint16_t);
  target.rawTimings = (uint16_t *)ps_malloc(bytes);
  if (!target.rawTimings) target.rawTimings = (uint16_t *)malloc(bytes);
  if (!target.rawTimings) return false;

  cursor = text;
  for (uint16_t i = 0; i < count; i++) {
    while (*cursor == ' ') cursor++;
    char *end = nullptr;
    uint32_t duration = strtoul(cursor, &end, 10);
    if (end == cursor) {
      free(target.rawTimings);
      target.rawTimings = nullptr;
      return false;
    }
    target.rawTimings[i] = (uint16_t)min(duration, (uint32_t)65535);
    cursor = end;
  }
  target.rawCount = count;
  return true;
}

Device *findRuntimeDevice(const char *id) {
  if (!id || !id[0]) return nullptr;
  for (uint8_t i = 0; i < DEVICE_COUNT; i++) {
    if (strcmp(devices[i].id, id) == 0) return &devices[i];
  }
  return nullptr;
}

DeviceCommand *findRuntimeCommand(Device *device, const char *id) {
  if (!device || !id || !id[0]) return nullptr;
  for (uint8_t i = 0; i < device->commandCount; i++) {
    if (strcmp(device->commands[i].id, id) == 0) return &device->commands[i];
  }
  return nullptr;
}

int8_t findPowerCommandIndex(Device &device, const char *const *names,
                             size_t nameCount) {
  for (size_t nameIndex = 0; nameIndex < nameCount; nameIndex++) {
    for (uint8_t i = 0; i < device.commandCount; i++) {
      String label(device.commands[i].label);
      label.trim();
      if (label.equalsIgnoreCase(names[nameIndex])) return (int8_t)i;
    }
  }
  return -1;
}

int8_t inferPowerOnCommandIndex(Device &device) {
  static const char *explicitOnNames[] = {
    "power on", "turn on", "on"
  };
  int8_t found = findPowerCommandIndex(device, explicitOnNames,
    sizeof(explicitOnNames) / sizeof(explicitOnNames[0]));
  if (found >= 0) return found;
  static const char *toggleNames[] = {
    "power", "power toggle", "power on/off", "on/off"
  };
  return findPowerCommandIndex(device, toggleNames,
    sizeof(toggleNames) / sizeof(toggleNames[0]));
}

int8_t inferPowerOffCommandIndex(Device &device) {
  static const char *explicitOffNames[] = {
    "power off", "turn off", "standby", "off"
  };
  int8_t found = findPowerCommandIndex(device, explicitOffNames,
    sizeof(explicitOffNames) / sizeof(explicitOffNames[0]));
  if (found >= 0) return found;
  static const char *toggleNames[] = {
    "power", "power toggle", "power on/off", "on/off"
  };
  return findPowerCommandIndex(device, toggleNames,
    sizeof(toggleNames) / sizeof(toggleNames[0]));
}

uint32_t stableIrFileHash(const String &text) {
  uint32_t hash = 2166136261UL;
  for (size_t i = 0; i < text.length(); i++) {
    hash ^= (uint8_t)text[i];
    hash *= 16777619UL;
  }
  return hash;
}

String stableIrFileId(const char *prefix, const String &text) {
  char buffer[24];
  snprintf(buffer, sizeof(buffer), "%s%08lx", prefix,
           (unsigned long)stableIrFileHash(text));
  return String(buffer);
}

String normaliseIrDevicePath(const String &name) {
  if (name.startsWith("/")) return name;
  if (name.startsWith("devices/")) return String("/") + name;
  return String("/devices/") + name;
}

String irDeviceDisplayName(const String &path) {
  String name = path.substring(path.lastIndexOf('/') + 1);
  if (name.endsWith(".ir") || name.endsWith(".IR")) name.remove(name.length() - 3);
  if (name.length() > 9 && name[name.length() - 9] == '_') {
    bool hashSuffix = true;
    for (size_t i = name.length() - 8; i < name.length(); i++) {
      char c = name[i];
      if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
            (c >= 'A' && c <= 'F'))) {
        hashSuffix = false;
        break;
      }
    }
    if (hashSuffix) name.remove(name.length() - 9);
  }
  name.replace('_', ' ');
  name.trim();
  return name.length() ? name : "IR Device";
}

String irSignalDisplayName(const String &signalName) {
  String label = signalName;
  label.replace('_', ' ');
  label.trim();
  return label.length() ? label : "Command";
}

uint32_t parseIrFileNumber(const String &text) {
  if (text.indexOf(' ') >= 0) return parseFlipperHex(text.c_str());
  return strtoul(text.c_str(), nullptr, 0);
}

bool loadIrDeviceFileIntoRuntime(const String &rawPath) {
  if (!sdReady || DEVICE_COUNT >= MAX_RUNTIME_DEVICES) return false;
  String path = normaliseIrDevicePath(rawPath);
  File file = SD.open(path, FILE_READ);
  if (!file) return false;
  String header = file.readStringUntil('\n');
  header.trim();
  if (header != "Filetype: IR signals file") {
    file.close();
    return false;
  }

  Device &device = devices[DEVICE_COUNT];
  String deviceId = stableIrFileId("irf_", path);
  String deviceName = irDeviceDisplayName(path);
  strlcpy(device.id, deviceId.c_str(), sizeof(device.id));
  strlcpy(device.name, deviceName.c_str(), sizeof(device.name));
  strlcpy(device.transport, "IR", sizeof(device.transport));

  DeviceCommand *command = nullptr;
  String signalName;
  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.startsWith("name:")) {
      if (device.commandCount >= MAX_DEVICE_COMMANDS) break;
      signalName = line.substring(5);
      signalName.trim();
      command = &device.commands[device.commandCount++];
      command->slot = device.commandCount - 1;
      String label = irSignalDisplayName(signalName);
      String commandId = stableIrFileId("irc_", path + "#" + signalName +
        "#" + String(device.commandCount - 1));
      strlcpy(command->label, label.c_str(), sizeof(command->label));
      strlcpy(command->id, commandId.c_str(), sizeof(command->id));
      command->showText = true;
    } else if (command && line.startsWith("type:")) {
      String type = line.substring(5);
      type.trim();
      if (type == "parsed") command->kind = DeviceCommand::PARSED;
    } else if (command && line.startsWith("protocol:")) {
      String protocol = line.substring(9);
      protocol.trim();
      strlcpy(command->protocol, protocol.c_str(), sizeof(command->protocol));
      if (protocol == "SIRC20") command->sonyBits = 20;
      else if (protocol == "SIRC15") command->sonyBits = 15;
      else if (protocol == "SIRC") command->sonyBits = 12;
    } else if (command && line.startsWith("address:")) {
      String value = line.substring(8);
      value.trim();
      command->address = parseIrFileNumber(value);
    } else if (command && line.startsWith("command:")) {
      String value = line.substring(8);
      value.trim();
      command->command = parseIrFileNumber(value);
    } else if (command && line.startsWith("frequency:")) {
      uint32_t frequency = (uint32_t)line.substring(10).toInt();
      command->frequencyKhz = constrain((int)(frequency / 1000U), 20, 60);
    } else if (command && line.startsWith("data:")) {
      String timings = line.substring(5);
      timings.trim();
      if (loadRawTimings(*command, timings.c_str())) command->kind = DeviceCommand::RAW;
    }
  }
  file.close();
  if (!device.commandCount) {
    memset(&device, 0, sizeof(device));
    return false;
  }
  DEVICE_COUNT++;
  return true;
}

void loadIrDeviceFilesIntoRuntime() {
  uint8_t loaded = 0;
  if (SD.exists(DEVICE_INDEX_PATH)) {
    File index = SD.open(DEVICE_INDEX_PATH, FILE_READ);
    while (index && index.available() && DEVICE_COUNT < MAX_RUNTIME_DEVICES) {
      String path = index.readStringUntil('\n');
      path.trim();
      if (path.length() && loadIrDeviceFileIntoRuntime(path)) loaded++;
    }
    if (index) index.close();
  }
  if (loaded) return;

  File root = SD.open("/devices");
  if (!root || !root.isDirectory()) {
    if (root) root.close();
    return;
  }
  File entry = root.openNextFile();
  while (entry && DEVICE_COUNT < MAX_RUNTIME_DEVICES) {
    String name = entry.name();
    bool isFile = !entry.isDirectory();
    entry.close();
    String lower = name;
    lower.toLowerCase();
    if (isFile && lower.endsWith(".ir")) loadIrDeviceFileIntoRuntime(name);
    entry = root.openNextFile();
  }
  root.close();
}

bool jsonDeviceArrayHasId(JsonArray array, const String &id) {
  for (JsonObjectConst device : array) {
    if (String(device["id"] | "") == id) return true;
  }
  return false;
}

bool appendIrDeviceFileSummary(JsonArray target, const String &rawPath) {
  String path = normaliseIrDevicePath(rawPath);
  File file = SD.open(path, FILE_READ);
  if (!file) return false;
  String header = file.readStringUntil('\n');
  header.trim();
  if (header != "Filetype: IR signals file") {
    file.close();
    return false;
  }

  String deviceId = stableIrFileId("irf_", path);
  if (jsonDeviceArrayHasId(target, deviceId)) {
    file.close();
    return false;
  }
  JsonObject device = target.add<JsonObject>();
  device["id"] = deviceId;
  device["name"] = irDeviceDisplayName(path);
  device["source"] = "OpenRemote Studio";
  device["transport"] = "ir";
  device["type"] = "IRDB device";
  device["protocol"] = "IR";
  device["fileBacked"] = true;
  device["filePath"] = path;
  JsonArray commands = device["commands"].to<JsonArray>();
  JsonObject command;
  String signalName;
  uint8_t commandIndex = 0;
  while (file.available() && commandIndex < MAX_DEVICE_COMMANDS) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.startsWith("name:")) {
      signalName = line.substring(5);
      signalName.trim();
      command = commands.add<JsonObject>();
      command["id"] = stableIrFileId("irc_", path + "#" + signalName +
        "#" + String(commandIndex));
      command["name"] = irSignalDisplayName(signalName);
      commandIndex++;
    } else if (!command.isNull() && line.startsWith("protocol:")) {
      String protocol = line.substring(9);
      protocol.trim();
      command["protocol"] = protocol;
      if (String(device["protocol"] | "IR") == "IR") device["protocol"] = protocol;
    } else if (!command.isNull() && line.startsWith("type:")) {
      String type = line.substring(5);
      type.trim();
      if (type == "raw") {
        command["protocol"] = "Raw IR";
        if (String(device["protocol"] | "IR") == "IR") device["protocol"] = "Raw IR";
      }
    }
  }
  file.close();
  return commandIndex > 0;
}

void appendIrDeviceFileSummaries(JsonArray target) {
  uint8_t appended = 0;
  if (SD.exists(DEVICE_INDEX_PATH)) {
    File index = SD.open(DEVICE_INDEX_PATH, FILE_READ);
    while (index && index.available()) {
      String path = index.readStringUntil('\n');
      path.trim();
      if (path.length() && appendIrDeviceFileSummary(target, path)) appended++;
    }
    if (index) index.close();
  }
  if (appended) return;

  File root = SD.open("/devices");
  if (!root || !root.isDirectory()) {
    if (root) root.close();
    return;
  }
  File entry = root.openNextFile();
  while (entry) {
    String name = entry.name();
    bool isFile = !entry.isDirectory();
    entry.close();
    String lower = name;
    lower.toLowerCase();
    if (isFile && lower.endsWith(".ir")) appendIrDeviceFileSummary(target, name);
    entry = root.openNextFile();
  }
  root.close();
}

void applyCommandFeedbackStyle(bool active) {
  lv_color_t red = lv_color_hex(0xFF453A);
  if (statusPill && lv_obj_is_valid(statusPill)) {
    lv_obj_set_style_bg_color(statusPill, active ? lv_color_hex(0x5A0000) : lv_color_black(), 0);
    lv_obj_set_style_bg_opa(statusPill, active ? LV_OPA_COVER : (lv_opa_t)115, 0);
    lv_obj_set_style_border_color(statusPill, active ? red : lv_color_white(), 0);
    lv_obj_set_style_border_opa(statusPill, active ? LV_OPA_COVER : (lv_opa_t)64, 0);
  }
  if (clockLabel && lv_obj_is_valid(clockLabel)) {
    lv_obj_set_style_text_color(clockLabel, active ? red : lv_color_white(), 0);
  }
  if (statusBattery && lv_obj_is_valid(statusBattery)) {
    lv_obj_set_style_border_color(statusBattery, active ? red : lv_color_white(), 0);
  }
  if (batteryFill && lv_obj_is_valid(batteryFill)) {
    lv_obj_set_style_bg_color(batteryFill, active ? red : lv_color_make(166, 255, 184), 0);
  }
  if (statusBatteryTerminal && lv_obj_is_valid(statusBatteryTerminal)) {
    lv_obj_set_style_bg_color(statusBatteryTerminal, active ? red : lv_color_white(), 0);
  }
}

void flashCommandFeedback() {
  commandFeedbackUntilMs = millis() + 80UL;
  if (!commandFeedbackActive) {
    commandFeedbackActive = true;
    applyCommandFeedbackStyle(true);
  }
  lv_refr_now(nullptr);
}

void serviceCommandFeedback(unsigned long now) {
  if (!commandFeedbackActive || (int32_t)(now - commandFeedbackUntilMs) < 0) return;
  commandFeedbackActive = false;
  applyCommandFeedbackStyle(false);
}

// Button Test mode never transmits a real IR/BLE command (see buttonTestModeActive()
// and its two call sites in serviceKeypad(), which continue before any binding lookup
// or send). This paints the status pill green instead of applyCommandFeedbackStyle's
// red, so a test-mode press is visually unmistakable from a genuine transmit.
void applyButtonTestFeedbackStyle(bool active) {
  lv_color_t green = lv_color_hex(0x30D158);
  if (statusPill && lv_obj_is_valid(statusPill)) {
    lv_obj_set_style_bg_color(statusPill, active ? lv_color_hex(0x0B3A1E) : lv_color_black(), 0);
    lv_obj_set_style_bg_opa(statusPill, active ? LV_OPA_COVER : (lv_opa_t)115, 0);
    lv_obj_set_style_border_color(statusPill, active ? green : lv_color_white(), 0);
    lv_obj_set_style_border_opa(statusPill, active ? LV_OPA_COVER : (lv_opa_t)64, 0);
  }
  if (clockLabel && lv_obj_is_valid(clockLabel)) {
    lv_obj_set_style_text_color(clockLabel, active ? green : lv_color_white(), 0);
  }
  if (statusBattery && lv_obj_is_valid(statusBattery)) {
    lv_obj_set_style_border_color(statusBattery, active ? green : lv_color_white(), 0);
  }
  if (batteryFill && lv_obj_is_valid(batteryFill)) {
    lv_obj_set_style_bg_color(batteryFill, active ? green : lv_color_make(166, 255, 184), 0);
  }
  if (statusBatteryTerminal && lv_obj_is_valid(statusBatteryTerminal)) {
    lv_obj_set_style_bg_color(statusBatteryTerminal, active ? green : lv_color_white(), 0);
  }
}

void flashButtonTestFeedback() {
  buttonTestFeedbackUntilMs = millis() + 80UL;
  if (!buttonTestFeedbackActive) {
    buttonTestFeedbackActive = true;
    applyButtonTestFeedbackStyle(true);
  }
  lv_refr_now(nullptr);
}

void serviceButtonTestFeedback(unsigned long now) {
  if (!buttonTestFeedbackActive || (int32_t)(now - buttonTestFeedbackUntilMs) < 0) return;
  buttonTestFeedbackActive = false;
  applyButtonTestFeedbackStyle(false);
}

bool buttonTestModeActive() {
  return buttonTestActive && pages[currentPage].kind == PAGE_REMOTE_SETTINGS &&
         settingsView == SETTINGS_BUTTONS;
}

void setButtonTestVisual(bool pulsing, const char *name = nullptr) {
  if (!buttonTestPanel || !lv_obj_is_valid(buttonTestPanel) ||
      !buttonTestLabel || !lv_obj_is_valid(buttonTestLabel)) return;
  lv_obj_set_style_bg_color(buttonTestPanel,
    pulsing ? lv_color_hex(0xA50016) : lv_color_hex(0x222327), 0);
  lv_obj_set_style_border_color(buttonTestPanel,
    pulsing ? lv_color_hex(0xFF453A) : lv_color_hex(0x36383E), 0);
  lv_obj_set_style_border_opa(buttonTestPanel,
    pulsing ? LV_OPA_COVER : LV_OPA_80, 0);
  lv_obj_set_style_text_color(buttonTestLabel,
    pulsing ? lv_color_white() : lv_color_hex(0x9696A0), 0);
  lv_label_set_text(buttonTestLabel,
    pulsing && name && name[0] ? name : "No button pressed");
}

void pulseButtonTest() {
  if (!buttonTestModeActive() || buttonTestHeldIndex == -1) return;
  setButtonTestVisual(true, buttonTestHeldName);
  flashButtonTestFeedback();
  buttonTestPulseUntilMs = millis() + 80UL;
}

void beginButtonTest(int8_t index, const char *name) {
  buttonTestHeldIndex = index;
  strlcpy(buttonTestHeldName, name ? name : "Unknown", sizeof(buttonTestHeldName));
  nextButtonTestRepeatMs = millis() + physicalRepeatDelayMs;
  pulseButtonTest();
  lastWakeMs = millis();
}

void endButtonTest(int8_t index) {
  if (buttonTestHeldIndex != index) return;
  buttonTestHeldIndex = -1;
  buttonTestHeldName[0] = '\0';
  buttonTestPulseUntilMs = 0;
  nextButtonTestRepeatMs = 0;
  setButtonTestVisual(false);
  buttonTestFeedbackActive = false;
  buttonTestFeedbackUntilMs = 0;
  applyButtonTestFeedbackStyle(false);
  lv_refr_now(nullptr);
}

void serviceButtonTest(unsigned long now) {
  if (!buttonTestModeActive()) {
    if (buttonTestHeldIndex != -1) endButtonTest(buttonTestHeldIndex);
    return;
  }
  if (buttonTestPulseUntilMs && (int32_t)(now - buttonTestPulseUntilMs) >= 0) {
    buttonTestPulseUntilMs = 0;
    setButtonTestVisual(false);
  }
  if (buttonTestHeldIndex == -1 || !physicalRepeatEnabled ||
      (int32_t)(now - nextButtonTestRepeatMs) < 0) return;
  pulseButtonTest();
  nextButtonTestRepeatMs = now +
    (uint16_t)max(50, 1000 / (int)physicalRepeatRateHz);
}

String normaliseHomebridgeAddress(String address) {
  address.trim();
  if (!address.length()) return "";
  if (!address.startsWith("http://") && !address.startsWith("https://")) {
    address = "http://" + address;
  }
  int schemeEnd = address.indexOf("://");
  int pathStart = address.indexOf('/', schemeEnd + 3);
  if (pathStart >= 0) address.remove(pathStart);
  while (address.endsWith("/")) address.remove(address.length() - 1);
  return address;
}

String resolvedHomebridgeAddress(const String &address) {
  int schemeEnd = address.indexOf("://");
  if (schemeEnd < 0) return address;
  int authorityStart = schemeEnd + 3;
  int authorityEnd = address.indexOf('/', authorityStart);
  if (authorityEnd < 0) authorityEnd = address.length();
  String authority = address.substring(authorityStart, authorityEnd);
  int colon = authority.indexOf(':');
  String host = colon >= 0 ? authority.substring(0, colon) : authority;
  String port = colon >= 0 ? authority.substring(colon) : "";
  String lowerHost = host;
  lowerHost.toLowerCase();
  if (!lowerHost.endsWith(".local")) return address;
  String mdnsHost = host.substring(0, host.length() - 6);
  IPAddress resolved = MDNS.queryHost(mdnsHost, 2500);
  if (!(resolved[0] || resolved[1] || resolved[2] || resolved[3])) return address;
  return address.substring(0, authorityStart) + resolved.toString() + port +
    address.substring(authorityEnd);
}

int homebridgeHttpOnce(const String &base, const char *method, const String &path,
                       const String &payload, const String &token, String &response) {
  HTTPClient http;
  http.setConnectTimeout(4000);
  http.setTimeout(8000);
  if (!http.begin(base + path)) return HTTPC_ERROR_CONNECTION_REFUSED;
  http.addHeader("Accept", "application/json");
  if (payload.length()) http.addHeader("Content-Type", "application/json");
  if (token.length()) http.addHeader("Authorization", "Bearer " + token);
  int status = strcmp(method, "GET") == 0 ? http.GET() :
    (strcmp(method, "PUT") == 0 ? http.PUT(payload) : http.POST(payload));
  response = status > 0 ? http.getString() : HTTPClient::errorToString(status);
  http.end();
  return status;
}

int homebridgeHttp(const String &address, const char *method, const String &path,
                   const String &payload, const String &token, String &response) {
  String base = normaliseHomebridgeAddress(address);
  if (!base.length()) {
    response = "Homebridge address is empty";
    return HTTPC_ERROR_CONNECTION_REFUSED;
  }
  String resolved = resolvedHomebridgeAddress(base);
  int status = homebridgeHttpOnce(resolved, method, path, payload, token, response);
  if (status < 0 && resolved != base) {
    status = homebridgeHttpOnce(base, method, path, payload, token, response);
  }
  return status;
}

String homebridgeResponseError(const String &response, const String &fallback) {
  JsonDocument doc(&psramJsonAllocator);
  if (!deserializeJson(doc, response)) {
    const char *message = doc["message"] | "";
    if (!message[0]) message = doc["error"] | "";
    if (message[0]) return String(message);
  }
  return response.length() && response.length() < 180 ? response : fallback;
}

bool homebridgeLogin(const String &address, const String &username,
                     const String &password, String &token, String &error) {
  if (WiFi.status() != WL_CONNECTED) {
    error = "Connect OpenRemote to Wi-Fi before using Homebridge";
    return false;
  }
  if (!username.length() || !password.length()) {
    error = "Homebridge username and password are required";
    return false;
  }
  JsonDocument request;
  request["username"] = username;
  request["password"] = password;
  String payload;
  serializeJson(request, payload);
  String response;
  int status = homebridgeHttp(address, "POST", "/api/auth/login", payload, "", response);
  if (status != 200 && status != 201) {
    error = homebridgeResponseError(response, String("Homebridge login failed (HTTP ") + status + ")");
    return false;
  }
  JsonDocument result(&psramJsonAllocator);
  if (deserializeJson(result, response) || !(result["access_token"] | "")[0]) {
    error = "Homebridge returned no access token";
    return false;
  }
  token = String((const char *)(result["access_token"] | ""));
  return true;
}

bool homebridgeAuthorizedRequest(const char *method, const String &path,
                                 const String &payload, String &response,
                                 int &status, String &error) {
  if (!homebridgeAddress.length() || !homebridgeUsername.length() ||
      !homebridgePassword.length()) {
    error = "Homebridge has not been configured in WebConfig";
    return false;
  }
  if (!ensureStationConnected()) {
    error = "Could not connect OpenRemote to its saved Wi-Fi network";
    scheduleNetworkShutdown();
    return false;
  }
  if (!homebridgeToken.length() && !homebridgeLogin(homebridgeAddress,
      homebridgeUsername, homebridgePassword, homebridgeToken, error)) {
    scheduleNetworkShutdown();
    return false;
  }
  status = homebridgeHttp(homebridgeAddress, method, path, payload,
                          homebridgeToken, response);
  if (status == 401 || status == 403) {
    homebridgeToken = "";
    if (!homebridgeLogin(homebridgeAddress, homebridgeUsername,
                         homebridgePassword, homebridgeToken, error)) {
      scheduleNetworkShutdown();
      return false;
    }
    status = homebridgeHttp(homebridgeAddress, method, path, payload,
                            homebridgeToken, response);
  }
  if (status < 200 || status >= 300) {
    error = homebridgeResponseError(response,
      String("Homebridge request failed (HTTP ") + status + ")");
    scheduleNetworkShutdown();
    return false;
  }
  scheduleNetworkShutdown();
  return true;
}

bool readHomebridgeCharacteristic(const DeviceCommand &command, JsonVariant value,
                                  String &error) {
  String response;
  int status = 0;
  String path = String("/api/accessories/") + command.homebridgeAccessoryId;
  if (!homebridgeAuthorizedRequest("GET", path, "", response, status, error)) return false;
  JsonDocument accessory(&psramJsonAllocator);
  if (deserializeJson(accessory, response)) {
    error = "Homebridge returned invalid accessory data";
    return false;
  }
  for (JsonObjectConst characteristic : accessory["serviceCharacteristics"].as<JsonArrayConst>()) {
    if (strcmp(characteristic["type"] | "", command.homebridgeCharacteristic) != 0) continue;
    value.set(characteristic["value"]);
    return true;
  }
  error = "Homebridge characteristic is no longer available";
  return false;
}

bool transmitHomebridgeCommand(const DeviceCommand &command) {
  if (!command.homebridgeAccessoryId[0] || !command.homebridgeCharacteristic[0]) return false;
  JsonDocument body;
  body["characteristicType"] = command.homebridgeCharacteristic;
  if (command.homebridgeOperation == 1 || command.homebridgeOperation == 2) {
    JsonDocument current;
    String error;
    if (!readHomebridgeCharacteristic(command, current["value"], error)) {
      Serial.printf("Homebridge read failed: %s\n", error.c_str());
      return false;
    }
    if (command.homebridgeOperation == 1) {
      body["value"] = !current["value"].as<bool>();
    } else {
      float value = current["value"].as<float>() + command.homebridgeStep;
      value = constrain(value, command.homebridgeMin, command.homebridgeMax);
      body["value"] = value;
    }
  } else if (command.homebridgeValueType == 1) {
    body["value"] = command.homebridgeValue >= 0.5f;
  } else if (command.homebridgeValueType == 2) {
    body["value"] = command.homebridgeValue;
  } else {
    body["value"] = command.homebridgeStringValue;
  }
  String payload;
  serializeJson(body, payload);
  String response;
  String error;
  int status = 0;
  String path = String("/api/accessories/") + command.homebridgeAccessoryId;
  if (!homebridgeAuthorizedRequest("PUT", path, payload, response, status, error)) {
    Serial.printf("Homebridge command failed: %s\n", error.c_str());
    return false;
  }
  flashCommandFeedback();
  return true;
}

uint16_t legacyKeyboardUsageToConsumer(uint16_t usage) {
  switch (usage) {
    case 0x4A: return 0x0223;  // Home
    case 0x29: return 0x0224;  // Back / Escape
    case 0x52: return 0x0042;  // Up
    case 0x51: return 0x0043;  // Down
    case 0x50: return 0x0044;  // Left
    case 0x4F: return 0x0045;  // Right
    case 0x28: return 0x0041;  // Enter / OK
    default: return 0;
  }
}

bool isVoiceSearchCommand(const DeviceCommand *command) {
  return command && command->kind == DeviceCommand::BLE_HID &&
    (strcmp(command->id, "ble_voice_search") == 0 ||
     (command->hidReport == 2 && command->hidUsage == 0x0221));
}

bool sendBleHidState(const DeviceCommand &command, bool pressed) {
  if (!bleConsumerInput ||
      (command.hidReport != 1 && command.hidReport != 2)) return false;
  uint8_t report[BLE_HID_INPUT_BYTES] = {0};
  if (pressed) {
    if (command.hidReport == 1) {
      uint16_t consumerUsage = legacyKeyboardUsageToConsumer(command.hidUsage);
      if (consumerUsage) {
        report[8] = (uint8_t)(consumerUsage & 0xFF);
        report[9] = (uint8_t)(consumerUsage >> 8);
      } else if (command.hidUsage <= 0xFF) {
        report[2] = (uint8_t)command.hidUsage;
      } else {
        return false;
      }
    } else {
      report[8] = (uint8_t)(command.hidUsage & 0xFF);
      report[9] = (uint8_t)(command.hidUsage >> 8);
    }
  }
  bleConsumerInput->setValue(report, sizeof(report));
  bleConsumerInput->notify();
  return true;
}

bool beginVoiceSearchHold(const DeviceCommand *command, bool fromTouch) {
  if (!isVoiceSearchCommand(command)) return false;
  if (heldVoiceSearchCommand == command) return true;
  if (heldVoiceSearchCommand) endVoiceSearchHold();
  if (!bleReady) applyBluetoothState();
  if (!bleReady || bleSuspended || !bleConnected || !command->hidUsage) return false;

  if (atvvMicOpenPending || atvvMicClosePending) serviceAtvvVoice(millis());
  if (atvvAudioStarted) atvvStopAudio(ATVV_AUDIO_STOP_NEW_STREAM);
  resetAtvvVoiceSession();
  bleKeepAliveUntilMs = millis() + BLE_POST_CONNECT_GRACE_MS;
  flashCommandFeedback();
  heldVoiceSearchCommand = command;
  heldVoiceSearchFromTouch = fromTouch;
  heldVoiceSearchPhysicalKey = 0;

  if (atvvInteractionModel == ATVV_INTERACTION_HOLD_TO_TALK) {
    uint8_t streamId = atvvNextStreamId++;
    if (!atvvNextStreamId || atvvNextStreamId > 0x80) atvvNextStreamId = 1;
    if (!atvvStartAudio(ATVV_AUDIO_START_HTT, streamId)) {
      heldVoiceSearchCommand = nullptr;
      heldVoiceSearchFromTouch = false;
      resetAtvvVoiceSession();
      return false;
    }
    Serial.printf("Voice Search: hold-to-talk stream %u started\n", streamId);
    return true;
  }

  atvvVoiceState = ATVV_VOICE_SEARCH_REQUESTED;
  atvvSearchRequestedMs = millis();
  bool controlSent = atvvNotifyControl(ATVV_START_SEARCH);
  delay(12);
  if (!sendBleHidState(*command, true)) {
    heldVoiceSearchCommand = nullptr;
    heldVoiceSearchFromTouch = false;
    resetAtvvVoiceSession();
    return false;
  }
  delay(20);
  sendBleHidState(*command, false);
  Serial.printf("Voice Search: held start, HID click sent%s\n",
                controlSent ? "" : " (ATVV CTL not subscribed)");
  return true;
}

void endVoiceSearchHold(const DeviceCommand *command) {
  if (!heldVoiceSearchCommand ||
      (command && command != heldVoiceSearchCommand)) return;
  heldVoiceSearchCommand = nullptr;
  heldVoiceSearchFromTouch = false;
  heldVoiceSearchPhysicalKey = 0;
  if (atvvInteractionModel == ATVV_INTERACTION_HOLD_TO_TALK &&
      atvvAudioStarted) {
    atvvStopAudio(ATVV_AUDIO_STOP_BUTTON_RELEASE);
    Serial.println("Voice Search: hold-to-talk release sent AUDIO_STOP reason 02");
    resetAtvvVoiceSession();
    return;
  }
  if (atvvVoiceState != ATVV_VOICE_IDLE || atvvMicOpenPending) {
    atvvVoiceReleasePending = true;
    if (atvvAudioStarted) atvvStopAfterMs = millis();
    Serial.println(atvvAudioStarted
      ? "Voice Search: released, AUDIO_STOP queued"
      : "Voice Search: released before MIC_OPEN, stop deferred");
  }
}

bool transmitIrCommand(const DeviceCommand &command) {
  if (command.kind == DeviceCommand::HOMEBRIDGE) {
    return transmitHomebridgeCommand(command);
  }
  if (command.kind == DeviceCommand::BLE_HID) {
    if (!bleReady) applyBluetoothState();
    if (!bleReady || bleSuspended || !bleConnected || !command.hidUsage) return false;
    bleKeepAliveUntilMs = millis() + BLE_POST_CONNECT_GRACE_MS;
    if (isVoiceSearchCommand(&command)) {
      if (!beginVoiceSearchHold(&command)) return false;
      delay(80);
      endVoiceSearchHold(&command);
      return true;
    }
    flashCommandFeedback();
    if (sendBleHidState(command, true)) {
      delay(20);
      sendBleHidState(command, false);
      return true;
    }
    return false;
  }
  if (command.kind == DeviceCommand::RAW && command.rawTimings && command.rawCount) {
    flashCommandFeedback();
    IrSender.sendRaw(command.rawTimings, command.rawCount,
                     command.frequencyKhz ? command.frequencyKhz : 38);
    return true;
  }
  if (command.kind != DeviceCommand::PARSED) return false;

  if (strcmp(command.protocol, "NEC") == 0) {
    flashCommandFeedback();
    IrSender.sendNEC((uint16_t)command.address, (uint16_t)command.command, 0);
  } else if (strcmp(command.protocol, "NECext") == 0 ||
             strcmp(command.protocol, "NEC1") == 0) {
    flashCommandFeedback();
    IrSender.sendOnkyo((uint16_t)command.address, (uint16_t)command.command, 0);
  } else if (strcmp(command.protocol, "Samsung32") == 0) {
    flashCommandFeedback();
    IrSender.sendSamsung((uint16_t)command.address, (uint16_t)command.command, 0);
  } else if (strcmp(command.protocol, "RC5") == 0 ||
             strcmp(command.protocol, "RC5X") == 0) {
    flashCommandFeedback();
    IrSender.sendRC5((uint8_t)command.address, (uint8_t)command.command, 0);
  } else if (strcmp(command.protocol, "RC6") == 0) {
    flashCommandFeedback();
    IrSender.sendRC6((uint8_t)command.address, (uint8_t)command.command, 0);
  } else if (strncmp(command.protocol, "SIRC", 4) == 0) {
    flashCommandFeedback();
    IrSender.sendSony((uint16_t)command.address, (uint8_t)command.command, 2,
                      command.sonyBits ? command.sonyBits : 12);
  } else {
    Serial.printf("IR protocol not yet supported: %s\n", command.protocol);
    return false;
  }
  return true;
}

bool configureBluetoothCommand(DeviceCommand &command, const char *label,
                               const char *reportName = nullptr, uint16_t usage = 0) {
  if (reportName && usage) {
    command.hidReport = strcmp(reportName, "keyboard") == 0 ? 1 : 2;
    command.hidUsage = usage;
    command.kind = DeviceCommand::BLE_HID;
    return true;
  }

  String key = label ? label : "";
  key.toLowerCase();
  key.replace("_", " ");
  key.trim();
  command.hidReport = 1;
  if (key == "home") command.hidUsage = 0x4A;
  else if (key == "back" || key == "menu / back" || key == "menu back") command.hidUsage = 0x29;
  else if (key == "up") command.hidUsage = 0x52;
  else if (key == "down") command.hidUsage = 0x51;
  else if (key == "left") command.hidUsage = 0x50;
  else if (key == "right") command.hidUsage = 0x4F;
  else if (key == "ok" || key == "select" || key == "enter") command.hidUsage = 0x28;
  else {
    command.hidReport = 2;
    if (key == "play / pause" || key == "play pause") command.hidUsage = 0x00CD;
    else if (key == "stop") command.hidUsage = 0x00B7;
    else if (key == "rewind") command.hidUsage = 0x00B4;
    else if (key == "fast forward") command.hidUsage = 0x00B3;
    else if (key == "previous" || key == "skip back") command.hidUsage = 0x00B6;
    else if (key == "next" || key == "skip forward") command.hidUsage = 0x00B5;
    else if (key == "volume up" || key == "vol up" || key == "vol_up") command.hidUsage = 0x00E9;
    else if (key == "volume down" || key == "vol down" || key == "vol_dn") command.hidUsage = 0x00EA;
    else if (key == "mute") command.hidUsage = 0x00E2;
    else if (key == "power") command.hidUsage = 0x0030;
    else if (key == "voice search" || key == "assistant" || key == "microphone") command.hidUsage = 0x0221;
  }
  if (!command.hidUsage) return false;
  command.kind = DeviceCommand::BLE_HID;
  return true;
}

const BluetoothPresetCommand *findChromecastPresetCommand(const char *id,
                                                           const char *label) {
  for (uint8_t i = 0; i < CHROMECAST_COMMAND_COUNT; i++) {
    const BluetoothPresetCommand &preset = CHROMECAST_COMMANDS[i];
    if ((id && id[0] && strcmp(id, preset.id) == 0) ||
        (label && label[0] && String(label).equalsIgnoreCase(preset.name))) {
      return &preset;
    }
  }
  return nullptr;
}

uint32_t parseThemeColour(const char *value, uint32_t fallback) {
  if (!value || value[0] != '#') return fallback;
  char *end = nullptr;
  uint32_t colour = strtoul(value + 1, &end, 16);
  return end && *end == '\0' ? colour : fallback;
}

uint8_t themeRowCountForSplit(int split) {
  static const uint16_t canonicalSplits[5] = {246, 195, 144, 93, 42};
  uint8_t bestRow = 1;
  int bestDistance = abs(split - (int)canonicalSplits[0]);
  for (uint8_t index = 1; index < 5; index++) {
    int distance = abs(split - (int)canonicalSplits[index]);
    if (distance < bestDistance) {
      bestDistance = distance;
      bestRow = index + 1;
    }
  }
  return bestRow;
}

void applyRuntimeThemeRowCalibration() {
  for (uint8_t index = 0; index < runtimeThemeCount; index++) {
    RuntimeThemeStyle &theme = runtimeThemes[index];
    if (theme.rowCount < 1 || theme.rowCount > 5) {
      theme.rowCount = themeRowCountForSplit(theme.split);
    }
    theme.split = debugRowPixels[theme.rowCount - 1];
  }
}

RuntimeThemeStyle *findRuntimeThemeStyle(const char *path) {
  if (!path || !path[0]) return nullptr;
  for (uint8_t i = 0; i < runtimeThemeCount; i++) {
    if (strcmp(runtimeThemes[i].path, path) == 0) return &runtimeThemes[i];
  }
  return nullptr;
}

void appendRuntimeSequenceStep(ActivityStep *steps, uint8_t &stepCount,
                               uint16_t *deviceMask, JsonObjectConst step,
                               JsonDocument &doc, uint8_t depth = 0) {
  if (stepCount >= MAX_ACTIVITY_STEPS || depth > 3) return;
  const char *type = step["type"] | "";
  if (strcmp(type, "command") == 0) {
    Device *device = findRuntimeDevice(step["deviceId"] | "");
    DeviceCommand *command = findRuntimeCommand(device, step["commandId"] | "");
    if (!device || !command) return;
    ActivityStep &runtimeStep = steps[stepCount++];
    runtimeStep.kind = ActivityStep::COMMAND;
    runtimeStep.deviceIndex = (uint8_t)(device - devices);
    runtimeStep.commandIndex = (uint8_t)(command - device->commands);
    runtimeStep.delayMs = 0;
    runtimeStep.delayWhenDevicePoweredOn = false;
    if (deviceMask && runtimeStep.deviceIndex < 16) {
      *deviceMask |= (uint16_t)(1U << runtimeStep.deviceIndex);
    }
    return;
  }
  if (strcmp(type, "delay") == 0) {
    ActivityStep &runtimeStep = steps[stepCount++];
    runtimeStep.kind = ActivityStep::DELAY;
    const char *condition = step["condition"] | "always";
    Device *conditionDevice = strcmp(condition, "device-powered-on") == 0
      ? findRuntimeDevice(step["conditionDeviceId"] | "") : nullptr;
    runtimeStep.deviceIndex = conditionDevice
      ? (uint8_t)(conditionDevice - devices) : 0xFF;
    runtimeStep.commandIndex = 0xFF;
    runtimeStep.delayMs = constrain((int)(step["ms"] | 250), 0, 120000);
    runtimeStep.delayWhenDevicePoweredOn = conditionDevice != nullptr;
    return;
  }
  if (strcmp(type, "macro") != 0) return;

  const char *macroId = step["macroId"] | "";
  for (JsonObjectConst macro : doc["macros"].as<JsonArrayConst>()) {
    if (strcmp(macro["id"] | "", macroId) != 0) continue;
    for (JsonObjectConst macroStep : macro["steps"].as<JsonArrayConst>()) {
      appendRuntimeSequenceStep(steps, stepCount, deviceMask, macroStep, doc,
                                depth + 1);
      if (stepCount >= MAX_ACTIVITY_STEPS) break;
    }
    break;
  }
}

void loadRuntimeModel(JsonDocument &doc) {
  activitySequenceActive = false;
  activitySequenceMacro = -1;
  // Runtime indexes point into arrays that are about to be rebuilt. Returning
  // transient activity/device pages to the Activities page prevents stale
  // indexes from being rendered after a WebConfig sync removes or reorders data.
  activeActivity = -1;
  activeDevice = -1;
  pendingDeviceOpen = -1;
  deviceReturnPage = 1;
  if (currentPage > 1) currentPage = 1;
  bool smartBindingsApplied[MAX_RUNTIME_DEVICES] = {};
  clearRuntimeCommands();
  DEVICE_COUNT = 0;
  ACTIVITY_COUNT = 0;
  MACRO_COUNT = 0;
  memset(devices, 0, sizeof(Device) * MAX_RUNTIME_DEVICES);
  memset(activities, 0, sizeof(Activity) * MAX_RUNTIME_ACTIVITIES);
  memset(macros, 0, sizeof(Macro) * MAX_RUNTIME_MACROS);
  memset(activityTiles, 0, sizeof(Tile) * MAX_RUNTIME_ACTIVITIES * MAX_ACTIVITY_TILES);
  memset(activityTileCounts, 0, MAX_RUNTIME_ACTIVITIES);
  memset(runtimeThemes, 0, sizeof(runtimeThemes));
  runtimeThemeCount = 0;
  activeRuntimeThemeStyle = nullptr;
  activitiesThemePath[0] = '\0';
  for (uint8_t deviceIndex = 0; deviceIndex < MAX_RUNTIME_DEVICES; deviceIndex++) {
    devices[deviceIndex].powerOnCommandIndex = -1;
    devices[deviceIndex].powerOffCommandIndex = -1;
    devices[deviceIndex].powerTrackingConfigured = false;
    devices[deviceIndex].powerTrackingEnabled = false;
    for (uint8_t buttonIndex = 0; buttonIndex < PHYSICAL_BUTTON_COUNT; buttonIndex++) {
      devices[deviceIndex].physicalBindings[buttonIndex].deviceIndex = 0xFF;
      devices[deviceIndex].physicalBindings[buttonIndex].commandIndex = 0xFF;
    }
  }
  for (uint8_t activityIndex = 0; activityIndex < MAX_RUNTIME_ACTIVITIES; activityIndex++) {
    for (uint8_t buttonIndex = 0; buttonIndex < PHYSICAL_BUTTON_COUNT; buttonIndex++) {
      activities[activityIndex].physicalBindings[buttonIndex].deviceIndex = 0xFF;
      activities[activityIndex].physicalBindings[buttonIndex].commandIndex = 0xFF;
    }
  }

  for (JsonObjectConst source : doc["themes"].as<JsonArrayConst>()) {
    if (runtimeThemeCount >= MAX_RUNTIME_THEMES) break;
    const char *path = source["runtimePath"] | "";
    if (!path[0]) continue;
    RuntimeThemeStyle &theme = runtimeThemes[runtimeThemeCount++];
    strlcpy(theme.path, path, sizeof(theme.path));
    int configuredSplit = constrain((int)(source["split"] | 144), 0, LCD_H - 1);
    theme.rowCount = themeRowCountForSplit(configuredSplit);
    theme.split = debugRowPixels[theme.rowCount - 1];
    theme.glassEnabled = source["glassEnabled"] | true;
    theme.glassColour = parseThemeColour(source["glassColour"] | "#24384d", 0x24384D);
    int opacityPercent = constrain((int)(source["glassTransparency"] | 25), 0, 100);
    theme.glassOpacity = (uint8_t)((opacityPercent * 255L) / 100L);
  }

  loadIrDeviceFilesIntoRuntime();

  for (JsonObjectConst source : doc["devices"].as<JsonArrayConst>()) {
    if (DEVICE_COUNT >= MAX_RUNTIME_DEVICES) break;
    const char *sourceId = source["id"] | "";
    if (sourceId[0] && findRuntimeDevice(sourceId)) continue;
    String bluetoothProfile = source["bluetoothProfile"] | "";
    String sourceName = source["name"] | "";
    bluetoothProfile.toLowerCase();
    sourceName.toLowerCase();
    bool androidTvProfile = strcmp(sourceId, "ble_chromecast") == 0 ||
      bluetoothProfile.indexOf("google-tv") >= 0 ||
      bluetoothProfile.indexOf("android-tv") >= 0 ||
      bluetoothProfile.indexOf("chromecast") >= 0 ||
      sourceName.indexOf("chromecast") >= 0 || sourceName.indexOf("google tv") >= 0;
    Device &device = devices[DEVICE_COUNT++];
    strlcpy(device.id, sourceId, sizeof(device.id));
    strlcpy(device.name, source["name"] | "Unnamed device", sizeof(device.name));
    const char *transport = source["protocol"] | "";
    if (!transport[0]) transport = source["transport"] | "IR";
    strlcpy(device.transport, transport, sizeof(device.transport));
    for (JsonObjectConst command : source["commands"].as<JsonArrayConst>()) {
      if (device.commandCount >= MAX_DEVICE_COMMANDS) break;
      DeviceCommand &runtimeCommand = device.commands[device.commandCount];
      const char *label = command["name"] | "";
      if (!label[0]) label = command["label"] | "Command";
      strlcpy(runtimeCommand.label, label, sizeof(runtimeCommand.label));
      strlcpy(runtimeCommand.id, command["id"] | "", sizeof(runtimeCommand.id));
      runtimeCommand.showText = true;
      runtimeCommand.repeatDefault = command["repeat"] | false;
      runtimeCommand.slot = device.commandCount;

      JsonObjectConst ir = command["ir"].as<JsonObjectConst>();
      const char *type = ir["type"] | "";
      if (strcmp(type, "raw") == 0) {
        uint32_t frequency = ir["frequency"] | 38000;
        runtimeCommand.frequencyKhz = constrain((int)(frequency / 1000U), 20, 60);
        if (loadRawTimings(runtimeCommand, ir["data"] | "")) {
          runtimeCommand.kind = DeviceCommand::RAW;
        }
      } else if (strcmp(type, "parsed") == 0) {
        strlcpy(runtimeCommand.protocol, ir["protocol"] | "",
                sizeof(runtimeCommand.protocol));
        const char *addressText = ir["address"] | "";
        const char *commandText = ir["command"] | "";
        runtimeCommand.address = addressText[0]
          ? parseFlipperHex(addressText)
          : (uint32_t)(ir["addressValue"] | 0U);
        runtimeCommand.command = commandText[0]
          ? parseFlipperHex(commandText)
          : (uint32_t)(ir["commandValue"] | 0U);
        runtimeCommand.sonyBits = ir["bits"] | 0;
        runtimeCommand.kind = DeviceCommand::PARSED;
      }
      JsonObjectConst hid = command["hid"].as<JsonObjectConst>();
      if (!hid.isNull()) {
        configureBluetoothCommand(runtimeCommand, label, hid["report"] | "consumer",
                                  (uint16_t)(hid["usage"] | 0));
      }
      JsonObjectConst homebridge = command["homebridge"].as<JsonObjectConst>();
      if (!homebridge.isNull()) {
        strlcpy(runtimeCommand.homebridgeAccessoryId,
                homebridge["accessoryId"] | "",
                sizeof(runtimeCommand.homebridgeAccessoryId));
        strlcpy(runtimeCommand.homebridgeCharacteristic,
                homebridge["characteristicType"] | "",
                sizeof(runtimeCommand.homebridgeCharacteristic));
        const char *operation = homebridge["operation"] | "set";
        runtimeCommand.homebridgeOperation = strcmp(operation, "toggle") == 0 ? 1 :
          (strcmp(operation, "relative") == 0 ? 2 : 0);
        runtimeCommand.homebridgeStep = homebridge["step"] | 0.0f;
        runtimeCommand.homebridgeMin = homebridge["min"] | 0.0f;
        runtimeCommand.homebridgeMax = homebridge["max"] | 100.0f;
        JsonVariantConst value = homebridge["value"];
        if (value.is<bool>()) {
          runtimeCommand.homebridgeValueType = 1;
          runtimeCommand.homebridgeValue = value.as<bool>() ? 1.0f : 0.0f;
        } else if (value.is<const char *>()) {
          runtimeCommand.homebridgeValueType = 3;
          strlcpy(runtimeCommand.homebridgeStringValue, value.as<const char *>(),
                  sizeof(runtimeCommand.homebridgeStringValue));
        } else {
          runtimeCommand.homebridgeValueType = 2;
          runtimeCommand.homebridgeValue = value | 0.0f;
        }
        if (runtimeCommand.homebridgeAccessoryId[0] &&
            runtimeCommand.homebridgeCharacteristic[0]) {
          runtimeCommand.kind = DeviceCommand::HOMEBRIDGE;
        }
      } else if (runtimeCommand.kind == DeviceCommand::NONE) {
        String deviceTransport = device.transport;
        deviceTransport.toLowerCase();
        if (deviceTransport.indexOf("ble") >= 0 ||
            deviceTransport.indexOf("bluetooth") >= 0) {
          configureBluetoothCommand(runtimeCommand, label);
        }
      }
      if (androidTvProfile) {
        const BluetoothPresetCommand *preset =
          findChromecastPresetCommand(runtimeCommand.id, label);
        if (preset) {
          configureBluetoothCommand(runtimeCommand, preset->name,
                                    preset->report, preset->usage);
        }
      }
      device.commandCount++;
    }
  }

  for (JsonObjectConst source : doc["macros"].as<JsonArrayConst>()) {
    if (MACRO_COUNT >= MAX_RUNTIME_MACROS) break;
    Macro &macro = macros[MACRO_COUNT++];
    strlcpy(macro.id, source["id"] | "", sizeof(macro.id));
    strlcpy(macro.name, source["name"] | "Unnamed macro", sizeof(macro.name));
    for (JsonObjectConst step : source["steps"].as<JsonArrayConst>()) {
      appendRuntimeSequenceStep(macro.steps, macro.stepCount, nullptr, step, doc);
      if (macro.stepCount >= MAX_ACTIVITY_STEPS) break;
    }
  }

  uint16_t accents[] = {0xFD20, 0x07FF, 0x07E0, 0xF81F, 0xAFE5, 0xFFE0};
  for (JsonObjectConst source : doc["activities"].as<JsonArrayConst>()) {
    if (ACTIVITY_COUNT >= MAX_RUNTIME_ACTIVITIES) break;
    uint8_t activityIndex = ACTIVITY_COUNT++;
    Activity &activity = activities[activityIndex];
    strlcpy(activity.id, source["id"] | "", sizeof(activity.id));
    strlcpy(activity.name, source["name"] | "Unnamed activity", sizeof(activity.name));
    const char *activityIconSource = source["iconSrc"] | "";
    activity.iconPath = duplicateRuntimeString(activityIconSource[0] == '/'
      ? String("S:") + activityIconSource : String(activityIconSource));
    strlcpy(activity.themePath, source["pageThemePath"] | "", sizeof(activity.themePath));
    activity.accent = accents[activityIndex % (sizeof(accents) / sizeof(accents[0]))];
    const char *activityBoxMode = source["boxMode"] | "global";
    activity.boxMode = strcmp(activityBoxMode, "on") == 0 ? 1 :
      (strcmp(activityBoxMode, "off") == 0 ? 2 : 0);
    JsonArrayConst items = source["pageItems"].as<JsonArrayConst>();
    if (items.isNull()) items = source["items"].as<JsonArrayConst>();
    uint8_t slotCursor = 0;
    for (JsonObjectConst item : items) {
      if (strcmp(item["type"] | "", "spacer") == 0) {
        if (slotCursor < 254) slotCursor++;
        continue;
      }
      if (activityTileCounts[activityIndex] >= MAX_ACTIVITY_TILES) break;
      Tile &tile = activityTiles[activityIndex][activityTileCounts[activityIndex]++];
      const char *itemType = item["type"] | "button";
      tile.kind = strcmp(itemType, "activity") == 0 ? Tile::ACTIVITY :
        (strcmp(itemType, "macro") == 0 ? Tile::MACRO : Tile::COMMAND);
      tile.slot = constrain((int)(item["slot"] | slotCursor), 0, 254);
      int nextSlot = (int)tile.slot + (tile.kind == Tile::ACTIVITY ? 3 : 1);
      slotCursor = (uint8_t)constrain(max((int)slotCursor, nextSlot), 0, 254);
      const char *label = item["name"] | "";
      if (!label[0]) label = item["label"] |
        (tile.kind == Tile::ACTIVITY ? "Activity" :
         (tile.kind == Tile::MACRO ? "Macro" : "Button"));
      strlcpy(tile.label, label, sizeof(tile.label));
      if (tile.kind == Tile::ACTIVITY) {
        strlcpy(tile.targetActivityId, item["refId"] | "",
                sizeof(tile.targetActivityId));
      } else if (tile.kind == Tile::MACRO) {
        strlcpy(tile.targetMacroId, item["refId"] | "",
                sizeof(tile.targetMacroId));
      }
      const char *iconSource = item["iconSrc"] | "";
      tile.iconPath = duplicateRuntimeString(iconSource[0] == '/'
        ? String("S:") + iconSource : String(iconSource));
      tile.showText = item["showText"] | true;
      const char *boxMode = item["boxMode"] | "global";
      tile.boxMode = strcmp(boxMode, "on") == 0 ? 1 : (strcmp(boxMode, "off") == 0 ? 2 : 0);
      tile.repeat = item["repeat"] | false;
      if (tile.kind == Tile::ACTIVITY || tile.kind == Tile::MACRO) {
        tile.deviceIndex = 0xFF;
        tile.commandIndex = 0xFF;
        continue;
      }
      Device *device = findRuntimeDevice(item["deviceId"] | "");
      DeviceCommand *command = findRuntimeCommand(device, item["commandId"] | "");
      if (device && command) {
        tile.deviceIndex = (uint8_t)(device - devices);
        tile.commandIndex = (uint8_t)(command - device->commands);
        activity.deviceMask |= (uint16_t)(1U << tile.deviceIndex);
        if (!item.containsKey("repeat")) tile.repeat = command->repeatDefault;
      } else {
        tile.deviceIndex = 0xFF;
        tile.commandIndex = 0xFF;
      }
    }

    JsonObjectConst bindings = source["physicalBindings"].as<JsonObjectConst>();
    for (JsonPairConst pair : bindings) {
      int8_t buttonIndex = physicalButtonIndex(pair.key().c_str());
      if (buttonIndex < 0) continue;
      JsonObjectConst binding = pair.value().as<JsonObjectConst>();
      if (strcmp(binding["type"] | "", "command") != 0) continue;
      Device *device = findRuntimeDevice(binding["deviceId"] | "");
      DeviceCommand *command = findRuntimeCommand(device, binding["commandId"] | "");
      if (!device || !command) continue;
      PhysicalBinding &runtimeBinding = activity.physicalBindings[buttonIndex];
      runtimeBinding.deviceIndex = (uint8_t)(device - devices);
      runtimeBinding.commandIndex = (uint8_t)(command - device->commands);
      runtimeBinding.repeat = binding["repeat"] | command->repeatDefault;
      activity.deviceMask |= (uint16_t)(1U << runtimeBinding.deviceIndex);
    }

    for (JsonObjectConst step : source["steps"].as<JsonArrayConst>()) {
      appendRuntimeSequenceStep(activity.steps, activity.stepCount,
                                &activity.deviceMask, step, doc);
      if (activity.stepCount >= MAX_ACTIVITY_STEPS) break;
    }
  }

  for (JsonObjectConst page : doc["pages"].as<JsonArrayConst>()) {
    if (strcmp(page["pageType"] | "", "activities") == 0) {
      strlcpy(activitiesThemePath, page["themePath"] | "", sizeof(activitiesThemePath));
      for (JsonObjectConst item : page["items"].as<JsonArrayConst>()) {
        if (strcmp(item["type"] | "", "activity") != 0) continue;
        const char *activityId = item["refId"] | "";
        if (!activityId[0]) continue;
        for (uint8_t i = 0; i < ACTIVITY_COUNT; i++) {
          if (strcmp(activities[i].id, activityId) != 0) continue;
          const char *boxMode = item["boxMode"] | "global";
          activities[i].boxMode = strcmp(boxMode, "on") == 0 ? 1 :
            (strcmp(boxMode, "off") == 0 ? 2 : 0);
          break;
        }
      }
      break;
    }
  }

  for (JsonObjectConst page : doc["devicePages"].as<JsonArrayConst>()) {
    Device *device = findRuntimeDevice(page["deviceId"] | "");
    if (!device) continue;
    uint8_t deviceIndex = (uint8_t)(device - devices);
    smartBindingsApplied[deviceIndex] =
      !page.containsKey("smartBindingsApplied") || (page["smartBindingsApplied"] | false);
    const char *displayName = page["name"] | "";
    if (displayName[0]) strlcpy(device->name, displayName, sizeof(device->name));
    strlcpy(device->themePath, page["themePath"] | "", sizeof(device->themePath));
    JsonObjectConst powerTracking = page["powerTracking"].as<JsonObjectConst>();
    if (!powerTracking.isNull()) {
      device->powerTrackingConfigured = true;
      device->powerTrackingEnabled = powerTracking["enabled"] | false;
    }
    if (device->powerTrackingEnabled) {
      DeviceCommand *powerOn = findRuntimeCommand(device, powerTracking["onCommandId"] | "");
      DeviceCommand *powerOff = findRuntimeCommand(device, powerTracking["offCommandId"] | "");
      if (powerOn) device->powerOnCommandIndex = (int8_t)(powerOn - device->commands);
      if (powerOff) device->powerOffCommandIndex = (int8_t)(powerOff - device->commands);
      device->powerTrackingEnabled = powerOn || powerOff;
    }
    uint8_t slotCursor = 0;
    for (JsonObjectConst item : page["items"].as<JsonArrayConst>()) {
      if (strcmp(item["type"] | "", "spacer") == 0) {
        if (slotCursor < 254) slotCursor++;
        continue;
      }
      DeviceCommand *command = findRuntimeCommand(device, item["commandId"] | "");
      uint8_t itemSlot = constrain((int)(item["slot"] | slotCursor), 0, 254);
      if (slotCursor < 254) slotCursor++;
      if (!command) continue;
      command->slot = itemSlot;
      const char *iconSource = item["iconSrc"] | "";
      if (command->iconPath) free(command->iconPath);
      command->iconPath = duplicateRuntimeString(iconSource[0] == '/'
        ? String("S:") + iconSource : String(iconSource));
      command->showText = item["showText"] | true;
      const char *boxMode = item["boxMode"] | "global";
      command->boxMode = strcmp(boxMode, "on") == 0 ? 1 : (strcmp(boxMode, "off") == 0 ? 2 : 0);
      if (item.containsKey("repeat")) command->repeatDefault = item["repeat"];
    }

    JsonObjectConst bindings = page["physicalBindings"].as<JsonObjectConst>();
    for (JsonPairConst pair : bindings) {
      int8_t buttonIndex = physicalButtonIndex(pair.key().c_str());
      if (buttonIndex < 0) continue;
      JsonObjectConst binding = pair.value().as<JsonObjectConst>();
      if (strcmp(binding["type"] | "", "command") != 0) continue;
      Device *bindingDevice = findRuntimeDevice(binding["deviceId"] | "");
      DeviceCommand *command = findRuntimeCommand(bindingDevice, binding["commandId"] | "");
      if (!bindingDevice || !command) continue;
      PhysicalBinding &runtimeBinding = device->physicalBindings[buttonIndex];
      runtimeBinding.deviceIndex = (uint8_t)(bindingDevice - devices);
      runtimeBinding.commandIndex = (uint8_t)(command - bindingDevice->commands);
      runtimeBinding.repeat = binding["repeat"] | command->repeatDefault;
    }
  }

  for (uint8_t deviceIndex = 0; deviceIndex < DEVICE_COUNT; deviceIndex++) {
    if (!smartBindingsApplied[deviceIndex]) applySmartRuntimeBindings(devices[deviceIndex]);
    if (!devices[deviceIndex].powerTrackingConfigured) {
      devices[deviceIndex].powerOnCommandIndex = inferPowerOnCommandIndex(devices[deviceIndex]);
      devices[deviceIndex].powerOffCommandIndex = inferPowerOffCommandIndex(devices[deviceIndex]);
      devices[deviceIndex].powerTrackingEnabled =
        devices[deviceIndex].powerOnCommandIndex >= 0 ||
        devices[deviceIndex].powerOffCommandIndex >= 0;
    }
  }

  applySettingsJson(doc["settings"]);
  applyRuntimeThemeRowCalibration();
  rebuildPages();
  requestPageStripRebuild();
  Serial.printf("Runtime model: %u devices, %u activities\n", DEVICE_COUNT, ACTIVITY_COUNT);
}

bool loadRuntimeConfig() {
  if (!sdReady || !SD.exists(RUNTIME_CONFIG_PATH)) return false;
  File file = SD.open(RUNTIME_CONFIG_PATH, FILE_READ);
  if (!file) return false;
  JsonDocument doc(&psramJsonAllocator);
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  if (error) {
    Serial.printf("Runtime config parse failed: %s\n", error.c_str());
    return false;
  }
  loadRuntimeModel(doc);
  return true;
}

bool saveRuntimeConfigDocument(JsonDocument &doc, String &error) {
  if (!sdReady) {
    error = "SD card unavailable";
    return false;
  }
  SD.remove(RUNTIME_CONFIG_PATH);
  File file = SD.open(RUNTIME_CONFIG_PATH, FILE_WRITE);
  if (!file) {
    error = "Could not open runtime config for writing";
    return false;
  }
  size_t written = serializeJson(doc, file);
  file.close();
  if (!written) {
    error = "Could not write runtime config";
    return false;
  }
  return true;
}

void scheduleRuntimeSettingsSave() {
  runtimeSettingsSavePending = true;
  runtimeSettingsSaveAtMs = millis() + 600UL;
}

bool persistSettingsToRuntimeConfig() {
  if (!sdReady || !SD.exists(RUNTIME_CONFIG_PATH)) return false;
  File file = SD.open(RUNTIME_CONFIG_PATH, FILE_READ);
  if (!file) return false;
  JsonDocument doc(&psramJsonAllocator);
  DeserializationError parseError = deserializeJson(doc, file);
  file.close();
  if (parseError) {
    Serial.printf("Settings runtime parse failed: %s\n", parseError.c_str());
    return false;
  }

  JsonObject settings = doc["settings"].as<JsonObject>();
  if (settings.isNull()) settings = doc["settings"].to<JsonObject>();
  settings["wifiEnabled"] = wifiOn;
  settings["bluetoothEnabled"] = bluetoothOn;
  settings["clockEnabled"] = clockEnabled;
  settings["clockUseInternetTime"] = clockUseInternetTime;
  settings["clockCity"] = clockCityName;
  settings["city"] = clockCityName;
  settings["clockUtcOffsetMinutes"] = clockUtcOffsetMinutes;
  settings["manualClockEpoch"] = manualClockEpoch;
  settings["brightness"] = brightness;
  settings["sleepSeconds"] = timeoutSeconds;
  settings["deepSleepMinutes"] = deepSleepMinutes;
  settings["wakeSensitivity"] = wakeSensitivity;
  settings["slideToUnlock"] = slideToUnlock;
  settings["displayGamma"] = displayGamma;
  settings["displaySaturation"] = displaySaturation;
  settings["displayRgb666"] = displayRgb666;
  settings["displayInverted"] = displayInverted;
  settings["physicalRepeatEnabled"] = physicalRepeatEnabled;
  settings["physicalRepeatDelayMs"] = physicalRepeatDelayMs;
  settings["physicalRepeatRateHz"] = physicalRepeatRateHz;
  settings["debugSplit"] = debugSplitEnabled;
  settings["debugTouch"] = debugTouchEnabled;
  settings["debugCpuRam"] = debugCpuRamEnabled;
  settings["debugAccelerometer"] = debugAccelerometerEnabled;
  settings["debugFps"] = debugFpsEnabled;
  settings["microphoneTestAudio"] = microphoneTestAudioEnabled;
  JsonArray debugRows = settings["debugRowPixels"].to<JsonArray>();
  for (uint8_t i = 0; i < 5; i++) debugRows.add(debugRowPixels[i]);
  settings["buttonIconSize"] = buttonIconSize;
  settings["buttonTextSize"] = buttonTextSize;
  settings["activityIconSize"] = activityIconSize;
  settings["activityTextSize"] = activityTextSize;
  settings["buttonBoxesEnabled"] = buttonBoxesEnabled;
  settings["activityBoxesEnabled"] = activityBoxesEnabled;
  settings["remoteName"] = remoteName;

  String error;
  bool saved = saveRuntimeConfigDocument(doc, error);
  Serial.printf("LCD settings runtime save: %s%s%s\n", saved ? "ok" : "failed",
                error.length() ? " - " : "", error.c_str());
  return saved;
}

bool ensureBluetoothRuntimeDevice() {
  if (!sdReady || !SD.exists(RUNTIME_CONFIG_PATH)) return false;
  File file = SD.open(RUNTIME_CONFIG_PATH, FILE_READ);
  if (!file) return false;
  JsonDocument doc(&psramJsonAllocator);
  DeserializationError parseError = deserializeJson(doc, file);
  file.close();
  if (parseError) return false;

  bool changed = false;
  JsonArray deviceList = doc["devices"].as<JsonArray>();
  if (deviceList.isNull()) deviceList = doc["devices"].to<JsonArray>();
  JsonObject bluetoothDevice;
  for (JsonObject candidate : deviceList) {
    String transport = candidate["transport"] | "";
    String protocol = candidate["protocol"] | "";
    transport.toLowerCase();
    protocol.toLowerCase();
    if (strcmp(candidate["id"] | "", "ble_chromecast") == 0 ||
        transport == "bluetooth" || protocol.indexOf("ble hid") >= 0) {
      bluetoothDevice = candidate;
      break;
    }
  }
  if (bluetoothDevice.isNull()) {
    bluetoothDevice = deviceList.add<JsonObject>();
    bluetoothDevice["id"] = "ble_chromecast";
    bluetoothDevice["name"] = "Chromecast";
    bluetoothDevice["source"] = "Bluetooth";
    bluetoothDevice["transport"] = "bluetooth";
    bluetoothDevice["type"] = "Streamer";
    bluetoothDevice["protocol"] = "BLE HID";
    bluetoothDevice["bluetoothProfile"] = "google-tv";
    JsonObject powerTracking = bluetoothDevice["powerTracking"].to<JsonObject>();
    powerTracking["enabled"] = false;
    powerTracking["onCommandId"] = "";
    powerTracking["offCommandId"] = "";
    changed = true;
  }
  if (strcmp(bluetoothDevice["bluetoothProfile"] | "", "google-tv") != 0) {
    bluetoothDevice["bluetoothProfile"] = "google-tv";
    changed = true;
  }

  const char *deviceId = bluetoothDevice["id"] | "ble_chromecast";
  JsonArray commandList = bluetoothDevice["commands"].as<JsonArray>();
  if (commandList.isNull()) commandList = bluetoothDevice["commands"].to<JsonArray>();
  String commandIds[CHROMECAST_COMMAND_COUNT];
  for (uint8_t i = 0; i < CHROMECAST_COMMAND_COUNT; i++) {
    const BluetoothPresetCommand &definition = CHROMECAST_COMMANDS[i];
    JsonObject command;
    for (JsonObject candidate : commandList) {
      if (strcmp(candidate["id"] | "", definition.id) == 0 ||
          String((const char *)(candidate["name"] | "")).equalsIgnoreCase(definition.name)) {
        command = candidate;
        break;
      }
    }
    if (command.isNull()) {
      command = commandList.add<JsonObject>();
      command["id"] = definition.id;
      command["name"] = definition.name;
      command["repeat"] = false;
      changed = true;
    }
    JsonObject hid = command["hid"].as<JsonObject>();
    if (hid.isNull()) hid = command["hid"].to<JsonObject>();
    const char *savedReport = hid["report"] | "";
    uint16_t savedUsage = (uint16_t)(hid["usage"] | 0);
    if (strcmp(savedReport, definition.report) != 0 ||
        savedUsage != definition.usage) {
      hid["report"] = definition.report;
      hid["usage"] = definition.usage;
      changed = true;
    }
    commandIds[i] = command["id"] | definition.id;
  }

  JsonArray pageList = doc["devicePages"].as<JsonArray>();
  if (pageList.isNull()) pageList = doc["devicePages"].to<JsonArray>();
  JsonObject devicePage;
  for (JsonObject candidate : pageList) {
    if (strcmp(candidate["deviceId"] | "", deviceId) == 0) {
      devicePage = candidate;
      break;
    }
  }
  if (devicePage.isNull()) {
    devicePage = pageList.add<JsonObject>();
    devicePage["deviceId"] = deviceId;
    devicePage["name"] = bluetoothDevice["name"] | "Chromecast";
    devicePage["pageTheme"] = "simple";
    devicePage["themePath"] = "";
    changed = true;
  }
  if (!(devicePage["smartBindingsApplied"] | false)) {
    devicePage["smartBindingsApplied"] = true;
    changed = true;
  }

  JsonArray itemList = devicePage["items"].as<JsonArray>();
  if (itemList.isNull()) itemList = devicePage["items"].to<JsonArray>();
  for (uint8_t i = 0; i < CHROMECAST_COMMAND_COUNT; i++) {
    bool found = false;
    for (JsonObjectConst item : itemList) {
      if (strcmp(item["commandId"] | "", commandIds[i].c_str()) == 0) {
        found = true;
        break;
      }
    }
    if (found) continue;
    const BluetoothPresetCommand &definition = CHROMECAST_COMMANDS[i];
    JsonObject item = itemList.add<JsonObject>();
    item["id"] = String("device_item_") + deviceId + "_" + commandIds[i];
    item["type"] = "button";
    item["name"] = definition.name;
    item["deviceId"] = deviceId;
    item["commandId"] = commandIds[i];
    item["refId"] = String(deviceId) + "::" + commandIds[i];
    item["iconSrc"] = definition.iconPath;
    item["iconName"] = definition.name;
    item["showText"] = true;
    item["repeat"] = false;
    item["boxMode"] = "global";
    item["slot"] = i;
    changed = true;
  }

  struct BluetoothBindingDefault {
    const char *button;
    uint8_t commandIndex;
    bool repeat;
  };
  static const BluetoothBindingDefault bindingDefaults[] = {
    {"Menu", 0, false}, {"Back", 1, false},
    {"D-pad Up", 2, true}, {"D-pad Down", 3, true},
    {"D-pad Left", 4, true}, {"D-pad Right", 5, true},
    {"OK", 6, false}, {"Play", 7, false}, {"Stop", 8, false},
    {"Rewind", 9, true}, {"Forward", 10, true},
    {"Volume Up", 13, true}, {"Volume Down", 14, true}, {"Mute", 15, false}
  };
  JsonObject bindings = devicePage["physicalBindings"].as<JsonObject>();
  if (bindings.isNull()) bindings = devicePage["physicalBindings"].to<JsonObject>();
  for (const BluetoothBindingDefault &mapping : bindingDefaults) {
    if (bindings.containsKey(mapping.button)) continue;
    const BluetoothPresetCommand &definition = CHROMECAST_COMMANDS[mapping.commandIndex];
    JsonObject binding = bindings[mapping.button].to<JsonObject>();
    binding["type"] = "command";
    binding["deviceId"] = deviceId;
    binding["commandId"] = commandIds[mapping.commandIndex];
    binding["label"] = String((const char *)(bluetoothDevice["name"] | "Chromecast")) +
      " - " + definition.name;
    binding["repeat"] = mapping.repeat;
    changed = true;
  }

  if (!changed) return true;
  String error;
  if (!saveRuntimeConfigDocument(doc, error)) {
    Serial.printf("BLE HID runtime save failed: %s\n", error.c_str());
    return false;
  }

  // BLE and the visible LVGL page can hold pointers into the runtime arrays.
  // Rebuilding the complete model here invalidates those pointers and caused
  // an immediate watchdog/reset after pairing. Patch matching commands in
  // place; the saved JSON receives the same mappings for future boots.
  Device *runtimeDevice = findRuntimeDevice(deviceId);
  if (runtimeDevice) {
    for (uint8_t i = 0; i < CHROMECAST_COMMAND_COUNT; i++) {
      DeviceCommand *runtimeCommand =
        findRuntimeCommand(runtimeDevice, commandIds[i].c_str());
      if (!runtimeCommand) continue;
      const BluetoothPresetCommand &definition = CHROMECAST_COMMANDS[i];
      configureBluetoothCommand(*runtimeCommand, definition.name,
                                definition.report, definition.usage);
    }
  }
  Serial.println("BLE HID: Chromecast commands migrated without runtime reload");
  return true;
}

void usbImportReply(Print &port, const String &body) {
  port.print("ORUSB ");
  port.println(body);
}

String sanitizeUsbFileName(const String &input) {
  String output;
  output.reserve(64);
  for (size_t i = 0; i < input.length() && output.length() < 56; i++) {
    char c = input[i];
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') || c == '-' || c == '_') {
      output += c;
    } else if (c == '.' && output.length() > 0) {
      output += c;
    } else if (output.length() && output[output.length() - 1] != '_') {
      output += '_';
    }
  }
  output.trim();
  while (output.endsWith("_")) output.remove(output.length() - 1);
  if (!output.length()) output = "ir_device";
  if (!output.endsWith(".ir") && !output.endsWith(".IR")) output += ".ir";
  return output;
}

uint16_t countSavedIrDeviceFiles() {
  if (!sdReady) return 0;
  uint16_t count = 0;
  if (SD.exists(DEVICE_INDEX_PATH)) {
    File index = SD.open(DEVICE_INDEX_PATH, FILE_READ);
    while (index && index.available()) {
      String path = index.readStringUntil('\n');
      path.trim();
      if (path.endsWith(".ir") && SD.exists(path)) count++;
    }
    if (index) index.close();
    if (count) return count;
  }

  File root = SD.open("/devices");
  if (!root || !root.isDirectory()) {
    if (root) root.close();
    return 0;
  }
  File entry = root.openNextFile();
  while (entry) {
    String name = entry.name();
    name.toLowerCase();
    if (!entry.isDirectory() && name.endsWith(".ir")) count++;
    entry.close();
    entry = root.openNextFile();
  }
  root.close();
  return count;
}

void rememberSavedIrDeviceFile(const String &path) {
  if (!sdReady || !path.endsWith(".ir")) return;
  bool found = false;
  String existing;
  if (SD.exists(DEVICE_INDEX_PATH)) {
    File index = SD.open(DEVICE_INDEX_PATH, FILE_READ);
    while (index && index.available()) {
      String line = index.readStringUntil('\n');
      line.trim();
      if (!line.length()) continue;
      if (line == path) found = true;
      existing += line + "\n";
    }
    if (index) index.close();
  }
  if (!found) existing += path + "\n";
  SD.remove(DEVICE_INDEX_PATH);
  File index = SD.open(DEVICE_INDEX_PATH, FILE_WRITE);
  if (index) {
    index.print(existing);
    index.close();
  }
}

void forgetSavedIrDeviceFile(const String &path) {
  if (!sdReady || !SD.exists(DEVICE_INDEX_PATH)) return;
  String remaining;
  File index = SD.open(DEVICE_INDEX_PATH, FILE_READ);
  while (index && index.available()) {
    String line = index.readStringUntil('\n');
    line.trim();
    if (line.length() && line != path) remaining += line + "\n";
  }
  if (index) index.close();
  SD.remove(DEVICE_INDEX_PATH);
  File replacement = SD.open(DEVICE_INDEX_PATH, FILE_WRITE);
  if (replacement) {
    replacement.print(remaining);
    replacement.close();
  }
}

void clearUsbUpload(UsbSerialSession &session, bool removePartial) {
  if (session.uploadFile) session.uploadFile.close();
  if (removePartial && session.uploadTempPath.length()) SD.remove(session.uploadTempPath);
  session.uploadTempPath = "";
  session.uploadFinalPath = "";
  session.uploadExpected = 0;
  session.uploadReceived = 0;
  session.uploadNextAck = 0;
  session.uploadLastDataMs = 0;
  session.uploadIsIrFile = false;
  session.uploadIsRuntimeConfig = false;
}

void failUsbUpload(Stream &port, UsbSerialSession &session, const String &error) {
  clearUsbUpload(session, true);
  usbImportReply(port, String("{\"ok\":false,\"error\":\"") + error + "\"}");
}

bool beginUsbFileUpload(Stream &port, UsbSerialSession &session,
                        const String &finalPath, size_t length,
                        bool isIrFile, bool isRuntimeConfig) {
  if (!sdReady) {
    usbImportReply(port, "{\"ok\":false,\"error\":\"SD card unavailable\"}");
    return false;
  }
  if (length == 0 || length > USB_IMPORT_MAX_BYTES) {
    usbImportReply(port, "{\"ok\":false,\"error\":\"USB payload is empty or too large\"}");
    return false;
  }

  clearUsbUpload(session, true);
  session.uploadFinalPath = finalPath;
  session.uploadTempPath = session.uploadFinalPath + ".part";
  SD.remove(session.uploadTempPath);
  session.uploadFile = SD.open(session.uploadTempPath, FILE_WRITE);
  if (!session.uploadFile) {
    clearUsbUpload(session, false);
    usbImportReply(port, "{\"ok\":false,\"error\":\"Could not open temporary SD file\"}");
    return false;
  }
  session.uploadExpected = length;
  session.uploadReceived = 0;
  session.uploadNextAck = min(length, USB_UPLOAD_WINDOW_BYTES);
  session.uploadLastDataMs = millis();
  session.uploadIsIrFile = isIrFile;
  session.uploadIsRuntimeConfig = isRuntimeConfig;
  usbImportReply(port, String("READY ") + String(USB_UPLOAD_WINDOW_BYTES));
  return true;
}

bool beginUsbIrUpload(Stream &port, UsbSerialSession &session,
                      const String &fileName, size_t length) {
  return beginUsbFileUpload(port, session,
    String("/devices/") + sanitizeUsbFileName(fileName), length, true, false);
}

bool uploadedIrFileLooksValid(const String &path) {
  File file = SD.open(path, FILE_READ);
  if (!file) return false;
  String header;
  header.reserve(96);
  while (file.available() && header.length() < 96) header += (char)file.read();
  file.close();
  return header.indexOf("Filetype: IR signals file") >= 0;
}

bool uploadedRuntimeConfigLooksValid(const String &path) {
  File file = SD.open(path, FILE_READ);
  if (!file) return false;
  JsonDocument doc(&psramJsonAllocator);
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  return !error && doc.is<JsonObject>();
}

bool uploadedWebConfigLooksValid(const String &path) {
  File file = SD.open(path, FILE_READ);
  if (!file) return false;
  String header;
  header.reserve(768);
  while (file.available() && header.length() < 768) header += (char)file.read();
  file.close();
  header.toLowerCase();
  return header.indexOf("<html") >= 0 &&
    header.indexOf("openremote-webconfig-version") >= 0;
}

bool uploadedFirmwareLooksValid(const String &path) {
  File file = SD.open(path, FILE_READ);
  if (!file) return false;
  int magic = file.read();
  file.close();
  return magic == 0xE9;
}

bool usbWritableSdPath(const String &path) {
  if (!path.startsWith("/") || path.indexOf("..") >= 0 ||
      path.indexOf('\\') >= 0 || path.indexOf(' ') >= 0) return false;
  if (path == RUNTIME_CONFIG_PATH || path == WEB_CONFIG_PATH ||
      path == "/config/version.json") return true;
  if (path.startsWith("/firmware/") && path.endsWith(".bin")) return true;
  if (path.startsWith("/icons/Default/") &&
      (path.endsWith(".png") || path.endsWith(".jpg") ||
       path.endsWith(".jpeg") || path.endsWith(".html"))) return true;
  return false;
}

void finishUsbFileUpload(Stream &port, UsbSerialSession &session) {
  if (session.uploadFile) session.uploadFile.close();
  String tempPath = session.uploadTempPath;
  String finalPath = session.uploadFinalPath;
  size_t received = session.uploadReceived;
  bool isIrFile = session.uploadIsIrFile;
  bool isRuntimeConfig = session.uploadIsRuntimeConfig;

  if (isIrFile && !uploadedIrFileLooksValid(tempPath)) {
    failUsbUpload(port, session, "USB payload is not a valid .ir file");
    return;
  }
  if (isRuntimeConfig && !uploadedRuntimeConfigLooksValid(tempPath)) {
    failUsbUpload(port, session, "USB payload is not valid runtime JSON");
    return;
  }
  if (finalPath == WEB_CONFIG_PATH && !uploadedWebConfigLooksValid(tempPath)) {
    failUsbUpload(port, session, "USB payload is not a valid OpenRemote WebConfig");
    return;
  }
  if (finalPath.startsWith("/firmware/") && !uploadedFirmwareLooksValid(tempPath)) {
    failUsbUpload(port, session, "USB payload is not a valid ESP32 firmware binary");
    return;
  }
  SD.remove(finalPath);
  if (!SD.rename(tempPath, finalPath)) {
    failUsbUpload(port, session, "Could not finish writing SD file");
    return;
  }

  clearUsbUpload(session, false);
  if (isIrFile) {
    rememberSavedIrDeviceFile(finalPath);
    loadRuntimeConfig();
  }
  if (isRuntimeConfig) loadRuntimeConfig();
  Serial.printf("USB file import: %s (%u byte(s))\n",
                finalPath.c_str(), (unsigned)received);
  String response = String("{\"ok\":true,\"firmwareVersion\":\"") +
    OPENREMOTE_VERSION_TEXT + "\",\"path\":\"" + finalPath + "\"";
  if (isIrFile) response += ",\"deviceFileCount\":" + String(countSavedIrDeviceFiles());
  response += "}";
  usbImportReply(port, response);
}

bool serviceUsbUpload(Stream &port, UsbSerialSession &session) {
  if (session.uploadExpected == 0) return false;
  if ((uint32_t)(millis() - session.uploadLastDataMs) > USB_UPLOAD_IDLE_TIMEOUT_MS) {
    failUsbUpload(port, session, "USB payload timed out");
    return false;
  }

  uint8_t buffer[USB_IO_CHUNK_BYTES];
  size_t budget = USB_IO_BUDGET_BYTES;
  while (budget && session.uploadReceived < session.uploadExpected && port.available()) {
    size_t wanted = min((size_t)port.available(), sizeof(buffer));
    wanted = min(wanted, session.uploadExpected - session.uploadReceived);
    wanted = min(wanted, budget);
    size_t got = 0;
    while (got < wanted) {
      int value = port.read();
      if (value < 0) break;
      buffer[got++] = (uint8_t)value;
    }
    if (!got) break;
    if (session.uploadFile.write(buffer, got) != got) {
      failUsbUpload(port, session, "SD write failed during USB upload");
      return false;
    }
    session.uploadReceived += got;
    session.uploadLastDataMs = millis();
    budget -= got;
  }

  if (session.uploadReceived == session.uploadExpected) {
    finishUsbFileUpload(port, session);
    return false;
  }
  if (session.uploadReceived >= session.uploadNextAck) {
    usbImportReply(port, String("ACK ") + String(session.uploadReceived));
    session.uploadNextAck = min(session.uploadExpected,
      session.uploadReceived + USB_UPLOAD_WINDOW_BYTES);
  }
  return true;
}

void clearUsbDownload(UsbSerialSession &session) {
  if (session.downloadFile) session.downloadFile.close();
  session.downloadTotal = 0;
  session.downloadSent = 0;
  session.downloadLastProgressMs = 0;
}

void beginUsbSdFileDownload(Stream &port, UsbSerialSession &session, const String &path) {
  clearUsbDownload(session);
  bool allowedPath = path == WEB_CONFIG_PATH || path == RUNTIME_CONFIG_PATH;
  if (!sdReady || !allowedPath || !SD.exists(path)) {
    usbImportReply(port, "{\"ok\":false,\"error\":\"Requested SD file is unavailable\"}");
    return;
  }
  session.downloadFile = SD.open(path, FILE_READ);
  if (!session.downloadFile) {
    usbImportReply(port, "{\"ok\":false,\"error\":\"Could not open SD file\"}");
    return;
  }
  session.downloadTotal = session.downloadFile.size();
  session.downloadSent = 0;
  session.downloadLastProgressMs = millis();
  port.print("ORUSB FILE ");
  port.print((uint32_t)session.downloadTotal);
  port.println(" CHUNKED");
}

void serviceUsbDownload(Stream &port, UsbSerialSession &session) {
  if (!session.downloadFile) return;
  if (session.downloadSent >= session.downloadTotal) {
    clearUsbDownload(session);
    port.println("ORUSB DONE");
    return;
  }
  int writable = port.availableForWrite();
  if (writable <= 0) {
    if ((uint32_t)(millis() - session.downloadLastProgressMs) > USB_UPLOAD_IDLE_TIMEOUT_MS) {
      clearUsbDownload(session);
    }
    return;
  }

  uint8_t buffer[USB_IO_CHUNK_BYTES];
  size_t wanted = min(sizeof(buffer), session.downloadTotal - session.downloadSent);
  size_t got = session.downloadFile.read(buffer, wanted);
  if (!got) {
    clearUsbDownload(session);
    usbImportReply(port, "{\"ok\":false,\"error\":\"SD read failed during USB download\"}");
    return;
  }
  port.print("ORUSB DATA ");
  port.print((uint32_t)session.downloadSent);
  port.print(' ');
  port.println((uint16_t)got);
  port.write(buffer, got);
  port.write('\n');
  session.downloadSent += got;
  session.downloadLastProgressMs = millis();
  if (session.downloadSent >= session.downloadTotal) {
    clearUsbDownload(session);
    port.println("ORUSB DONE");
  }
}

void handleUsbCommand(Stream &port, UsbSerialSession &session, String command) {
  command.trim();
  if (command == "ORUSB PING") {
    clearUsbDownload(session);
    usbImportReply(port, String("{\"ok\":true,\"firmwareVersion\":\"") +
      OPENREMOTE_VERSION_TEXT + "\",\"deviceFileCount\":" +
      String(countSavedIrDeviceFiles()) + "}");
  } else if (command == "ORUSB STATUS") {
    usbImportReply(port, buildStatusJson());
  } else if (command == "ORUSB PREPARESD") {
    if (!sdReady) sdReady = initSdStorage();
    if (!sdReady) {
      usbImportReply(port, "{\"ok\":false,\"error\":\"SD card unavailable; insert a FAT32 card into the remote and retry\"}");
      return;
    }
    usbImportReply(port, String("{\"ok\":true,\"sdReady\":true,\"runtimeExists\":") +
      (SD.exists(RUNTIME_CONFIG_PATH) ? "true" : "false") +
      ",\"cardMb\":" + String((uint32_t)(SD.cardSize() / (1024ULL * 1024ULL))) + "}");
  } else if (command == "ORUSB NEXT") {
    serviceUsbDownload(port, session);
  } else if (command == "ORUSB CANCEL") {
    clearUsbDownload(session);
    clearUsbUpload(session, true);
    usbImportReply(port, "{\"ok\":true,\"cancelled\":true}");
  } else if (command.startsWith("ORUSB READ ")) {
    beginUsbSdFileDownload(port, session, command.substring(11));
  } else if (command.startsWith("ORUSB WRITE ")) {
    int pathEnd = command.indexOf(' ', 12);
    if (pathEnd < 0) {
      usbImportReply(port, "{\"ok\":false,\"error\":\"Missing USB file path or length\"}");
      return;
    }
    String path = command.substring(12, pathEnd);
    size_t length = (size_t)command.substring(pathEnd + 1).toInt();
    if (!usbWritableSdPath(path)) {
      usbImportReply(port, "{\"ok\":false,\"error\":\"USB writes are not allowed for this path\"}");
      return;
    }
    clearUsbDownload(session);
    beginUsbFileUpload(port, session, path, length, false,
                       path == RUNTIME_CONFIG_PATH);
  } else if (command.startsWith("ORUSB IRFILE ")) {
    int nameEnd = command.indexOf(' ', 14);
    if (nameEnd < 0) {
      usbImportReply(port, "{\"ok\":false,\"error\":\"Missing .ir filename or length\"}");
      return;
    }
    String fileName = command.substring(14, nameEnd);
    size_t length = (size_t)command.substring(nameEnd + 1).toInt();
    clearUsbDownload(session);
    beginUsbIrUpload(port, session, fileName, length);
  }
}

void serviceUsbSerialImport(Stream &port, UsbSerialSession &session) {
  if (serviceUsbUpload(port, session)) return;

  size_t budget = USB_IO_BUDGET_BYTES;
  while (budget-- && port.available()) {
    char c = (char)port.read();
    if (c == '\r') continue;
    if (c != '\n') {
      if (session.commandLine.length() < 180) session.commandLine += c;
      continue;
    }
    String command = session.commandLine;
    session.commandLine = "";
    handleUsbCommand(port, session, command);
    if (session.uploadExpected) break;
  }
  serviceUsbUpload(port, session);
}

bool runtimeConfigFileLooksValid(const char *path, String &errorText) {
  File file = SD.open(path, FILE_READ);
  if (!file) {
    errorText = "Could not open uploaded runtime config";
    return false;
  }
  JsonDocument doc(&psramJsonAllocator);
  DeserializationError parseError = deserializeJson(doc, file);
  file.close();
  if (parseError || !doc.is<JsonObject>()) {
    errorText = String("Invalid runtime JSON: ") + parseError.c_str();
    return false;
  }
  if (!doc["devices"].is<JsonArray>() ||
      !doc["activities"].is<JsonArray>() ||
      !doc["pages"].is<JsonArray>()) {
    errorText = "Runtime config is missing devices, activities or pages";
    return false;
  }
  return true;
}

bool commitRuntimeConfigTemp(String &errorText) {
  if (SD.exists(RUNTIME_CONFIG_BACKUP_PATH) &&
      !SD.remove(RUNTIME_CONFIG_BACKUP_PATH)) {
    errorText = "Could not replace the previous runtime rollback file";
    return false;
  }

  bool hadCurrentConfig = SD.exists(RUNTIME_CONFIG_PATH);
  if (hadCurrentConfig &&
      !SD.rename(RUNTIME_CONFIG_PATH, RUNTIME_CONFIG_BACKUP_PATH)) {
    errorText = "Could not preserve the current runtime config";
    return false;
  }
  if (!SD.rename(RUNTIME_CONFIG_UPLOAD_PATH, RUNTIME_CONFIG_PATH)) {
    if (hadCurrentConfig) {
      SD.rename(RUNTIME_CONFIG_BACKUP_PATH, RUNTIME_CONFIG_PATH);
    }
    errorText = "Could not install the uploaded runtime config";
    return false;
  }
  runtimeReloadCanRollback = hadCurrentConfig;
  return true;
}

void scheduleRuntimeReloadAfterSync() {
  pendingRuntimeReload = true;
  // Let the JSON response fully leave the TCP stack before parsing the new
  // model and rebuilding LVGL objects from SD.
  runtimeReloadAfterMs = millis() + 1200UL;
}

void handleRuntimeConfigUploadData() {
  HTTPUpload &upload = webServer.upload();
  if (upload.status == UPLOAD_FILE_START) {
    webConfigTransferCancelRequested = false;
    webConfigTransferActive = true;
    runtimeConfigUploadStarted = true;
    runtimeConfigUploadOk = requestAuthorized() && sdReady;
    runtimeConfigUploadBytes = 0;
    runtimeConfigUploadError = runtimeConfigUploadOk ? "" :
      (sdReady ? "Not authorized" : "SD card unavailable");
    if (runtimeConfigUploadFile) runtimeConfigUploadFile.close();
    SD.remove(RUNTIME_CONFIG_UPLOAD_PATH);
    if (runtimeConfigUploadOk) {
      runtimeConfigUploadFile = SD.open(RUNTIME_CONFIG_UPLOAD_PATH, FILE_WRITE);
      runtimeConfigUploadOk = (bool)runtimeConfigUploadFile;
      if (!runtimeConfigUploadOk) {
        runtimeConfigUploadError = "Could not open temporary runtime config";
      }
    }
  } else if (upload.status == UPLOAD_FILE_WRITE && runtimeConfigUploadOk) {
    if (webConfigTransferCancelRequested) {
      if (runtimeConfigUploadFile) runtimeConfigUploadFile.close();
      SD.remove(RUNTIME_CONFIG_UPLOAD_PATH);
      runtimeConfigUploadOk = false;
      runtimeConfigUploadError = "Runtime sync cancelled";
      webServer.client().stop();
      webConfigTransferActive = false;
      return;
    }
    static const size_t MAX_RUNTIME_UPLOAD_BYTES = 2UL * 1024UL * 1024UL;
    if (runtimeConfigUploadBytes + upload.currentSize > MAX_RUNTIME_UPLOAD_BYTES) {
      runtimeConfigUploadOk = false;
      runtimeConfigUploadError = "Runtime config is larger than 2 MB";
    } else if (runtimeConfigUploadFile.write(upload.buf, upload.currentSize) !=
               upload.currentSize) {
      runtimeConfigUploadOk = false;
      runtimeConfigUploadError = "SD write failed during runtime sync";
    } else {
      runtimeConfigUploadBytes += upload.currentSize;
    }
    serviceUiDuringLongHttpTransfer();
  } else if (upload.status == UPLOAD_FILE_END) {
    if (runtimeConfigUploadFile) runtimeConfigUploadFile.close();
    if (!runtimeConfigUploadBytes && runtimeConfigUploadOk) {
      runtimeConfigUploadOk = false;
      runtimeConfigUploadError = "Runtime config upload was empty";
    }
    if (runtimeConfigUploadOk) {
      runtimeConfigUploadOk = runtimeConfigFileLooksValid(
        RUNTIME_CONFIG_UPLOAD_PATH, runtimeConfigUploadError);
    }
    if (runtimeConfigUploadOk) {
      runtimeConfigUploadOk = commitRuntimeConfigTemp(runtimeConfigUploadError);
    }
    if (!runtimeConfigUploadOk) SD.remove(RUNTIME_CONFIG_UPLOAD_PATH);
    webConfigTransferActive = false;
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    if (runtimeConfigUploadFile) runtimeConfigUploadFile.close();
    SD.remove(RUNTIME_CONFIG_UPLOAD_PATH);
    runtimeConfigUploadOk = false;
    runtimeConfigUploadError = "Runtime config upload was interrupted";
    webConfigTransferActive = false;
  }
}

void handleRuntimeConfigUpload() {
  if (runtimeConfigUploadStarted) {
    bool ok = runtimeConfigUploadOk;
    String response = ok
      ? String("{\"ok\":true,\"firmwareVersion\":\"") +
          OPENREMOTE_VERSION_TEXT + "\",\"bytes\":" +
          String(runtimeConfigUploadBytes) + "}"
      : String("{\"ok\":false,\"error\":\"") +
          runtimeConfigUploadError + "\"}";
    sendJson(ok ? 200 : 400, response);
    runtimeConfigUploadStarted = false;
    if (ok) scheduleRuntimeReloadAfterSync();
    return;
  }

  if (!requestAuthorized()) {
    sendJson(403, "{\"ok\":false,\"error\":\"Not authorized\"}");
    return;
  }
  if (!sdReady) {
    sendJson(503, "{\"ok\":false,\"error\":\"SD card unavailable\"}");
    return;
  }

  JsonDocument doc(&psramJsonAllocator);
  DeserializationError error = deserializeJson(doc, webServer.arg("plain"));
  if (error) {
    sendJson(400, "{\"ok\":false,\"error\":\"Invalid JSON\"}");
    return;
  }

  JsonArray savedDevices = doc["devices"].as<JsonArray>();
  for (int index = (int)savedDevices.size() - 1; index >= 0; index--) {
    if (savedDevices[index]["fileBacked"] | false) savedDevices.remove(index);
  }
  SD.remove(RUNTIME_CONFIG_UPLOAD_PATH);
  File file = SD.open(RUNTIME_CONFIG_UPLOAD_PATH, FILE_WRITE);
  size_t expected = measureJson(doc);
  size_t written = file ? serializeJson(doc, file) : 0;
  if (file) file.close();
  String commitError;
  if (!written || written != expected ||
      !commitRuntimeConfigTemp(commitError)) {
    SD.remove(RUNTIME_CONFIG_UPLOAD_PATH);
    sendJson(500, String("{\"ok\":false,\"error\":\"") +
      (commitError.length() ? commitError : "Could not write runtime config") + "\"}");
    return;
  }
  sendJson(200, String("{\"ok\":true,\"firmwareVersion\":\"") + OPENREMOTE_VERSION_TEXT + "\"}");
  scheduleRuntimeReloadAfterSync();
}

void handleRuntimeConfigDownload() {
  if (!requestAuthorized()) {
    sendJson(403, "{\"ok\":false,\"error\":\"Not authorized\"}");
    return;
  }
  if (!sdReady) {
    sendJson(503, "{\"ok\":false,\"error\":\"SD card unavailable\"}");
    return;
  }
  JsonDocument doc(&psramJsonAllocator);
  if (SD.exists(RUNTIME_CONFIG_PATH)) {
    File file = SD.open(RUNTIME_CONFIG_PATH, FILE_READ);
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    if (error) {
      sendJson(500, "{\"ok\":false,\"error\":\"Runtime configuration is invalid\"}");
      return;
    }
  } else {
    doc["ok"] = true;
    doc["schemaVersion"] = 1;
    doc["activities"].to<JsonArray>();
    doc["macros"].to<JsonArray>();
  }
  JsonArray configDevices = doc["devices"].as<JsonArray>();
  if (configDevices.isNull()) configDevices = doc["devices"].to<JsonArray>();
  appendIrDeviceFileSummaries(configDevices);
  size_t responseSize = measureJson(doc);
  uint8_t *response = static_cast<uint8_t *>(
    psramFound() ? ps_malloc(responseSize + 1) : malloc(responseSize + 1));
  if (!response) {
    sendJson(503, "{\"ok\":false,\"error\":\"Not enough memory to send runtime configuration\"}");
    return;
  }
  size_t written = serializeJson(doc, reinterpret_cast<char *>(response),
                                 responseSize + 1);
  if (written != responseSize) {
    free(response);
    sendJson(500, "{\"ok\":false,\"error\":\"Could not serialize runtime configuration\"}");
    return;
  }

  webServer.sendHeader("Cache-Control", "no-store");
  webServer.setContentLength(written);
  webServer.send(200, "application/json", "");
  WiFiClient client = webServer.client();
  size_t sent = 0;
  unsigned long lastProgressMs = millis();
  webConfigTransferCancelRequested = false;
  webConfigTransferActive = true;
  while (!webConfigTransferCancelRequested && client.connected() && sent < written) {
    size_t chunk = min((size_t)1024, written - sent);
    size_t count = client.write(response + sent, chunk);
    if (count) {
      sent += count;
      lastProgressMs = millis();
    } else if (millis() - lastProgressMs > 2000UL) {
      break;
    }
    serviceUiDuringLongHttpTransfer();
  }
  if (webConfigTransferCancelRequested) client.stop();
  webConfigTransferActive = false;
  free(response);
  Serial.printf("Runtime API: %u/%u bytes sent\n", (unsigned)sent,
                (unsigned)written);
}

void handleIrdbDownload() {
  if (!requestAuthorized()) {
    sendJson(403, "{\"ok\":false,\"error\":\"Not authorized\"}");
    return;
  }
  if (!sdReady || !SD.exists(IRDB_PATH)) {
    sendJson(404, "{\"ok\":false,\"error\":\"Copy OpenRemote.irdb to /irdb/OpenRemote.irdb\"}");
    return;
  }
  File file = SD.open(IRDB_PATH, FILE_READ);
  if (!file) {
    sendJson(500, "{\"ok\":false,\"error\":\"Could not open OpenRemote.irdb\"}");
    return;
  }
  size_t total = file.size();
  webServer.sendHeader("Cache-Control", "no-store");
  webServer.sendHeader("Content-Disposition", "inline; filename=OpenRemote.irdb");
  webServer.setContentLength(total);
  webServer.send(200, "application/vnd.sqlite3", "");
  WiFiClient client = webServer.client();
  uint8_t buffer[IRDB_STREAM_CHUNK_BYTES];
  size_t sent = 0;
  unsigned long lastLogMs = millis();
  while (client.connected() && file.available()) {
    size_t n = file.read(buffer, sizeof(buffer));
    if (!n) break;
    size_t written = client.write(buffer, n);
    sent += written;
    if (written != n) {
      Serial.println("IRDB stream: client write short");
      break;
    }
    unsigned long now = millis();
    if (now - lastLogMs >= 2000UL) {
      lastLogMs = now;
      Serial.printf("IRDB stream: %u/%u KB\n",
                    (unsigned)(sent / 1024U), (unsigned)(total / 1024U));
    }
    delay(1);
  }
  client.flush();
  file.close();
  Serial.printf("IRDB stream: sent %u/%u bytes\n", (unsigned)sent, (unsigned)total);
}

String normalizedSearchText(String value) {
  value.trim();
  value.toLowerCase();
  return value;
}

bool lineMatchesSearchTerms(const String &line, const String terms[], uint8_t termCount) {
  if (!termCount) return false;
  String haystack = line;
  haystack.toLowerCase();
  for (uint8_t i = 0; i < termCount; i++) {
    if (terms[i].length() && haystack.indexOf(terms[i]) < 0) return false;
  }
  return true;
}

void handleIrdbSearch() {
  if (!requestAuthorized()) {
    sendJson(403, "{\"ok\":false,\"error\":\"Not authorized\"}");
    return;
  }
  if (!sdReady || !SD.exists(IRDB_SEARCH_INDEX_PATH)) {
    sendJson(404, "{\"ok\":false,\"error\":\"Copy /irdb/search.jsonl to the SD card from the latest OpenRemote package\"}");
    return;
  }

  String query = normalizedSearchText(webServer.arg("q"));
  if (query.length() < 2) {
    sendJson(200, "{\"ok\":true,\"results\":[],\"shown\":0,\"total\":0,\"message\":\"Type at least 2 characters\"}");
    return;
  }

  String terms[5];
  uint8_t termCount = 0;
  int start = 0;
  while (start < (int)query.length() && termCount < 5) {
    int space = query.indexOf(' ', start);
    if (space < 0) space = query.length();
    String term = query.substring(start, space);
    term.trim();
    if (term.length()) terms[termCount++] = term;
    start = space + 1;
  }

  File file = SD.open(IRDB_SEARCH_INDEX_PATH, FILE_READ);
  if (!file) {
    sendJson(500, "{\"ok\":false,\"error\":\"Could not open /irdb/search.jsonl\"}");
    return;
  }

  const uint8_t maxResults = 25;
  uint16_t total = 0;
  uint8_t shown = 0;
  uint32_t lines = 0;
  unsigned long lastYieldMs = millis();

  webServer.sendHeader("Cache-Control", "no-store");
  webServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
  webServer.send(200, "application/json", "");
  webServer.sendContent("{\"ok\":true,\"results\":[");

  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    lines++;
    if (line.length() && lineMatchesSearchTerms(line, terms, termCount)) {
      total++;
      if (shown < maxResults) {
        if (shown) webServer.sendContent(",");
        webServer.sendContent(line);
        shown++;
      }
    }
    unsigned long now = millis();
    if (now - lastYieldMs >= 10UL) {
      lastYieldMs = now;
      delay(1);
    }
  }
  file.close();

  webServer.sendContent("],\"shown\":");
  webServer.sendContent(String(shown));
  webServer.sendContent(",\"total\":");
  webServer.sendContent(String(total));
  webServer.sendContent(",\"limit\":");
  webServer.sendContent(String(maxResults));
  webServer.sendContent(",\"scanned\":");
  webServer.sendContent(String(lines));
  webServer.sendContent("}");
  webServer.sendContent("");
  Serial.printf("IRDB search \"%s\": %u/%u shown from %u index lines\n",
                query.c_str(), (unsigned)shown, (unsigned)total, (unsigned)lines);
}

void handleIrdbDetail() {
  if (!requestAuthorized()) {
    sendJson(403, "{\"ok\":false,\"error\":\"Not authorized\"}");
    return;
  }
  if (!sdReady || !SD.exists(IRDB_DETAIL_DIR)) {
    sendJson(404, "{\"ok\":false,\"error\":\"Copy /irdb/details to the SD card from the latest OpenRemote package\"}");
    return;
  }

  String id = webServer.arg("id");
  id.trim();
  if (!id.length()) {
    sendJson(400, "{\"ok\":false,\"error\":\"Missing device id\"}");
    return;
  }
  for (uint16_t i = 0; i < id.length(); i++) {
    char c = id[i];
    if (!isalnum((unsigned char)c) && c != '_' && c != '-') {
      sendJson(400, "{\"ok\":false,\"error\":\"Invalid device id\"}");
      return;
    }
  }
  String prefix = id.substring(0, id.length() >= 2 ? 2 : id.length());
  String path = String(IRDB_DETAIL_DIR) + "/" + prefix + "/" + id + ".json";
  File file = SD.open(path, FILE_READ);
  if (!file) {
    sendJson(404, "{\"ok\":false,\"error\":\"Device detail file not found\"}");
    return;
  }

  webServer.sendHeader("Cache-Control", "no-store");
  webServer.streamFile(file, "application/json");
  file.close();
  Serial.printf("IRDB detail \"%s\": %s\n", id.c_str(), path.c_str());
}

void deleteSdTree(const String &path) {
  File root = SD.open(path);
  if (!root) return;
  if (!root.isDirectory()) {
    root.close();
    SD.remove(path);
    return;
  }
  while (true) {
    File entry = root.openNextFile();
    if (!entry) break;
    String child = entry.path();
    bool directory = entry.isDirectory();
    entry.close();
    if (directory) deleteSdTree(child);
    else SD.remove(child);
  }
  root.close();
  if (path != "/") SD.rmdir(path);
}

void handleSdRebuild() {
  if (!requestAuthorized()) {
    sendJson(403, "{\"ok\":false,\"error\":\"Not authorized\"}");
    return;
  }
  JsonDocument request;
  deserializeJson(request, webServer.arg("plain"));
  if (String((const char *)(request["confirmation"] | "")) != "FORMAT") {
    sendJson(400, "{\"ok\":false,\"error\":\"Confirmation required\"}");
    return;
  }
  const char *erasableFolders[] = {
    "/config", "/themes", "/icons", "/devices", "/activities",
    "/macros", "/irdb", "/logs", "/tmp"
  };
  for (const char *folder : erasableFolders) deleteSdTree(folder);
  bool ok = true;
  for (uint8_t i = 0; i < SD_FOLDER_COUNT; i++) ok = createSdFolderIfMissing(sdFolders[i]) && ok;
  sdReady = ok;
  sendJson(ok ? 200 : 500, ok ? "{\"ok\":true}" : "{\"ok\":false,\"error\":\"SD rebuild failed\"}");
}

bool copySdFile(const String &sourcePath, const String &destinationPath) {
  File source = SD.open(sourcePath, FILE_READ);
  if (!source || source.isDirectory()) {
    if (source) source.close();
    return false;
  }
  SD.remove(destinationPath);
  File destination = SD.open(destinationPath, FILE_WRITE);
  if (!destination) {
    source.close();
    return false;
  }
  uint8_t buffer[1024];
  bool ok = true;
  while (source.available()) {
    size_t count = source.read(buffer, sizeof(buffer));
    if (!count || destination.write(buffer, count) != count) {
      ok = false;
      break;
    }
    delay(1);
  }
  source.close();
  destination.close();
  if (!ok) SD.remove(destinationPath);
  return ok;
}

bool copySdTree(const String &sourcePath, const String &destinationPath) {
  File source = SD.open(sourcePath);
  if (!source) return false;
  if (!source.isDirectory()) {
    source.close();
    return copySdFile(sourcePath, destinationPath);
  }
  if (!SD.exists(destinationPath) && !SD.mkdir(destinationPath)) {
    source.close();
    return false;
  }
  bool ok = true;
  while (true) {
    File entry = source.openNextFile();
    if (!entry) break;
    String entryPath = entry.path();
    String name = entryPath.substring(entryPath.lastIndexOf('/') + 1);
    bool directory = entry.isDirectory();
    entry.close();
    if (!name.length()) continue;
    String destination = destinationPath + "/" + name;
    ok = (directory ? copySdTree(entryPath, destination)
                    : copySdFile(entryPath, destination)) && ok;
  }
  source.close();
  return ok;
}

uint16_t countSdFiles(const String &path) {
  File root = SD.open(path);
  if (!root || !root.isDirectory()) {
    if (root) root.close();
    return 0;
  }
  uint16_t count = 0;
  while (true) {
    File entry = root.openNextFile();
    if (!entry) break;
    String child = entry.path();
    bool directory = entry.isDirectory();
    entry.close();
    count += directory ? countSdFiles(child) : 1;
  }
  root.close();
  return count;
}

String runtimeAssetPath(const char *source, const char *folder) {
  String value = source ? source : "";
  int start = value.indexOf(folder);
  if (start < 0) return "";
  value = value.substring(start);
  int query = value.indexOf('?');
  if (query >= 0) value.remove(query);
  return value;
}

String runtimeThemePathForId(JsonArrayConst themes, const char *themeId) {
  if (!themeId || !themeId[0]) return "";
  for (JsonObjectConst theme : themes) {
    if (strcmp(theme["id"] | "", themeId) == 0) {
      return String((const char *)(theme["runtimePath"] | ""));
    }
  }
  return "";
}

void normaliseRuntimeItemIcons(JsonArray items) {
  for (JsonObject item : items) {
    String path = runtimeAssetPath(item["iconSrc"] | "", "/icons/");
    item["iconSrc"] = path;
  }
}

bool convertWebBackupToRuntime(JsonDocument &backup, String &error) {
  JsonObjectConst data = backup["data"].as<JsonObjectConst>();
  if (data.isNull() || !data["devices"].is<JsonArrayConst>() ||
      !data["activities"].is<JsonArrayConst>() ||
      !data["macros"].is<JsonArrayConst>() ||
      !data["themes"].is<JsonArrayConst>()) {
    error = "Backup categories are incomplete";
    return false;
  }

  JsonDocument runtime(&psramJsonAllocator);
  if (SD.exists(RUNTIME_CONFIG_PATH)) {
    File current = SD.open(RUNTIME_CONFIG_PATH, FILE_READ);
    if (current) {
      deserializeJson(runtime, current);
      current.close();
    }
  }
  runtime["schemaVersion"] = 1;
  runtime["webConfigVersion"] = backup["appVersion"] | installedWebConfigVersion();
  runtime["savedAt"] = backup["exportedAt"] | "";
  runtime["devices"].set(data["devices"]);
  runtime["activities"].set(data["activities"]);
  runtime["macros"].set(data["macros"]);
  runtime["themes"].set(data["themes"]);

  JsonArray devicesJson = runtime["devices"].as<JsonArray>();
  JsonArrayConst learned = data["learned"].as<JsonArrayConst>();
  for (JsonObjectConst candidate : learned) {
    const char *candidateId = candidate["id"] | "";
    bool found = false;
    for (JsonObjectConst existing : devicesJson) {
      if (candidateId[0] && strcmp(existing["id"] | "", candidateId) == 0) {
        found = true;
        break;
      }
    }
    if (!found) devicesJson.add(candidate);
  }

  JsonArrayConst themes = runtime["themes"].as<JsonArrayConst>();
  JsonArray devicePages = runtime["devicePages"].to<JsonArray>();
  for (JsonObjectConst device : devicesJson) {
    JsonObject page = devicePages.add<JsonObject>();
    page["deviceId"] = device["id"] | "";
    page["name"] = device["name"] | "Unnamed device";
    page["filePath"] = device["filePath"] | "";
    const char *pageTheme = device["pageTheme"] | "simple";
    page["pageTheme"] = pageTheme;
    page["themePath"] = runtimeThemePathForId(themes, pageTheme);
    page["items"].set(device["pageItems"]);
    normaliseRuntimeItemIcons(page["items"].as<JsonArray>());
    page["physicalBindings"].set(device["physicalBindings"]);
    page["powerTracking"].set(device["powerTracking"]);
  }

  JsonArray activitiesJson = runtime["activities"].as<JsonArray>();
  for (JsonObject activity : activitiesJson) {
    const char *pageTheme = activity["pageTheme"] | "simple";
    activity["pageThemePath"] = runtimeThemePathForId(themes, pageTheme);
    String iconPath = runtimeAssetPath(activity["iconSrc"] | "", "/icons/");
    activity["iconSrc"] = iconPath;
    normaliseRuntimeItemIcons(activity["pageItems"].as<JsonArray>());
  }

  JsonObjectConst systemThemes = backup["systemPageThemes"].as<JsonObjectConst>();
  const char *settingsTheme = systemThemes["settings"] | "simple";
  const char *activitiesTheme = systemThemes["activities"] | "simple";
  JsonArray pageRecords = runtime["pages"].to<JsonArray>();
  JsonObject settingsPage = pageRecords.add<JsonObject>();
  settingsPage["name"] = "Remote Settings";
  settingsPage["pageType"] = "settings";
  settingsPage["theme"] = settingsTheme;
  settingsPage["themePath"] = runtimeThemePathForId(themes, settingsTheme);
  settingsPage["items"].to<JsonArray>();

  JsonObject activitiesPage = pageRecords.add<JsonObject>();
  activitiesPage["name"] = "Activities";
  activitiesPage["pageType"] = "activities";
  activitiesPage["theme"] = activitiesTheme;
  activitiesPage["themePath"] = runtimeThemePathForId(themes, activitiesTheme);
  JsonArray landingItems = activitiesPage["items"].to<JsonArray>();
  for (JsonObjectConst activity : activitiesJson) {
    JsonObject item = landingItems.add<JsonObject>();
    item["id"] = String("item_") + String((const char *)(activity["id"] | ""));
    item["type"] = "activity";
    item["name"] = activity["name"] | "Activity";
    item["refId"] = activity["id"] | "";
    item["iconSrc"] = activity["iconSrc"] | "";
    item["iconName"] = activity["iconName"] | "";
    item["showText"] = true;
    item["boxMode"] = activity["boxMode"] | "global";
  }

  return saveRuntimeConfigDocument(runtime, error);
}

void backupDateStrings(char *fileStamp, size_t fileStampSize,
                       char *exportedAt, size_t exportedAtSize) {
  time_t now = time(nullptr);
  tm local = {};
  if (now >= 1700000000 && localtime_r(&now, &local)) {
    strftime(fileStamp, fileStampSize, "%Y-%m-%d_%H-%M-%S", &local);
    strftime(exportedAt, exportedAtSize, "%Y-%m-%dT%H:%M:%S", &local);
  } else {
    snprintf(fileStamp, fileStampSize, "uptime_%010lu", (unsigned long)(millis() / 1000UL));
    snprintf(exportedAt, exportedAtSize, "Uptime %lu seconds", (unsigned long)(millis() / 1000UL));
  }
}

bool createLcdFullBackup(String &createdName, String &error) {
  if (!sdReady || !SD.exists(RUNTIME_CONFIG_PATH)) {
    error = "Runtime configuration is unavailable";
    return false;
  }
  JsonDocument runtime(&psramJsonAllocator);
  File runtimeFile = SD.open(RUNTIME_CONFIG_PATH, FILE_READ);
  DeserializationError parseError = deserializeJson(runtime, runtimeFile);
  runtimeFile.close();
  if (parseError) {
    error = String("Runtime parse failed: ") + parseError.c_str();
    return false;
  }

  char stamp[32];
  char exportedAt[32];
  backupDateStrings(stamp, sizeof(stamp), exportedAt, sizeof(exportedAt));
  createdName = String("OpenRemote_Backup_") + stamp + ".json";
  String backupPath = String("/backups/") + createdName;
  String assetsPath = backupPath.substring(0, backupPath.length() - 5) + "_assets";
  deleteSdTree(assetsPath);
  if (!SD.mkdir(assetsPath)) {
    error = "Could not create backup asset folder";
    return false;
  }

  const char *sourceFolders[] = {
    "/config", "/devices", "/activities", "/macros",
    "/themes/Default", "/themes/Custom", "/icons/Custom"
  };
  bool assetsOk = true;
  for (const char *source : sourceFolders) {
    String destination = assetsPath + source;
    int slash = destination.lastIndexOf('/');
    String parent = destination.substring(0, slash);
    if (!SD.exists(parent)) SD.mkdir(parent);
    assetsOk = copySdTree(source, destination) && assetsOk;
  }
  if (!assetsOk) {
    deleteSdTree(assetsPath);
    error = "Could not copy all SD backup data";
    return false;
  }

  JsonDocument backup(&psramJsonAllocator);
  backup["format"] = "OpenRemote Full Backup";
  backup["version"] = 2;
  backup["appVersion"] = installedWebConfigVersion();
  backup["firmwareVersion"] = OPENREMOTE_VERSION_TEXT;
  backup["category"] = "full-backup";
  backup["exportedAt"] = exportedAt;
  backup["nativeAssets"] = assetsPath;
  JsonObject counts = backup["counts"].to<JsonObject>();
  counts["devices"] = DEVICE_COUNT;
  counts["learned"] = 0;
  counts["activities"] = ACTIVITY_COUNT;
  counts["macros"] = runtime["macros"].as<JsonArrayConst>().size();
  counts["icons"] = countSdFiles("/icons/Custom");
  counts["themes"] = runtime["themes"].as<JsonArrayConst>().size();

  JsonObject data = backup["data"].to<JsonObject>();
  data["devices"].set(runtime["devices"]);
  data["activities"].set(runtime["activities"]);
  data["macros"].set(runtime["macros"]);
  data["themes"].set(runtime["themes"]);
  JsonArray learned = data["learned"].to<JsonArray>();
  for (JsonObjectConst device : runtime["devices"].as<JsonArrayConst>()) {
    String source = device["source"] | "";
    source.toLowerCase();
    if ((device["learned"] | false) || source == "learned") learned.add(device);
  }
  counts["learned"] = learned.size();
  data["icons"].to<JsonArray>();
  backup["runtimeConfig"].set(runtime.as<JsonVariantConst>());

  for (JsonObjectConst page : runtime["pages"].as<JsonArrayConst>()) {
    const char *pageType = page["pageType"] | "";
    if (strcmp(pageType, "settings") == 0) {
      backup["systemPageThemes"]["settings"] = page["theme"] | "simple";
    } else if (strcmp(pageType, "activities") == 0) {
      backup["systemPageThemes"]["activities"] = page["theme"] | "simple";
    }
  }

  String temporaryPath = "/tmp/lcd-full-backup.json";
  SD.remove(temporaryPath);
  File output = SD.open(temporaryPath, FILE_WRITE);
  size_t written = output ? serializeJsonPretty(backup, output) : 0;
  if (output) output.close();
  if (!written) {
    SD.remove(temporaryPath);
    deleteSdTree(assetsPath);
    error = "Could not write backup file";
    return false;
  }
  SD.remove(backupPath);
  if (!SD.rename(temporaryPath, backupPath)) {
    SD.remove(temporaryPath);
    deleteSdTree(assetsPath);
    error = "Could not finalise backup file";
    return false;
  }
  return true;
}

bool restoreNativeBackupAssets(const String &assetsPath, String &error) {
  if (!assetsPath.startsWith("/backups/") || !assetsPath.endsWith("_assets") ||
      !SD.exists(assetsPath + "/config/runtime.json")) {
    error = "Native backup assets are incomplete";
    return false;
  }
  const char *targets[] = {
    "/config", "/devices", "/activities", "/macros",
    "/themes/Default", "/themes/Custom", "/icons/Custom"
  };
  for (const char *target : targets) {
    String source = assetsPath + target;
    if (!SD.exists(source)) continue;
    deleteSdTree(target);
    int slash = String(target).lastIndexOf('/');
    String parent = String(target).substring(0, slash);
    if (parent.length() && !SD.exists(parent)) SD.mkdir(parent);
    if (!copySdTree(source, target)) {
      error = String("Could not restore ") + target;
      return false;
    }
  }
  return true;
}

bool restoreLcdFullBackup(const char *name, String &error) {
  String clean = sanitizeBackupFileName(name ? name : "");
  if (!clean.endsWith(".json")) {
    error = "Invalid backup filename";
    return false;
  }
  File file = SD.open(String("/backups/") + clean, FILE_READ);
  if (!file) {
    error = "Backup file not found";
    return false;
  }
  JsonDocument backup(&psramJsonAllocator);
  DeserializationError parseError = deserializeJson(backup, file);
  file.close();
  if (parseError || strcmp(backup["category"] | "", "full-backup") != 0) {
    error = "This is not a valid full backup";
    return false;
  }

  String assetsPath = backup["nativeAssets"] | "";
  bool restored = assetsPath.length()
    ? restoreNativeBackupAssets(assetsPath, error)
    : convertWebBackupToRuntime(backup, error);
  if (!restored) return false;
  if (!loadRuntimeConfig()) {
    error = "Backup restored, but runtime reload failed";
    return false;
  }
  saveSettings();
  return true;
}

void formatBackupDisplayDate(const char *exportedAt, char *output, size_t outputSize) {
  int year = 0, month = 0, day = 0, hour = 0, minute = 0;
  if (exportedAt && sscanf(exportedAt, "%d-%d-%dT%d:%d", &year, &month, &day, &hour, &minute) == 5) {
    static const char *months[] = {
      "Jan", "Feb", "Mar", "Apr", "May", "Jun",
      "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    int displayHour = hour % 12;
    if (!displayHour) displayHour = 12;
    snprintf(output, outputSize, "%02d %s %04d  %d:%02d %s",
             day, months[constrain(month, 1, 12) - 1], year,
             displayHour, minute, hour >= 12 ? "PM" : "AM");
  } else {
    strlcpy(output, "Date unavailable", outputSize);
  }
}

void loadLcdBackupEntries() {
  lcdBackupCount = 0;
  File root = SD.open("/backups");
  if (!root || !root.isDirectory()) {
    if (root) root.close();
    return;
  }
  while (lcdBackupCount < MAX_LCD_BACKUPS) {
    File entry = root.openNextFile();
    if (!entry) break;
    if (!entry.isDirectory()) {
      String path = entry.path();
      String name = path.substring(path.lastIndexOf('/') + 1);
      String lower = name;
      lower.toLowerCase();
      if (lower.endsWith(".json")) {
        JsonDocument filter;
        filter["category"] = true;
        filter["exportedAt"] = true;
        JsonDocument summary(&psramJsonAllocator);
        DeserializationError jsonError = deserializeJson(
          summary, entry, DeserializationOption::Filter(filter));
        if (!jsonError && strcmp(summary["category"] | "", "full-backup") == 0) {
          LcdBackupEntry &backup = lcdBackupEntries[lcdBackupCount++];
          strlcpy(backup.name, name.c_str(), sizeof(backup.name));
          strlcpy(backup.exportedAt, summary["exportedAt"] | "", sizeof(backup.exportedAt));
          formatBackupDisplayDate(backup.exportedAt, backup.displayDate, sizeof(backup.displayDate));
        }
      }
    }
    entry.close();
  }
  root.close();

  for (uint8_t i = 0; i < lcdBackupCount; i++) {
    for (uint8_t j = i + 1; j < lcdBackupCount; j++) {
      if (strcmp(lcdBackupEntries[j].exportedAt, lcdBackupEntries[i].exportedAt) > 0) {
        LcdBackupEntry temporary = lcdBackupEntries[i];
        lcdBackupEntries[i] = lcdBackupEntries[j];
        lcdBackupEntries[j] = temporary;
      }
    }
  }
}

String sanitizeBackupFileName(String name) {
  int slash = max(name.lastIndexOf('/'), name.lastIndexOf('\\'));
  if (slash >= 0) name = name.substring(slash + 1);
  String output;
  output.reserve(64);
  for (size_t i = 0; i < name.length() && output.length() < 60; i++) {
    char c = name[i];
    if (isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.') output += c;
  }
  String lower = output;
  lower.toLowerCase();
  bool allowed = lower.endsWith(".json") || lower.endsWith(".ir") ||
                 lower.endsWith(".txt") || lower.endsWith(".gc") ||
                 lower.endsWith(".gcir");
  return allowed ? output : "";
}

void handleBackupList() {
  if (!requestAuthorized()) {
    sendJson(403, "{\"ok\":false,\"error\":\"Not authorized\"}");
    return;
  }
  JsonDocument doc(&psramJsonAllocator);
  doc["ok"] = true;
  JsonArray files = doc["files"].to<JsonArray>();
  File root = SD.open("/backups");
  if (root && root.isDirectory()) {
    while (true) {
      File entry = root.openNextFile();
      if (!entry) break;
      if (!entry.isDirectory()) {
        String path = entry.path();
        String name = path.substring(path.lastIndexOf('/') + 1);
        String lowerName = name;
        lowerName.toLowerCase();
        if (lowerName.endsWith(".json")) {
          JsonDocument filter;
          filter["category"] = true;
          filter["exportedAt"] = true;
          filter["appVersion"] = true;
          filter["counts"] = true;
          JsonDocument summary(&psramJsonAllocator);
          DeserializationError error = deserializeJson(
            summary, entry, DeserializationOption::Filter(filter));
          const char *category = summary["category"] | "";
          if (!error && strcmp(category, "full-backup") == 0) {
            JsonObject item = files.add<JsonObject>();
            item["name"] = name;
            item["size"] = entry.size();
            item["exportedAt"] = summary["exportedAt"] | "";
            item["appVersion"] = summary["appVersion"] | "";
            JsonObjectConst counts = summary["counts"];
            item["devices"] = counts["devices"] | 0;
            item["learned"] = counts["learned"] | 0;
            item["activities"] = counts["activities"] | 0;
            item["macros"] = counts["macros"] | 0;
            item["icons"] = counts["icons"] | 0;
            item["themes"] = counts["themes"] | 0;
          }
        }
      }
      entry.close();
    }
  }
  if (root) root.close();
  String body;
  serializeJson(doc, body);
  sendJson(200, body);
}

void handleBackupDownload() {
  if (!requestAuthorized()) {
    sendJson(403, "{\"ok\":false,\"error\":\"Not authorized\"}");
    return;
  }
  String name = sanitizeBackupFileName(webServer.arg("name"));
  if (!name.length()) {
    sendJson(400, "{\"ok\":false,\"error\":\"Invalid backup filename\"}");
    return;
  }
  File file = SD.open(String("/backups/") + name, FILE_READ);
  if (!file) {
    sendJson(404, "{\"ok\":false,\"error\":\"Backup file not found\"}");
    return;
  }
  webServer.sendHeader("Cache-Control", "no-store");
  webServer.sendHeader("Content-Disposition", String("attachment; filename=\"") + name + "\"");
  webServer.streamFile(file, "application/octet-stream");
  file.close();
}

void handleBackupDelete() {
  if (!requestAuthorized()) {
    sendJson(403, "{\"ok\":false,\"error\":\"Not authorized\"}");
    return;
  }
  String name = sanitizeBackupFileName(webServer.arg("name"));
  String lowerName = name;
  lowerName.toLowerCase();
  if (!name.length() || !lowerName.endsWith(".json")) {
    sendJson(400, "{\"ok\":false,\"error\":\"Invalid backup filename\"}");
    return;
  }
  String path = String("/backups/") + name;
  if (!SD.exists(path)) {
    sendJson(404, "{\"ok\":false,\"error\":\"Backup file not found\"}");
    return;
  }
  String nativeAssets;
  File metadata = SD.open(path, FILE_READ);
  if (metadata) {
    JsonDocument filter;
    filter["nativeAssets"] = true;
    JsonDocument summary(&psramJsonAllocator);
    if (!deserializeJson(summary, metadata, DeserializationOption::Filter(filter))) {
      nativeAssets = summary["nativeAssets"] | "";
    }
    metadata.close();
  }
  bool removed = SD.remove(path);
  if (removed && nativeAssets.startsWith("/backups/") && nativeAssets.endsWith("_assets")) {
    deleteSdTree(nativeAssets);
  }
  sendJson(removed ? 200 : 500,
           removed ? "{\"ok\":true}" : "{\"ok\":false,\"error\":\"Could not delete backup\"}");
}

void handleBackupUploadData() {
  HTTPUpload &upload = webServer.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String name = sanitizeBackupFileName(upload.filename);
    backupUploadOk = requestAuthorized() && sdReady && name.length();
    backupUploadPath = backupUploadOk ? String("/backups/") + name : "";
    SD.remove("/tmp/backup.upload");
    if (backupUploadOk) backupUploadFile = SD.open("/tmp/backup.upload", FILE_WRITE);
    backupUploadOk = backupUploadOk && (bool)backupUploadFile;
  } else if (upload.status == UPLOAD_FILE_WRITE && backupUploadOk) {
    backupUploadOk = backupUploadFile.write(upload.buf, upload.currentSize) == upload.currentSize;
  } else if (upload.status == UPLOAD_FILE_END) {
    if (backupUploadFile) backupUploadFile.close();
    if (backupUploadOk) {
      SD.remove(backupUploadPath);
      backupUploadOk = SD.rename("/tmp/backup.upload", backupUploadPath);
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    if (backupUploadFile) backupUploadFile.close();
    SD.remove("/tmp/backup.upload");
    backupUploadOk = false;
  }
}

void handleFactoryReset() {
  if (!requestAuthorized()) {
    sendJson(403, "{\"ok\":false,\"error\":\"Not authorized\"}");
    return;
  }
  JsonDocument request;
  deserializeJson(request, webServer.arg("plain"));
  if (String((const char *)(request["confirmation"] | "")) != "RESET") {
    sendJson(400, "{\"ok\":false,\"error\":\"Confirmation required\"}");
    return;
  }

  const char *userFolders[] = {
    "/config", "/devices", "/activities", "/macros",
    "/icons/Custom", "/themes/Custom", "/logs", "/tmp"
  };
  for (const char *folder : userFolders) deleteSdTree(folder);
  bool ok = true;
  for (uint8_t i = 0; i < SD_FOLDER_COUNT; i++) ok = createSdFolderIfMissing(sdFolders[i]) && ok;
  preferences.begin(PREFERENCES_NAMESPACE, false);
  preferences.clear();
  preferences.end();
  if (!ok) {
    sendJson(500, "{\"ok\":false,\"error\":\"Factory reset could not rebuild user folders\"}");
    return;
  }
  sendJson(200, "{\"ok\":true,\"restarting\":true,\"sdFormatted\":false}");
  restartPending = true;
}

void handleFirmwareUploadData() {
  HTTPUpload &upload = webServer.upload();
  if (upload.status == UPLOAD_FILE_START) {
    firmwareUploadOk = requestAuthorized() && upload.filename.endsWith(".bin") && Update.begin();
    Serial.printf("Firmware upload: %s\n", upload.filename.c_str());
  } else if (upload.status == UPLOAD_FILE_WRITE && firmwareUploadOk) {
    firmwareUploadOk = Update.write(upload.buf, upload.currentSize) == upload.currentSize;
  } else if (upload.status == UPLOAD_FILE_END && firmwareUploadOk) {
    firmwareUploadOk = Update.end(true);
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    Update.abort();
    firmwareUploadOk = false;
  }
}

void handleFirmwareStageUploadData() {
  HTTPUpload &upload = webServer.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String lowerName = upload.filename;
    lowerName.toLowerCase();
    firmwareStageOk = requestAuthorized() && sdReady && lowerName.endsWith(".bin");
    firmwareStageBytes = 0;
    SD.remove(FIRMWARE_STAGE_PATH);
    if (firmwareStageOk) firmwareStageFile = SD.open(FIRMWARE_STAGE_PATH, FILE_WRITE);
    firmwareStageOk = firmwareStageOk && (bool)firmwareStageFile;
    Serial.printf("Firmware stage: %s\n", upload.filename.c_str());
  } else if (upload.status == UPLOAD_FILE_WRITE && firmwareStageOk) {
    if ((firmwareStageBytes == 0 && (upload.currentSize == 0 || upload.buf[0] != 0xE9)) ||
        firmwareStageBytes + upload.currentSize > 3300000UL) {
      firmwareStageOk = false;
    } else {
      firmwareStageOk = firmwareStageFile.write(upload.buf, upload.currentSize) == upload.currentSize;
      if (firmwareStageOk) firmwareStageBytes += upload.currentSize;
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (firmwareStageFile) firmwareStageFile.close();
    firmwareStageOk = firmwareStageOk && firmwareStageBytes >= 65536UL;
    if (!firmwareStageOk) SD.remove(FIRMWARE_STAGE_PATH);
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    if (firmwareStageFile) firmwareStageFile.close();
    SD.remove(FIRMWARE_STAGE_PATH);
    firmwareStageOk = false;
    firmwareStageBytes = 0;
  }
}

void handleFirmwareInstall() {
  if (!requestAuthorized()) {
    sendJson(403, "{\"ok\":false,\"error\":\"Not authorized\"}");
    return;
  }
  if (!sdReady || !SD.exists(FIRMWARE_STAGE_PATH)) {
    sendJson(404, "{\"ok\":false,\"error\":\"Upload a firmware .bin before installing\"}");
    return;
  }
  File file = SD.open(FIRMWARE_STAGE_PATH, FILE_READ);
  size_t total = file ? file.size() : 0;
  if (!file || total < 65536UL || file.read() != 0xE9 || !file.seek(0)) {
    if (file) file.close();
    sendJson(400, "{\"ok\":false,\"error\":\"The staged file is not an ESP32 firmware image\"}");
    return;
  }
  bool ok = Update.begin(total, U_FLASH);
  uint8_t buffer[1024];
  while (ok && file.available()) {
    size_t count = file.read(buffer, sizeof(buffer));
    if (!count) break;
    ok = Update.write(buffer, count) == count;
    delay(0);
  }
  file.close();
  ok = ok && Update.end(true);
  if (!ok) {
    Update.abort();
    sendJson(500, String("{\"ok\":false,\"error\":\"Firmware installation failed: ") +
                  Update.errorString() + "\"}");
    return;
  }
  SD.remove(FIRMWARE_STAGE_PATH);
  sendJson(200, "{\"ok\":true,\"restarting\":true}");
  restartPending = true;
}

void handleWebConfigUploadData() {
  HTTPUpload &upload = webServer.upload();
  if (upload.status == UPLOAD_FILE_START) {
    webConfigUploadOk = requestAuthorized() && sdReady && upload.filename.endsWith(".html");
    SD.remove("/tmp/index.upload.html");
    if (webConfigUploadOk) webConfigUploadFile = SD.open("/tmp/index.upload.html", FILE_WRITE);
    webConfigUploadOk = webConfigUploadOk && (bool)webConfigUploadFile;
    Serial.printf("WebConfig upload: %s\n", upload.filename.c_str());
  } else if (upload.status == UPLOAD_FILE_WRITE && webConfigUploadOk) {
    webConfigUploadOk = webConfigUploadFile.write(upload.buf, upload.currentSize) == upload.currentSize;
  } else if (upload.status == UPLOAD_FILE_END) {
    if (webConfigUploadFile) webConfigUploadFile.close();
    if (webConfigUploadOk) {
      SD.remove("/backups/index.previous.html");
      if (SD.exists(WEB_CONFIG_PATH)) SD.rename(WEB_CONFIG_PATH, "/backups/index.previous.html");
      webConfigUploadOk = SD.rename("/tmp/index.upload.html", WEB_CONFIG_PATH);
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    if (webConfigUploadFile) webConfigUploadFile.close();
    SD.remove("/tmp/index.upload.html");
    webConfigUploadOk = false;
  }
}

void handleIrdbUploadData() {
  HTTPUpload &upload = webServer.upload();
  if (upload.status == UPLOAD_FILE_START) {
    irdbUploadOk = requestAuthorized() && sdReady && upload.filename.endsWith(".irdb");
    SD.remove("/tmp/OpenRemote.upload.irdb");
    if (irdbUploadOk) irdbUploadFile = SD.open("/tmp/OpenRemote.upload.irdb", FILE_WRITE);
    irdbUploadOk = irdbUploadOk && (bool)irdbUploadFile;
    Serial.printf("IRDB upload: %s\n", upload.filename.c_str());
  } else if (upload.status == UPLOAD_FILE_WRITE && irdbUploadOk) {
    irdbUploadOk = irdbUploadFile.write(upload.buf, upload.currentSize) == upload.currentSize;
  } else if (upload.status == UPLOAD_FILE_END) {
    if (irdbUploadFile) irdbUploadFile.close();
    if (irdbUploadOk) {
      SD.remove("/backups/OpenRemote.previous.irdb");
      if (SD.exists(IRDB_PATH)) SD.rename(IRDB_PATH, "/backups/OpenRemote.previous.irdb");
      irdbUploadOk = SD.rename("/tmp/OpenRemote.upload.irdb", IRDB_PATH);
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    if (irdbUploadFile) irdbUploadFile.close();
    SD.remove("/tmp/OpenRemote.upload.irdb");
    irdbUploadOk = false;
  }
}

void serviceIrLearning(unsigned long now) {
  if (!irLearningActive) return;
  if ((int32_t)(now - irLearningStartedMs) >= (int32_t)IR_LEARN_TIMEOUT_MS) {
    IrReceiver.stop();
    irLearningActive = false;
    irLearningError = "No IR signal received within 12 seconds";
    return;
  }
  if (!IrReceiver.decode()) return;

  if (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_WAS_OVERFLOW) {
    IrReceiver.stop();
    irLearningActive = false;
    irLearningError = "IR signal exceeded the receiver buffer";
    return;
  }
  IRRawlenType rawLength = IrReceiver.decodedIRData.rawlen;
  if (rawLength <= 2) {
    IrReceiver.resume();
    return;
  }

  String timings;
  timings.reserve(min((size_t)rawLength * 7U, (size_t)6144U));
  uint16_t captured = min((uint16_t)(rawLength - 1), IR_LEARN_MAX_TIMINGS);
  for (uint16_t i = 1; i <= captured; i++) {
    uint32_t duration = (uint32_t)IrReceiver.irparams.rawbuf[i] * MICROS_PER_TICK;
    if (i & 1) {
      duration = duration > MARK_EXCESS_MICROS ? duration - MARK_EXCESS_MICROS : duration;
    } else {
      duration += MARK_EXCESS_MICROS;
    }
    if (i > 1) timings += ' ';
    timings += String(min(duration, (uint32_t)65535));
  }

  JsonDocument capture(&psramJsonAllocator);
  capture["type"] = "raw";
  capture["frequency"] = 38000;
  capture["data"] = timings;
  capture["timingCount"] = captured;
  capture["protocol"] = getProtocolString(IrReceiver.decodedIRData.protocol);
  serializeJson(capture, irLearningResult);
  IrReceiver.stop();
  irLearningActive = false;
  irLearningError = "";
  Serial.printf("IR learn: captured %u timing(s), protocol %s\n", captured,
                getProtocolString(IrReceiver.decodedIRData.protocol));
}

void handleIrLearnStart() {
  if (!requestAuthorized()) {
    sendJson(403, "{\"ok\":false,\"error\":\"Not authorized\"}");
    return;
  }
  irLearningResult = "";
  irLearningError = "";
  irLearningStartedMs = millis();
  irLearningActive = true;
  IrReceiver.start();
  sendJson(202, "{\"ok\":true,\"state\":\"listening\",\"timeoutMs\":12000}");
}

void handleIrLearnStatus() {
  if (!requestAuthorized()) {
    sendJson(403, "{\"ok\":false,\"error\":\"Not authorized\"}");
    return;
  }
  if (irLearningActive) {
    sendJson(200, "{\"ok\":true,\"state\":\"listening\"}");
  } else if (irLearningResult.length()) {
    sendJson(200, String("{\"ok\":true,\"state\":\"captured\",\"ir\":") +
                  irLearningResult + "}");
  } else if (irLearningError.length()) {
    sendJson(200, String("{\"ok\":true,\"state\":\"error\",\"error\":\"") +
                  irLearningError + "\"}");
  } else {
    sendJson(200, "{\"ok\":true,\"state\":\"idle\"}");
  }
}

void handleIrLearnCancel() {
  if (!requestAuthorized()) {
    sendJson(403, "{\"ok\":false,\"error\":\"Not authorized\"}");
    return;
  }
  IrReceiver.stop();
  irLearningActive = false;
  irLearningResult = "";
  irLearningError = "";
  sendJson(200, "{\"ok\":true,\"state\":\"idle\"}");
}

String sanitizeIconFileName(String name) {
  int slash = max(name.lastIndexOf('/'), name.lastIndexOf('\\'));
  if (slash >= 0) name = name.substring(slash + 1);
  String output;
  output.reserve(56);
  for (size_t i = 0; i < name.length() && output.length() < 52; i++) {
    char c = name[i];
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') || c == '-' || c == '_') {
      output += c;
    } else if (c == '.') {
      output += c;
    } else if (output.length() && output[output.length() - 1] != '_') {
      output += '_';
    }
  }
  output.trim();
  String lower = output;
  lower.toLowerCase();
  if (!(lower.endsWith(".png") || lower.endsWith(".jpg") || lower.endsWith(".jpeg"))) {
    output += ".png";
  }
  return output.length() ? output : "custom_icon.png";
}

String iconDisplayName(String path) {
  String name = path.substring(path.lastIndexOf('/') + 1);
  int dot = name.lastIndexOf('.');
  if (dot > 0) name.remove(dot);
  name.replace('_', ' ');
  bool upperNext = true;
  for (size_t i = 0; i < name.length(); i++) {
    if (name[i] == ' ') upperNext = true;
    else if (upperNext) {
      name.setCharAt(i, toupper((unsigned char)name[i]));
      upperNext = false;
    }
  }
  return name;
}

void appendIconDirectory(JsonArray target, const char *directory) {
  File root = SD.open(directory);
  if (!root || !root.isDirectory()) {
    if (root) root.close();
    return;
  }
  File entry = root.openNextFile();
  while (entry) {
    String path = entry.path();
    bool isFile = !entry.isDirectory();
    entry.close();
    String lower = path;
    lower.toLowerCase();
    if (isFile && (lower.endsWith(".png") || lower.endsWith(".jpg") || lower.endsWith(".jpeg"))) {
      JsonObject icon = target.add<JsonObject>();
      icon["name"] = iconDisplayName(path);
      icon["path"] = path;
    }
    entry = root.openNextFile();
  }
  root.close();
}

void handleIconList() {
  if (!requestAuthorized()) {
    sendJson(403, "{\"ok\":false,\"error\":\"Not authorized\"}");
    return;
  }
  JsonDocument doc(&psramJsonAllocator);
  doc["ok"] = true;
  appendIconDirectory(doc["defaults"].to<JsonArray>(), "/icons/Default");
  appendIconDirectory(doc["custom"].to<JsonArray>(), "/icons/Custom");
  String body;
  serializeJson(doc, body);
  sendJson(200, body);
}

void handleCustomIconUploadData() {
  HTTPUpload &upload = webServer.upload();
  if (upload.status == UPLOAD_FILE_START) {
    customIconUploadOk = requestAuthorized() && sdReady;
    String requestedName = webServer.arg("name");
    if (!requestedName.length()) requestedName = upload.filename;
    customIconUploadPath = String("/icons/Custom/") + sanitizeIconFileName(requestedName);
    SD.remove("/tmp/custom_icon.upload");
    if (customIconUploadOk) customIconUploadFile = SD.open("/tmp/custom_icon.upload", FILE_WRITE);
    customIconUploadOk = customIconUploadOk && (bool)customIconUploadFile;
  } else if (upload.status == UPLOAD_FILE_WRITE && customIconUploadOk) {
    customIconUploadOk = customIconUploadFile.write(upload.buf, upload.currentSize) == upload.currentSize;
  } else if (upload.status == UPLOAD_FILE_END) {
    if (customIconUploadFile) customIconUploadFile.close();
    if (customIconUploadOk) {
      SD.remove(customIconUploadPath);
      customIconUploadOk = SD.rename("/tmp/custom_icon.upload", customIconUploadPath);
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    if (customIconUploadFile) customIconUploadFile.close();
    SD.remove("/tmp/custom_icon.upload");
    customIconUploadOk = false;
  }
}

String sanitizeThemeFileName(String name) {
  String lowerName = name;
  lowerName.toLowerCase();
  bool jpeg = lowerName.endsWith(".jpg") || lowerName.endsWith(".jpeg");
  bool png = lowerName.endsWith(".png");
  String output;
  output.reserve(56);
  for (size_t i = 0; i < name.length() && output.length() < 48; i++) {
    char c = name[i];
    if (isalnum((unsigned char)c) || c == '-' || c == '_') output += c;
    else if (c == '.') output += c;
  }
  String lowerOutput = output;
  lowerOutput.toLowerCase();
  if (jpeg) {
    if (!(lowerOutput.endsWith(".jpg") || lowerOutput.endsWith(".jpeg"))) output += ".jpg";
  } else if (png) {
    if (!lowerOutput.endsWith(".png")) output += ".png";
  } else if (!lowerOutput.endsWith(".rgb565")) {
    output += ".rgb565";
  }
  return output.length() > 4 ? output : (jpeg ? "theme.jpg" : (png ? "theme.png" : "theme.rgb565"));
}

void handleThemeUploadData() {
  HTTPUpload &upload = webServer.upload();
  if (upload.status == UPLOAD_FILE_START) {
    themeUploadOk = requestAuthorized() && sdReady;
    String requestedName = webServer.arg("name");
    if (!requestedName.length()) requestedName = upload.filename;
    String library = webServer.arg("library");
    library = library.equalsIgnoreCase("Default") ? "Default" : "Custom";
    String libraryPath = String("/themes/") + library;
    themeUploadOk = themeUploadOk && createSdFolderIfMissing(libraryPath.c_str());
    themeUploadPath = libraryPath + "/" + sanitizeThemeFileName(requestedName);
    SD.remove("/tmp/theme.upload");
    if (themeUploadOk) themeUploadFile = SD.open("/tmp/theme.upload", FILE_WRITE);
    themeUploadOk = themeUploadOk && (bool)themeUploadFile;
  } else if (upload.status == UPLOAD_FILE_WRITE && themeUploadOk) {
    themeUploadOk = themeUploadFile.write(upload.buf, upload.currentSize) == upload.currentSize;
  } else if (upload.status == UPLOAD_FILE_END) {
    if (themeUploadFile) themeUploadFile.close();
    String lowerPath = themeUploadPath;
    lowerPath.toLowerCase();
    bool validSize = lowerPath.endsWith(".rgb565")
      ? upload.totalSize == LCD_W * LCD_H * sizeof(uint16_t)
      : upload.totalSize > 0;
    themeUploadOk = themeUploadOk && validSize;
    if (themeUploadOk) {
      SD.remove(themeUploadPath);
      themeUploadOk = SD.rename("/tmp/theme.upload", themeUploadPath);
    } else {
      SD.remove("/tmp/theme.upload");
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    if (themeUploadFile) themeUploadFile.close();
    SD.remove("/tmp/theme.upload");
    themeUploadOk = false;
  }
}

void handleCustomIconDelete() {
  if (!requestAuthorized()) {
    sendJson(403, "{\"ok\":false,\"error\":\"Not authorized\"}");
    return;
  }
  JsonDocument doc;
  deserializeJson(doc, webServer.arg("plain"));
  String path = doc["path"] | "";
  if (!path.startsWith("/icons/Custom/") || path.indexOf("..") >= 0) {
    sendJson(400, "{\"ok\":false,\"error\":\"Invalid custom icon path\"}");
    return;
  }
  bool removed = SD.remove(path);
  sendJson(removed ? 200 : 404,
           removed ? "{\"ok\":true}" : "{\"ok\":false,\"error\":\"Icon file not found\"}");
}

void handleCustomIconRename() {
  if (!requestAuthorized()) {
    sendJson(403, "{\"ok\":false,\"error\":\"Not authorized\"}");
    return;
  }
  JsonDocument doc;
  deserializeJson(doc, webServer.arg("plain"));
  String oldPath = doc["path"] | "";
  String newName = doc["name"] | "";
  if (!oldPath.startsWith("/icons/Custom/") || oldPath.indexOf("..") >= 0 || !newName.length()) {
    sendJson(400, "{\"ok\":false,\"error\":\"Invalid custom icon rename\"}");
    return;
  }
  String newPath = String("/icons/Custom/") + sanitizeIconFileName(newName);
  SD.remove(newPath);
  if (!SD.rename(oldPath, newPath)) {
    sendJson(500, "{\"ok\":false,\"error\":\"Could not rename custom icon\"}");
    return;
  }
  sendJson(200, String("{\"ok\":true,\"path\":\"") + newPath +
                "\",\"name\":\"" + iconDisplayName(newPath) + "\"}");
}

void handleCommandTest() {
  if (!requestAuthorized()) {
    sendJson(403, "{\"ok\":false,\"error\":\"Not authorized\"}");
    return;
  }
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, webServer.arg("plain"));
  if (error) {
    sendJson(400, "{\"ok\":false,\"error\":\"Invalid command request\"}");
    return;
  }
  Device *device = findRuntimeDevice(doc["deviceId"] | "");
  DeviceCommand *command = findRuntimeCommand(device, doc["commandId"] | "");
  if (!device || !command) {
    sendJson(404, "{\"ok\":false,\"error\":\"Device command was not found\"}");
    return;
  }
  if (transmitRuntimeCommand(command) != RUNTIME_COMMAND_SENT) {
    sendJson(422, "{\"ok\":false,\"error\":\"This command cannot be transmitted by the installed firmware\"}");
    return;
  }
  sendJson(200, "{\"ok\":true,\"sent\":true}");
}

bool homebridgeCharacteristicWritable(JsonObjectConst characteristic) {
  if (characteristic["canWrite"] | false) return true;
  for (JsonVariantConst permission : characteristic["perms"].as<JsonArrayConst>()) {
    if (strcmp(permission.as<const char *>(), "pw") == 0) return true;
  }
  return false;
}

bool homebridgeCommandFromJson(JsonObjectConst spec, DeviceCommand &command,
                               String &error) {
  memset(&command, 0, sizeof(command));
  strlcpy(command.homebridgeAccessoryId, spec["accessoryId"] | "",
          sizeof(command.homebridgeAccessoryId));
  strlcpy(command.homebridgeCharacteristic, spec["characteristicType"] | "",
          sizeof(command.homebridgeCharacteristic));
  if (!command.homebridgeAccessoryId[0] || !command.homebridgeCharacteristic[0]) {
    error = "Homebridge command is missing its accessory or characteristic";
    return false;
  }
  const char *operation = spec["operation"] | "set";
  command.homebridgeOperation = strcmp(operation, "toggle") == 0 ? 1 :
    (strcmp(operation, "relative") == 0 ? 2 : 0);
  command.homebridgeStep = spec["step"] | 0.0f;
  command.homebridgeMin = spec["min"] | 0.0f;
  command.homebridgeMax = spec["max"] | 100.0f;
  JsonVariantConst value = spec["value"];
  if (value.is<bool>()) {
    command.homebridgeValueType = 1;
    command.homebridgeValue = value.as<bool>() ? 1.0f : 0.0f;
  } else if (value.is<const char *>()) {
    command.homebridgeValueType = 3;
    strlcpy(command.homebridgeStringValue, value.as<const char *>(),
            sizeof(command.homebridgeStringValue));
  } else {
    command.homebridgeValueType = 2;
    command.homebridgeValue = value | 0.0f;
  }
  command.kind = DeviceCommand::HOMEBRIDGE;
  return true;
}

void handleHomebridgeDiscover() {
  if (!requestAuthorized()) {
    sendJson(403, "{\"ok\":false,\"error\":\"Not authorized\"}");
    return;
  }
  JsonDocument request(&psramJsonAllocator);
  if (deserializeJson(request, webServer.arg("plain"))) {
    sendJson(400, "{\"ok\":false,\"error\":\"Invalid Homebridge request\"}");
    return;
  }
  String address = normaliseHomebridgeAddress(request["address"] | homebridgeAddress.c_str());
  String username = String((const char *)(request["username"] | homebridgeUsername.c_str()));
  String password = String((const char *)(request["password"] | ""));
  if (!password.length()) password = homebridgePassword;
  String token;
  String error;
  if (!homebridgeLogin(address, username, password, token, error)) {
    JsonDocument response;
    response["ok"] = false;
    response["error"] = error;
    String body;
    serializeJson(response, body);
    sendJson(401, body);
    return;
  }

  String raw;
  int status = homebridgeHttp(address, "GET", "/api/accessories", "", token, raw);
  if (status < 200 || status >= 300) {
    JsonDocument response;
    response["ok"] = false;
    response["error"] = homebridgeResponseError(raw,
      String("Accessory discovery failed (HTTP ") + status + ")");
    String body;
    serializeJson(response, body);
    sendJson(status == 400 ? 400 : 502, body);
    return;
  }

  JsonDocument source(&psramJsonAllocator);
  if (deserializeJson(source, raw)) {
    sendJson(502, "{\"ok\":false,\"error\":\"Homebridge returned invalid accessory data\"}");
    return;
  }
  JsonArrayConst services = source.as<JsonArrayConst>();
  if (services.isNull()) services = source["services"].as<JsonArrayConst>();
  JsonDocument response(&psramJsonAllocator);
  response["ok"] = true;
  response["address"] = address;
  JsonArray accessories = response["accessories"].to<JsonArray>();
  for (JsonObjectConst service : services) {
    if (accessories.size() >= 40) break;
    JsonArrayConst characteristics = service["serviceCharacteristics"].as<JsonArrayConst>();
    uint8_t writableCount = 0;
    for (JsonObjectConst characteristic : characteristics) {
      if (homebridgeCharacteristicWritable(characteristic)) writableCount++;
    }
    if (!writableCount) continue;
    JsonObject accessory = accessories.add<JsonObject>();
    accessory["uniqueId"] = service["uniqueId"] | "";
    const char *name = service["serviceName"] | "";
    if (!name[0]) name = service["displayName"] | "";
    if (!name[0]) name = service["name"] | "";
    if (!name[0]) name = service["type"] | "Homebridge accessory";
    accessory["name"] = name;
    const char *serviceType = service["type"] | "";
    if (!serviceType[0]) serviceType = service["serviceType"] | "Accessory";
    accessory["type"] = serviceType;
    JsonArray writable = accessory["characteristics"].to<JsonArray>();
    for (JsonObjectConst characteristic : characteristics) {
      if (!homebridgeCharacteristicWritable(characteristic)) continue;
      JsonObject output = writable.add<JsonObject>();
      output["type"] = characteristic["type"] | "";
      const char *displayName = characteristic["displayName"] | "";
      if (!displayName[0]) displayName = characteristic["description"] | "";
      if (!displayName[0]) displayName = characteristic["type"] | "Control";
      output["displayName"] = displayName;
      output["format"] = characteristic["format"] | "";
      output["value"].set(characteristic["value"]);
      if (characteristic.containsKey("minValue")) output["minValue"] = characteristic["minValue"];
      if (characteristic.containsKey("maxValue")) output["maxValue"] = characteristic["maxValue"];
      if (characteristic.containsKey("minStep")) output["minStep"] = characteristic["minStep"];
      if (characteristic["validValues"].is<JsonArrayConst>()) {
        output["validValues"].set(characteristic["validValues"]);
      }
    }
  }
  response["count"] = accessories.size();
  saveHomebridgeCredentials(address, username, password);
  homebridgeToken = token;
  String body;
  serializeJson(response, body);
  sendJson(200, body);
}

void handleHomebridgeStatus() {
  if (!requestAuthorized()) {
    sendJson(403, "{\"ok\":false,\"error\":\"Not authorized\"}");
    return;
  }
  JsonDocument response;
  response["ok"] = true;
  response["configured"] = homebridgeAddress.length() && homebridgeUsername.length();
  response["address"] = homebridgeAddress;
  response["username"] = homebridgeUsername;
  response["connected"] = false;
  if (response["configured"].as<bool>() && WiFi.status() == WL_CONNECTED) {
    String raw;
    String error;
    int status = 0;
    response["connected"] = homebridgeAuthorizedRequest(
      "GET", "/api/auth/check", "", raw, status, error);
    if (!response["connected"].as<bool>()) response["error"] = error;
  }
  String body;
  serializeJson(response, body);
  sendJson(200, body);
}

void handleHomebridgeControl() {
  if (!requestAuthorized()) {
    sendJson(403, "{\"ok\":false,\"error\":\"Not authorized\"}");
    return;
  }
  JsonDocument request(&psramJsonAllocator);
  if (deserializeJson(request, webServer.arg("plain"))) {
    sendJson(400, "{\"ok\":false,\"error\":\"Invalid Homebridge command\"}");
    return;
  }
  JsonObjectConst spec = request["homebridge"].as<JsonObjectConst>();
  if (spec.isNull()) spec = request.as<JsonObjectConst>();
  DeviceCommand command;
  String error;
  if (!homebridgeCommandFromJson(spec, command, error)) {
    JsonDocument response;
    response["ok"] = false;
    response["error"] = error;
    String body;
    serializeJson(response, body);
    sendJson(400, body);
    return;
  }
  if (!transmitHomebridgeCommand(command)) {
    sendJson(502, "{\"ok\":false,\"error\":\"Homebridge did not accept the command\"}");
    return;
  }
  sendJson(200, "{\"ok\":true,\"sent\":true}");
}

void handleBluetoothApi() {
  if (!requestAuthorized()) {
    sendJson(403, "{\"ok\":false,\"error\":\"Not authorized\"}");
    return;
  }
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, webServer.arg("plain"));
  String action = doc["action"] | "";
  if (error || !action.length()) {
    sendJson(400, "{\"ok\":false,\"error\":\"Choose start or forget\"}");
    return;
  }
  if (action == "start") {
    startBluetoothPairing();
  } else if (action == "forget") {
    bluetoothOn = true;
    applyBluetoothState();
    forgetBluetoothPairing();
  } else {
    sendJson(400, "{\"ok\":false,\"error\":\"Unknown Bluetooth action\"}");
    return;
  }
  sendJson(200, buildStatusJson());
}

void handleFileBackedDeviceDelete() {
  if (!requestAuthorized()) {
    sendJson(403, "{\"ok\":false,\"error\":\"Not authorized\"}");
    return;
  }
  if (!sdReady) {
    sendJson(503, "{\"ok\":false,\"error\":\"SD card unavailable\"}");
    return;
  }
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, webServer.arg("plain"));
  String path = doc["path"] | "";
  String lowerPath = path;
  lowerPath.toLowerCase();
  if (error || !path.startsWith("/devices/") || path.indexOf("..") >= 0 ||
      !lowerPath.endsWith(".ir")) {
    sendJson(400, "{\"ok\":false,\"error\":\"Invalid SD device path\"}");
    return;
  }
  if (!SD.exists(path)) {
    forgetSavedIrDeviceFile(path);
    sendJson(404, "{\"ok\":false,\"error\":\"The SD device file no longer exists\"}");
    return;
  }
  if (!SD.remove(path)) {
    sendJson(500, "{\"ok\":false,\"error\":\"Could not remove the SD device file\"}");
    return;
  }
  forgetSavedIrDeviceFile(path);
  sendJson(200, "{\"ok\":true,\"removed\":true}");
  pendingRuntimeReload = true;
  runtimeReloadAfterMs = millis() + 150UL;
}

void handleReboot() {
  if (!requestAuthorized()) {
    sendJson(403, "{\"ok\":false,\"error\":\"Not authorized\"}");
    return;
  }
  sendJson(200, "{\"ok\":true,\"restarting\":true}");
  restartPending = true;
}

void serveCaptivePortal() {
  String url = setupApActive
    ? String("http://192.168.4.1/?token=") + setupToken
    : webConfigUrl();
  String plainAddress = setupApActive ? "http://192.168.4.1" :
    String("http://") + WiFi.localIP().toString();
  String html =
    "<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>OpenRemote</title><style>body{margin:0;background:#07101c;color:#fff;font-family:-apple-system,BlinkMacSystemFont,sans-serif;display:grid;place-items:center;min-height:100vh}"
    "main{width:min(360px,calc(100% - 40px));text-align:center}i{display:block;width:72px;height:72px;border:10px solid #2f8cff;border-radius:50%;margin:0 auto 22px;box-shadow:0 0 30px #2f8cff66}"
    "h1{font-size:30px;margin:0 0 10px}p{color:#9fb1c9;line-height:1.5;margin:0 0 18px}a{display:block;background:#1677ef;color:#fff;text-decoration:none;font-weight:800;padding:15px;border-radius:8px}small{display:block;color:#7890ad;line-height:1.45;margin-top:18px}b{color:#d9e9ff}</style></head>"
    "<body><main><i></i><h1>OpenRemote</h1><p>The setup network is connected. Open WebConfig in your normal browser.</p><a href='" +
    url + "' onclick=\"location.href=this.href;return false\">Open WebConfig</a><small>If this setup window does not open your browser, open Safari, Chrome or Edge and enter <b>" +
    plainAddress + "</b>.</small></main></body></html>";
  webServer.sendHeader("Cache-Control", "no-store");
  webServer.send(200, "text/html; charset=utf-8", html);
}

void configureWebServer() {
  if (webServerConfigured) return;
  const char *headers[] = {"X-OpenRemote-Token"};
  webServer.collectHeaders(headers, 1);
  webServer.on("/", HTTP_GET, serveWebConfig);
  webServer.on("/index.html", HTTP_GET, serveWebConfig);
  webServer.on("/hotspot-detect.html", HTTP_GET, serveCaptivePortal);
  webServer.on("/library/test/success.html", HTTP_GET, serveCaptivePortal);
  webServer.on("/generate_204", HTTP_GET, serveCaptivePortal);
  webServer.on("/gen_204", HTTP_GET, serveCaptivePortal);
  webServer.on("/fwlink", HTTP_GET, serveCaptivePortal);
  webServer.on("/connecttest.txt", HTTP_GET, serveCaptivePortal);
  webServer.on("/api/status", HTTP_GET, handleStatusApi);
  webServer.on("/api/clock", HTTP_POST, handleClockSettingsApi);
  webServer.on("/api/wifi/networks", HTTP_GET, handleWifiNetworksApi);
  webServer.on("/api/themes/status", HTTP_GET, handleThemeStatusApi);
  webServer.on("/api/wifi/scan", HTTP_POST, handleWifiScanApi);
  webServer.on("/api/wifi/connect", HTTP_POST, handleWifiConnectApi);
  webServer.on("/api/wifi/forget", HTTP_POST, handleWifiForgetApi);
  webServer.on("/api/reboot", HTTP_POST, handleReboot);
  webServer.on("/api/config", HTTP_GET, handleRuntimeConfigDownload);
  webServer.on("/api/config", HTTP_POST, handleRuntimeConfigUpload,
               handleRuntimeConfigUploadData);
  webServer.on("/api/command/test", HTTP_POST, handleCommandTest);
  webServer.on("/api/homebridge/discover", HTTP_POST, handleHomebridgeDiscover);
  webServer.on("/api/homebridge/status", HTTP_GET, handleHomebridgeStatus);
  webServer.on("/api/homebridge/control", HTTP_POST, handleHomebridgeControl);
  webServer.on("/api/bluetooth/pair", HTTP_POST, handleBluetoothApi);
  webServer.on("/api/devices/file", HTTP_DELETE, handleFileBackedDeviceDelete);
  webServer.on("/api/ir/learn/start", HTTP_POST, handleIrLearnStart);
  webServer.on("/api/ir/learn/status", HTTP_GET, handleIrLearnStatus);
  webServer.on("/api/ir/learn/cancel", HTTP_POST, handleIrLearnCancel);
  webServer.on("/api/icons", HTTP_GET, handleIconList);
  webServer.on("/api/icons/custom", HTTP_POST, []() {
    if (!requestAuthorized()) webServer.send(403, "application/json", "{\"ok\":false}");
    else sendJson(customIconUploadOk ? 200 : 400,
                  customIconUploadOk
                    ? String("{\"ok\":true,\"path\":\"") + customIconUploadPath +
                      "\",\"name\":\"" + iconDisplayName(customIconUploadPath) + "\"}"
                    : "{\"ok\":false,\"error\":\"Custom icon upload failed\"}");
  }, handleCustomIconUploadData);
  webServer.on("/api/icons/custom", HTTP_DELETE, handleCustomIconDelete);
  webServer.on("/api/icons/custom/rename", HTTP_POST, handleCustomIconRename);
  webServer.on("/api/themes", HTTP_POST, []() {
    if (!requestAuthorized()) webServer.send(403, "application/json", "{\"ok\":false}");
    else sendJson(themeUploadOk ? 200 : 400,
                  themeUploadOk
                    ? String("{\"ok\":true,\"path\":\"") + themeUploadPath + "\"}"
                    : "{\"ok\":false,\"error\":\"Theme upload failed\"}");
  }, handleThemeUploadData);
  webServer.on("/api/irdb/search", HTTP_GET, handleIrdbSearch);
  webServer.on("/api/irdb/detail", HTTP_GET, handleIrdbDetail);
  webServer.on("/api/irdb", HTTP_GET, handleIrdbDownload);
  webServer.on("/api/irdb", HTTP_POST, []() {
    if (!requestAuthorized()) webServer.send(403, "application/json", "{\"ok\":false}");
    else sendJson(irdbUploadOk ? 200 : 400,
                  irdbUploadOk ? "{\"ok\":true}" : "{\"ok\":false,\"error\":\"IRDB upload failed; use OpenRemote Studio's .irdb file\"}");
  }, handleIrdbUploadData);
  webServer.on("/api/sd/format", HTTP_POST, handleSdRebuild);
  webServer.on("/api/factory-reset", HTTP_POST, handleFactoryReset);
  webServer.on("/api/backups", HTTP_GET, handleBackupList);
  webServer.on("/api/backups/file", HTTP_GET, handleBackupDownload);
  webServer.on("/api/backups/file", HTTP_DELETE, handleBackupDelete);
  webServer.on("/api/backups/file", HTTP_POST, []() {
    if (!requestAuthorized()) webServer.send(403, "application/json", "{\"ok\":false}");
    else sendJson(backupUploadOk ? 200 : 400,
                  backupUploadOk
                    ? String("{\"ok\":true,\"path\":\"") + backupUploadPath + "\"}"
                    : "{\"ok\":false,\"error\":\"Backup upload failed\"}");
  }, handleBackupUploadData);
  webServer.on("/api/firmware", HTTP_POST, []() {
    if (!requestAuthorized()) webServer.send(403, "application/json", "{\"ok\":false}");
    else {
      sendJson(firmwareUploadOk ? 200 : 400,
               firmwareUploadOk ? "{\"ok\":true,\"restarting\":true}" : "{\"ok\":false,\"error\":\"Firmware upload failed; use an ESP32-S3 .bin file\"}");
      restartPending = firmwareUploadOk;
    }
  }, handleFirmwareUploadData);
  webServer.on("/api/firmware/stage", HTTP_POST, []() {
    if (!requestAuthorized()) webServer.send(403, "application/json", "{\"ok\":false}");
    else sendJson(firmwareStageOk ? 200 : 400,
                  firmwareStageOk
                    ? String("{\"ok\":true,\"bytes\":") + firmwareStageBytes + "}"
                    : "{\"ok\":false,\"error\":\"Firmware staging failed; select a valid ESP32 .bin file\"}");
  }, handleFirmwareStageUploadData);
  webServer.on("/api/firmware/install", HTTP_POST, handleFirmwareInstall);
  webServer.on("/api/webconfig", HTTP_POST, []() {
    if (!requestAuthorized()) webServer.send(403, "application/json", "{\"ok\":false}");
    else sendJson(webConfigUploadOk ? 200 : 400,
                  webConfigUploadOk ? "{\"ok\":true}" : "{\"ok\":false,\"error\":\"WebConfig upload failed; use a complete .html file\"}");
  }, handleWebConfigUploadData);
  webServer.serveStatic("/icons/", SD, "/icons/", "max-age=300");
  webServer.serveStatic("/themes/", SD, "/themes/", "max-age=300");
  webServer.onNotFound([]() {
    if (webServer.uri().startsWith("/api/")) sendJson(404, "{\"ok\":false,\"error\":\"Not found\"}");
    else serveWebConfig();
  });
  webServerConfigured = true;
}

void webServerTask(void *parameter) {
  Serial.printf("WebConfig HTTP worker: core %d, priority %u\n",
                xPortGetCoreID(), (unsigned)uxTaskPriorityGet(nullptr));
  for (;;) {
    if (webServerStopRequested) {
      WiFiClient activeClient = webServer.client();
      if (activeClient) activeClient.stop();
      webServer.stop();
      webServerStarted = false;
      webServerStopRequested = false;
      webConfigTransferActive = false;
      Serial.println("WebConfig server: stopped");
    }

    if (webServerListenRequested && webServerConfigured) {
      if (webServerStarted || webServerRebindRequested) webServer.stop();
      webServer.begin();
      webServerStarted = true;
      webServerListenRequested = false;
      webServerRebindRequested = false;
      webServerStopRequested = false;
      webConfigTransferCancelRequested = false;
      Serial.println("WebConfig server: listening on port 80");
    }

    if (webServerStarted) {
      webServer.handleClient();
      vTaskDelay(1);
    } else {
      // Consume no scheduler time or battery while WebConfig is unavailable.
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }
  }
}

void ensureWebServerTask() {
  if (webServerTaskHandle) return;
  BaseType_t created = xTaskCreatePinnedToCore(
    webServerTask, "openremote_http", 12288, nullptr, 1,
    &webServerTaskHandle, 0);
  if (created != pdPASS) {
    webServerTaskHandle = nullptr;
    Serial.println("WebConfig HTTP worker: could not start");
  }
}

void requestWebServerListen(bool rebind) {
  configureWebServer();
  ensureWebServerTask();
  if (!webServerTaskHandle) return;
  webServerRebindRequested = webServerRebindRequested || rebind;
  webServerStopRequested = false;
  webServerListenRequested = true;
  xTaskNotifyGive(webServerTaskHandle);
}

void requestWebServerStop() {
  webConfigTransferCancelRequested = true;
  webServerListenRequested = false;
  webServerStopRequested = true;
  if (webServerTaskHandle) xTaskNotifyGive(webServerTaskHandle);
}

bool requestWebServerStopAndWait(uint32_t timeoutMs) {
  requestWebServerStop();
  unsigned long started = millis();
  while ((webServerStarted || webServerStopRequested) &&
         (uint32_t)(millis() - started) < timeoutMs) {
    delay(2);
  }
  return !webServerStarted && !webServerStopRequested;
}

void ensureSetupIdentity() {
  if (setupApSsid.length() && setupToken.length()) return;
  uint64_t chipId = ESP.getEfuseMac();
  char suffix[5];
  snprintf(suffix, sizeof(suffix), "%04X", (uint16_t)(chipId & 0xFFFF));
  setupApSsid = String("OpenRemote-") + suffix;
  setupToken = String((uint32_t)(chipId >> 16), HEX) + String((uint32_t)chipId, HEX);
}

bool recoverWifiRadio(wifi_mode_t targetMode, const char *reason) {
  if (webServerStarted && !requestWebServerStopAndWait()) {
    Serial.printf("Wi-Fi: HTTP worker blocked recovery for %s\n",
                  reason ? reason : "operation");
    return false;
  }
  if (wifiScanPending) WiFi.scanDelete();
  wifiScanPending = false;
  wifiScanStartPending = false;
  MDNS.end();
  WiFi.persistent(false);
  WiFi.setAutoReconnect(false);
  // Always perform a cold radio transition. Leaving STA partially alive while
  // BLE owns the shared RF hardware is what made scans/AP startup remain dead
  // until a physical ESP32 reset.
  WiFi.disconnect(false, false);
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  networkStackActive = false;
  delay(180);

  for (uint8_t attempt = 1; attempt <= 3; attempt++) {
    if (WiFi.mode(targetMode)) {
      delay(120);
      if (WiFi.getMode() == targetMode) {
        WiFi.setSleep(false);
        Serial.printf("Wi-Fi: radio recovered for %s on attempt %u\n",
                      reason ? reason : "operation", (unsigned)attempt);
        return true;
      }
    }
    WiFi.mode(WIFI_OFF);
    delay(180);
  }
  Serial.printf("Wi-Fi: radio recovery failed for %s\n",
                reason ? reason : "operation");
  return false;
}

void startNetworkStack() {
  if (!wifiOn) return;
  ensureSetupIdentity();

  networkShutdownAtMs = 0;
  WiFi.persistent(false);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);

  wifi_mode_t actualMode = WiFi.getMode();
  bool stationMode = actualMode == WIFI_STA || actualMode == WIFI_AP_STA;
  if (!stationMode) networkStackActive = false;
  if (WiFi.status() == WL_CONNECTED && stationMode) {
    networkStackActive = true;
    if (MDNS.begin("openremote")) MDNS.addService("http", "tcp", 80);
    return;
  }
  if (!recoverWifiRadio(WIFI_STA, "station startup")) return;
  WiFi.setAutoReconnect(true);

  if (hasSelectedWifiProfile()) {
    String password = savedWifiPassword(selectedWifiSsid);
    WiFi.begin(selectedWifiSsid.c_str(), password.c_str());
    wifiConnectStartedMs = millis();
  }
  networkStackActive = true;
  if (MDNS.begin("openremote")) MDNS.addService("http", "tcp", 80);
  Serial.println("Wi-Fi: station mode ready");
}

void parkNetworkStackForBle() {
  stopNetworkStack();
  Serial.println("Wi-Fi: fully stopped for BLE coexistence");
}

void startSetupAccessPoint() {
  if (setupApActive) return;
  ensureSetupIdentity();
  if (!requestWebServerStopAndWait()) return;

  if (wifiScanPending) {
    WiFi.scanDelete();
    wifiScanPending = false;
    wifiScanStartPending = false;
  }
  if (bleReady) {
    setupApPausedBle = true;
    stopBluetoothRadio("setup AP");
    delay(120);
  }

  // Keep this as close as possible to the stock Arduino ESP32 open-AP example.
  // Phones must associate first; DNS/WebServer captive routing happens after.
  bool apStarted = false;
  for (uint8_t attempt = 1; attempt <= 3 && !apStarted; attempt++) {
    if (!recoverWifiRadio(WIFI_AP, "setup AP")) continue;
    apStarted = WiFi.softAP(setupApSsid.c_str());
    if (!apStarted) {
      WiFi.mode(WIFI_OFF);
      delay(220);
    }
  }
  if (!apStarted) {
    Serial.println("Setup AP: failed to start");
    if (setupApPausedBle && bluetoothOn) {
      setupApPausedBle = false;
      applyBluetoothState();
    }
    return;
  }
  delay(250);
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  dnsServerStarted = true;
  setupApActive = true;
  // The core 0 worker owns the listener and rebinds it after the interface
  // transition, without blocking the LCD/touch loop on core 1.
  requestWebServerListen(true);
  lastWakeMs = millis();
  Serial.printf("Setup AP: %s / open / %s\n", setupApSsid.c_str(),
                WiFi.softAPIP().toString().c_str());
}

void stopSetupAccessPoint(bool resumeStation) {
  bool setupWasActive = dnsServerStarted || setupApActive || setupApPausedBle;
  if (dnsServerStarted) {
    dnsServer.stop();
    dnsServerStarted = false;
  }
  if (setupApActive) {
    WiFi.softAPdisconnect(true);
    setupApActive = false;
    networkStackActive = false;
    Serial.println("Setup AP: off");
  }
  if (setupApPausedBle) {
    setupApPausedBle = false;
    if (bluetoothOn) applyBluetoothState();
  }
  if (resumeStation && wifiOn) startNetworkStack();
  if (setupWasActive) lastWakeMs = millis();
}

void stopNetworkStack() {
  requestWebServerStopAndWait(800UL);
  stopSetupAccessPoint(false);
  if (wifiScanPending) WiFi.scanDelete();
  wifiScanPending = false;
  wifiScanStartPending = false;
  MDNS.end();
  if (WiFi.getMode() != WIFI_OFF) {
    WiFi.disconnect(false, false);
    WiFi.mode(WIFI_OFF);
  }
  networkStackActive = false;
  Serial.println("Wi-Fi: off");
}

// ---------------------------------------------------------------------------
// LVGL display/touch bridge
// ---------------------------------------------------------------------------

void *allocateDisplayMemory(size_t bytes) {
  void *memory = psramFound() ? ps_malloc(bytes) : nullptr;
  return memory ? memory : malloc(bytes);
}

bool ensureDisplayFlushBuffers(size_t pixelCount = LCD_W * 32) {
  if (displayFlush565 && displayFlushPixelCapacity >= pixelCount) return true;

  if (displayFlush565) {
    free(displayFlush565);
    displayFlush565 = nullptr;
    displayFlushPixelCapacity = 0;
  }

  // LVGL never emits more than one 32-row band with this partial draw buffer.
  // The calibrated source is passed directly to pushPixelsDMA(), so it must
  // live in internal DMA-capable memory rather than PSRAM.
  const size_t allocationPixels = max(pixelCount, (size_t)(LCD_W * 32));
  displayFlush565 = static_cast<uint16_t *>(
    heap_caps_malloc(allocationPixels * sizeof(uint16_t),
                     MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
  if (!displayFlush565) return false;
  displayFlushPixelCapacity = allocationPixels;
  Serial.printf("LCD DMA staging: %lu pixels, DMA=%s, internal=%s\n",
                (unsigned long)displayFlushPixelCapacity,
                esp_ptr_dma_capable(displayFlush565) ? "yes" : "no",
                esp_ptr_internal(displayFlush565) ? "yes" : "no");
  return true;
}

void rebuildDisplayColourLut() {
  displayColourLutActive = displayGamma != 100 || displaySaturation != 100;
  if (!displayColourLutActive) return;
  if (!displayColourLut) {
    displayColourLut = static_cast<uint16_t *>(allocateDisplayMemory(65536UL * sizeof(uint16_t)));
  }
  if (!displayColourLut) {
    displayColourLutActive = false;
    Serial.println("Display calibration LUT allocation failed");
    return;
  }

  const float saturation = displaySaturation / 100.0f;
  const float inverseGamma = 100.0f / displayGamma;
  for (uint32_t colour = 0; colour < 65536UL; colour++) {
    float red = ((colour >> 11) & 0x1F) / 31.0f;
    float green = ((colour >> 5) & 0x3F) / 63.0f;
    float blue = (colour & 0x1F) / 31.0f;
    const float luminance = 0.2126f * red + 0.7152f * green + 0.0722f * blue;
    red = constrain(luminance + (red - luminance) * saturation, 0.0f, 1.0f);
    green = constrain(luminance + (green - luminance) * saturation, 0.0f, 1.0f);
    blue = constrain(luminance + (blue - luminance) * saturation, 0.0f, 1.0f);
    red = powf(red, inverseGamma);
    green = powf(green, inverseGamma);
    blue = powf(blue, inverseGamma);
    displayColourLut[colour] =
      ((uint16_t)roundf(red * 31.0f) << 11) |
      ((uint16_t)roundf(green * 63.0f) << 5) |
      (uint16_t)roundf(blue * 31.0f);
  }
  Serial.printf("Display calibration: gamma %.2f, saturation %u%%\n",
                displayGamma / 100.0f, displaySaturation);
}

void applyDisplayControllerSettings() {
  if (!lcdControllerReady) return;
  ensureDisplayFlushBuffers();
  tft.waitDMA();
  tft.setColorDepth(displayRgb666 ? 18 : 16);
  tft.invertDisplay(displayInverted);
  Serial.printf("LCD transfer: %s, inversion: %s\n",
                displayRgb666 ? "RGB666" : "RGB565", displayInverted ? "on" : "off");
}

void setLcdControllerSleeping(bool sleeping) {
  if (!lcdControllerReady) return;
  tft.waitDMA();
  if (sleeping) {
    tft.sleep();
  } else {
    tft.wakeup();
    applyDisplayControllerSettings();
  }
}

void lvFlush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *colour) {
  int32_t w = area->x2 - area->x1 + 1;
  int32_t h = area->y2 - area->y1 + 1;
  const uint32_t pixelCount = (uint32_t)w * h;
  uint16_t *source = reinterpret_cast<uint16_t *>(colour);

  // Colour calibration uses one shared staging buffer. Keep it owned by the
  // display bus until each band has fully transferred before reusing it.
  tft.waitDMA();
  if (displayColourLutActive && ensureDisplayFlushBuffers(pixelCount)) {
    for (uint32_t i = 0; i < pixelCount; i++) displayFlush565[i] = displayColourLut[source[i]];
    source = displayFlush565;
  }

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushPixelsDMA(source, pixelCount);
  tft.waitDMA();
  tft.endWrite();
  lv_disp_flush_ready(disp);
}

void lvMonitor(lv_disp_drv_t *disp, uint32_t time, uint32_t pixels) {
  (void)disp;
  (void)time;
  (void)pixels;
  debugDisplayFrameCount++;
}

bool lvSdReady(lv_fs_drv_t *drv) {
  (void)drv;
  return sdReady;
}

void *lvSdOpen(lv_fs_drv_t *drv, const char *path, lv_fs_mode_t mode) {
  (void)drv;
  if (!sdReady || !(mode & LV_FS_MODE_RD)) return nullptr;
  File opened = SD.open(path, FILE_READ);
  if (!opened) return nullptr;
  return new File(opened);
}

lv_fs_res_t lvSdClose(lv_fs_drv_t *drv, void *filePointer) {
  (void)drv;
  File *file = static_cast<File *>(filePointer);
  if (!file) return LV_FS_RES_INV_PARAM;
  file->close();
  delete file;
  return LV_FS_RES_OK;
}

lv_fs_res_t lvSdRead(lv_fs_drv_t *drv, void *filePointer, void *buffer,
                     uint32_t bytesToRead, uint32_t *bytesRead) {
  (void)drv;
  File *file = static_cast<File *>(filePointer);
  if (!file) return LV_FS_RES_INV_PARAM;
  *bytesRead = file->read(static_cast<uint8_t *>(buffer), bytesToRead);
  return LV_FS_RES_OK;
}

lv_fs_res_t lvSdSeek(lv_fs_drv_t *drv, void *filePointer, uint32_t position,
                     lv_fs_whence_t whence) {
  (void)drv;
  File *file = static_cast<File *>(filePointer);
  if (!file) return LV_FS_RES_INV_PARAM;
  uint32_t target = position;
  if (whence == LV_FS_SEEK_CUR) target = file->position() + position;
  else if (whence == LV_FS_SEEK_END) target = file->size() + position;
  return file->seek(target) ? LV_FS_RES_OK : LV_FS_RES_FS_ERR;
}

lv_fs_res_t lvSdTell(lv_fs_drv_t *drv, void *filePointer, uint32_t *position) {
  (void)drv;
  File *file = static_cast<File *>(filePointer);
  if (!file) return LV_FS_RES_INV_PARAM;
  *position = file->position();
  return LV_FS_RES_OK;
}

void clearPageIconCache() {
  if (!boundPageUi) return;
  lv_img_cache_invalidate_src(nullptr);
  for (uint8_t i = 0; i < boundPageUi->iconCacheCount; i++) {
    free(boundPageUi->iconCache[i].pixels);
    boundPageUi->iconCache[i] = {};
  }
  boundPageUi->iconCacheCount = 0;
}

const void *cachedPageIconSource(const char *path) {
  if (!boundPageUi || !path || !path[0]) return path;
  for (uint8_t i = 0; i < boundPageUi->iconCacheCount; i++) {
    if (strcmp(boundPageUi->iconCache[i].path, path) == 0) {
      return &boundPageUi->iconCache[i].descriptor;
    }
  }
  const char *extension = strrchr(path, '.');
  if (!extension || strcasecmp(extension, ".png") != 0 ||
      boundPageUi->iconCacheCount >= MAX_PAGE_ICON_CACHE) return path;

  lv_img_decoder_dsc_t decoded;
  if (lv_img_decoder_open(&decoded, path, lv_color_black(), 0) != LV_RES_OK ||
      !decoded.img_data || decoded.header.cf != LV_IMG_CF_TRUE_COLOR_ALPHA) {
    lv_img_decoder_close(&decoded);
    return path;
  }

  size_t bytes = LV_IMG_BUF_SIZE_TRUE_COLOR_ALPHA(decoded.header.w, decoded.header.h);
  uint8_t *pixels = static_cast<uint8_t *>(psramFound() ? ps_malloc(bytes) : malloc(bytes));
  if (!pixels) {
    lv_img_decoder_close(&decoded);
    return path;
  }
  memcpy(pixels, decoded.img_data, bytes);

  CachedPageIcon &entry = boundPageUi->iconCache[boundPageUi->iconCacheCount++];
  strlcpy(entry.path, path, sizeof(entry.path));
  entry.pixels = pixels;
  entry.descriptor.header = decoded.header;
  entry.descriptor.data_size = bytes;
  entry.descriptor.data = pixels;
  lv_img_decoder_close(&decoded);
  return &entry.descriptor;
}

const lv_font_t *fontForSize(uint8_t size) {
  if (size <= 10) return &lv_font_montserrat_10;
  if (size <= 12) return &lv_font_montserrat_12;
  if (size <= 14) return &lv_font_montserrat_14;
  if (size <= 16) return &lv_font_montserrat_16;
  if (size <= 18) return &lv_font_montserrat_18;
  if (size <= 20) return &lv_font_montserrat_20;
  if (size <= 22) return &lv_font_montserrat_22;
  return &lv_font_montserrat_24;
}

bool applyRuntimeTheme(const char *path) {
  if (!boundPageUi || !wallpaper) return false;
  activeRuntimeThemeStyle = findRuntimeThemeStyle(path);
  if (path && strcmp(path, "builtin:burnt-orange") == 0) {
    if (boundPageUi->wallpaperPixels) {
      lv_img_set_src(wallpaper, nullptr);
      lv_img_cache_invalidate_src(&boundPageUi->wallpaperDescriptor);
      free(boundPageUi->wallpaperPixels);
      boundPageUi->wallpaperPixels = nullptr;
      memset(&boundPageUi->wallpaperDescriptor, 0,
             sizeof(boundPageUi->wallpaperDescriptor));
    }
    strlcpy(boundPageUi->themePath, path, sizeof(boundPageUi->themePath));
    lv_img_set_src(wallpaper, &cinemaWallpaper);
    lv_obj_clear_flag(wallpaper, LV_OBJ_FLAG_HIDDEN);
    return true;
  }
  if (path && path[0] && boundPageUi->wallpaperPixels &&
      strcmp(path, boundPageUi->themePath) == 0) {
    lv_img_set_src(wallpaper, &boundPageUi->wallpaperDescriptor);
    lv_obj_clear_flag(wallpaper, LV_OBJ_FLAG_HIDDEN);
    return true;
  }
  if (!path || !path[0] || !sdReady || !SD.exists(path)) {
    if (boundPageUi->wallpaperPixels) {
      lv_img_set_src(wallpaper, nullptr);
      lv_img_cache_invalidate_src(&boundPageUi->wallpaperDescriptor);
      free(boundPageUi->wallpaperPixels);
      boundPageUi->wallpaperPixels = nullptr;
      memset(&boundPageUi->wallpaperDescriptor, 0,
             sizeof(boundPageUi->wallpaperDescriptor));
    }
    boundPageUi->themePath[0] = '\0';
    lv_obj_add_flag(wallpaper, LV_OBJ_FLAG_HIDDEN);
    return false;
  }
  File file = SD.open(path, FILE_READ);
  const size_t expected = LCD_W * LCD_H * sizeof(uint16_t);
  if (!file || file.size() != expected) {
    if (file) file.close();
    lv_obj_add_flag(wallpaper, LV_OBJ_FLAG_HIDDEN);
    return false;
  }
  uint16_t *pixels = static_cast<uint16_t *>(ps_malloc(expected));
  if (!pixels || file.read(reinterpret_cast<uint8_t *>(pixels), expected) != expected) {
    if (pixels) free(pixels);
    file.close();
    lv_obj_add_flag(wallpaper, LV_OBJ_FLAG_HIDDEN);
    return false;
  }
  file.close();
  if (boundPageUi->wallpaperPixels) {
    lv_img_set_src(wallpaper, nullptr);
    lv_img_cache_invalidate_src(&boundPageUi->wallpaperDescriptor);
    free(boundPageUi->wallpaperPixels);
  }
  boundPageUi->wallpaperPixels = pixels;
  boundPageUi->wallpaperDescriptor.header.always_zero = 0;
  boundPageUi->wallpaperDescriptor.header.w = LCD_W;
  boundPageUi->wallpaperDescriptor.header.h = LCD_H;
  boundPageUi->wallpaperDescriptor.header.cf = LV_IMG_CF_TRUE_COLOR;
  boundPageUi->wallpaperDescriptor.data_size = expected;
  boundPageUi->wallpaperDescriptor.data =
    reinterpret_cast<const uint8_t *>(boundPageUi->wallpaperPixels);
  strlcpy(boundPageUi->themePath, path, sizeof(boundPageUi->themePath));
  lv_img_set_src(wallpaper, &boundPageUi->wallpaperDescriptor);
  lv_obj_clear_flag(wallpaper, LV_OBJ_FLAG_HIDDEN);
  return true;
}

void addTouchTrailPoint(uint16_t x, uint16_t y, unsigned long now, bool force = false) {
  if (!debugTouchEnabled || (!force && now - lastTouchTrailPointMs < 70UL)) return;
  TouchTrailPoint &point = touchTrail[nextTouchTrailPoint];
  nextTouchTrailPoint = (nextTouchTrailPoint + 1) % TOUCH_TRAIL_POINT_COUNT;
  if (!point.dot || !lv_obj_is_valid(point.dot)) return;
  point.createdMs = now;
  point.active = true;
  lastTouchTrailPointMs = now;
  lv_obj_set_pos(point.dot, constrain((int)x - 3, 0, LCD_W - 6),
                 constrain((int)y - 3, 0, LCD_H - 6));
  lv_obj_set_style_bg_opa(point.dot, LV_OPA_COVER, 0);
  lv_obj_clear_flag(point.dot, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(point.dot);
}

void showLiveTouchDiagnostic(uint16_t x, uint16_t y, unsigned long now) {
  if (!debugTouchEnabled) return;
  if (touchDot) {
    lv_obj_clear_flag(touchDot, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_pos(touchDot, constrain((int)x - 22, 0, LCD_W - 44),
                   constrain((int)y - 22, 0, LCD_H - 44));
    lv_obj_move_foreground(touchDot);
  }
  if (touchDiagnosticLabel) {
    char coordinates[18];
    snprintf(coordinates, sizeof(coordinates), "T %u,%u", x, y);
    lv_label_set_text(touchDiagnosticLabel, coordinates);
    lv_obj_set_style_text_color(touchDiagnosticLabel, lv_color_hex(0xFF3C45), 0);
    lv_obj_clear_flag(touchDiagnosticLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(touchDiagnosticLabel);
  }
  touchDiagnosticHoldUntilMs = 0;
  addTouchTrailPoint(x, y, now);
}

void holdReleasedTouchDiagnostic(uint16_t x, uint16_t y, unsigned long now) {
  if (!debugTouchEnabled) return;
  if (touchDot) lv_obj_add_flag(touchDot, LV_OBJ_FLAG_HIDDEN);
  addTouchTrailPoint(x, y, now, true);
  if (touchDiagnosticLabel) {
    char coordinates[18];
    snprintf(coordinates, sizeof(coordinates), "T %u,%u", x, y);
    lv_label_set_text(touchDiagnosticLabel, coordinates);
    lv_obj_set_style_text_color(touchDiagnosticLabel, lv_color_hex(0xFF9D2E), 0);
    lv_obj_clear_flag(touchDiagnosticLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(touchDiagnosticLabel);
  }
  touchDiagnosticHoldUntilMs = now + 5000UL;
}

void lvTouchRead(lv_indev_drv_t *drv, lv_indev_data_t *data) {
  uint16_t x = 0;
  uint16_t y = 0;
  unsigned long now = millis();

  if (pageStripRendering) {
    data->state = LV_INDEV_STATE_REL;
    lvTouchDown = false;
    touchWasDown = false;
    touchPendingConfirmCount = 0;
    return;
  }

  if (touchQuarantineActive) {
    uint16_t ignoredX = 0;
    uint16_t ignoredY = 0;
    TouchSampleStatus status = touchFound
      ? readTouchSample(ignoredX, ignoredY) : TOUCH_SAMPLE_RELEASED;
    if (status == TOUCH_SAMPLE_RELEASED) {
      if (!touchReleasedSinceMs) touchReleasedSinceMs = now;
      if ((int32_t)(now - touchAcceptAfterMs) >= 0 &&
          (uint32_t)(now - touchReleasedSinceMs) >= 120UL) {
        touchQuarantineActive = false;
        touchReleasedSinceMs = 0;
        Serial.println("Touch: stable release accepted");
      }
    } else {
      touchReleasedSinceMs = 0;
    }
    // A PRESSED or INVALID read above resets the wait indefinitely, which can
    // leave the quarantine stuck forever if the bus is noisy (e.g. right after
    // a light-sleep wake) or a finger is already down when it starts. Force a
    // clear after a bounded wait and, since that most likely means the I2C
    // bus itself is wedged, run the existing (previously unused) rail-cycle
    // recovery instead of just resuming with a possibly-dead controller.
    if (touchQuarantineActive &&
        (uint32_t)(now - touchQuarantineStartedMs) >= 2000UL) {
      Serial.println("Touch: quarantine timeout, forcing recovery");
      touchQuarantineActive = false;
      touchReleasedSinceMs = 0;
      if (touchFound) recoverTouchControllerPower();
    }
    data->state = LV_INDEV_STATE_REL;
    lvTouchDown = false;
    touchWasDown = false;
    touchPendingConfirmCount = 0;
    return;
  }

  if (touchFound && readTouch(x, y)) {
    if (!touchWasDown) {
      // A single spurious sample with otherwise-valid, in-range coordinates
      // is the signature of electrical/capacitive noise (worst right after a
      // sleep/wake power transition), not a real finger, which stays down for
      // many consecutive poll cycles. Require one confirming read at a
      // similar position before treating this as a genuine new touch-down.
      if (touchPendingConfirmCount == 0 ||
          abs((int)x - (int)touchPendingX) > 12 ||
          abs((int)y - (int)touchPendingY) > 12) {
        touchPendingConfirmCount = 1;
        touchPendingX = x;
        touchPendingY = y;
        data->state = LV_INDEV_STATE_REL;
        lvTouchDown = false;
        return;
      }
      touchPendingConfirmCount = 0;
    }
    data->state = LV_INDEV_STATE_PR;
    lvTouchDown = true;
    data->point.x = x;
    data->point.y = y;
    bool touchMoved = !touchWasDown ||
      abs((int)x - (int)touchLastX) > 1 ||
      abs((int)y - (int)touchLastY) > 1;
    if (!touchWasDown) {
      touchWasDown = true;
      touchStartX = x;
      touchStartY = y;
    }
    touchLastX = x;
    touchLastY = y;

    if (brightnessOverlay) brightnessLastActivityMs = millis();
    showLiveTouchDiagnostic(x, y, now);
    if (touchMoved) lastWakeMs = millis();
  } else {
    touchPendingConfirmCount = 0;
    data->state = LV_INDEV_STATE_REL;
    lvTouchDown = false;
    if (touchWasDown) {
      holdReleasedTouchDiagnostic(touchLastX, touchLastY, now);
      touchWasDown = false;
    }
    if (touchDot && !debugTouchEnabled) lv_obj_add_flag(touchDot, LV_OBJ_FLAG_HIDDEN);
  }
}

// ---------------------------------------------------------------------------
// LVGL styling and object helpers
// ---------------------------------------------------------------------------

lv_color_t lvRgb(uint8_t r, uint8_t g, uint8_t b) {
  return lv_color_make(r, g, b);
}

lv_color_t textPrimary() {
  // The 1-bpp fonts have no blended edge pixels, so true white remains sharp.
  return lv_color_white();
}

void stylePanel(lv_obj_t *obj, lv_color_t bg, lv_color_t border, lv_opa_t opa = LV_OPA_COVER) {
  lv_obj_set_style_bg_color(obj, bg, 0);
  lv_obj_set_style_bg_opa(obj, opa, 0);
  lv_obj_set_style_border_color(obj, border, 0);
  lv_obj_set_style_border_width(obj, 1, 0);
  lv_obj_set_style_radius(obj, 10, 0);
  lv_obj_set_style_pad_all(obj, 8, 0);
}

lv_obj_t *makeLabel(lv_obj_t *parent, const char *text, int x, int y, const lv_font_t *font, lv_color_t colour) {
  lv_obj_t *label = lv_label_create(parent);
  lv_label_set_text(label, text);
  lv_obj_set_style_text_font(label, font, 0);
  lv_obj_set_style_text_color(label, colour, 0);
  lv_obj_set_pos(label, x, y);
  lv_obj_clear_flag(label, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(label, LV_OBJ_FLAG_SCROLLABLE);
  return label;
}

lv_obj_t *makeButton(lv_obj_t *parent, const char *text, int x, int y, int w, int h, lv_color_t bg) {
  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_set_pos(btn, x, y);
  lv_obj_set_size(btn, w, h);
  lv_obj_add_flag(btn, LV_OBJ_FLAG_GESTURE_BUBBLE);
  lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_color_filter_opa(btn, LV_OPA_TRANSP, LV_STATE_PRESSED);
  stylePanel(btn, bg, lvRgb(70, 95, 130), LV_OPA_COVER);
  lv_obj_t *label = lv_label_create(btn);
  lv_label_set_text(label, text);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(label, textPrimary(), 0);
  lv_obj_clear_flag(label, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(label, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_center(label);
  return btn;
}

void physicalVoicePulseAnimation(void *target, int32_t size) {
  lv_obj_t *pulse = static_cast<lv_obj_t *>(target);
  if (!pulse || !physicalVoiceOverlay) return;
  lv_obj_set_size(pulse, size, size);
  lv_obj_align(pulse, LV_ALIGN_CENTER, 0, -16);
  lv_obj_set_style_border_opa(
    pulse, static_cast<lv_opa_t>(LV_OPA_80 - ((size - 88) * 56 / 70)), 0);
}

void createPhysicalVoiceOverlay() {
  if (physicalVoiceOverlay) return;

  physicalVoiceOverlay = lv_obj_create(lv_layer_top());
  lv_obj_set_pos(physicalVoiceOverlay, 0, 0);
  lv_obj_set_size(physicalVoiceOverlay, LCD_W, LCD_H);
  lv_obj_clear_flag(physicalVoiceOverlay, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(physicalVoiceOverlay, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_bg_color(physicalVoiceOverlay, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(physicalVoiceOverlay, LV_OPA_80, 0);
  lv_obj_set_style_border_width(physicalVoiceOverlay, 0, 0);
  lv_obj_set_style_radius(physicalVoiceOverlay, 0, 0);
  lv_obj_set_style_pad_all(physicalVoiceOverlay, 0, 0);

  physicalVoicePulse = lv_obj_create(physicalVoiceOverlay);
  lv_obj_set_size(physicalVoicePulse, 88, 88);
  lv_obj_align(physicalVoicePulse, LV_ALIGN_CENTER, 0, -16);
  lv_obj_clear_flag(physicalVoicePulse, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(physicalVoicePulse, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_bg_opa(physicalVoicePulse, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_color(physicalVoicePulse, lvRgb(45, 143, 255), 0);
  lv_obj_set_style_border_width(physicalVoicePulse, 4, 0);
  lv_obj_set_style_border_opa(physicalVoicePulse, LV_OPA_80, 0);
  lv_obj_set_style_radius(physicalVoicePulse, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_pad_all(physicalVoicePulse, 0, 0);

  lv_obj_t *micBody = lv_obj_create(physicalVoiceOverlay);
  lv_obj_set_size(micBody, 46, 72);
  lv_obj_align(micBody, LV_ALIGN_CENTER, 0, -24);
  lv_obj_clear_flag(micBody, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(micBody, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_bg_color(micBody, lvRgb(45, 143, 255), 0);
  lv_obj_set_style_bg_opa(micBody, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(micBody, 0, 0);
  lv_obj_set_style_radius(micBody, 23, 0);
  lv_obj_set_style_pad_all(micBody, 0, 0);

  lv_obj_t *micStem = lv_obj_create(physicalVoiceOverlay);
  lv_obj_set_size(micStem, 8, 27);
  lv_obj_align(micStem, LV_ALIGN_CENTER, 0, 25);
  lv_obj_clear_flag(micStem, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(micStem, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_bg_color(micStem, lvRgb(45, 143, 255), 0);
  lv_obj_set_style_bg_opa(micStem, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(micStem, 0, 0);
  lv_obj_set_style_radius(micStem, 4, 0);
  lv_obj_set_style_pad_all(micStem, 0, 0);

  lv_obj_t *micBase = lv_obj_create(physicalVoiceOverlay);
  lv_obj_set_size(micBase, 58, 8);
  lv_obj_align(micBase, LV_ALIGN_CENTER, 0, 38);
  lv_obj_clear_flag(micBase, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(micBase, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_bg_color(micBase, lvRgb(45, 143, 255), 0);
  lv_obj_set_style_bg_opa(micBase, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(micBase, 0, 0);
  lv_obj_set_style_radius(micBase, 4, 0);
  lv_obj_set_style_pad_all(micBase, 0, 0);

  lv_obj_t *title = makeLabel(physicalVoiceOverlay, "LISTENING", 0, 0,
                              &lv_font_montserrat_20, textPrimary());
  lv_obj_align(title, LV_ALIGN_CENTER, 0, 78);
  lv_obj_t *hint = makeLabel(physicalVoiceOverlay, "Release button to search", 0, 0,
                             &lv_font_montserrat_10, lvRgb(135, 195, 255));
  lv_obj_align(hint, LV_ALIGN_CENTER, 0, 105);
  lv_obj_add_flag(physicalVoiceOverlay, LV_OBJ_FLAG_HIDDEN);
}

void showPhysicalVoiceOverlay() {
  createPhysicalVoiceOverlay();
  if (!physicalVoiceOverlay || physicalVoiceOverlayVisible) return;
  lv_obj_clear_flag(physicalVoiceOverlay, LV_OBJ_FLAG_HIDDEN);
  physicalVoiceOverlayVisible = true;
}

void hidePhysicalVoiceOverlay() {
  if (!physicalVoiceOverlay || !physicalVoiceOverlayVisible) return;
  lv_obj_add_flag(physicalVoiceOverlay, LV_OBJ_FLAG_HIDDEN);
  physicalVoiceOverlayVisible = false;
}

void servicePhysicalVoiceOverlay() {
  bool physicalVoiceHeld = heldVoiceSearchCommand &&
                           !heldVoiceSearchFromTouch &&
                           heldVoiceSearchPhysicalKey;
  unsigned long now = millis();
  if (!physicalVoiceHeld) {
    physicalVoiceOverlayShowAfterMs = 0;
    hidePhysicalVoiceOverlay();
    return;
  }
  if ((int32_t)(now - physicalVoiceOverlayShowAfterMs) < 0) return;
  showPhysicalVoiceOverlay();
  if (!physicalVoicePulse) return;
  uint16_t phase = (now / 6U) % 140U;
  int32_t pulseSize = phase <= 70U ? 88 + phase : 158 - (phase - 70U);
  physicalVoicePulseAnimation(physicalVoicePulse, pulseSize);
}

void clearModalObjects() {
  if (deviceModal) {
    lv_obj_del(deviceModal);
    deviceModal = nullptr;
  }
  if (lockOverlay) {
    lv_obj_del(lockOverlay);
    lockOverlay = nullptr;
  }
  closeBrightnessPanel();
}

// ---------------------------------------------------------------------------
// Page model
// ---------------------------------------------------------------------------

void rebuildPages() {
  pages[0] = {PAGE_REMOTE_SETTINGS, "Settings"};
  pages[1] = {PAGE_ACTIVITIES, "Activities"};
  pageCount = 2;

  if (activeActivity >= 0) {
    pages[pageCount++] = {PAGE_ACTIVITY, activities[activeActivity].name};
  }

  if (activeDevice >= 0) {
    pages[pageCount++] = {PAGE_DEVICE, devices[activeDevice].name};
  }

  if (currentPage >= pageCount) currentPage = pageCount - 1;
  configurePageStripDirections();
}

void drawDots() {
  if (!dots) return;
  if (lockActive || (currentPage == 0 && settingsView != SETTINGS_HOME)) {
    lv_obj_add_flag(dots, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  lv_obj_clear_flag(dots, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clean(dots);

  for (uint8_t i = 0; i < pageCount; i++) {
    lv_obj_t *dot = lv_obj_create(dots);
    lv_obj_set_size(dot, 8, 8);
    lv_obj_set_pos(dot, i * 16, 0);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_set_style_bg_color(dot, (i == currentPage) ? lv_color_white() : lvRgb(125, 125, 135), 0);
    lv_obj_set_style_bg_opa(dot, (i == currentPage) ? LV_OPA_90 : LV_OPA_50, 0);
  }
  lv_obj_set_width(dots, pageCount * 16);
  lv_obj_align(dots, LV_ALIGN_BOTTOM_MID, 0, -8);
}

void setCinematicBackground(bool enabled) {
  if (!wallpaper) return;
  if (enabled && boundPageUi &&
      (boundPageUi->wallpaperPixels || boundPageUi->themePath[0])) {
    lv_obj_clear_flag(wallpaper, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(wallpaper, LV_OBJ_FLAG_HIDDEN);
  }
}

void configureContent(int y, int height, bool transparent) {
  lv_obj_scroll_to(content, 0, 0, LV_ANIM_OFF);
  lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_style_pad_bottom(content, 0, 0);
  lv_obj_set_pos(content, 0, y);
  lv_obj_set_size(content, LCD_W, height);
  lv_obj_set_style_bg_color(content, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(content, transparent ? LV_OPA_TRANSP : LV_OPA_COVER, 0);
}

void resetSplitDiagnosticAnchor() {
  splitDiagnosticAnchor = nullptr;
  splitDiagnosticAnchorY = INT16_MAX;
  splitDiagnosticLastY = INT16_MIN;
}

void registerSplitDiagnosticAnchor(lv_obj_t *obj) {
  if (!obj) return;
  int16_t localY = lv_obj_get_y(obj);
  if (!splitDiagnosticAnchor || localY < splitDiagnosticAnchorY) {
    splitDiagnosticAnchor = obj;
    splitDiagnosticAnchorY = localY;
  }
}

void updateSplitDiagnostic() {
  if (!splitDiagnosticLabel || !lv_obj_is_valid(splitDiagnosticLabel)) return;
  if (!debugSplitEnabled) {
    lv_obj_add_flag(splitDiagnosticLabel, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  lv_obj_clear_flag(splitDiagnosticLabel, LV_OBJ_FLAG_HIDDEN);
  int16_t y = content ? lv_obj_get_y(content) - lv_obj_get_scroll_y(content) : 0;
  if (splitDiagnosticAnchor && lv_obj_is_valid(splitDiagnosticAnchor)) {
    lv_area_t coordinates;
    lv_obj_get_coords(splitDiagnosticAnchor, &coordinates);
    y = coordinates.y1;
  }
  y -= 4;
  if (y == splitDiagnosticLastY) return;
  splitDiagnosticLastY = y;
  char text[14];
  snprintf(text, sizeof(text), "S %d", y);
  lv_label_set_text(splitDiagnosticLabel, text);
}

void setDebugObjectVisible(lv_obj_t *obj, bool visible) {
  if (!obj || !lv_obj_is_valid(obj)) return;
  if (visible) lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
  else lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

void refreshDebugOverlayVisibility() {
  setDebugObjectVisible(splitDiagnosticLabel, debugSplitEnabled);
  setDebugObjectVisible(cpuRamDiagnosticLabel, debugCpuRamEnabled);
  setDebugObjectVisible(accelerometerDiagnosticLabel, debugAccelerometerEnabled);
  setDebugObjectVisible(fpsDiagnosticLabel, debugFpsEnabled);
  setDebugObjectVisible(touchDiagnosticLabel, debugTouchEnabled &&
                        (lvTouchDown || (int32_t)(touchDiagnosticHoldUntilMs - millis()) > 0));
  setDebugObjectVisible(touchDot, debugTouchEnabled && lvTouchDown);
  if (!debugTouchEnabled) {
    touchDiagnosticHoldUntilMs = 0;
    for (TouchTrailPoint &point : touchTrail) {
      point.active = false;
      setDebugObjectVisible(point.dot, false);
    }
  }
  if (debugSplitEnabled) updateSplitDiagnostic();
}

uint8_t debugCpuUsagePercent() {
  static uint32_t previousTotal = 0;
  static uint64_t previousIdle = 0;
  TaskStatus_t states[40];
  uint32_t total = 0;
  UBaseType_t count = uxTaskGetSystemState(states, 40, &total);
  uint64_t idle = 0;
  for (UBaseType_t i = 0; i < count; i++) {
    if (strncmp(states[i].pcTaskName, "IDLE", 4) == 0) idle += states[i].ulRunTimeCounter;
  }
  uint8_t usage = 0;
  if (previousTotal && total > previousTotal && idle >= previousIdle) {
    uint64_t available = (uint64_t)(total - previousTotal) * portNUM_PROCESSORS;
    uint64_t idleDelta = idle - previousIdle;
    usage = available ? (uint8_t)constrain(
      100 - (int)(idleDelta * 100ULL / available), 0, 100) : 0;
  }
  previousTotal = total;
  previousIdle = idle;
  return usage;
}

void serviceDebugOverlay(unsigned long now) {
  if (displaySleeping || !screenRoot) return;
  if (debugTouchEnabled) {
    for (TouchTrailPoint &point : touchTrail) {
      if (!point.active || !point.dot || !lv_obj_is_valid(point.dot)) continue;
      uint32_t age = now - point.createdMs;
      if (age >= 3000UL) {
        point.active = false;
        lv_obj_add_flag(point.dot, LV_OBJ_FLAG_HIDDEN);
        continue;
      }
      lv_opa_t opacity = (lv_opa_t)map(age, 0, 3000, LV_OPA_COVER, LV_OPA_10);
      lv_obj_set_style_bg_opa(point.dot, opacity, 0);
      lv_obj_move_foreground(point.dot);
    }
    if (!lvTouchDown && touchDiagnosticHoldUntilMs &&
        (int32_t)(now - touchDiagnosticHoldUntilMs) >= 0) {
      touchDiagnosticHoldUntilMs = 0;
      setDebugObjectVisible(touchDiagnosticLabel, false);
    }
  }
  if (debugCpuRamEnabled && (int32_t)(now - nextDebugCpuRamRefreshMs) >= 0) {
    nextDebugCpuRamRefreshMs = now + 1000UL;
    uint32_t total = ESP.getHeapSize();
    uint32_t freeBytes = ESP.getFreeHeap();
    uint32_t used = total > freeBytes ? total - freeBytes : 0;
    char text[36];
    snprintf(text, sizeof(text), "CPU %u%% RAM %lu/%luK",
             debugCpuUsagePercent(), (unsigned long)(used / 1024UL),
             (unsigned long)(freeBytes / 1024UL));
    lv_label_set_text(cpuRamDiagnosticLabel, text);
  }
  if (debugAccelerometerEnabled &&
      (int32_t)(now - nextDebugAccelerometerRefreshMs) >= 0) {
    nextDebugAccelerometerRefreshMs = now + 100UL;
    int16_t x = 0, y = 0, z = 0;
    char text[38];
    if (lis3dhReady && readLIS3DH(x, y, z)) {
      snprintf(text, sizeof(text), "ACC X%d Y%d Z%d", x, y, z);
    } else {
      strlcpy(text, "ACC unavailable", sizeof(text));
    }
    lv_label_set_text(accelerometerDiagnosticLabel, text);
  }
  if (debugFpsEnabled && (int32_t)(now - nextDebugFpsRefreshMs) >= 0) {
    if (!debugLastFpsSampleMs) debugLastFpsSampleMs = now;
    uint32_t elapsed = max(1UL, now - debugLastFpsSampleMs);
    uint32_t frames = debugDisplayFrameCount;
    uint32_t fps = (frames - debugLastDisplayFrameCount) * 1000UL / elapsed;
    char text[14];
    snprintf(text, sizeof(text), "FPS %lu", (unsigned long)fps);
    lv_label_set_text(fpsDiagnosticLabel, text);
    debugLastDisplayFrameCount = frames;
    debugLastFpsSampleMs = now;
    nextDebugFpsRefreshMs = now + 1000UL;
  }
  if (debugCpuRamEnabled) lv_obj_move_foreground(cpuRamDiagnosticLabel);
  if (debugAccelerometerEnabled) lv_obj_move_foreground(accelerometerDiagnosticLabel);
  if (debugFpsEnabled) lv_obj_move_foreground(fpsDiagnosticLabel);
  if (debugSplitEnabled) lv_obj_move_foreground(splitDiagnosticLabel);
  if (debugTouchEnabled &&
      (lvTouchDown || (int32_t)(touchDiagnosticHoldUntilMs - now) > 0)) {
    lv_obj_move_foreground(touchDiagnosticLabel);
  }
  if (debugTouchEnabled && lvTouchDown) lv_obj_move_foreground(touchDot);
}

bool updateChargingState() {
  unsigned long now = millis();
  bool rawCharging = digitalRead(PIN_CHARGE_STATUS) == LOW;
  bool chargerConnected = rawCharging;

  // CRG_STAT becomes high-impedance both when USB is removed and when the
  // TP4056 completes a charge. Retain the connected state at a full, stable
  // battery; after unplugging, the MAX17048 negative rate releases it.
  if (!rawCharging && (chargingState || batteryPowerModeCharging)) {
    float percent = readBatteryPercent();
    float rate = readBatteryRatePerHour();
    chargerConnected = percent >= 99.0f &&
      (isnan(rate) || rate >= -0.02f);
  }

  if (chargerConnected != chargingCandidate) {
    chargingCandidate = chargerConnected;
    chargingCandidateSinceMs = now;
  }

  if (chargingCandidate != chargingState &&
      (uint32_t)(now - chargingCandidateSinceMs) >= CHARGE_STATE_DEBOUNCE_MS) {
    chargingState = chargingCandidate;
    if (chargingState) chargingAnimationStartMs = now;
    resetBatteryMeasurementWindow(chargingState);
  }
  return chargingState;
}

void initialiseChargingState() {
  bool rawCharging = digitalRead(PIN_CHARGE_STATUS) == LOW;
  float percent = readBatteryPercent();
  float rate = readBatteryRatePerHour();
  bool chargerConnected = rawCharging;
  if (!rawCharging && batteryPowerModeKnown && batteryPowerModeCharging) {
    chargerConnected = percent >= 99.0f &&
      (isnan(rate) || rate >= -0.02f);
  }
  chargingState = chargerConnected;
  chargingCandidate = chargerConnected;
  chargingCandidateSinceMs = millis();
  if (!batteryPowerModeKnown || batteryPowerModeCharging != chargerConnected) {
    resetBatteryMeasurementWindow(chargerConnected);
  }
}

void refreshStatusPill() {
  static unsigned long lastClockRefreshMs = 0;
  static unsigned long lastBatteryRefreshMs = 0;
  static int16_t lastBatteryWidth = -1;
  static bool wasCharging = false;
  static lv_obj_t *lastBatteryObject = nullptr;
  unsigned long now = millis();
  bool clockNeedsInitialValue = clockLabel && strcmp(lv_label_get_text(clockLabel), "--:--") == 0;
  if (clockLabel && (clockNeedsInitialValue || lastClockRefreshMs == 0 ||
                     now - lastClockRefreshMs >= 1000)) {
    lastClockRefreshMs = now;
    char timeText[12];
    struct tm timeInfo;
    if (getLocalTime(&timeInfo, 5)) {
      strftime(timeText, sizeof(timeText), "%l:%M %p", &timeInfo);
      if (timeText[0] == ' ') memmove(timeText, timeText + 1, strlen(timeText));
    } else {
      snprintf(timeText, sizeof(timeText), "--:--");
    }
    if (strcmp(lv_label_get_text(clockLabel), timeText) != 0) {
      lv_label_set_text(clockLabel, timeText);
    }
  }

  if (batteryFill) {
    if (batteryFill != lastBatteryObject) {
      lastBatteryObject = batteryFill;
      lastBatteryWidth = -1;
      lastBatteryRefreshMs = 0;
    }
    bool charging = updateChargingState();
    int16_t requestedWidth = lastBatteryWidth;
    if (charging) {
      // Conventional charging loop: fill smoothly from empty to full, reset
      // once, then immediately begin the next empty-to-full pass.
      uint32_t phase = (now - chargingAnimationStartMs) % CHARGE_ANIMATION_FILL_MS;
      requestedWidth = 2 + (12UL * phase) / CHARGE_ANIMATION_FILL_MS;
    } else if (wasCharging || lastBatteryRefreshMs == 0 ||
               now - lastBatteryRefreshMs >= BATTERY_REFRESH_MS) {
      lastBatteryRefreshMs = now;
      float percent = readBatteryPercent();
      if (percent < 0.0f) percent = 0.0f;
      percent = constrain(percent, 0.0f, 100.0f);
      requestedWidth = max(2, (int)(13.0f * percent / 100.0f));
    }
    wasCharging = charging;
    if (requestedWidth != lastBatteryWidth || lv_obj_get_width(batteryFill) != requestedWidth) {
      lastBatteryWidth = requestedWidth;
      lv_obj_set_width(batteryFill, requestedWidth);
    }
  }
}

void renderTopBar(const char *title, bool allowDevices) {
  lv_obj_clean(topBar);
  brightnessBatteryLabel = nullptr;
  lv_obj_set_style_bg_opa(topBar, LV_OPA_TRANSP, 0);

  int titleX = 8;
  int titleWidth = clockEnabled ? 126 : 182;

  lv_obj_t *titleLabel = lv_label_create(topBar);
  lv_label_set_text(titleLabel, title);
  lv_label_set_long_mode(titleLabel, LV_LABEL_LONG_DOT);
  lv_obj_set_pos(titleLabel, titleX, 9);
  lv_obj_set_width(titleLabel, titleWidth);
  lv_obj_set_style_text_align(titleLabel, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_style_text_font(titleLabel, allowDevices ? &lv_font_montserrat_16 : &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(titleLabel, textPrimary(), 0);
  if (allowDevices) {
    lv_obj_add_flag(titleLabel, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(titleLabel, LV_OBJ_FLAG_PRESS_LOCK);
    lv_obj_set_ext_click_area(titleLabel, 6);
    lv_obj_add_event_cb(titleLabel, [](lv_event_t *e) { showDevicePicker(); }, LV_EVENT_CLICKED, nullptr);
  }

  if (!allowDevices) {
    lv_obj_t *hint = makeLabel(topBar, LV_SYMBOL_DOWN, 119, 13, &lv_font_montserrat_10, textPrimary());
    lv_obj_set_style_text_opa(hint, LV_OPA_60, 0);
  }

  statusPill = lv_btn_create(topBar);
  lv_obj_t *pill = statusPill;
  lv_obj_set_pos(pill, clockEnabled ? 138 : 196, 5);
  lv_obj_set_size(pill, clockEnabled ? 96 : 38, 31);
  lv_obj_set_style_radius(pill, 16, 0);
  lv_obj_set_style_bg_color(pill, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(pill, (lv_opa_t)115, 0);
  lv_obj_set_style_border_color(pill, lv_color_white(), 0);
  lv_obj_set_style_border_opa(pill, (lv_opa_t)64, 0);
  lv_obj_set_style_border_width(pill, 1, 0);
  lv_obj_set_style_shadow_width(pill, 0, 0);
  lv_obj_set_style_color_filter_opa(pill, LV_OPA_TRANSP, LV_STATE_PRESSED);
  lv_obj_set_style_pad_all(pill, 0, 0);
  lv_obj_clear_flag(pill, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(pill, [](lv_event_t *e) { toggleBrightnessPanel(); }, LV_EVENT_CLICKED, nullptr);

  clockLabel = nullptr;
  if (clockEnabled) {
    clockLabel = makeLabel(pill, "--:--", 4, 8, &lv_font_montserrat_12, textPrimary());
    lv_obj_set_width(clockLabel, 54);
    lv_obj_set_style_text_align(clockLabel, LV_TEXT_ALIGN_RIGHT, 0);
  }

  lv_obj_t *battery = lv_obj_create(pill);
  statusBattery = battery;
  lv_obj_remove_style_all(battery);
  lv_obj_set_pos(battery, clockEnabled ? 61 : 7, 9);
  lv_obj_set_size(battery, 18, 12);
  lv_obj_set_style_radius(battery, 2, 0);
  lv_obj_set_style_border_color(battery, lv_color_white(), 0);
  lv_obj_set_style_border_opa(battery, LV_OPA_90, 0);
  lv_obj_set_style_border_width(battery, 1, 0);
  lv_obj_set_style_bg_opa(battery, LV_OPA_TRANSP, 0);

  batteryFill = lv_obj_create(battery);
  lv_obj_remove_style_all(batteryFill);
  lv_obj_set_pos(batteryFill, 2, 2);
  lv_obj_set_size(batteryFill, 11, 6);
  lv_obj_set_style_radius(batteryFill, 1, 0);
  lv_obj_set_style_bg_color(batteryFill, lvRgb(166, 255, 184), 0);
  lv_obj_set_style_bg_opa(batteryFill, LV_OPA_COVER, 0);

  lv_obj_t *terminal = lv_obj_create(pill);
  statusBatteryTerminal = terminal;
  lv_obj_remove_style_all(terminal);
  lv_obj_set_pos(terminal, clockEnabled ? 79 : 25, 12);
  lv_obj_set_size(terminal, 2, 6);
  lv_obj_set_style_bg_color(terminal, lv_color_white(), 0);
  lv_obj_set_style_bg_opa(terminal, LV_OPA_80, 0);

  refreshStatusPill();
  if (boundPageUi) {
    boundPageUi->clockLabel = clockLabel;
    boundPageUi->statusPill = statusPill;
    boundPageUi->statusBattery = statusBattery;
    boundPageUi->statusBatteryTerminal = statusBatteryTerminal;
    boundPageUi->batteryFill = batteryFill;
  }
  lv_obj_move_foreground(topBar);
}

// ---------------------------------------------------------------------------
// Pages
// ---------------------------------------------------------------------------

void switchEvent(lv_event_t *e) {
  bool *target = (bool *)lv_event_get_user_data(e);
  *target = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
  if (target == &buttonTestActive) {
    endHeldIrCommand();
    buttonTestHeldIndex = -1;
    buttonTestHeldName[0] = '\0';
    buttonTestPulseUntilMs = 0;
    nextButtonTestRepeatMs = 0;
    setButtonTestVisual(false);
    lastWakeMs = millis();
    return;
  } else if (target == &wifiOn) {
    if (!wifiOn) stopNetworkStack();
  } else if (target == &bluetoothOn) {
    applyBluetoothState();
  } else if (target == &clockEnabled) {
    pendingUiRefresh = true;
  } else if (target == &clockUseInternetTime) {
    if (clockUseInternetTime && (!wifiOn || !hasSelectedWifiProfile())) {
      clockUseInternetTime = false;
    }
    applyClockMode();
    if (clockUseInternetTime) requestInternetTimeSync();
    pendingUiRefresh = settingsView == SETTINGS_CLOCK;
  } else if (target == &slideToUnlock) {
    // The new value takes effect the next time the screen wakes.
  } else if (target == &physicalRepeatEnabled) {
    endHeldIrCommand();
    pendingUiRefresh = settingsView == SETTINGS_BUTTONS;
  } else if (target == &debugSplitEnabled || target == &debugTouchEnabled ||
             target == &debugCpuRamEnabled ||
             target == &debugAccelerometerEnabled ||
             target == &debugFpsEnabled) {
    refreshDebugOverlayVisibility();
  } else if (target == &displayRgb666 || target == &displayInverted) {
    applyDisplayControllerSettings();
    lv_obj_invalidate(lv_scr_act());
    pendingUiRefresh = settingsView == SETTINGS_DISPLAY;
  }
  saveSettings();
  scheduleRuntimeSettingsSave();
}

lv_obj_t *makeSettingRow(const char *name, const char *sub, int y, bool *switchTarget,
                         lv_event_cb_t clickCallback = nullptr,
                         lv_obj_t **nameLabelOut = nullptr,
                         lv_obj_t **subLabelOut = nullptr) {
  lv_obj_t *row = lv_obj_create(content);
  lv_obj_set_pos(row, 8, y);
  lv_obj_set_size(row, 224, 44);
  lv_obj_add_flag(row, LV_OBJ_FLAG_GESTURE_BUBBLE);
  lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
  if (clickCallback) {
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(row, clickCallback, LV_EVENT_CLICKED, nullptr);
  } else {
    lv_obj_clear_flag(row, LV_OBJ_FLAG_CLICKABLE);
  }
  stylePanel(row, lvRgb(34, 35, 39), lvRgb(54, 56, 62));
  lv_obj_t *nameLabel = makeLabel(row, name, 8, 2, &lv_font_montserrat_16, textPrimary());
  lv_obj_t *subLabel = makeLabel(row, sub, 8, 24, &lv_font_montserrat_10, lvRgb(150, 150, 160));
  if (nameLabelOut) *nameLabelOut = nameLabel;
  if (subLabelOut) *subLabelOut = subLabel;

  if (switchTarget) {
    lv_obj_t *sw = lv_switch_create(row);
    lv_obj_set_size(sw, 38, 22);
    lv_obj_align(sw, LV_ALIGN_RIGHT_MID, -4, 0);
    if (*switchTarget) lv_obj_add_state(sw, LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw, switchEvent, LV_EVENT_VALUE_CHANGED, switchTarget);
  }
  return row;
}

void openSettingsView(SettingsView view) {
  bool leavingQrPage = settingsView == SETTINGS_WIFI_QR && view != SETTINGS_WIFI_QR;
  bool enteringQrPage = settingsView != SETTINGS_WIFI_QR && view == SETTINGS_WIFI_QR;
  bool enteringWifiPage = settingsView != SETTINGS_WIFI && view == SETTINGS_WIFI;
  bool keepingStationForWifi = view == SETTINGS_WIFI ||
                               view == SETTINGS_WIFI_PASSWORD;
  if (settingsView == SETTINGS_BUTTONS && view != SETTINGS_BUTTONS) {
    if (buttonTestHeldIndex != -1) endButtonTest(buttonTestHeldIndex);
    buttonTestActive = false;
  }
  if (leavingQrPage) {
    stationFallbackToSetupAp = false;
    wifiConnectPending = false;
    webConfigPausedBle = false;
    requestWebServerStop();
  }
  settingsView = view;
  if (leavingQrPage && !keepingStationForWifi) scheduleNetworkShutdown(50UL);
  if (leavingQrPage) applyBluetoothState();
  if (enteringWifiPage && wifiOn && hasSelectedWifiProfile()) {
    networkShutdownAtMs = 0;
    startNetworkStack();
    if (WiFi.status() != WL_CONNECTED && !wifiConnectPending) {
      wifiConnectPending = true;
      wifiConnectStartedMs = millis();
    }
  }
  if (enteringQrPage) {
    networkShutdownAtMs = 0;
    wifiConnectPending = false;
    stationFallbackToSetupAp = false;
    webConfigPausedBle = true;
    if (bleReady && !bleSuspended) {
      stopBluetoothRadio("WebConfig session");
      delay(120);
    }
    if (wifiOn && hasSelectedWifiProfile()) {
      startNetworkStack();
      if (WiFi.status() == WL_CONNECTED) {
        requestWebServerListen(true);
      } else {
        wifiConnectPending = true;
        stationFallbackToSetupAp = true;
        wifiConnectStartedMs = millis();
      }
    } else {
      startSetupAccessPoint();
    }
  }
  // Entering and leaving the QR page both begin a fresh sleep interval. The QR
  // page adds its awake grace period without modifying the saved timeout.
  if (leavingQrPage || enteringQrPage) lastWakeMs = millis();
  pendingUiRefresh = true;
}

void backToSettings(lv_event_t *e) {
  SettingsView previousView = settingsView;
  openSettingsView(SETTINGS_HOME);
  if (previousView == SETTINGS_WIFI || previousView == SETTINGS_WIFI_PASSWORD) {
    wifiConnectPending = false;
    scheduleNetworkShutdown(100UL);
  }
}

void renderSettingsBackButton() {
  lv_obj_t *back = makeButton(content, LV_SYMBOL_LEFT, 8, 6, 42, 30, lvRgb(30, 38, 50));
  lv_obj_add_event_cb(back, backToSettings, LV_EVENT_CLICKED, nullptr);
}

void renderSettingsHome() {
  setCinematicBackground(false);
  configureContent(42, 250, false);
  lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLL_MOMENTUM);
  lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLL_ELASTIC);
  lv_obj_set_scroll_dir(content, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_ACTIVE);
  lv_obj_set_style_pad_bottom(content, 20, 0);
  renderTopBar("Settings", false);
  String wifiState = !wifiOn ? "Off" : (WiFi.status() == WL_CONNECTED ? WiFi.SSID() : "Tap to scan networks");
  makeSettingRow("Wi-Fi", wifiState.c_str(), 8, &wifiOn,
    [](lv_event_t *e) {
      if (lv_event_get_target(e) == lv_event_get_current_target(e) && wifiOn) openSettingsView(SETTINGS_WIFI);
    });
  const char *bluetoothState = bleConnected ? "Connected" :
    (blePairingMode ? "Pairing" : (bleBonded ? "Paired" : (bluetoothOn ? "Not paired" : "Off")));
  makeSettingRow("Bluetooth", bluetoothState, 58, &bluetoothOn,
    [](lv_event_t *e) {
      if (lv_event_get_target(e) == lv_event_get_current_target(e)) {
        openSettingsView(SETTINGS_BLUETOOTH);
      }
    });
  makeSettingRow("Clock", clockUseInternetTime ? "Internet time and manual wheels" : "Manual time wheels", 108, &clockEnabled,
    [](lv_event_t *e) {
      if (lv_event_get_target(e) == lv_event_get_current_target(e)) openSettingsView(SETTINGS_CLOCK);
    });
  makeSettingRow("Wi-Fi Config", WiFi.status() == WL_CONNECTED ? "Scan QR to open WebConfig" : "Scan QR to join setup network", 158, nullptr,
    [](lv_event_t *e) { openSettingsView(SETTINGS_WIFI_QR); });
  makeSettingRow("Display", "Brightness, sleep, wake, lock", 208, nullptr,
    [](lv_event_t *e) { openSettingsView(SETTINGS_DISPLAY); });
  makeSettingRow("Buttons", "Repeat timing and button test", 258, nullptr,
    [](lv_event_t *e) { openSettingsView(SETTINGS_BUTTONS); });
  makeSettingRow("Debug", "Touch, display, sensors and microphone", 308, nullptr,
    [](lv_event_t *e) { openSettingsView(SETTINGS_DEBUG); });
  makeSettingRow("Backup / Restore", "Full configuration backups", 358, nullptr,
    [](lv_event_t *e) { openSettingsView(SETTINGS_BACKUP); });
  makeSettingRow("About", "Version, device and battery information", 408, nullptr,
    [](lv_event_t *e) { openSettingsView(SETTINGS_ABOUT); });
}

void bluetoothPairEvent(lv_event_t *e) {
  startBluetoothPairing();
  pendingUiRefresh = true;
}

void bluetoothForgetEvent(lv_event_t *e) {
  forgetBluetoothPairing();
  pendingUiRefresh = true;
}

void renderBluetoothPage() {
  setCinematicBackground(false);
  configureContent(42, 278, false);
  renderTopBar("Bluetooth", false);
  renderSettingsBackButton();

  const char *state = bleConnected ? "Connected to streamer" :
    (blePairingMode ? "Ready to pair" : (bleBonded ? "Paired - waiting to reconnect" : "Not paired"));
  makeLabel(content, state, 58, 10, &lv_font_montserrat_16,
            bleConnected ? lvRgb(120, 255, 155) : textPrimary());
  makeLabel(content, BLE_HID_NAME, 12, 52, &lv_font_montserrat_12, lvRgb(150, 190, 240));
  makeLabel(content, "On Chromecast open Settings >", 12, 76,
            &lv_font_montserrat_10, lvRgb(175, 175, 185));
  makeLabel(content, "Remotes & Accessories > Pair", 12, 92,
            &lv_font_montserrat_10, lvRgb(175, 175, 185));
  makeLabel(content, "remote, then choose OpenRemote HID.", 12, 108,
            &lv_font_montserrat_10, lvRgb(175, 175, 185));

  lv_obj_t *pair = makeButton(content, bleBonded ? "Pair another" : "Start pairing",
                              12, 138, 216, 42, lvRgb(24, 105, 220));
  lv_obj_add_event_cb(pair, bluetoothPairEvent, LV_EVENT_CLICKED, nullptr);

  if (bleBonded) {
    lv_obj_t *forget = makeButton(content, "Forget pairing", 12, 188, 216, 38,
                                  lvRgb(115, 38, 45));
    lv_obj_add_event_cb(forget, bluetoothForgetEvent, LV_EVENT_CLICKED, nullptr);
  }
  makeLabel(content, "Keyboard and media controls only. No voice.", 12, 238,
            &lv_font_montserrat_10, lvRgb(145, 145, 155));
}

void beginWifiScan(lv_event_t *e) {
  if (!wifiOn) return;
  stopSetupAccessPoint(false);
  WiFi.scanDelete();
  networkShutdownAtMs = 0;
  startNetworkStack();
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  WiFi.mode(WIFI_STA);
  networkStackActive = true;
  wifiScanCount = -2;
  wifiScanKeepSetupAp = false;
  wifiScanResultCount = 0;
  wifiScanPending = true;
  wifiScanStartPending = true;
  wifiScanAttempt = 0;
  wifiScanStartAtMs = millis() + WIFI_SCAN_SETTLE_MS;
  wifiScanStartedMs = 0;
  pendingUiRefresh = true;
}

void finishWifiScan(int result) {
  wifiScanResultCount = 0;
  for (int i = 0; i < result && wifiScanResultCount < MAX_WIFI_SCAN_RESULTS; i++) {
    String ssid = WiFi.SSID(i);
    ssid.trim();
    if (!ssid.length()) continue;
    bool duplicate = false;
    for (uint8_t j = 0; j < wifiScanResultCount; j++) {
      if (wifiScanResults[j].ssid == ssid) {
        duplicate = true;
        if (WiFi.RSSI(i) > wifiScanResults[j].rssi) {
          wifiScanResults[j].rssi = WiFi.RSSI(i);
          wifiScanResults[j].encryption = WiFi.encryptionType(i);
        }
        break;
      }
    }
    if (duplicate) continue;
    WifiScanEntry &entry = wifiScanResults[wifiScanResultCount++];
    entry.ssid = ssid;
    entry.rssi = WiFi.RSSI(i);
    entry.encryption = WiFi.encryptionType(i);
  }
  WiFi.scanDelete();
  wifiScanCount = wifiScanResultCount;
  wifiScanPending = false;
  wifiScanStartPending = false;
  wifiScanStartedMs = 0;
  snprintf(webWifiStatusText, sizeof(webWifiStatusText),
           "%u network%s found", (unsigned)wifiScanResultCount,
           wifiScanResultCount == 1 ? "" : "s");
  wifiScanKeepSetupAp = false;
  pendingUiRefresh = settingsView == SETTINGS_WIFI;
  Serial.printf("Wi-Fi scan: %d network(s) found\n", wifiScanCount);
}

void retryOrFinishWifiScan(unsigned long now) {
  WiFi.scanDelete();
  if (wifiScanAttempt < WIFI_SCAN_MAX_ATTEMPTS) {
    wifiScanStartPending = true;
    wifiScanStartAtMs = now + WIFI_SCAN_RETRY_DELAY_MS;
    wifiScanStartedMs = 0;
    Serial.println("Wi-Fi scan: retrying");
  } else {
    finishWifiScan(0);
  }
}

void serviceWifiScan(unsigned long now) {
  if (!wifiScanPending) return;

  if (wifiScanStartPending) {
    if ((int32_t)(now - wifiScanStartAtMs) < 0) return;
    wifiScanStartPending = false;
    wifiScanAttempt++;
    WiFi.mode(wifiScanKeepSetupAp ? WIFI_AP_STA : WIFI_STA);
    delay(80);
    // Synchronous scanning is intentional here. The UI already shows its
    // scanning state, and this avoids the ESP32-S3 async scan race that was
    // returning WIFI_SCAN_FAILED before any channel results were available.
    int result = WiFi.scanNetworks(false, false, false, 300);
    if (result >= 0) {
      finishWifiScan(result);
    } else {
      retryOrFinishWifiScan(now);
    }
    return;
  }
}

void chooseWifiNetwork(lv_event_t *e) {
  int index = (int)(intptr_t)lv_event_get_user_data(e);
  if (index < 0 || index >= wifiScanResultCount) return;
  selectedWifiSsid = wifiScanResults[index].ssid;
  if (wifiScanResults[index].encryption == WIFI_AUTH_OPEN) {
    saveWifiCredentials(selectedWifiSsid, "");
    WiFi.disconnect(false, false);
    WiFi.setAutoReconnect(true);
    WiFi.mode(WIFI_STA);
    WiFi.begin(selectedWifiSsid.c_str());
    networkStackActive = true;
    wifiConnectPending = true;
    wifiConnectStartedMs = millis();
    pendingUiRefresh = true;
    return;
  }
  openSettingsView(SETTINGS_WIFI_PASSWORD);
}

void renderWifiPage() {
  setCinematicBackground(false);
  configureContent(42, 278, false);
  lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLL_MOMENTUM);
  lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLL_ELASTIC);
  lv_obj_set_scroll_dir(content, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_ACTIVE);
  renderTopBar("Wi-Fi", false);
  renderSettingsBackButton();

  char status[64];
  if (WiFi.status() == WL_CONNECTED) {
    snprintf(status, sizeof(status), "%s  %s", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
  } else if (wifiConnectPending) {
    snprintf(status, sizeof(status), "Connecting to %s...", selectedWifiSsid.c_str());
  } else if (findWifiProfile(selectedWifiSsid) >= 0) {
    snprintf(status, sizeof(status), "Saved: %s", selectedWifiSsid.c_str());
  } else {
    snprintf(status, sizeof(status), "Not connected");
  }
  makeLabel(content, status, 58, 13, &lv_font_montserrat_10,
            WiFi.status() == WL_CONNECTED ? lvRgb(166, 255, 184) : lvRgb(170, 175, 185));

  lv_obj_t *scan = makeButton(content, wifiScanPending ? "Scanning..." : "Scan networks", 8, 45, 224, 34, lvRgb(35, 86, 140));
  lv_obj_add_event_cb(scan, beginWifiScan, LV_EVENT_CLICKED, nullptr);
  if (wifiScanPending) lv_obj_add_state(scan, LV_STATE_DISABLED);

  int visible = max(0, min(wifiScanCount, 10));
  if (wifiScanCount == 0) makeLabel(content, "No Wi-Fi networks found", 12, 94, &lv_font_montserrat_12, lvRgb(170, 175, 185));
  for (int i = 0; i < visible; i++) {
    String label = wifiScanResults[i].ssid;
    if (findWifiProfile(label) >= 0) label += "  Saved";
    else if (wifiScanResults[i].encryption != WIFI_AUTH_OPEN) label += "  " LV_SYMBOL_EYE_CLOSE;
    lv_obj_t *network = makeButton(content, label.c_str(), 8, 88 + i * 38, 224, 34, lvRgb(32, 35, 42));
    lv_obj_add_event_cb(network, chooseWifiNetwork, LV_EVENT_CLICKED, (void *)(intptr_t)i);
  }
}

void connectSelectedWifi(const String &password, bool saveCredential) {
  if (!selectedWifiSsid.length()) return;
  if (saveCredential) saveWifiCredentials(selectedWifiSsid, password);
  networkShutdownAtMs = 0;
  stopSetupAccessPoint(false);
  WiFi.disconnect(false, false);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(selectedWifiSsid.c_str(), password.c_str());
  networkStackActive = true;
  wifiConnectPending = true;
  wifiConnectStartedMs = millis();
  openSettingsView(SETTINGS_WIFI);
}

void serviceWebControlRequests(unsigned long now) {
  bool startScan = false;
  if (webWifiScanRequested && !wifiScanPending) {
    webWifiScanRequested = false;
    startScan = true;
  }

  WebClockRequest clockRequest;
  bool hasClockRequest = false;
  portENTER_CRITICAL(&webControlMux);
  if (webClockRequest.pending) {
    clockRequest = webClockRequest;
    webClockRequest.pending = false;
    hasClockRequest = true;
  }
  portEXIT_CRITICAL(&webControlMux);

  if (hasClockRequest) {
    clockEnabled = clockRequest.enabled;
    clockUseInternetTime = clockRequest.useInternetTime;
    clockUtcOffsetMinutes = clockRequest.utcOffsetMinutes;
    clockCityName = clockRequest.city;
    if (!clockUseInternetTime && clockRequest.hasManualEpoch) {
      manualClockEpoch = clockRequest.manualEpoch;
    }
    applyClockMode();
    saveSettings();
    scheduleRuntimeSettingsSave();
    if (clockUseInternetTime) requestInternetTimeSync();
    pendingUiRefresh = true;
    Serial.printf("WebConfig Clock: %s, %s, %s, UTC offset %+d min\n",
                  clockEnabled ? "visible" : "hidden",
                  clockUseInternetTime ? "Internet" : "manual",
                  clockCityName.c_str(), (int)clockUtcOffsetMinutes);
  }

  if (startScan) {
    wifiOn = true;
    saveSettings();
    networkShutdownAtMs = 0;
    WiFi.scanDelete();
    wifiScanKeepSetupAp = setupApActive;
    WiFi.persistent(false);
    WiFi.setSleep(false);
    WiFi.setAutoReconnect(true);
    if (setupApActive) {
      WiFi.mode(WIFI_AP_STA);
    } else {
      startNetworkStack();
      WiFi.mode(WIFI_STA);
      networkStackActive = true;
    }
    wifiScanCount = -2;
    wifiScanResultCount = 0;
    wifiScanPending = true;
    wifiScanStartPending = true;
    wifiScanAttempt = 0;
    wifiScanStartAtMs = now + WIFI_SCAN_SETTLE_MS;
    wifiScanStartedMs = 0;
  }

  if ((int32_t)(now - webWifiActionNotBeforeMs) < 0) return;
  WebWifiRequest wifiRequest;
  bool hasWifiRequest = false;
  portENTER_CRITICAL(&webControlMux);
  if (webWifiRequest.action != WEB_WIFI_NONE) {
    wifiRequest = webWifiRequest;
    webWifiRequest.action = WEB_WIFI_NONE;
    hasWifiRequest = true;
  }
  portEXIT_CRITICAL(&webControlMux);
  if (!hasWifiRequest) return;

  String ssid = wifiRequest.ssid;
  if (wifiRequest.action == WEB_WIFI_FORGET) {
    bool wasConnected = WiFi.status() == WL_CONNECTED && WiFi.SSID() == ssid;
    bool wasSelected = selectedWifiSsid == ssid;
    forgetWifiCredentials(ssid);
    if (wasConnected || wasSelected) {
      WiFi.setAutoReconnect(false);
      WiFi.disconnect(false, true);
      wifiConnectPending = false;
      stationFallbackToSetupAp = false;
    }
    snprintf(webWifiStatusText, sizeof(webWifiStatusText), "Forgot %s", ssid.c_str());
    if (webConfigQrPageActive() && (wasConnected || wasSelected)) {
      startSetupAccessPoint();
    }
    pendingUiRefresh = settingsView == SETTINGS_WIFI;
    return;
  }

  String password = wifiRequest.useSavedPassword
    ? savedWifiPassword(ssid) : String(wifiRequest.password);
  if (wifiRequest.useSavedPassword && findWifiProfile(ssid) < 0) {
    snprintf(webWifiStatusText, sizeof(webWifiStatusText),
             "No saved password for %s", ssid.c_str());
    return;
  }
  wifiOn = true;
  selectedWifiSsid = ssid;
  if (!wifiRequest.useSavedPassword) saveWifiCredentials(ssid, password);
  else saveSettings();
  networkShutdownAtMs = 0;
  wifiScanPending = false;
  wifiScanStartPending = false;
  WiFi.scanDelete();
  WiFi.persistent(false);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  if (setupApActive) {
    WiFi.mode(WIFI_AP_STA);
  } else {
    WiFi.disconnect(false, false);
    WiFi.mode(WIFI_STA);
    networkStackActive = true;
  }
  WiFi.begin(ssid.c_str(), password.c_str());
  wifiConnectPending = true;
  wifiConnectStartedMs = now;
  snprintf(webWifiStatusText, sizeof(webWifiStatusText),
           "Connecting to %s...", ssid.c_str());
  pendingUiRefresh = settingsView == SETTINGS_WIFI;
  Serial.printf("WebConfig Wi-Fi: connecting to %s%s\n", ssid.c_str(),
                setupApActive ? " while setup AP remains active" : "");
}

void useSavedWifiEvent(lv_event_t *e) {
  int profile = findWifiProfile(selectedWifiSsid);
  if (profile < 0) {
    pendingUiRefresh = true;
    return;
  }
  connectSelectedWifi(wifiProfiles[profile].password, false);
}

void forgetSavedWifiEvent(lv_event_t *e) {
  String forgottenSsid = selectedWifiSsid;
  bool wasConnected = WiFi.status() == WL_CONNECTED && WiFi.SSID() == forgottenSsid;
  forgetWifiCredentials(forgottenSsid);
  if (wasConnected) {
    WiFi.setAutoReconnect(false);
    WiFi.disconnect(false, true);
  }
  // Keep the unsaved SSID only as the password-entry target. Automatic station
  // connection paths require a saved profile and will ignore this value.
  selectedWifiSsid = forgottenSsid;
  wifiConnectPending = false;
  stationFallbackToSetupAp = false;
  wifiPasswordArea = nullptr;
  pendingUiRefresh = true;
}

void wifiKeyboardEvent(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED || !wifiPasswordArea) return;
  const char *key = (const char *)lv_event_get_user_data(e);
  if (!key) return;

  if (strcmp(key, "<OK>") == 0) {
    String password = lv_textarea_get_text(wifiPasswordArea);
    connectSelectedWifi(password, true);
  } else if (strcmp(key, "<CANCEL>") == 0) {
    openSettingsView(SETTINGS_WIFI);
  } else if (strcmp(key, "<DEL>") == 0) {
    lv_textarea_del_char(wifiPasswordArea);
  } else if (strcmp(key, "<SPACE>") == 0) {
    lv_textarea_add_text(wifiPasswordArea, " ");
  } else if (strcmp(key, "<CAPS>") == 0 || strcmp(key, "<SYM>") == 0) {
    if (strcmp(key, "<CAPS>") == 0) customKeyboardCaps = !customKeyboardCaps;
    else customKeyboardSymbols = !customKeyboardSymbols;
    pendingUiRefresh = true;
  } else {
    lv_textarea_add_text(wifiPasswordArea, key);
  }
  lastWakeMs = millis();
}

lv_obj_t *makeKeyboardKey(const char *label, const char *key, int x, int y, int w, int h,
                          lv_color_t bg = lv_color_make(208, 232, 246)) {
  lv_obj_t *button = lv_btn_create(wifiKeyboard);
  lv_obj_set_pos(button, x, y);
  lv_obj_set_size(button, w, h);
  lv_obj_clear_flag(button, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_radius(button, 6, 0);
  lv_obj_set_style_bg_color(button, bg, 0);
  lv_obj_set_style_bg_opa(button, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(button, 1, 0);
  lv_obj_set_style_border_color(button, lvRgb(120, 154, 180), 0);
  lv_obj_set_style_shadow_width(button, 0, 0);
  lv_obj_set_style_pad_all(button, 0, 0);
  lv_obj_add_event_cb(button, wifiKeyboardEvent, LV_EVENT_CLICKED, (void *)key);
  lv_obj_t *text = lv_label_create(button);
  lv_label_set_text(text, label);
  lv_obj_set_style_text_font(text, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(text, lvRgb(20, 40, 58), 0);
  lv_obj_clear_flag(text, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_center(text);
  return button;
}

void addKeyboardTextRow(const char *keys, int count, int x, int y, int keyW, bool letters) {
  char label[2] = {0, 0};
  for (int i = 0; i < count; i++) {
    char c = keys[i];
    if (letters && customKeyboardCaps) c = toupper((unsigned char)c);
    label[0] = c;
    static char keyStore[64][2];
    static uint8_t keyIndex = 0;
    keyStore[keyIndex][0] = c;
    keyStore[keyIndex][1] = '\0';
    makeKeyboardKey(label, keyStore[keyIndex], x + i * (keyW + 2), y, keyW, 27);
    keyIndex = (keyIndex + 1) % 64;
  }
}

void renderCompactWifiKeyboard() {
  wifiKeyboard = lv_obj_create(content);
  lv_obj_remove_style_all(wifiKeyboard);
  lv_obj_set_pos(wifiKeyboard, 0, 82);
  lv_obj_set_size(wifiKeyboard, 240, 164);
  lv_obj_set_style_bg_color(wifiKeyboard, lvRgb(31, 93, 151), 0);
  lv_obj_set_style_bg_opa(wifiKeyboard, LV_OPA_COVER, 0);
  lv_obj_set_style_pad_all(wifiKeyboard, 0, 0);
  lv_obj_clear_flag(wifiKeyboard, LV_OBJ_FLAG_SCROLLABLE);

  addKeyboardTextRow("1234567890", 10, 4, 4, 21, false);
  addKeyboardTextRow(customKeyboardSymbols ? "!@#$%^&*()" : "qwertyuiop", 10, 4, 35, 21, !customKeyboardSymbols);
  addKeyboardTextRow(customKeyboardSymbols ? "-_=+[]{};" : "asdfghjkl", 9, 15, 66, 21, !customKeyboardSymbols);
  makeKeyboardKey(customKeyboardCaps ? "abc" : "ABC", "<CAPS>", 4, 97, 34, 27, lvRgb(182, 215, 238));
  addKeyboardTextRow(customKeyboardSymbols ? ".,:?/\\'" : "zxcvbnm", 7, 42, 97, 20, !customKeyboardSymbols);
  makeKeyboardKey(LV_SYMBOL_BACKSPACE, "<DEL>", 188, 97, 46, 27, lvRgb(182, 215, 238));
  makeKeyboardKey("@#", "<SYM>", 4, 128, 36, 27, lvRgb(182, 215, 238));
  makeKeyboardKey("Cancel", "<CANCEL>", 43, 128, 54, 27, lvRgb(80, 96, 118));
  makeKeyboardKey("space", "<SPACE>", 100, 128, 58, 27, lvRgb(182, 215, 238));
  makeKeyboardKey("Connect", "<OK>", 161, 128, 73, 27, lvRgb(42, 155, 230));
}

void renderWifiPasswordPage() {
  setCinematicBackground(false);
  configureContent(42, 278, false);
  renderTopBar("Password", false);
  renderSettingsBackButton();
  makeLabel(content, selectedWifiSsid.c_str(), 58, 13, &lv_font_montserrat_12, textPrimary());

  wifiPasswordArea = nullptr;
  if (findWifiProfile(selectedWifiSsid) >= 0) {
    makeLabel(content, "A password is saved for this network.", 10, 50,
              &lv_font_montserrat_10, lvRgb(165, 180, 200));
    lv_obj_t *useSaved = makeButton(content, "Use saved password", 8, 78, 224, 42,
                                    lvRgb(24, 105, 220));
    lv_obj_add_event_cb(useSaved, useSavedWifiEvent, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *forget = makeButton(content, "Forget & enter password", 8, 132, 224, 42,
                                  lvRgb(115, 38, 45));
    lv_obj_add_event_cb(forget, forgetSavedWifiEvent, LV_EVENT_CLICKED, nullptr);
    return;
  }

  wifiPasswordArea = lv_textarea_create(content);
  lv_obj_set_pos(wifiPasswordArea, 8, 41);
  lv_obj_set_size(wifiPasswordArea, 224, 36);
  lv_textarea_set_one_line(wifiPasswordArea, true);
  lv_textarea_set_password_mode(wifiPasswordArea, true);
  lv_textarea_set_placeholder_text(wifiPasswordArea, "Wi-Fi password");

  renderCompactWifiKeyboard();
}

void refreshSetupApStatusLabel() {
  if (!setupApStatusLabel) return;
  char apStatus[80];
  if (wifiConnectPending && !setupApActive) {
    snprintf(apStatus, sizeof(apStatus), "Connecting to %s...", selectedWifiSsid.c_str());
  } else if (!setupApActive && WiFi.status() == WL_CONNECTED) {
    snprintf(apStatus, sizeof(apStatus), "%s\n%s", WiFi.SSID().c_str(),
             WiFi.localIP().toString().c_str());
  } else {
    snprintf(apStatus, sizeof(apStatus), "IP %s  Clients %u",
             WiFi.softAPIP().toString().c_str(), WiFi.softAPgetStationNum());
  }
  lv_label_set_text(setupApStatusLabel, apStatus);
}

void renderWifiQrPage() {
  setCinematicBackground(false);
  configureContent(42, 278, false);
  renderTopBar("WebConfig", false);
  renderSettingsBackButton();

  bool stationConnected = !setupApActive && WiFi.status() == WL_CONNECTED;
  bool connectionReady = stationConnected || setupApActive;
  if (!connectionReady) {
    makeLabel(content, "Connecting to saved Wi-Fi...", 18, 112,
              &lv_font_montserrat_12, textPrimary());
    makeLabel(content, "Setup AP starts automatically if needed", 15, 140,
              &lv_font_montserrat_10, lvRgb(155, 165, 180));
    setupApStatusLabel = makeLabel(content, "", 18, 172,
                                  &lv_font_montserrat_10, lvRgb(155, 165, 180));
    refreshSetupApStatusLabel();
    nextSetupApStatusRefreshMs = millis() + 1000UL;
    return;
  }
  String payload = stationConnected
    ? webConfigUrl()
    : String("WIFI:T:nopass;S:") + setupApSsid + ";;";
  lv_obj_t *qr = lv_qrcode_create(content, 166, lv_color_black(), lv_color_white());
  lv_obj_set_pos(qr, 37, 43);
  lv_qrcode_update(qr, payload.c_str(), payload.length());
  makeLabel(content, stationConnected ? "Scan to open WebConfig" : "Open network - no password", 35, 220,
            &lv_font_montserrat_12, textPrimary());
  makeLabel(content, stationConnected ? "Connected Wi-Fi" :
            (setupApActive ? setupApSsid.c_str() : "Turn Wi-Fi on to start setup"), 24, 240,
            &lv_font_montserrat_10, lvRgb(155, 165, 180));
  setupApStatusLabel = makeLabel(content, "", 24, 255, &lv_font_montserrat_10, lvRgb(155, 165, 180));
  refreshSetupApStatusLabel();
  nextSetupApStatusRefreshMs = millis() + 1000UL;
}

void buildNumberOptions(char *buffer, size_t size, int start, int end, uint8_t width) {
  buffer[0] = '\0';
  size_t used = 0;
  for (int value = start; value <= end && used + 2 < size; value++) {
    int written = snprintf(buffer + used, size - used, width == 2 ? "%02d\n" : "%d\n", value);
    if (written <= 0 || (size_t)written >= size - used) break;
    used += written;
  }
  if (used > 0 && buffer[used - 1] == '\n') buffer[used - 1] = '\0';
}

lv_obj_t *makeClockRoller(int index, const char *label, int x, int y, int w,
                          const char *options, int selected) {
  makeLabel(content, label, x, y - 16, &lv_font_montserrat_10, lvRgb(150, 160, 176));
  lv_obj_t *roller = lv_roller_create(content);
  lv_obj_set_pos(roller, x, y);
  lv_obj_set_size(roller, w, 64);
  lv_roller_set_options(roller, options, LV_ROLLER_MODE_NORMAL);
  lv_roller_set_visible_row_count(roller, 3);
  lv_roller_set_selected(roller, selected, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(roller, lvRgb(28, 32, 40), 0);
  lv_obj_set_style_bg_opa(roller, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(roller, lvRgb(58, 72, 92), 0);
  lv_obj_set_style_border_width(roller, 1, 0);
  lv_obj_set_style_radius(roller, 8, 0);
  lv_obj_set_style_text_font(roller, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(roller, textPrimary(), 0);
  lv_obj_set_style_bg_color(roller, lvRgb(44, 92, 140), LV_PART_SELECTED);
  lv_obj_set_style_text_color(roller, lv_color_white(), LV_PART_SELECTED);
  clockRollers[index] = roller;
  return roller;
}

void setManualClockEvent(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  tm manual = {};
  manual.tm_year = 2024 + lv_roller_get_selected(clockRollers[0]) - 1900;
  manual.tm_mon = lv_roller_get_selected(clockRollers[1]);
  manual.tm_mday = 1 + lv_roller_get_selected(clockRollers[2]);
  manual.tm_hour = lv_roller_get_selected(clockRollers[3]);
  manual.tm_min = lv_roller_get_selected(clockRollers[4]);
  manual.tm_sec = 0;
  manual.tm_isdst = -1;
  setenv("TZ", OPENREMOTE_TZ, 1);
  tzset();
  time_t epoch = mktime(&manual);
  if (epoch > 0) {
    manualClockEpoch = (uint64_t)epoch;
    clockUseInternetTime = false;
    saveSettings();
    scheduleRuntimeSettingsSave();
    applyClockMode();
    pendingUiRefresh = true;
  }
}

void chooseClockCity(lv_event_t *e) {
  const char *city = (const char *)lv_event_get_user_data(e);
  if (!city || !wifiOn || !hasSelectedWifiProfile()) return;
  clockCityName = city;
  clockUseInternetTime = true;
  saveSettings();
  scheduleRuntimeSettingsSave();
  applyClockMode();
  requestInternetTimeSync();
  openSettingsView(SETTINGS_CLOCK);
}

void renderClockCityPage() {
  setCinematicBackground(false);
  configureContent(42, 278, false);
  lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(content, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_AUTO);
  renderTopBar("Search City", false);
  renderSettingsBackButton();

  if (!wifiOn || !hasSelectedWifiProfile()) {
    makeLabel(content, "Connect Wi-Fi before selecting\nan Internet time city.", 12, 70,
              &lv_font_montserrat_12, textPrimary());
    return;
  }

  static const char *cities[] = {
    "Canberra", "Sydney", "Melbourne", "Brisbane", "Perth",
    "Adelaide", "Darwin", "Hobart", "UTC"
  };
  makeLabel(content, "Select nearest city", 58, 13, &lv_font_montserrat_12, textPrimary());
  for (uint8_t i = 0; i < sizeof(cities) / sizeof(cities[0]); i++) {
    lv_obj_t *city = makeButton(content, cities[i], 8, 48 + i * 38, 224, 34, lvRgb(32, 35, 42));
    lv_obj_add_event_cb(city, chooseClockCity, LV_EVENT_CLICKED, (void *)cities[i]);
  }
}

void renderClockPage() {
  setCinematicBackground(false);
  configureContent(42, 278, false);
  renderTopBar("Clock", false);
  renderSettingsBackButton();

  makeSettingRow("Status bar", clockEnabled ? "Show time in top right" : "Battery only", 42, &clockEnabled);
  makeSettingRow("Internet time", wifiOn && hasSelectedWifiProfile()
    ? "Boot and daily 3am sync" : "Needs a saved Wi-Fi network", 92,
    &clockUseInternetTime);

  if (clockUseInternetTime) {
    makeSettingRow("City", clockCityName.c_str(), 142, nullptr,
      [](lv_event_t *e) { openSettingsView(SETTINGS_CLOCK_CITY); });
    makeLabel(content, "Wi-Fi turns on only while time syncs.", 12, 200,
              &lv_font_montserrat_12, lvRgb(170, 175, 185));
    return;
  }

  makeLabel(content, "Set manually", 10, 148, &lv_font_montserrat_12, textPrimary());
  static char yearOptions[72];
  static char monthOptions[36];
  static char dayOptions[96];
  static char hourOptions[72];
  static char minuteOptions[180];
  buildNumberOptions(yearOptions, sizeof(yearOptions), 2024, 2035, 4);
  buildNumberOptions(monthOptions, sizeof(monthOptions), 1, 12, 2);
  buildNumberOptions(dayOptions, sizeof(dayOptions), 1, 31, 2);
  buildNumberOptions(hourOptions, sizeof(hourOptions), 0, 23, 2);
  buildNumberOptions(minuteOptions, sizeof(minuteOptions), 0, 59, 2);

  time_t now = time(nullptr);
  if (now < 1700000000 && manualClockEpoch > 0) now = (time_t)manualClockEpoch;
  tm timeInfo = {};
  if (now >= 1700000000) localtime_r(&now, &timeInfo);
  else {
    timeInfo.tm_year = 2026 - 1900;
    timeInfo.tm_mon = 6;
    timeInfo.tm_mday = 18;
    timeInfo.tm_hour = 12;
    timeInfo.tm_min = 0;
  }

  makeClockRoller(0, "Year", 8, 176, 50, yearOptions, constrain(timeInfo.tm_year + 1900 - 2024, 0, 11));
  makeClockRoller(1, "Mon", 62, 176, 39, monthOptions, constrain(timeInfo.tm_mon, 0, 11));
  makeClockRoller(2, "Day", 105, 176, 39, dayOptions, constrain(timeInfo.tm_mday - 1, 0, 30));
  makeClockRoller(3, "Hr", 148, 176, 39, hourOptions, constrain(timeInfo.tm_hour, 0, 23));
  makeClockRoller(4, "Min", 191, 176, 39, minuteOptions, constrain(timeInfo.tm_min, 0, 59));

  lv_obj_t *set = makeButton(content, "Set manual time", 8, 250, 224, 24, lvRgb(35, 86, 140));
  lv_obj_add_event_cb(set, setManualClockEvent, LV_EVENT_CLICKED, nullptr);
}

void restoreScrollSafeSlider(lv_obj_t *slider, ScrollSafeSliderState *state) {
  if (!slider || !state || state->restoring ||
      lv_slider_get_value(slider) == state->committedValue) return;
  state->restoring = true;
  lv_slider_set_value(slider, state->committedValue, LV_ANIM_OFF);
  state->restoring = false;
}

void scrollSafeSliderEvent(lv_event_t *e) {
  ScrollSafeSliderState *state =
    static_cast<ScrollSafeSliderState *>(lv_event_get_user_data(e));
  lv_obj_t *slider = lv_event_get_target(e);
  if (!state || state->restoring) return;
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_PRESSED) {
    state->tracking = true;
    state->horizontal = false;
    state->vertical = false;
    lv_indev_t *indev = lv_indev_get_act();
    if (indev) lv_indev_get_point(indev, &state->startPoint);
    restoreScrollSafeSlider(slider, state);
  } else if (code == LV_EVENT_PRESSING && state->tracking) {
    lv_indev_t *indev = lv_indev_get_act();
    if (!indev) return;
    lv_point_t point;
    lv_indev_get_point(indev, &point);
    int dx = abs((int)point.x - (int)state->startPoint.x);
    int dy = abs((int)point.y - (int)state->startPoint.y);
    if (!state->horizontal && !state->vertical && max(dx, dy) >= 6) {
      state->horizontal = dx > dy;
      state->vertical = !state->horizontal;
    }
    if (!state->horizontal) restoreScrollSafeSlider(slider, state);
  } else if (code == LV_EVENT_VALUE_CHANGED) {
    if (!state->tracking || !state->horizontal) {
      restoreScrollSafeSlider(slider, state);
    }
  } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
    if (state->horizontal) state->committedValue = lv_slider_get_value(slider);
    else restoreScrollSafeSlider(slider, state);
    state->tracking = false;
    state->horizontal = false;
    state->vertical = false;
  }
}

void attachScrollSafeSlider(lv_obj_t *slider, int32_t initialValue) {
  if (!slider || scrollSafeSliderCount >= MAX_SCROLL_SAFE_SLIDERS) return;
  ScrollSafeSliderState *state = &scrollSafeSliderStates[scrollSafeSliderCount++];
  memset(state, 0, sizeof(*state));
  state->committedValue = initialValue;
  lv_obj_add_flag(slider, LV_OBJ_FLAG_GESTURE_BUBBLE);
  lv_obj_add_event_cb(slider, scrollSafeSliderEvent, LV_EVENT_ALL, state);
}

void displaySliderEvent(lv_event_t *e) {
  int setting = (int)(intptr_t)lv_event_get_user_data(e);
  int value = lv_slider_get_value(lv_event_get_target(e));
  if (setting == 0) {
    brightness = value;
    applyBrightness();
  } else if (setting == 1) {
    timeoutSeconds = value;
  } else if (setting == 2) {
    deepSleepMinutes = deepSleepMinutesForIndex(value);
    value = deepSleepMinutes;
  } else if (setting == 3) {
    wakeSensitivity = constrain(value, 1, 100);
    raiseToWake = true;
    value = wakeSensitivity;
  } else if (setting == 4) {
    displayGamma = constrain(value, 50, 250);
    value = displayGamma;
  } else if (setting == 5) {
    displaySaturation = constrain(value, 0, 200);
    value = displaySaturation;
  }
  if (setting >= 0 && setting < 6 && displayValueLabels[setting]) {
    char valueText[12];
    if (setting == 4) snprintf(valueText, sizeof(valueText), "%.2f", value / 100.0f);
    else if (setting == 2) snprintf(valueText, sizeof(valueText), "%dmin", value);
    else if (setting == 3) snprintf(valueText, sizeof(valueText), "%udeg", motionWakeAngleDegrees());
    else snprintf(valueText, sizeof(valueText), "%d%s", value, setting == 1 ? "s" : "%");
    lv_label_set_text(displayValueLabels[setting], valueText);
  }
  if (lv_event_get_code(e) == LV_EVENT_RELEASED) {
    if (setting == 4 || setting == 5) {
      rebuildDisplayColourLut();
      lv_obj_invalidate(lv_scr_act());
    }
    saveSettings();
    scheduleRuntimeSettingsSave();
  }
  lastWakeMs = millis();
}

void makeDisplaySlider(const char *label, int y, int minValue, int maxValue, int value, int setting) {
  makeLabel(content, label, 10, y, &lv_font_montserrat_12, textPrimary());
  char valueText[12];
  if (setting == 4) snprintf(valueText, sizeof(valueText), "%.2f", value / 100.0f);
  else if (setting == 2) snprintf(valueText, sizeof(valueText), "%dmin", value);
  else if (setting == 3) snprintf(valueText, sizeof(valueText), "%udeg", motionWakeAngleDegrees());
  else snprintf(valueText, sizeof(valueText), "%d%s", value, setting == 1 ? "s" : "%");
  lv_obj_t *number = makeLabel(content, valueText, 190, y, &lv_font_montserrat_12, lvRgb(95, 180, 255));
  displayValueLabels[setting] = number;
  lv_obj_set_width(number, 38);
  lv_obj_set_style_text_align(number, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_t *slider = lv_slider_create(content);
  lv_obj_set_pos(slider, 10, y + 23);
  lv_obj_set_size(slider, 220, 12);
  if (setting == 2) {
    uint8_t index = deepSleepSliderIndex(value);
    lv_slider_set_range(slider, 0, 6);
    lv_slider_set_value(slider, index, LV_ANIM_OFF);
    attachScrollSafeSlider(slider, index);
  } else {
    lv_slider_set_range(slider, minValue, maxValue);
    lv_slider_set_value(slider, value, LV_ANIM_OFF);
    attachScrollSafeSlider(slider, value);
  }
  lv_obj_add_event_cb(slider, displaySliderEvent, LV_EVENT_VALUE_CHANGED, (void *)(intptr_t)setting);
  lv_obj_add_event_cb(slider, displaySliderEvent, LV_EVENT_RELEASED, (void *)(intptr_t)setting);
}

void renderDisplayPage() {
  setCinematicBackground(false);
  configureContent(42, 278, false);
  lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(content, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_AUTO);
  lv_obj_set_style_pad_bottom(content, 14, 0);
  renderTopBar("Display", false);
  renderSettingsBackButton();
  makeDisplaySlider("Brightness", 46, 5, 100, brightness, 0);
  makeDisplaySlider("Sleep timer", 100, 5, 120, timeoutSeconds, 1);
  makeDisplaySlider("Deep Sleep", 154, 1, 30, deepSleepMinutes, 2);
  makeDisplaySlider("Motion sensitivity", 208, 1, 100, wakeSensitivity, 3);
  makeDisplaySlider("Gamma", 262, 50, 250, displayGamma, 4);
  makeDisplaySlider("Saturation", 316, 0, 200, displaySaturation, 5);
  makeSettingRow("Slide to unlock", slideToUnlock ? "Required after sleep" : "Wake directly", 370, &slideToUnlock);
  makeSettingRow("Colour Depth", displayRgb666 ? "RGB666 panel transfer" : "RGB565 panel transfer", 420, &displayRgb666);
}

void buttonSliderEvent(lv_event_t *e) {
  int setting = (int)(intptr_t)lv_event_get_user_data(e);
  lv_obj_t *slider = lv_event_get_target(e);
  int value = lv_slider_get_value(slider);
  if (setting == 0) {
    value = constrain(value * 50,
      (int)BUTTON_REPEAT_DELAY_MIN_MS, (int)BUTTON_REPEAT_DELAY_MAX_MS);
    physicalRepeatDelayMs = value;
  } else {
    value = constrain(value, (int)BUTTON_REPEAT_RATE_MIN_HZ,
                      (int)BUTTON_REPEAT_RATE_MAX_HZ);
    physicalRepeatRateHz = value;
  }
  if (setting >= 0 && setting < 2 && buttonValueLabels[setting]) {
    char text[16];
    if (setting == 0) snprintf(text, sizeof(text), "%dms", value);
    else snprintf(text, sizeof(text), "%d/s", value);
    lv_label_set_text(buttonValueLabels[setting], text);
  }
  if (lv_event_get_code(e) == LV_EVENT_RELEASED) {
    saveSettings();
    scheduleRuntimeSettingsSave();
  }
  lastWakeMs = millis();
}

void makeButtonTimingSlider(const char *label, int y, int minValue,
                            int maxValue, int value, int setting) {
  makeLabel(content, label, 10, y, &lv_font_montserrat_12, textPrimary());
  char text[16];
  if (setting == 0) snprintf(text, sizeof(text), "%dms", value);
  else snprintf(text, sizeof(text), "%d/s", value);
  lv_obj_t *number = makeLabel(content, text, 184, y,
                               &lv_font_montserrat_12, lvRgb(95, 180, 255));
  buttonValueLabels[setting] = number;
  lv_obj_set_width(number, 44);
  lv_obj_set_style_text_align(number, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_t *slider = lv_slider_create(content);
  lv_obj_set_pos(slider, 10, y + 23);
  lv_obj_set_size(slider, 220, 12);
  int sliderValue = value;
  if (setting == 0) {
    lv_slider_set_range(slider, minValue / 50, maxValue / 50);
    sliderValue = value / 50;
  } else {
    lv_slider_set_range(slider, minValue, maxValue);
  }
  lv_slider_set_value(slider, sliderValue, LV_ANIM_OFF);
  attachScrollSafeSlider(slider, sliderValue);
  lv_obj_add_event_cb(slider, buttonSliderEvent, LV_EVENT_VALUE_CHANGED,
                      (void *)(intptr_t)setting);
  lv_obj_add_event_cb(slider, buttonSliderEvent, LV_EVENT_RELEASED,
                      (void *)(intptr_t)setting);
}

void renderButtonsPage() {
  setCinematicBackground(false);
  configureContent(42, 278, false);
  lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLL_MOMENTUM);
  lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLL_ELASTIC);
  lv_obj_set_scroll_dir(content, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_AUTO);
  lv_obj_set_style_pad_bottom(content, 18, 0);
  renderTopBar("Buttons", false);
  renderSettingsBackButton();

  makeSettingRow("Repeat", physicalRepeatEnabled ? "Repeat while held" : "Send once per press",
                 44, &physicalRepeatEnabled);
  int testRowY = 98;
  if (physicalRepeatEnabled) {
    makeButtonTimingSlider("Delay before repeat", 98,
      BUTTON_REPEAT_DELAY_MIN_MS, BUTTON_REPEAT_DELAY_MAX_MS,
      physicalRepeatDelayMs, 0);
    makeButtonTimingSlider("Repeat speed", 152,
      BUTTON_REPEAT_RATE_MIN_HZ, BUTTON_REPEAT_RATE_MAX_HZ,
      physicalRepeatRateHz, 1);
    testRowY = 206;
  }

  makeSettingRow("Test Button", "No commands are transmitted", testRowY,
                 &buttonTestActive);
  buttonTestPanel = lv_obj_create(content);
  lv_obj_set_pos(buttonTestPanel, 8, testRowY + 50);
  lv_obj_set_size(buttonTestPanel, 224, 54);
  lv_obj_clear_flag(buttonTestPanel, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(buttonTestPanel, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_radius(buttonTestPanel, 6, 0);
  lv_obj_set_style_border_width(buttonTestPanel, 1, 0);
  buttonTestLabel = makeLabel(buttonTestPanel, "No button pressed", 8, 13,
                              &lv_font_montserrat_16, lvRgb(150, 150, 160));
  lv_obj_set_width(buttonTestLabel, 206);
  lv_obj_set_style_text_align(buttonTestLabel, LV_TEXT_ALIGN_CENTER, 0);
  setButtonTestVisual(false);
}

void debugRowDropdownEvent(lv_event_t *e) {
  uint8_t row = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
  if (row >= 5) return;
  uint16_t selected = lv_dropdown_get_selected(lv_event_get_target(e));
  debugRowPixels[row] = constrain(debugRowDropdownMinimums[row] + (int)selected,
                                  0, LCD_H - 1);
  applyRuntimeThemeRowCalibration();
  saveSettings();
  scheduleRuntimeSettingsSave();
  pendingUiRefresh = true;
  lastWakeMs = millis();
}

void styleDebugDropdown(lv_obj_t *dropdown) {
  lv_obj_set_style_text_font(dropdown, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(dropdown, textPrimary(), 0);
  lv_obj_set_style_bg_color(dropdown, lvRgb(25, 42, 62), 0);
  lv_obj_set_style_bg_opa(dropdown, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(dropdown, lvRgb(55, 132, 205), 0);
  lv_obj_set_style_border_width(dropdown, 1, 0);
  lv_obj_set_style_radius(dropdown, 6, 0);
  lv_obj_set_style_pad_left(dropdown, 8, 0);
  lv_obj_add_flag(dropdown, LV_OBJ_FLAG_GESTURE_BUBBLE);
}

void makeDebugRowDropdown(uint8_t row, int y) {
  char title[10];
  snprintf(title, sizeof(title), "Row %u", row + 1);
  lv_obj_t *panel = lv_obj_create(content);
  lv_obj_set_pos(panel, 8, y);
  lv_obj_set_size(panel, 224, 42);
  lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(panel, LV_OBJ_FLAG_CLICKABLE);
  stylePanel(panel, lvRgb(34, 35, 39), lvRgb(54, 56, 62));
  makeLabel(panel, title, 8, 7, &lv_font_montserrat_14, textPrimary());

  int minimum = max(0, (int)debugRowPixels[row] - 10);
  int maximum = min(LCD_H - 1, (int)debugRowPixels[row] + 10);
  debugRowDropdownMinimums[row] = minimum;
  String options;
  for (int value = minimum; value <= maximum; value++) {
    if (options.length()) options += '\n';
    options += String(value) + " px";
  }
  lv_obj_t *dropdown = lv_dropdown_create(panel);
  debugRowDropdowns[row] = dropdown;
  lv_obj_set_pos(dropdown, 116, 1);
  lv_obj_set_size(dropdown, 92, 32);
  styleDebugDropdown(dropdown);
  lv_dropdown_set_options(dropdown, options.c_str());
  lv_dropdown_set_selected(dropdown, debugRowPixels[row] - minimum);
  lv_obj_add_event_cb(dropdown, debugRowDropdownEvent, LV_EVENT_VALUE_CHANGED,
                      (void *)(uintptr_t)row);
}

void microphoneSourceDropdownEvent(lv_event_t *e) {
  microphoneTestAudioEnabled = lv_dropdown_get_selected(lv_event_get_target(e)) == 1;
  saveSettings();
  scheduleRuntimeSettingsSave();
  lastWakeMs = millis();
}

void makeMicrophoneSourceRow(int y) {
  lv_obj_t *panel = lv_obj_create(content);
  lv_obj_set_pos(panel, 8, y);
  lv_obj_set_size(panel, 224, 44);
  lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(panel, LV_OBJ_FLAG_CLICKABLE);
  stylePanel(panel, lvRgb(34, 35, 39), lvRgb(54, 56, 62));
  makeLabel(panel, "Microphone", 8, 8, &lv_font_montserrat_14, textPrimary());
  lv_obj_t *dropdown = lv_dropdown_create(panel);
  lv_obj_set_pos(dropdown, 104, 1);
  lv_obj_set_size(dropdown, 104, 32);
  styleDebugDropdown(dropdown);
  lv_dropdown_set_options(dropdown, "I2S Mic\nTest Only");
  lv_dropdown_set_selected(dropdown, microphoneTestAudioEnabled ? 1 : 0);
  lv_obj_add_event_cb(dropdown, microphoneSourceDropdownEvent,
                      LV_EVENT_VALUE_CHANGED, nullptr);
}

void confirmDebugReboot(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED || !lcdRebootConfirmBox) return;
  const char *choice = lv_msgbox_get_active_btn_text(lcdRebootConfirmBox);
  bool confirmed = choice && strcmp(choice, "Reboot") == 0;
  lv_msgbox_close(lcdRebootConfirmBox);
  lcdRebootConfirmBox = nullptr;
  if (!confirmed) return;
  if (lcdRebootConfirmHard) hardRestartPending = true;
  else restartPending = true;
}

void showDebugRebootConfirmation(bool hard) {
  if (lcdRebootConfirmBox) return;
  lcdRebootConfirmHard = hard;
  static const char *buttons[] = {"Reboot", "Cancel", ""};
  lcdRebootConfirmBox = lv_msgbox_create(
    lv_scr_act(), hard ? "Hard reboot?" : "Soft reboot?",
    hard ? "Reset the complete ESP32 digital system now?"
         : "Restart OpenRemote now?",
    buttons, false);
  lv_obj_set_width(lcdRebootConfirmBox, 220);
  lv_obj_center(lcdRebootConfirmBox);
  lv_obj_add_event_cb(lcdRebootConfirmBox, confirmDebugReboot,
                      LV_EVENT_VALUE_CHANGED, nullptr);
}

void debugRebootButtonEvent(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  showDebugRebootConfirmation((bool)(uintptr_t)lv_event_get_user_data(e));
}

void renderDebugPage() {
  setCinematicBackground(false);
  configureContent(42, 278, false);
  lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLL_MOMENTUM);
  lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLL_ELASTIC);
  lv_obj_set_scroll_dir(content, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_AUTO);
  lv_obj_set_style_pad_bottom(content, 18, 0);
  renderTopBar("Debug", false);
  renderSettingsBackButton();

  makeSettingRow("Split Line", "Show live first-row Y position", 44,
                 &debugSplitEnabled);
  makeLabel(content, "Saved row positions", 10, 94,
            &lv_font_montserrat_12, lvRgb(170, 178, 190));
  for (uint8_t row = 0; row < 5; row++) makeDebugRowDropdown(row, 116 + row * 46);
  makeSettingRow("Touch", "Live reticle, coordinates and trail", 352,
                 &debugTouchEnabled);
  makeSettingRow("CPU / RAM", "Processor load and heap used/free", 402,
                 &debugCpuRamEnabled);
  makeSettingRow("Accelerometer", "Live X,Y,Z tilt samples", 452,
                 &debugAccelerometerEnabled);
  makeSettingRow("FPS", "Display frames per second", 502, &debugFpsEnabled);
  makeMicrophoneSourceRow(552);
  lv_obj_t *softButton = makeButton(content, "Soft reboot", 8, 606, 108, 42,
                                    lvRgb(28, 91, 150));
  lv_obj_t *hardButton = makeButton(content, "Hard reboot", 124, 606, 108, 42,
                                    lvRgb(132, 43, 48));
  lv_obj_add_event_cb(softButton, debugRebootButtonEvent, LV_EVENT_CLICKED,
                      (void *)(uintptr_t)false);
  lv_obj_add_event_cb(hardButton, debugRebootButtonEvent, LV_EVENT_CLICKED,
                      (void *)(uintptr_t)true);
}

String signedBatteryValue(float value, const char *suffix) {
  if (isnan(value)) return "Collecting data";
  char text[28];
  snprintf(text, sizeof(text), "%+.2f%%%s", value, suffix);
  return String(text);
}

const char *batteryEstimateLabel(const BatteryMetrics &metrics) {
  return metrics.chargerConnected ? "Estimated until full" : "Estimated until empty";
}

String batteryEstimateText(const BatteryMetrics &metrics) {
  if (isnan(metrics.estimatedHours)) {
    if (metrics.chargerConnected && metrics.percent >= 99.95f) return "Full";
    if (isnan(metrics.change1h)) return "Collecting data";
    return "Stable";
  }
  uint32_t totalMinutes = (uint32_t)roundf(metrics.estimatedHours * 60.0f);
  uint32_t hours = totalMinutes / 60U;
  uint32_t minutes = totalMinutes % 60U;
  char remaining[40];
  if (hours >= 24U) {
    uint32_t roundedHours = (totalMinutes + 30U) / 60U;
    uint32_t days = roundedHours / 24U;
    uint32_t remainingHours = roundedHours % 24U;
    snprintf(remaining, sizeof(remaining), "%lu day%s %lu hour%s",
             (unsigned long)days, days == 1U ? "" : "s",
             (unsigned long)remainingHours, remainingHours == 1U ? "" : "s");
    return String(remaining);
  }
  snprintf(remaining, sizeof(remaining), "%luh %lumin",
           (unsigned long)hours, (unsigned long)minutes);
  return String(remaining);
}

void updateBatteryMetricLabels(BatteryMetrics metrics) {
  char voltage[20];
  char level[20];
  snprintf(voltage, sizeof(voltage), metrics.voltage >= 0.0f ? "%.2f V" : "Unavailable",
           metrics.voltage);
  snprintf(level, sizeof(level), metrics.percent >= 0.0f ? "%.2f%%" : "Unavailable",
           metrics.percent);
  String values[6] = {
    voltage,
    level,
    signedBatteryValue(metrics.ratePerHour, " per hour"),
    signedBatteryValue(metrics.change1h, " in last hour"),
    signedBatteryValue(metrics.change24h, " in last 24 hours"),
    batteryEstimateText(metrics)
  };
  const char *names[6] = {
    "Voltage", "Battery Level", "Rate", "Last hour", "Last 24 hours",
    batteryEstimateLabel(metrics)
  };
  for (uint8_t i = 0; i < 6; i++) {
    if (batteryMetricNameLabels[i]) lv_label_set_text(batteryMetricNameLabels[i], names[i]);
    if (batteryMetricValueLabels[i]) lv_label_set_text(batteryMetricValueLabels[i], values[i].c_str());
  }
}

void makeBatteryMetricRows(int firstY) {
  BatteryMetrics metrics = currentBatteryMetrics();

  char voltage[20];
  char level[20];
  snprintf(voltage, sizeof(voltage), metrics.voltage >= 0.0f ? "%.2f V" : "Unavailable",
           metrics.voltage);
  snprintf(level, sizeof(level), metrics.percent >= 0.0f ? "%.2f%%" : "Unavailable",
           metrics.percent);
  String values[6] = {
    voltage,
    level,
    signedBatteryValue(metrics.ratePerHour, " per hour"),
    signedBatteryValue(metrics.change1h, " in last hour"),
    signedBatteryValue(metrics.change24h, " in last 24 hours"),
    batteryEstimateText(metrics)
  };
  const char *names[6] = {
    "Voltage", "Battery Level", "Rate", "Last hour", "Last 24 hours",
    batteryEstimateLabel(metrics)
  };
  for (uint8_t i = 0; i < 6; i++) {
    makeSettingRow(names[i], values[i].c_str(), firstY + i * 50, nullptr, nullptr,
                   &batteryMetricNameLabels[i], &batteryMetricValueLabels[i]);
  }
  nextBatteryPageRefreshMs = millis() + 1000UL;
}

void renderBatteryPage() {
  setCinematicBackground(false);
  configureContent(42, 278, false);
  lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(content, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_AUTO);
  renderTopBar("Battery", false);
  renderSettingsBackButton();

  makeBatteryMetricRows(44);
}

void setLcdBackupStatus(const String &message) {
  strlcpy(lcdBackupStatus, message.c_str(), sizeof(lcdBackupStatus));
  if (lcdBackupStatusLabel) {
    lv_label_set_text(lcdBackupStatusLabel, lcdBackupStatus);
    lv_refr_now(nullptr);
  }
}

void createBackupFromLcd(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  setLcdBackupStatus("Creating full backup...");
  String name;
  String error;
  if (createLcdFullBackup(name, error)) {
    setLcdBackupStatus("Backup created successfully");
  } else {
    setLcdBackupStatus(error);
  }
  pendingUiRefresh = true;
}

void confirmBackupRestore(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED || !lcdBackupConfirmBox) return;
  const char *choice = lv_msgbox_get_active_btn_text(lcdBackupConfirmBox);
  bool restore = choice && strcmp(choice, "Restore") == 0;
  lv_msgbox_close(lcdBackupConfirmBox);
  lcdBackupConfirmBox = nullptr;
  if (!restore) return;

  setLcdBackupStatus("Restoring full backup...");
  String error;
  if (restoreLcdFullBackup(lcdPendingBackupName, error)) {
    setLcdBackupStatus("Restore complete");
  } else {
    setLcdBackupStatus(error);
  }
  pendingUiRefresh = true;
}

void chooseLcdBackup(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  int index = (int)(intptr_t)lv_event_get_user_data(e);
  if (index < 0 || index >= lcdBackupCount) return;
  strlcpy(lcdPendingBackupName, lcdBackupEntries[index].name,
          sizeof(lcdPendingBackupName));
  static const char *buttons[] = {"Restore", "Cancel", ""};
  lcdBackupConfirmBox = lv_msgbox_create(
    lv_scr_act(), "Restore full backup?", lcdBackupEntries[index].displayDate,
    buttons, false);
  lv_obj_set_width(lcdBackupConfirmBox, 220);
  lv_obj_center(lcdBackupConfirmBox);
  lv_obj_add_event_cb(lcdBackupConfirmBox, confirmBackupRestore,
                      LV_EVENT_VALUE_CHANGED, nullptr);
}

void renderBackupRestorePage() {
  setCinematicBackground(false);
  configureContent(42, 278, false);
  lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLL_MOMENTUM);
  lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLL_ELASTIC);
  lv_obj_set_scroll_dir(content, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_ACTIVE);
  lv_obj_set_style_pad_bottom(content, 18, 0);
  renderTopBar("Backup / Restore", false);
  renderSettingsBackButton();

  lv_obj_t *create = makeButton(content, "Create full backup", 8, 44, 224, 38,
                                lvRgb(35, 86, 140));
  lv_obj_add_event_cb(create, createBackupFromLcd, LV_EVENT_CLICKED, nullptr);
  lcdBackupStatusLabel = makeLabel(
    content, lcdBackupStatus[0] ? lcdBackupStatus : "Backups are saved on the SD card",
    10, 90, &lv_font_montserrat_10, lvRgb(160, 170, 185));

  loadLcdBackupEntries();
  makeLabel(content, "Restore a backup", 10, 116, &lv_font_montserrat_12, textPrimary());
  if (!lcdBackupCount) {
    makeLabel(content, "No full backups found", 10, 145,
              &lv_font_montserrat_12, lvRgb(160, 170, 185));
    return;
  }
  for (uint8_t index = 0; index < lcdBackupCount; index++) {
    lv_obj_t *backup = makeButton(content, lcdBackupEntries[index].displayDate,
                                  8, 140 + index * 42, 224, 36,
                                  lvRgb(32, 35, 42));
    lv_obj_add_event_cb(backup, chooseLcdBackup, LV_EVENT_CLICKED,
                        (void *)(intptr_t)index);
  }
}

void renderAboutPage() {
  setCinematicBackground(false);
  configureContent(42, 278, false);
  lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(content, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_AUTO);
  renderTopBar("About", false);
  renderSettingsBackButton();
  makeLabel(content, "OpenRemote", 58, 12, &lv_font_montserrat_16, textPrimary());
  makeSettingRow("Firmware", OPENREMOTE_VERSION_TEXT, 52, nullptr);
  makeSettingRow("Hardware", "OMOTE Rev 5 / ESP32-S3", 102, nullptr);
  String webVersion = installedWebConfigVersion();
  makeSettingRow("WebConfig", webVersion.length() ? webVersion.c_str() : "Not installed", 152, nullptr);
  makeSettingRow("Setup network", setupApSsid.c_str(), 202, nullptr);
  makeSettingRow("SD Card", sdStatusText, 252, nullptr);

  makeLabel(content, "Battery", 10, 310, &lv_font_montserrat_16, textPrimary());
  makeBatteryMetricRows(336);
}

void renderSettingsPage() {
  switch (settingsView) {
    case SETTINGS_WIFI: renderWifiPage(); break;
    case SETTINGS_WIFI_PASSWORD: renderWifiPasswordPage(); break;
    case SETTINGS_WIFI_QR: renderWifiQrPage(); break;
    case SETTINGS_BLUETOOTH: renderBluetoothPage(); break;
    case SETTINGS_CLOCK: renderClockPage(); break;
    case SETTINGS_CLOCK_CITY: renderClockCityPage(); break;
    case SETTINGS_DISPLAY: renderDisplayPage(); break;
    case SETTINGS_BUTTONS: renderButtonsPage(); break;
    case SETTINGS_DEBUG: renderDebugPage(); break;
    case SETTINGS_BATTERY: renderBatteryPage(); break;
    case SETTINGS_BACKUP: renderBackupRestorePage(); break;
    case SETTINGS_ABOUT: renderAboutPage(); break;
    default: renderSettingsHome(); break;
  }
}

void activityThumbAnim(void *object, int32_t x) {
  lv_obj_set_x((lv_obj_t *)object, x);
}

void resetActivityThumb(ActivitySliderUi *ui) {
  lv_anim_t animation;
  lv_anim_init(&animation);
  lv_anim_set_var(&animation, ui->thumb);
  lv_anim_set_exec_cb(&animation, activityThumbAnim);
  lv_anim_set_values(&animation, lv_obj_get_x(ui->thumb), 4);
  lv_anim_set_time(&animation, 180);
  lv_anim_set_path_cb(&animation, lv_anim_path_ease_out);
  lv_anim_start(&animation);
}

void activitySliderEvent(lv_event_t *e) {
  ActivitySliderUi *ui = (ActivitySliderUi *)lv_event_get_user_data(e);
  lv_event_code_t code = lv_event_get_code(e);

  if (code == LV_EVENT_PRESSED) {
    activityDragActive = true;
    lastWakeMs = millis();
  } else if (code == LV_EVENT_PRESSING) {
    lastWakeMs = millis();
  }

  if (code == LV_EVENT_PRESSING) {
    lv_indev_t *indev = lv_indev_get_act();
    if (!indev) return;

    lv_point_t point;
    lv_area_t cardArea;
    lv_indev_get_point(indev, &point);
    lv_obj_get_coords(ui->card, &cardArea);

    int minX = 4;
    int maxX = max(minX, (int)lv_obj_get_width(ui->card) -
                             (int)lv_obj_get_width(ui->thumb) - 4);
    int x = point.x - cardArea.x1 - lv_obj_get_width(ui->thumb) / 2;
    lv_obj_set_x(ui->thumb, constrain(x, minX, maxX));
  }

  if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
    activityDragActive = false;
    int maxX = max(4, (int)lv_obj_get_width(ui->card) -
                       (int)lv_obj_get_width(ui->thumb) - 4);
    if (lv_obj_get_x(ui->thumb) >= maxX - 8) {
      pendingActivityOpen = ui->activityIndex;
      lv_obj_clear_flag(ui->thumb, LV_OBJ_FLAG_CLICKABLE);
    } else {
      resetActivityThumb(ui);
    }
  }
}

uint8_t activitySliderThumbPixels() {
  return constrain((int)map(activityIconSize, 20, 64, 22, 40), 22, 40);
}

int activitySliderCardPixels(uint8_t thumbSize) {
  return max(44, (int)thumbSize + 6);
}

void renderActivitiesPage() {
  applyRuntimeTheme(activitiesThemePath);
  configureContent(0, LCD_H, true);
  renderTopBar("Activities", true);

  if (ACTIVITY_COUNT == 0) {
    lv_obj_t *empty = makeLabel(content, "No activities\nSync real activities in WebConfig", 18, 142,
                                &lv_font_montserrat_12, textPrimary());
    lv_obj_set_width(empty, 204);
    lv_obj_set_style_text_align(empty, LV_TEXT_ALIGN_CENTER, 0);
    return;
  }

  for (uint8_t i = 0; i < ACTIVITY_COUNT; i++) {
    uint8_t thumbSize = activitySliderThumbPixels();
    int cardHeight = activitySliderCardPixels(thumbSize);
    int rowPitch = 52;
    lv_obj_t *card = lv_obj_create(content);
    int cardStart = activeRuntimeThemeStyle ? activeRuntimeThemeStyle->split + 4 : 104;
    lv_obj_set_pos(card, 8, cardStart + i * rowPitch);
    registerSplitDiagnosticAnchor(card);
    lv_obj_set_size(card, 224, cardHeight);
    lv_color_t cardColour = activeRuntimeThemeStyle
      ? lv_color_hex(activeRuntimeThemeStyle->glassColour) : lvRgb(92, 46, 28);
    lv_opa_t cardOpacity = activeRuntimeThemeStyle
      ? (lv_opa_t)activeRuntimeThemeStyle->glassOpacity : (lv_opa_t)58;
    bool themeAllowsBoundary = !activeRuntimeThemeStyle || activeRuntimeThemeStyle->glassEnabled;
    bool showCard = activities[i].boxMode == 1 ||
      (activities[i].boxMode == 0 && activityBoxesEnabled && themeAllowsBoundary);
    stylePanel(card, cardColour, lv_color_white(), cardOpacity);
    lv_obj_set_style_bg_opa(card, showCard ? cardOpacity : LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(card, showCard ? LV_OPA_50 : LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(card, showCard ? 1 : 0, 0);
    lv_obj_set_style_radius(card, 9, 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    // Keep the card scrollable (with no chaining) so it, not the page strip,
    // is the ancestor LVGL picks up as the drag target when the thumb below
    // is dragged. The card has no overflowing content so nothing actually
    // scrolls, but its presence stops the gesture from bubbling up and being
    // read as a horizontal page swipe after only a couple of pixels of drag.
    lv_obj_add_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(card, LV_DIR_HOR);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLL_CHAIN_VER);
    lv_obj_set_scrollbar_mode(card, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *thumb = lv_obj_create(card);
    lv_obj_remove_style_all(thumb);
    lv_obj_set_pos(thumb, 4, 3);
    lv_obj_set_size(thumb, thumbSize, thumbSize);
    lv_obj_set_style_radius(thumb, 8, 0);
    lv_obj_set_style_bg_opa(thumb, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(thumb, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(thumb, 0, 0);
    lv_obj_add_flag(thumb, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(thumb, LV_OBJ_FLAG_PRESS_LOCK);
    lv_obj_clear_flag(thumb, LV_OBJ_FLAG_SCROLLABLE);

    if (activities[i].iconPath && activities[i].iconPath[0]) {
      lv_obj_t *icon = lv_img_create(thumb);
      lv_img_set_src(icon, cachedPageIconSource(activities[i].iconPath));
      lv_img_set_zoom(icon, (uint16_t)(256UL * max(14, (int)thumbSize - 6) / 64UL));
      lv_obj_center(icon);
      lv_obj_clear_flag(icon, LV_OBJ_FLAG_CLICKABLE);
    } else {
      lv_obj_t *icon = lv_label_create(thumb);
      lv_label_set_text(icon, LV_SYMBOL_PLAY);
      lv_obj_set_style_text_font(icon, &lv_font_montserrat_16, 0);
      lv_obj_set_style_text_color(icon, textPrimary(), 0);
      lv_obj_center(icon);
      lv_obj_clear_flag(icon, LV_OBJ_FLAG_CLICKABLE);
    }

    activitySliderUi[i] = {card, thumb, i};
    lv_obj_add_event_cb(thumb, activitySliderEvent, LV_EVENT_ALL, &activitySliderUi[i]);

    int textX = thumbSize + 14;
    int nameY = max(3, (cardHeight - 36) / 2);
    lv_obj_t *name = makeLabel(card, activities[i].name, textX, nameY,
                               fontForSize(activityTextSize), textPrimary());
    lv_obj_set_width(name, max(72, 185 - textX));
    lv_label_set_long_mode(name, LV_LABEL_LONG_DOT);
    lv_obj_clear_flag(name, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t *instruction = makeLabel(card, "Slide to activate", textX, nameY + 22,
                                      &lv_font_montserrat_10, textPrimary());
    lv_obj_set_style_text_opa(instruction, (lv_opa_t)166, 0);
    lv_obj_clear_flag(instruction, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *chevrons = makeLabel(card, LV_SYMBOL_RIGHT " " LV_SYMBOL_RIGHT,
                                   193, (cardHeight - 16) / 2,
                                   &lv_font_montserrat_12, textPrimary());
    lv_obj_set_style_text_opa(chevrons, LV_OPA_40, 0);
    lv_obj_clear_flag(chevrons, LV_OBJ_FLAG_CLICKABLE);
  }
}

const Tile *currentActivityTiles(uint8_t &count) {
  if (activeActivity < 0 || activeActivity >= ACTIVITY_COUNT) {
    count = 0;
    return activityTiles[0];
  }
  count = activityTileCounts[activeActivity];
  return activityTiles[activeActivity];
}

int8_t findRuntimeActivityIndex(const char *activityId) {
  if (!activityId || !activityId[0]) return -1;
  for (uint8_t index = 0; index < ACTIVITY_COUNT; index++) {
    if (strcmp(activities[index].id, activityId) == 0) return (int8_t)index;
  }
  return -1;
}

int8_t findRuntimeMacroIndex(const char *macroId) {
  if (!macroId || !macroId[0]) return -1;
  for (uint8_t index = 0; index < MACRO_COUNT; index++) {
    if (strcmp(macros[index].id, macroId) == 0) return (int8_t)index;
  }
  return -1;
}

void activateMacro(uint8_t index) {
  if (index >= MACRO_COUNT) return;
  Macro &macro = macros[index];
  Serial.printf("Activate macro: %s (%u steps)\n", macro.name, macro.stepCount);
  endHeldIrCommand();
  activitySequenceMacro = (int8_t)index;
  activitySequenceStep = 0;
  activitySequenceNextMs = millis();
  activitySequencePoweredOnMask = 0;
  activitySequenceActive = macro.stepCount > 0;
  lastWakeMs = millis();
  serviceActivitySequence(millis());
}

void makeNestedActivitySlider(const Tile &tile, uint8_t targetActivityIndex,
                              ActivitySliderUi *ui) {
  uint8_t row = tile.slot / 3;
  int gridStart = activeRuntimeThemeStyle
    ? max(5, (int)activeRuntimeThemeStyle->split - 42 + 4) : 5;
  int y = gridStart + row * 52;
  uint8_t thumbSize = activitySliderThumbPixels();
  int cardHeight = activitySliderCardPixels(thumbSize);

  lv_obj_t *card = lv_obj_create(content);
  lv_obj_set_pos(card, 8, y);
  registerSplitDiagnosticAnchor(card);
  lv_obj_set_size(card, 224, cardHeight);
  lv_color_t cardColour = activeRuntimeThemeStyle
    ? lv_color_hex(activeRuntimeThemeStyle->glassColour) : lvRgb(92, 46, 28);
  lv_opa_t cardOpacity = activeRuntimeThemeStyle
    ? (lv_opa_t)activeRuntimeThemeStyle->glassOpacity : (lv_opa_t)58;
  bool themeAllowsBoundary = !activeRuntimeThemeStyle || activeRuntimeThemeStyle->glassEnabled;
  bool showCard = tile.boxMode == 1 ||
    (tile.boxMode == 0 && activityBoxesEnabled && themeAllowsBoundary);
  stylePanel(card, cardColour, lv_color_white(), cardOpacity);
  lv_obj_set_style_bg_opa(card, showCard ? cardOpacity : LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_opa(card, showCard ? LV_OPA_50 : LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(card, showCard ? 1 : 0, 0);
  lv_obj_set_style_radius(card, 9, 0);
  lv_obj_set_style_pad_all(card, 0, 0);
  // See makeNestedActivitySlider's sibling in renderActivitiesPage: keep the
  // card scrollable (with no chaining) so LVGL picks it, not the page strip,
  // as the drag target for the thumb below, preventing the drag from
  // bubbling up into a horizontal page swipe.
  lv_obj_add_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(card, LV_DIR_HOR);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLL_CHAIN_VER);
  lv_obj_set_scrollbar_mode(card, LV_SCROLLBAR_MODE_OFF);

  lv_obj_t *thumb = lv_obj_create(card);
  lv_obj_remove_style_all(thumb);
  lv_obj_set_pos(thumb, 4, 3);
  lv_obj_set_size(thumb, thumbSize, thumbSize);
  lv_obj_set_style_radius(thumb, 8, 0);
  lv_obj_set_style_bg_opa(thumb, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_opa(thumb, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(thumb, 0, 0);
  lv_obj_add_flag(thumb, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(thumb, LV_OBJ_FLAG_PRESS_LOCK);
  lv_obj_clear_flag(thumb, LV_OBJ_FLAG_SCROLLABLE);

  const char *iconPath = tile.iconPath && tile.iconPath[0]
    ? tile.iconPath : activities[targetActivityIndex].iconPath;
  if (iconPath && iconPath[0]) {
    lv_obj_t *icon = lv_img_create(thumb);
    lv_img_set_src(icon, cachedPageIconSource(iconPath));
    lv_img_set_zoom(icon, (uint16_t)(256UL * max(14, (int)thumbSize - 6) / 64UL));
    lv_obj_center(icon);
    lv_obj_clear_flag(icon, LV_OBJ_FLAG_CLICKABLE);
  } else {
    lv_obj_t *icon = lv_label_create(thumb);
    lv_label_set_text(icon, LV_SYMBOL_PLAY);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(icon, textPrimary(), 0);
    lv_obj_center(icon);
    lv_obj_clear_flag(icon, LV_OBJ_FLAG_CLICKABLE);
  }

  *ui = {card, thumb, targetActivityIndex};
  lv_obj_add_event_cb(thumb, activitySliderEvent, LV_EVENT_ALL, ui);

  if (tile.showText) {
    int textX = thumbSize + 14;
    int nameY = max(3, (cardHeight - 36) / 2);
    lv_obj_t *name = makeLabel(card, tile.label, textX, nameY,
                               fontForSize(activityTextSize), textPrimary());
    lv_obj_set_width(name, max(72, 185 - textX));
    lv_label_set_long_mode(name, LV_LABEL_LONG_DOT);
    lv_obj_clear_flag(name, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t *instruction = makeLabel(card, "Slide to activate", textX, nameY + 22,
                                      &lv_font_montserrat_10, textPrimary());
    lv_obj_set_style_text_opa(instruction, (lv_opa_t)166, 0);
    lv_obj_clear_flag(instruction, LV_OBJ_FLAG_CLICKABLE);
  }

  lv_obj_t *chevrons = makeLabel(card, LV_SYMBOL_RIGHT " " LV_SYMBOL_RIGHT,
                                 193, (cardHeight - 16) / 2,
                                 &lv_font_montserrat_12, textPrimary());
  lv_obj_set_style_text_opa(chevrons, LV_OPA_40, 0);
  lv_obj_clear_flag(chevrons, LV_OBJ_FLAG_CLICKABLE);
}

void tileEvent(lv_event_t *e) {
  UiCommandBinding *binding = static_cast<UiCommandBinding *>(lv_event_get_user_data(e));
  lv_event_code_t code = lv_event_get_code(e);
  if (binding && isVoiceSearchCommand(binding->command)) {
    if (code == LV_EVENT_PRESSED) {
      beginHeldIrCommand(binding->command, false, true);
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
      endHeldIrCommand(binding->command);
    }
    return;
  }
  if (code == LV_EVENT_SHORT_CLICKED) {
    if (binding && binding->macro) {
      activateMacro((uint8_t)(binding->macro - macros));
      return;
    }
    if (!binding || !binding->command) {
      Serial.println("Tile pressed without a command binding");
      return;
    }
    Serial.printf("IR command: %s\n", binding->command->label);
    transmitRuntimeCommand(binding->command);
    lastWakeMs = millis();
  }
}

void makeTile(uint8_t slot, const char *label, const char *iconPath, bool showText,
              uint8_t boxMode, bool repeat, DeviceCommand *command, Macro *macro) {
  uint8_t col = slot % 3;
  uint8_t row = slot / 3;
  int x = 8 + col * 76;
  int iconPixels = constrain((int)map(buttonIconSize, 20, 64, 16, 40), 16, 40);
  int gridStart = activeRuntimeThemeStyle
    ? max(5, (int)activeRuntimeThemeStyle->split - 42 + 4) : 5;
  int y = gridStart + row * 52;

  lv_color_t tileColour = activeRuntimeThemeStyle
    ? lv_color_hex(activeRuntimeThemeStyle->glassColour) : lvRgb(76, 48, 38);
  lv_obj_t *tile = makeButton(content, "", x, y, 68, 44, tileColour);
  registerSplitDiagnosticAnchor(tile);
  bool themeAllowsBoundary = !activeRuntimeThemeStyle || activeRuntimeThemeStyle->glassEnabled;
  bool showBox = boxMode == 1 ||
    (boxMode == 0 && buttonBoxesEnabled && themeAllowsBoundary);
  lv_opa_t tileOpacity = activeRuntimeThemeStyle
    ? (lv_opa_t)activeRuntimeThemeStyle->glassOpacity : LV_OPA_30;
  lv_obj_set_style_bg_opa(tile, showBox ? tileOpacity : LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_color(tile, lv_color_white(), 0);
  lv_obj_set_style_border_opa(tile, showBox ? LV_OPA_20 : LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(tile, showBox ? 1 : 0, 0);
  lv_obj_set_style_shadow_width(tile, 0, 0);
  lv_obj_set_style_bg_opa(tile, showBox ? tileOpacity : LV_OPA_TRANSP, LV_STATE_PRESSED);
  lv_obj_set_style_border_opa(tile, showBox ? LV_OPA_20 : LV_OPA_TRANSP, LV_STATE_PRESSED);
  UiCommandBinding *binding = nullptr;
  if (uiCommandBindingCount < MAX_DEVICE_COMMANDS) {
    binding = &uiCommandBindings[uiCommandBindingCount++];
    binding->command = command;
    binding->macro = macro;
    binding->repeat = repeat;
  }
  lv_obj_add_event_cb(tile, tileEvent, LV_EVENT_ALL, binding);

  if (!showText && iconPath && iconPath[0]) {
    lv_obj_t *icon = lv_img_create(tile);
    lv_img_set_src(icon, cachedPageIconSource(iconPath));
    lv_img_set_zoom(icon, (uint16_t)(256UL * iconPixels / 64UL));
    lv_obj_center(icon);
    lv_obj_clear_flag(icon, LV_OBJ_FLAG_CLICKABLE);
  } else {
    lv_obj_t *l = lv_label_create(tile);
    lv_label_set_text(l, label);
    lv_label_set_long_mode(l, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(l, 56);
    lv_obj_set_style_text_font(l, fontForSize(buttonTextSize), 0);
    lv_obj_set_style_text_color(l, textPrimary(), 0);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_clear_flag(l, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(l, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(l);
  }
}

void renderActivityPage() {
  applyRuntimeTheme(activeActivity >= 0 ? activities[activeActivity].themePath : "");
  configureContent(42, LCD_H - 62, true);
  lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLL_MOMENTUM);
  lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLL_ELASTIC);
  lv_obj_set_scroll_dir(content, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_ACTIVE);
  lv_obj_set_style_pad_bottom(content, 12, 0);
  uiCommandBindingCount = 0;
  const char *name = (activeActivity >= 0) ? activities[activeActivity].name : "Activity";
  renderTopBar(name, true);

  uint8_t count = 0;
  const Tile *tiles = currentActivityTiles(count);
  for (uint8_t i = 0; i < count && i < MAX_ACTIVITY_TILES; i++) {
    if (tiles[i].kind == Tile::ACTIVITY) {
      int8_t targetActivityIndex = findRuntimeActivityIndex(tiles[i].targetActivityId);
      if (targetActivityIndex >= 0) {
        makeNestedActivitySlider(tiles[i], (uint8_t)targetActivityIndex,
                                 &activitySliderUi[i]);
        continue;
      }
    }
    DeviceCommand *command = nullptr;
    Macro *macro = nullptr;
    if (tiles[i].kind == Tile::MACRO) {
      int8_t macroIndex = findRuntimeMacroIndex(tiles[i].targetMacroId);
      if (macroIndex >= 0) macro = &macros[macroIndex];
    }
    if (tiles[i].deviceIndex < DEVICE_COUNT &&
        tiles[i].commandIndex < devices[tiles[i].deviceIndex].commandCount) {
      command = &devices[tiles[i].deviceIndex].commands[tiles[i].commandIndex];
    }
    makeTile(tiles[i].slot, tiles[i].label, tiles[i].iconPath ? tiles[i].iconPath : "", tiles[i].showText,
             tiles[i].boxMode, tiles[i].repeat, command, macro);
  }
}

void renderDevicePage() {
  applyRuntimeTheme(devices[activeDevice].themePath);
  configureContent(42, LCD_H - 62, true);
  uiCommandBindingCount = 0;
  lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLL_MOMENTUM);
  lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLL_ELASTIC);
  lv_obj_set_scroll_dir(content, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_ACTIVE);
  lv_obj_set_style_pad_bottom(content, 12, 0);
  if (activeDevice < 0) activeDevice = 0;
  renderTopBar(devices[activeDevice].name, true);

  uint8_t count = devices[activeDevice].commandCount;
  for (uint8_t i = 0; i < count; i++) {
    makeTile(devices[activeDevice].commands[i].slot, devices[activeDevice].commands[i].label,
             devices[activeDevice].commands[i].iconPath ? devices[activeDevice].commands[i].iconPath : "",
             devices[activeDevice].commands[i].showText,
             devices[activeDevice].commands[i].boxMode,
             devices[activeDevice].commands[i].repeatDefault,
             &devices[activeDevice].commands[i], nullptr);
  }
}

void renderButtonDiagnosticPage() {
  setCinematicBackground(false);
  configureContent(42, 300, true);
  lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(content, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_AUTO);
  renderTopBar("Buttons", true);

  makeLabel(content, "Rev 5 raw keypad mapper", 10, 8,
            &lv_font_montserrat_12, lvRgb(110, 190, 255));
  if (buttonDiagnosticRequested < PHYSICAL_BUTTON_COUNT) {
    char progress[32];
    snprintf(progress, sizeof(progress), "Button %u of %u", buttonDiagnosticRequested + 1,
             PHYSICAL_BUTTON_COUNT);
    makeLabel(content, progress, 10, 33, &lv_font_montserrat_10, lvRgb(155, 165, 180));
    makeLabel(content, "Press:", 10, 61, &lv_font_montserrat_12, textPrimary());
    lv_obj_t *request = makeLabel(content, PHYSICAL_BUTTON_NAMES[buttonDiagnosticRequested],
                                  10, 84, &lv_font_montserrat_20, textPrimary());
    lv_obj_set_width(request, 220);
    lv_label_set_long_mode(request, LV_LABEL_LONG_WRAP);
    if (buttonDiagnosticRequested > 0) {
      uint8_t last = buttonDiagnosticRequested - 1;
      char raw[112];
      snprintf(raw, sizeof(raw),
               "Last: %s\nKey %u  S%u\nRow %u  Col %u\nIRQ GPIO%d",
               PHYSICAL_BUTTON_NAMES[last], buttonDiagnosticKeys[last],
               buttonDiagnosticSwitches[last], buttonDiagnosticRows[last],
               buttonDiagnosticColumns[last], PIN_TCA_INT);
      makeLabel(content, raw, 10, 132, &lv_font_montserrat_12, lvRgb(190, 205, 220));
    }
    return;
  }

  makeLabel(content, "Complete - raw mapping", 10, 34,
            &lv_font_montserrat_12, textPrimary());
  makeLabel(content, "Swipe or tap title to leave", 10, 55,
            &lv_font_montserrat_10, lvRgb(155, 165, 180));
  for (uint8_t i = 0; i < PHYSICAL_BUTTON_COUNT; i++) {
    char line[72];
    snprintf(line, sizeof(line), "%s  K%u S%u R%u C%u", PHYSICAL_BUTTON_NAMES[i],
             buttonDiagnosticKeys[i], buttonDiagnosticSwitches[i],
             buttonDiagnosticRows[i], buttonDiagnosticColumns[i]);
    makeLabel(content, line, 10, 80 + i * 17, &lv_font_montserrat_10, textPrimary());
  }
}

void completeUnlock(void *unused) {
  (void)unused;
  lockActive = false;
  if (lockOverlay) {
    lv_obj_del(lockOverlay);
    lockOverlay = nullptr;
  }
  drawDots();
  lv_refr_now(nullptr);
}

void unlockSliderEvent(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *slider = lv_event_get_target(e);
  if (code == LV_EVENT_RELEASED) {
    if (lv_slider_get_value(slider) >= 92) {
      lv_slider_set_value(slider, 100, LV_ANIM_OFF);
      lv_async_call(completeUnlock, nullptr);
    } else {
      lv_slider_set_value(slider, 0, LV_ANIM_ON);
    }
  } else if (code == LV_EVENT_PRESS_LOST) {
    lv_slider_set_value(slider, 0, LV_ANIM_ON);
  }
}

void renderUnlockPage() {
  lockOverlay = lv_obj_create(screenRoot);
  lv_obj_remove_style_all(lockOverlay);
  lv_obj_set_pos(lockOverlay, 0, 0);
  lv_obj_set_size(lockOverlay, LCD_W, LCD_H);
  lv_obj_set_style_bg_color(lockOverlay, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(lockOverlay, LV_OPA_COVER, 0);
  lv_obj_clear_flag(lockOverlay, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_move_foreground(lockOverlay);

  char timeText[12] = "--:--";
  struct tm timeInfo;
  if (getLocalTime(&timeInfo, 5)) {
    strftime(timeText, sizeof(timeText), "%l:%M", &timeInfo);
    if (timeText[0] == ' ') memmove(timeText, timeText + 1, strlen(timeText));
  }
  lv_obj_t *time = makeLabel(lockOverlay, timeText, 0, 66, &lv_font_montserrat_20, textPrimary());
  lv_obj_set_width(time, LCD_W);
  lv_obj_set_style_text_align(time, LV_TEXT_ALIGN_CENTER, 0);
  makeLabel(lockOverlay, "OpenRemote", 79, 96, &lv_font_montserrat_12, lvRgb(190, 205, 220));

  lv_obj_t *slider = lv_slider_create(lockOverlay);
  lv_obj_set_pos(slider, 20, 246);
  lv_obj_set_size(slider, 200, 42);
  lv_slider_set_range(slider, 0, 100);
  lv_slider_set_value(slider, 0, LV_ANIM_OFF);
  lv_obj_set_style_radius(slider, 21, LV_PART_MAIN);
  lv_obj_set_style_bg_color(slider, lv_color_black(), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(slider, LV_OPA_60, LV_PART_MAIN);
  lv_obj_set_style_bg_color(slider, lvRgb(65, 145, 230), LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(slider, LV_OPA_70, LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(slider, lv_color_white(), LV_PART_KNOB);
  lv_obj_set_style_pad_all(slider, 13, LV_PART_KNOB);
  lv_obj_add_event_cb(slider, unlockSliderEvent, LV_EVENT_ALL, nullptr);
  lv_obj_t *hint = makeLabel(lockOverlay, "slide to unlock", 0, 260, &lv_font_montserrat_12, textPrimary());
  lv_obj_set_width(hint, LCD_W);
  lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_move_background(hint);
}

void pageTranslateAnimation(void *target, int32_t value) {
  lv_obj_t *obj = static_cast<lv_obj_t *>(target);
  if (obj && lv_obj_is_valid(obj)) lv_obj_set_style_translate_x(obj, value, 0);
}

void pageOpacityAnimation(void *target, int32_t value) {
  lv_obj_t *obj = static_cast<lv_obj_t *>(target);
  if (obj && lv_obj_is_valid(obj)) lv_obj_set_style_opa(obj, (lv_opa_t)value, 0);
}

void animatePageEntrance(lv_obj_t *obj, int8_t direction) {
  if (!obj || !direction) return;
  lv_anim_del(obj, pageTranslateAnimation);
  lv_obj_set_style_translate_x(obj, direction > 0 ? 16 : -16, 0);
  lv_obj_set_style_opa(obj, LV_OPA_COVER, 0);

  lv_anim_t slide;
  lv_anim_init(&slide);
  lv_anim_set_var(&slide, obj);
  lv_anim_set_exec_cb(&slide, pageTranslateAnimation);
  lv_anim_set_values(&slide, direction > 0 ? 16 : -16, 0);
  lv_anim_set_time(&slide, 110);
  lv_anim_set_path_cb(&slide, lv_anim_path_ease_out);
  lv_anim_start(&slide);
}

void beginUiMutation() {
  if (uiMutationDepth++ > 0) return;
  pageStripRendering = true;
  if (touchInputDevice) lv_indev_reset(touchInputDevice, nullptr);
  lvTouchDown = false;
  touchWasDown = false;
  touchPendingConfirmCount = 0;
  activityDragActive = false;
  touchQuarantineActive = true;
  touchQuarantineStartedMs = millis();
  touchAcceptAfterMs = millis() + 80UL;
  touchReleasedSinceMs = 0;
}

void endUiMutation() {
  if (uiMutationDepth == 0 || --uiMutationDepth > 0) return;
  if (touchInputDevice) lv_indev_reset(touchInputDevice, nullptr);
  lvTouchDown = false;
  touchWasDown = false;
  touchPendingConfirmCount = 0;
  activityDragActive = false;
  touchQuarantineActive = true;
  touchQuarantineStartedMs = millis();
  touchAcceptAfterMs = millis() + 80UL;
  touchReleasedSinceMs = 0;
  pageStripRendering = false;
}

void renderCurrentPage() {
  beginUiMutation();
  if (!boundPageUi) bindPageUi(currentPage);
  clearModalObjects();
  // A scrollable page can retain its displacement after its children are
  // deleted. Reset it while the old page still has valid scroll extents.
  lv_obj_scroll_to(content, 0, 0, LV_ANIM_OFF);
  lv_anim_del(content, pageTranslateAnimation);
  lv_anim_del(content, pageOpacityAnimation);
  lv_anim_del(topBar, pageTranslateAnimation);
  lv_anim_del(topBar, pageOpacityAnimation);
  lv_obj_set_style_translate_x(content, 0, 0);
  lv_obj_set_style_translate_x(topBar, 0, 0);
  lv_obj_set_style_opa(content, LV_OPA_COVER, 0);
  lv_obj_set_style_opa(topBar, LV_OPA_COVER, 0);
  lv_obj_clean(topBar);
  lv_obj_clean(content);
  memset(batteryMetricNameLabels, 0, sizeof(batteryMetricNameLabels));
  memset(batteryMetricValueLabels, 0, sizeof(batteryMetricValueLabels));
  memset(displayValueLabels, 0, sizeof(displayValueLabels));
  memset(buttonValueLabels, 0, sizeof(buttonValueLabels));
  memset(debugRowDropdowns, 0, sizeof(debugRowDropdowns));
  memset(scrollSafeSliderStates, 0, sizeof(scrollSafeSliderStates));
  scrollSafeSliderCount = 0;
  buttonTestPanel = nullptr;
  buttonTestLabel = nullptr;
  clearPageIconCache();
  setupApStatusLabel = nullptr;
  resetSplitDiagnosticAnchor();

  switch (pages[currentPage].kind) {
    case PAGE_REMOTE_SETTINGS: renderSettingsPage(); break;
    case PAGE_ACTIVITIES: renderActivitiesPage(); break;
    case PAGE_ACTIVITY: renderActivityPage(); break;
    case PAGE_DEVICE: renderDevicePage(); break;
  }

  if (lockActive) renderUnlockPage();
  drawDots();
  updateSplitDiagnostic();
  pendingPageTransition = 0;
  configurePageStripDirections();
  endUiMutation();
}

void bindPageUi(uint8_t index) {
  if (index >= PAGE_SLOT_COUNT) return;
  boundPageUi = &pageUi[index];
  screenRoot = boundPageUi->root;
  wallpaper = boundPageUi->wallpaper;
  topBar = boundPageUi->topBar;
  content = boundPageUi->content;
  dots = boundPageUi->dots;
  clockLabel = boundPageUi->clockLabel;
  statusPill = boundPageUi->statusPill;
  statusBattery = boundPageUi->statusBattery;
  statusBatteryTerminal = boundPageUi->statusBatteryTerminal;
  batteryFill = boundPageUi->batteryFill;
  uiCommandBindings = boundPageUi->commandBindings;
  activitySliderUi = boundPageUi->sliderUi;
}

void configurePageStripDirections() {
  if (!pageStrip) return;
  for (uint8_t i = 0; i < PAGE_SLOT_COUNT; i++) {
    lv_dir_t direction = LV_DIR_NONE;
    if (i < pageCount) {
      if (i > 0) direction = (lv_dir_t)(direction | LV_DIR_LEFT);
      if (i + 1 < pageCount) direction = (lv_dir_t)(direction | LV_DIR_RIGHT);
      if (i == 0 && settingsView != SETTINGS_HOME) direction = LV_DIR_NONE;
    }
    reinterpret_cast<lv_tileview_tile_t *>(pageUi[i].tile)->dir = direction;
  }
  lv_obj_t *activeTile = lv_tileview_get_tile_act(pageStrip);
  if (activeTile) {
    lv_obj_set_scroll_dir(pageStrip,
      reinterpret_cast<lv_tileview_tile_t *>(activeTile)->dir);
  }
}

void renderAllPageSlots() {
  if (!pageStrip || pageStripRendering) return;
  beginUiMutation();
  uint8_t active = min(currentPage, (uint8_t)(pageCount - 1));

  // Tile coordinates are produced by LVGL's layout pass. Selecting a tile
  // before that pass leaves the strip at x=0 even when currentPage is not 0,
  // so controls are built on a different tile from the wallpaper on screen.
  lv_obj_update_layout(pageStrip);

  for (uint8_t i = 0; i < pageCount; i++) {
    if (i == active) continue;
    currentPage = i;
    bindPageUi(i);
    renderCurrentPage();
  }

  // Invalidations generated for an off-screen tile are clipped by LVGL. Move
  // the strip first, then build the destination while it is the visible tile.
  // This preserves the persistent object strip and finger-following navigation
  // without leaving only the slot's top/bottom wallpaper bands on screen.
  currentPage = active;
  lv_obj_set_tile(pageStrip, pageUi[active].tile, LV_ANIM_OFF);
  lv_obj_update_layout(pageStrip);
  bindPageUi(active);
  renderCurrentPage();
  lv_obj_update_layout(pageUi[active].root);
  configurePageStripDirections();
  pendingUiRefresh = false;
  pageStripRebuildPending = false;
  endUiMutation();

  // Cleaning and rebuilding a transparent content layer invalidates several
  // overlapping regions. Submit one final complete frame only after every
  // replacement object exists, instead of allowing the old top/bottom bands
  // to remain around an undrawn content region.
  lv_obj_invalidate(lv_scr_act());
  lv_refr_now(nullptr);

  lv_area_t tileArea;
  lv_obj_get_coords(pageUi[active].tile, &tileArea);
  Serial.printf("Page strip ready: page=%u count=%u scroll=%d tile=(%d,%d)-(%d,%d) top=%u content=%u heap=%lu psram=%lu\n",
                currentPage, pageCount,
                (int)lv_obj_get_scroll_x(pageStrip),
                tileArea.x1, tileArea.y1, tileArea.x2, tileArea.y2,
                (unsigned)lv_obj_get_child_cnt(topBar),
                (unsigned)lv_obj_get_child_cnt(content),
                (unsigned long)ESP.getFreeHeap(), (unsigned long)ESP.getFreePsram());
}

void requestPageStripRebuild() {
  pageStripRebuildPending = true;
}

void pageStripEvent(lv_event_t *event) {
  if (lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED || pageStripRendering) return;
  lv_obj_t *activeTile = lv_tileview_get_tile_act(pageStrip);
  for (uint8_t i = 0; i < pageCount; i++) {
    if (pageUi[i].tile == activeTile) {
      pageStripPendingPage = i;
      pageStripChangePending = true;
      return;
    }
  }
}

void servicePageStripChange() {
  if (!pageStripChangePending || touchWasDown || lvTouchDown ||
      (pageStrip && lv_obj_is_scrolling(pageStrip))) return;
  pageStripChangePending = false;
  uint8_t previous = currentPage;
  uint8_t target = min(pageStripPendingPage, (uint8_t)(pageCount - 1));

  if (pages[previous].kind == PAGE_DEVICE && target != previous) {
    activeDevice = -1;
    buttonDiagnosticActive = false;
    rebuildPages();
    target = min(deviceReturnPage, (uint8_t)(pageCount - 1));
    currentPage = target;
    renderAllPageSlots();
  } else {
    currentPage = target;
    bindPageUi(currentPage);
    configurePageStripDirections();
  }
  applyBluetoothState();
  lastWakeMs = millis();
}

// ---------------------------------------------------------------------------
// Modal controls
// ---------------------------------------------------------------------------

void deviceChoiceEvent(lv_event_t *e) {
  int index = (int)(intptr_t)lv_event_get_user_data(e);
  if (index < 0 || index >= DEVICE_COUNT) return;
  // Let LVGL finish dispatching the event before the main loop deletes the
  // picker object tree and builds the selected device page.
  pendingDeviceOpen = index;
  if (deviceModal) lv_obj_add_flag(deviceModal, LV_OBJ_FLAG_HIDDEN);
}

void showDevicePicker() {
  if (deviceModal) {
    lv_obj_del(deviceModal);
    deviceModal = nullptr;
    return;
  }

  const int modalY = 44;
  const int pageDotsTop = 292;
  const int rowHeight = 32;
  const int pickerCount = DEVICE_COUNT;
  const int desiredHeight = 44 + pickerCount * rowHeight;
  const int modalHeight = min(pageDotsTop - modalY, desiredHeight);

  deviceModal = lv_obj_create(screenRoot);
  lv_obj_set_pos(deviceModal, 8, modalY);
  lv_obj_set_size(deviceModal, 224, modalHeight);
  stylePanel(deviceModal, lvRgb(18, 22, 30), lvRgb(60, 180, 220), LV_OPA_COVER);
  lv_obj_clear_flag(deviceModal, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(deviceModal, LV_DIR_NONE);
  makeLabel(deviceModal, "Devices", 8, 4, &lv_font_montserrat_16, textPrimary());

  lv_obj_t *list = lv_obj_create(deviceModal);
  lv_obj_remove_style_all(list);
  lv_obj_set_pos(list, 8, 30);
  lv_obj_set_size(list, 204, modalHeight - 42);
  lv_obj_set_scroll_dir(list, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_AUTO);
  lv_obj_set_style_pad_all(list, 0, 0);
  if (pickerCount * rowHeight > modalHeight - 42) {
    lv_obj_add_flag(list, LV_OBJ_FLAG_SCROLLABLE);
  } else {
    lv_obj_clear_flag(list, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);
  }

  for (uint8_t i = 0; i < DEVICE_COUNT; i++) {
    lv_obj_t *btn = makeButton(list, devices[i].name, 0, i * rowHeight, 204, 28, lvRgb(34, 42, 56));
    lv_obj_add_event_cb(btn, deviceChoiceEvent, LV_EVENT_CLICKED, (void *)(intptr_t)i);
  }
  lv_obj_move_foreground(deviceModal);
}

void brightnessEvent(lv_event_t *e) {
  brightness = lv_slider_get_value(lv_event_get_target(e));
  applyBrightness();
  if (lv_event_get_code(e) == LV_EVENT_RELEASED) {
    saveSettings();
    scheduleRuntimeSettingsSave();
  }
  lastWakeMs = millis();
  brightnessLastActivityMs = millis();
}

void closeBrightnessPanel() {
  if (brightnessOverlay) {
    lv_obj_del(brightnessOverlay);
    brightnessOverlay = nullptr;
    brightnessPanel = nullptr;
  } else if (brightnessPanel) {
    lv_obj_del(brightnessPanel);
    brightnessPanel = nullptr;
  }
  if (brightnessBatteryLabel) {
    lv_obj_del(brightnessBatteryLabel);
    brightnessBatteryLabel = nullptr;
  }
  if (statusBattery) lv_obj_clear_flag(statusBattery, LV_OBJ_FLAG_HIDDEN);
  if (statusBatteryTerminal) lv_obj_clear_flag(statusBatteryTerminal, LV_OBJ_FLAG_HIDDEN);
  brightnessLastActivityMs = 0;
}

void brightnessOverlayEvent(lv_event_t *e) {
  if (lv_event_get_target(e) == brightnessOverlay) closeBrightnessPanel();
}

void toggleBrightnessPanel() {
  if (brightnessOverlay || brightnessPanel) {
    closeBrightnessPanel();
    return;
  }

  brightnessOverlay = lv_obj_create(screenRoot);
  lv_obj_remove_style_all(brightnessOverlay);
  lv_obj_set_pos(brightnessOverlay, 0, 0);
  lv_obj_set_size(brightnessOverlay, LCD_W, LCD_H);
  lv_obj_set_style_bg_opa(brightnessOverlay, LV_OPA_TRANSP, 0);
  lv_obj_add_flag(brightnessOverlay, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(brightnessOverlay, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(brightnessOverlay, brightnessOverlayEvent, LV_EVENT_CLICKED, nullptr);

  brightnessPanel = lv_obj_create(brightnessOverlay);
  lv_obj_set_pos(brightnessPanel, 192, 40);
  lv_obj_set_size(brightnessPanel, 40, 244);
  stylePanel(brightnessPanel, lvRgb(18, 22, 30), lv_color_white(), (lv_opa_t)205);
  lv_obj_set_style_border_opa(brightnessPanel, LV_OPA_30, 0);
  lv_obj_clear_flag(brightnessPanel, LV_OBJ_FLAG_SCROLLABLE);

  if (statusBattery) lv_obj_add_flag(statusBattery, LV_OBJ_FLAG_HIDDEN);
  if (statusBatteryTerminal) lv_obj_add_flag(statusBatteryTerminal, LV_OBJ_FLAG_HIDDEN);
  float batteryPercent = readBatteryPercent();
  char batteryText[8];
  snprintf(batteryText, sizeof(batteryText), batteryPercent >= 0.0f ? "%d%%" : "--%%",
           batteryPercent >= 0.0f ? (int)roundf(batteryPercent) : 0);
  brightnessBatteryLabel = makeLabel(statusPill, batteryText, clockEnabled ? 59 : 4, 9,
                                     &lv_font_montserrat_10, textPrimary());
  lv_obj_set_width(brightnessBatteryLabel, 32);
  lv_obj_set_style_text_align(brightnessBatteryLabel, LV_TEXT_ALIGN_CENTER, 0);

  lv_obj_t *slider = lv_slider_create(brightnessPanel);
  lv_obj_set_size(slider, 14, 184);
  lv_obj_align(slider, LV_ALIGN_CENTER, 0, 8);
  lv_slider_set_range(slider, 0, 100);
  lv_slider_set_value(slider, brightness, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(slider, lvRgb(75, 78, 86), LV_PART_MAIN);
  lv_obj_set_style_bg_color(slider, lv_color_white(), LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(slider, lv_color_white(), LV_PART_KNOB);
  lv_obj_set_style_pad_all(slider, 5, LV_PART_KNOB);
  lv_obj_add_event_cb(slider, brightnessEvent, LV_EVENT_VALUE_CHANGED, nullptr);
  lv_obj_add_event_cb(slider, brightnessEvent, LV_EVENT_RELEASED, nullptr);

  brightnessLastActivityMs = millis();
  lv_obj_move_foreground(brightnessOverlay);
}

// ---------------------------------------------------------------------------
// Actions and gestures
// ---------------------------------------------------------------------------

void activateActivity(uint8_t index) {
  if (index >= ACTIVITY_COUNT) return;
  Serial.printf("Activate activity: %s\n", activities[index].name);
  activeActivity = index;
  activeDevice = -1;
  buttonDiagnosticActive = false;
  rebuildPages();
  currentPage = 2;
  pendingPageTransition = 1;
  renderAllPageSlots();
  applyBluetoothState();
  activitySequenceMacro = -1;
  activitySequenceActivity = index;
  activitySequenceStep = 0;
  activitySequenceNextMs = millis();
  activitySequencePoweredOnMask = 0;
  activitySequenceActive = activities[index].stepCount > 0;
  serviceActivitySequence(millis());
}

void openDevice(uint8_t index) {
  if (index >= DEVICE_COUNT) return;
  Serial.printf("Open device page: %s\n", devices[index].name);
  if (pages[currentPage].kind != PAGE_DEVICE) deviceReturnPage = currentPage;
  activeDevice = index;
  buttonDiagnosticActive = false;
  rebuildPages();
  currentPage = pageCount - 1;
  pendingPageTransition = 1;
  renderAllPageSlots();
  applyBluetoothState();
}

void openButtonDiagnostic() {
  Serial.println("Open Buttons diagnostic page");
  activeDevice = -1;
  buttonDiagnosticActive = true;
  buttonDiagnosticRequested = 0;
  memset(buttonDiagnosticKeys, 0, sizeof(buttonDiagnosticKeys));
  memset(buttonDiagnosticSwitches, 0, sizeof(buttonDiagnosticSwitches));
  memset(buttonDiagnosticRows, 0, sizeof(buttonDiagnosticRows));
  memset(buttonDiagnosticColumns, 0, sizeof(buttonDiagnosticColumns));
  rebuildPages();
  currentPage = pageCount - 1;
  pendingPageTransition = 1;
  renderAllPageSlots();
}

void changePage(int delta) {
  if (!pageStrip || delta == 0) return;
  activityDragActive = false;
  int next = (int)currentPage + delta;
  if (next < 0) next = 0;
  if (next >= pageCount) next = pageCount - 1;
  if (next == currentPage) return;
  pageStripPendingPage = (uint8_t)next;
  lv_obj_set_tile(pageStrip, pageUi[next].tile, LV_ANIM_ON);
}

void rootGestureEvent(lv_event_t *e) {
  lv_indev_t *indev = lv_indev_get_act();
  if (!indev) return;
  lv_dir_t dir = lv_indev_get_gesture_dir(indev);
  if (dir == LV_DIR_LEFT) changePage(1);
  if (dir == LV_DIR_RIGHT) changePage(-1);
}

// ---------------------------------------------------------------------------
// Sleep and heartbeat
// ---------------------------------------------------------------------------

void captureSleepBaseline() {
  if (!lis3dhReady || !readLIS3DH(sleepBaseX, sleepBaseY, sleepBaseZ)) {
    sleepBaseX = sleepBaseY = sleepBaseZ = 0;
  }
}

void neutraliseRev5DisplayBusForDeepSleep() {
  const int displayPins[] = {
    PIN_LCD_DC, PIN_LCD_CS, PIN_LCD_WR, PIN_LCD_RD,
    PIN_LCD_D0, PIN_LCD_D1, PIN_LCD_D2, PIN_LCD_D3,
    PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7
  };
  for (int pin : displayPins) {
    pinMode(pin, INPUT_PULLDOWN);
  }
}

void isolateRev5SdBusForDeepSleep() {
  pinMode(PIN_SD_CS, INPUT_PULLDOWN);
  pinMode(PIN_SD_MISO, INPUT_PULLDOWN);
  pinMode(PIN_SD_MOSI, INPUT_PULLDOWN);
  pinMode(PIN_SD_SCK, INPUT_PULLDOWN);
}

void isolateRev5I2cBusForDeepSleep() {
  // Wake sources are already configured and latched before this runs. Driving
  // the bus low briefly, then parking both lines low, avoids feeding the
  // unpowered touch controller through its I2C protection diodes.
  Wire.end();
  pinMode(PIN_I2C_SCL, OUTPUT);
  digitalWrite(PIN_I2C_SCL, LOW);
  delay(2);
  pinMode(PIN_I2C_SDA, OUTPUT);
  digitalWrite(PIN_I2C_SDA, LOW);
  pinMode(PIN_I2C_SCL, INPUT_PULLDOWN);
  pinMode(PIN_I2C_SDA, INPUT_PULLDOWN);
}

void releaseDeepSleepPinHolds() {
  gpio_deep_sleep_hold_dis();
  gpio_hold_dis((gpio_num_t)PIN_LCD_EN);
  gpio_hold_dis((gpio_num_t)PIN_LCD_BL);
  gpio_hold_dis((gpio_num_t)PIN_BUTTON_BL);
  gpio_hold_dis((gpio_num_t)PIN_IR_VCC);
  gpio_hold_dis((gpio_num_t)PIN_IR_LED);
  gpio_hold_dis((gpio_num_t)PIN_SD_EN);
  gpio_hold_dis((gpio_num_t)PIN_MIC_POWER);
  gpio_hold_dis((gpio_num_t)PIN_ACC_INT);
  gpio_hold_dis((gpio_num_t)PIN_TCA_INT);
}

void saveDeepSleepRuntimeState() {
  memset(&deepSleepRuntimeState, 0, sizeof(deepSleepRuntimeState));
  deepSleepRuntimeState.magic = DEEP_SLEEP_RUNTIME_MAGIC;
  if (activeActivity >= 0 && activeActivity < ACTIVITY_COUNT) {
    strlcpy(deepSleepRuntimeState.activityId, activities[activeActivity].id,
            sizeof(deepSleepRuntimeState.activityId));
  }
  if (activeDevice >= 0 && activeDevice < DEVICE_COUNT) {
    strlcpy(deepSleepRuntimeState.deviceId, devices[activeDevice].id,
            sizeof(deepSleepRuntimeState.deviceId));
  }
  deepSleepRuntimeState.powerMemoryCount =
    min(powerMemoryCount, (uint8_t)MAX_RUNTIME_DEVICES);
  memcpy(deepSleepRuntimeState.powerMemory, powerMemory,
         deepSleepRuntimeState.powerMemoryCount * sizeof(PowerMemoryEntry));
}

void restoreDeepSleepRuntimeState(esp_sleep_wakeup_cause_t wakeCause) {
  if ((wakeCause != ESP_SLEEP_WAKEUP_EXT1 && wakeCause != ESP_SLEEP_WAKEUP_TIMER) ||
      deepSleepRuntimeState.magic != DEEP_SLEEP_RUNTIME_MAGIC) {
    deepSleepRuntimeState.magic = 0;
    return;
  }

  powerMemoryCount =
    min(deepSleepRuntimeState.powerMemoryCount, (uint8_t)MAX_RUNTIME_DEVICES);
  memcpy(powerMemory, deepSleepRuntimeState.powerMemory,
         powerMemoryCount * sizeof(PowerMemoryEntry));
  activeActivity = -1;
  activeDevice = -1;
  for (uint8_t i = 0; i < ACTIVITY_COUNT; i++) {
    if (strcmp(activities[i].id, deepSleepRuntimeState.activityId) == 0) {
      activeActivity = i;
      break;
    }
  }
  for (uint8_t i = 0; i < DEVICE_COUNT; i++) {
    if (strcmp(devices[i].id, deepSleepRuntimeState.deviceId) == 0) {
      activeDevice = i;
      break;
    }
  }
  Serial.printf("Deep sleep state: activity=%d device=%d power=%u\n",
                activeActivity, activeDevice, powerMemoryCount);
}

bool enterDeepPowerSleep(bool allowQrPage) {
  if (!lis3dhReady || !raiseToWake ||
      (webConfigQrPageActive() && !allowQrPage) ||
      webConfigTransferActive || usbSdTransferActive() || ntpSyncPending ||
      bluetoothActivitySessionRequired()) {
    Serial.printf(
      "Deep sleep deferred: accelerometer=%s raise=%s qr=%s transfer=%s usb=%s ntp=%s ble=%s\n",
      lis3dhReady ? "ready" : "missing", raiseToWake ? "on" : "off",
      webConfigQrPageActive() ? "active" : "off",
      webConfigTransferActive ? "active" : "off",
      usbSdTransferActive() ? "active" : "off",
      ntpSyncPending ? "active" : "off",
      bluetoothActivitySessionRequired() ? "required" : "off");
    return false;
  }

  serviceKeypad(millis());
  if (!configureLis3dhDeepSleepWake() || !waitForDeepWakeInputsIdle()) {
    configureLis3dhAwake();
    Serial.println("Deep sleep deferred: Rev 5 wake input active");
    return false;
  }
  saveDeepSleepRuntimeState();
  serviceBatteryHistory(millis(), true);
  if (bleReady) stopBluetoothRadio("deep sleep");
  stopNetworkStack();
  IrReceiver.stop();
  pinMode(PIN_IR_RX, INPUT);
  digitalWrite(PIN_IR_LED, HIGH);
  digitalWrite(PIN_IR_VCC, LOW);
  setLcdControllerSleeping(true);
  buttonBacklight(false);
  if (backlightPwmReady) ledcWrite(PIN_LCD_BL, BACKLIGHT_PWM_MAX);
  SD.end();
  digitalWrite(PIN_SD_EN, HIGH);
  isolateRev5SdBusForDeepSleep();

  // The unpowered ILI9341 can otherwise be back-fed through any parallel-bus
  // signal left high. Park every data and control signal low before removing
  // panel and touch power.
  neutraliseRev5DisplayBusForDeepSleep();
  digitalWrite(PIN_LCD_EN, HIGH);

  // External gate pulls keep the switched rails off. Input modes avoid output
  // pad leakage; deep-sleep holds preserve those states when GPIO power drops.
  if (backlightPwmReady) ledcDetach(PIN_LCD_BL);
  if (buttonBacklightPwmReady) ledcDetach(PIN_BUTTON_BL);
  pinMode(PIN_LCD_BL, INPUT_PULLUP);
  pinMode(PIN_LCD_EN, INPUT_PULLUP);
  pinMode(PIN_BUTTON_BL, INPUT_PULLDOWN);
  pinMode(PIN_IR_VCC, INPUT_PULLDOWN);
  pinMode(PIN_IR_LED, INPUT_PULLUP);
  pinMode(PIN_IR_RX, INPUT);
  pinMode(PIN_SD_EN, INPUT_PULLUP);
  pinMode(PIN_MIC_POWER, INPUT_PULLDOWN);
  pinMode(PIN_CHARGE_STATUS, INPUT_PULLDOWN);

  isolateRev5I2cBusForDeepSleep();

  gpio_hold_en((gpio_num_t)PIN_LCD_EN);
  gpio_hold_en((gpio_num_t)PIN_LCD_BL);
  gpio_hold_en((gpio_num_t)PIN_BUTTON_BL);
  gpio_hold_en((gpio_num_t)PIN_IR_VCC);
  gpio_hold_en((gpio_num_t)PIN_IR_LED);
  gpio_hold_en((gpio_num_t)PIN_SD_EN);
  gpio_hold_en((gpio_num_t)PIN_MIC_POWER);
  gpio_hold_en((gpio_num_t)PIN_ACC_INT);
  gpio_hold_en((gpio_num_t)PIN_TCA_INT);
  gpio_deep_sleep_hold_en();

  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
  esp_sleep_enable_ext1_wakeup(DEEP_SLEEP_WAKE_MASK, ESP_EXT1_WAKEUP_ANY_LOW);
  if (clockUseInternetTime) {
    time_t epoch = time(nullptr);
    if (epoch > 1700000000) {
      tm nextSync = {};
      localtime_r(&epoch, &nextSync);
      nextSync.tm_hour = 3;
      nextSync.tm_min = 0;
      nextSync.tm_sec = 0;
      time_t nextEpoch = mktime(&nextSync);
      if (nextEpoch <= epoch) nextEpoch += 86400;
      esp_sleep_enable_timer_wakeup((uint64_t)(nextEpoch - epoch) * 1000000ULL);
    }
  }
  Serial.printf("Deep sleep: Rev 5 EXT1 armed mask=0x%llX acc=%d keypad=%d\n",
                DEEP_SLEEP_WAKE_MASK, digitalRead(PIN_ACC_INT),
                digitalRead(PIN_TCA_INT));
  Serial.flush();
  delay(20);
  esp_deep_sleep_start();
  return true;
}

bool configureApplicationPowerMode(bool connectedIdle) {
  (void)connectedIdle;
  // The pinned Arduino framework already enables tickless idle and Bluetooth
  // controller modem sleep. Reconfiguring global DFS after the live UART/BLE
  // stack starts is unsafe on Rev 5, so BLE idle uses the proven fixed 80 MHz
  // clock and IR-only activities retain explicit light/deep sleep.
  powerManagementReady = false;
  return false;
}

void enterBleConnectedIdle() {
  if (bleConnectedIdleActive || !displaySleeping ||
      !bluetoothActivitySessionRequired() || webConfigQrPageActive() ||
      webConfigTransferActive || usbSdTransferActive() || ntpSyncPending ||
      wifiConnectPending) return;

  // ESP-IDF's Bluetooth controller owns a power-management lock while radio
  // work is due. Between connection events, tickless idle may stop the CPU
  // without dropping HID or preventing an IR/button/motion wake.
  if (networkStackActive || WiFi.getMode() != WIFI_OFF) stopNetworkStack();
  lcdBacklight(false);
  digitalWrite(PIN_SD_CS, HIGH);

  bleConnectedIdleRestoreCpuMhz = getCpuFrequencyMhz();
  bool accelerometerWake = configureLis3dhBleIdleOrientation();
  if (accelerometerWake) {
    delay(80);
    captureSleepBaseline();
  }
  bool keypadWake = tca8418Ready;
  bleIdleAccelerometerWake = accelerometerWake;
  bleIdleKeypadWake = keypadWake;
  bleIdleUartWake = false;

  bleConnectedIdleActive = true;
  nextBleConnectedIdleMotionMs = millis();
  bleIdleMotionAboveSinceMs = 0;
  Serial.printf("BLE connected idle: HID retained, PM=%s, motion=%s angle=%u deg keypad=%s USB=on\n",
                powerManagementReady ? "DFS/modem" : "80MHz/modem",
                bleIdleAccelerometerWake ? "on" : "off",
                motionWakeAngleDegrees(),
                bleIdleKeypadWake ? "on" : "off");
  Serial.flush();

  // The BLE controller independently enters modem sleep between its scheduled
  // connection windows. The application uses the proven fixed 80 MHz clock.
  if (!configureApplicationPowerMode(true)) {
    if (bleConnectedIdleRestoreCpuMhz > BLE_CONNECTED_IDLE_MAX_CPU_MHZ &&
        !setCpuFrequencyMhz(BLE_CONNECTED_IDLE_MAX_CPU_MHZ)) {
      Serial.println("BLE connected idle: CPU reduction unavailable");
    }
  }
}

void leaveBleConnectedIdle() {
  if (!bleConnectedIdleActive) return;
  bleIdleAccelerometerWake = false;
  bleIdleKeypadWake = false;
  bleIdleUartWake = false;

  uint32_t restoreMhz = bleConnectedIdleRestoreCpuMhz
    ? bleConnectedIdleRestoreCpuMhz : 240;
  if (!configureApplicationPowerMode(false) &&
      getCpuFrequencyMhz() != restoreMhz && !setCpuFrequencyMhz(restoreMhz)) {
    Serial.printf("BLE connected idle: could not restore %lu MHz\n",
                  (unsigned long)restoreMhz);
  }
  configureLis3dhAwake();
  // The controller retains the host-negotiated HID interval. Asking Bluedroid
  // to renegotiate at the same instant modem sleep starts can block its GAP
  // task; native controller PM already wakes only for scheduled radio events.
  bleIdleConnectionProfileRequested = false;

  // Keep both backlights hidden until wakeDisplay() reveals a complete frame.
  lcdBacklight(false);

  bleConnectedIdleActive = false;
  nextBleConnectedIdleMotionMs = 0;
  bleIdleMotionAboveSinceMs = 0;
  Serial.printf("BLE connected idle: awake, CPU=%lu MHz\n",
                (unsigned long)getCpuFrequencyMhz());
}

void enterLowPowerWait() {
  if (!displaySleeping || webConfigQrPageActive() || webConfigTransferActive ||
      usbSdTransferActive() || ntpSyncPending || wifiConnectPending ||
      bluetoothActivitySessionRequired()) return;

  serviceKeypad(millis());
  bool accelerometerWake = configureLis3dhMotionWake();
  bool keypadWake = tca8418Ready;
  if ((!accelerometerWake && !keypadWake) ||
      !waitForWakeInputsIdle(accelerometerWake, keypadWake)) {
    configureLis3dhAwake();
    return;
  }
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
  esp_err_t accelerometerPinResult = ESP_OK;
  esp_err_t keypadPinResult = ESP_OK;
  esp_err_t gpioWakeResult = ESP_OK;
  if (accelerometerWake) {
    accelerometerPinResult = gpio_wakeup_enable(
      (gpio_num_t)PIN_ACC_INT, GPIO_INTR_HIGH_LEVEL);
  }
  if (keypadWake) {
    keypadPinResult = gpio_wakeup_enable(
      (gpio_num_t)PIN_TCA_INT, GPIO_INTR_LOW_LEVEL);
  }
  if (accelerometerPinResult == ESP_OK && keypadPinResult == ESP_OK) {
    gpioWakeResult = esp_sleep_enable_gpio_wakeup();
  }
  if (accelerometerPinResult != ESP_OK || keypadPinResult != ESP_OK ||
      gpioWakeResult != ESP_OK) {
    if (accelerometerWake) gpio_wakeup_disable((gpio_num_t)PIN_ACC_INT);
    if (keypadWake) gpio_wakeup_disable((gpio_num_t)PIN_TCA_INT);
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_GPIO);
    configureLis3dhAwake();
    Serial.printf("Light sleep wake setup failed: motionPin=%d keypadPin=%d gpioWake=%d\n",
                  (int)accelerometerPinResult, (int)keypadPinResult,
                  (int)gpioWakeResult);
    return;
  }

  if (bleReady) stopBluetoothRadio("light sleep");
  stopNetworkStack();
  uint32_t idleMs = millis() -
    (displaySleepStartedMs ? displaySleepStartedMs : lastWakeMs);
  uint32_t deepSleepAfterMs = (uint32_t)deepSleepMinutes * 60UL * 1000UL;
  // Retry a deferred deep-sleep transition calmly. A one-millisecond timer
  // loop wastes power while an accelerometer or keypad interrupt is settling.
  uint64_t remainingUs = idleMs >= deepSleepAfterMs
    ? 5000000ULL : (uint64_t)(deepSleepAfterMs - idleMs) * 1000ULL;
  esp_sleep_enable_timer_wakeup(remainingUs);
  lightSleepArmed = true;
  Serial.printf("Light sleep: motion=%s keypad=%s\n",
                accelerometerWake ? "on" : "off", keypadWake ? "on" : "off");
  esp_err_t result = ESP_OK;
  esp_sleep_wakeup_cause_t cause = ESP_SLEEP_WAKEUP_UNDEFINED;
  int accelerometerLevel = LOW;
  int keypadLevel = HIGH;
  uint8_t motionSource = 0;
  uint8_t keypadSource = 0;
  uint8_t startupEventsIgnored = 0;
  do {
    result = esp_light_sleep_start();
    cause = esp_sleep_get_wakeup_cause();
    accelerometerLevel = digitalRead(PIN_ACC_INT);
    keypadLevel = digitalRead(PIN_TCA_INT);
    motionSource = 0;
    keypadSource = 0;
    if (lis3dhReady) readReg8(ADDR_LIS3DH, 0x31, motionSource);
    if (tca8418Ready) tcaReadRegister(0x02, keypadSource);

    bool startupEvent = cause == ESP_SLEEP_WAKEUP_GPIO &&
                        motionSource == 0x55 && keypadLevel == HIGH &&
                        keypadSource == 0 && startupEventsIgnored < 3;
    if (!startupEvent) break;
    startupEventsIgnored++;
    Serial.printf("LIS3DH startup event ignored (%u)\n", startupEventsIgnored);
    if (!waitForWakeInputsIdle(accelerometerWake, keypadWake, 600)) break;
  } while (true);
  lightSleepArmed = false;

  if (accelerometerWake) gpio_wakeup_disable((gpio_num_t)PIN_ACC_INT);
  if (keypadWake) gpio_wakeup_disable((gpio_num_t)PIN_TCA_INT);
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_GPIO);
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
  configureLis3dhAwake();
  Serial.printf("Light sleep wake: result=%d cause=%d acc=%d lis=0x%02X keypad=%d tca=0x%02X\n",
                (int)result, (int)cause, accelerometerLevel, motionSource,
                keypadLevel, keypadSource);

  if (cause == ESP_SLEEP_WAKEUP_TIMER) {
    enterDeepPowerSleep();
    // This timer is an internal power-management event, not user activity. If
    // deep sleep is deferred, remain dark and retry instead of lighting the LCD
    // and restarting the user's normal display-sleep timer.
    nextDeepSleepAttemptMs = millis() + 5000UL;
    return;
  }
  wakeDisplay();
}

void enterDisplaySleep() {
  if (displaySleeping) return;
  Serial.println("Display sleep: controller and backlights off");
  if (runtimeSettingsSavePending) {
    runtimeSettingsSavePending = !persistSettingsToRuntimeConfig();
    if (runtimeSettingsSavePending) runtimeSettingsSaveAtMs = millis() + 1000UL;
  }

  captureSleepBaseline();
  fadeBacklightsToOff();
  sleepTouchController();
  lv_obj_add_flag(uiRoot, LV_OBJ_FLAG_HIDDEN);
  lv_refr_now(nullptr);
  tft.fillScreen(TFT_BLACK);
  setLcdControllerSleeping(true);
  IrReceiver.stop();
  digitalWrite(PIN_IR_VCC, LOW);
  suspendBacklightPwmForSleep();
  displaySleeping = true;
  displaySleepStartedMs = millis();
  nextDeepSleepAttemptMs = displaySleepStartedMs +
    (uint32_t)deepSleepMinutes * 60UL * 1000UL;
  if (!webConfigQrPageActive() && !ntpSyncPending &&
      !webConfigTransferActive && !usbSdTransferActive()) {
    if (bluetoothActivitySessionRequired()) enterBleConnectedIdle();
    else enterLowPowerWait();
  }
}

void wakeDisplay() {
  Serial.println("Movement wake");
  // Restore full application speed before touching the LCD bus or beginning
  // an IR command from the physical key that caused this wake.
  leaveBleConnectedIdle();
  restoreBacklightPwmAfterSleep();
  setLcdControllerSleeping(false);
  digitalWrite(PIN_IR_VCC, HIGH);
  wakeTouchController(100);
  lv_obj_clear_flag(uiRoot, LV_OBJ_FLAG_HIDDEN);
  displaySleeping = false;
  displaySleepStartedMs = 0;
  nextDeepSleepAttemptMs = 0;
  lockActive = slideToUnlock;
  lastWakeMs = millis();
  bindPageUi(currentPage);
  refreshStatusPill();
  lv_obj_invalidate(pageUi[currentPage].tile);
  lv_refr_now(nullptr);
  lcdBacklight(true);
  applyBluetoothState();
}

void serviceIrBlink() {
  unsigned long now = millis();
  if (irOffAtMs != 0 && now >= irOffAtMs) {
    irLed(false);
    irOffAtMs = 0;
  }
  if (now >= nextIrBlinkMs && !displaySleeping) {
    irLed(true);
    irOffAtMs = now + 70;
    nextIrBlinkMs = now + 5000;
  }
}

// ---------------------------------------------------------------------------
// Setup and loop
// ---------------------------------------------------------------------------

void setupLvgl() {
  lv_init();
  // A decoded 64x64 RGBA PNG consumes roughly 12 KB in LVGL's 64 KB heap.
  // Multiple cache entries starve image decoding and produce white boxes.
  lv_img_cache_set_size(1);
  lv_fs_drv_init(&sdLvglFsDriver);
  sdLvglFsDriver.letter = 'S';
  sdLvglFsDriver.ready_cb = lvSdReady;
  sdLvglFsDriver.open_cb = lvSdOpen;
  sdLvglFsDriver.close_cb = lvSdClose;
  sdLvglFsDriver.read_cb = lvSdRead;
  sdLvglFsDriver.seek_cb = lvSdSeek;
  sdLvglFsDriver.tell_cb = lvSdTell;
  lv_fs_drv_register(&sdLvglFsDriver);

  lvBuf1 = lvFallbackBuf1;
  lvBuf2 = lvFallbackBuf2;
  lvDrawBufferPixels = LCD_W * 32;
  lvFullFrameDoubleBuffer = false;
  if (displayColourLutActive) ensureDisplayFlushBuffers(lvDrawBufferPixels);
  lv_disp_draw_buf_init(&drawBuf, lvBuf1, lvBuf2, lvDrawBufferPixels);

  lv_disp_drv_init(&dispDrv);
  dispDrv.hor_res = LCD_W;
  dispDrv.ver_res = LCD_H;
  dispDrv.flush_cb = lvFlush;
  dispDrv.monitor_cb = lvMonitor;
  dispDrv.draw_buf = &drawBuf;
  dispDrv.full_refresh = 0;
  lv_disp_drv_register(&dispDrv);
  Serial.printf("LVGL display: persistent tile strip, dual 32-row DMA buffers, %lu pixels each, PSRAM free=%lu\n",
                (unsigned long)lvDrawBufferPixels,
                (unsigned long)ESP.getFreePsram());

  lv_indev_drv_init(&touchDrv);
  touchDrv.type = LV_INDEV_TYPE_POINTER;
  touchDrv.read_cb = lvTouchRead;
  touchDrv.long_press_time = IR_REPEAT_DELAY_MS;
  touchDrv.long_press_repeat_time = IR_REPEAT_INTERVAL_MS;
  touchDrv.scroll_limit = LV_INDEV_DEF_SCROLL_LIMIT;
  touchDrv.scroll_throw = LV_INDEV_DEF_SCROLL_THROW;
  touchInputDevice = lv_indev_drv_register(&touchDrv);
}

void setupUiRoot() {
  uiRoot = lv_obj_create(lv_scr_act());
  lv_obj_remove_style_all(uiRoot);
  lv_obj_set_size(uiRoot, LCD_W, LCD_H);
  lv_obj_set_style_bg_color(uiRoot, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(uiRoot, LV_OPA_COVER, 0);

  pageStrip = lv_tileview_create(uiRoot);
  lv_obj_remove_style_all(pageStrip);
  lv_obj_set_size(pageStrip, LCD_W, LCD_H);
  lv_obj_set_pos(pageStrip, 0, 0);
  lv_obj_set_style_bg_color(pageStrip, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(pageStrip, LV_OPA_COVER, 0);
  lv_obj_set_scrollbar_mode(pageStrip, LV_SCROLLBAR_MODE_OFF);
  lv_obj_clear_flag(pageStrip, LV_OBJ_FLAG_SCROLL_ELASTIC);
  lv_obj_add_flag(pageStrip, LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_ONE);
  lv_obj_add_event_cb(pageStrip, pageStripEvent, LV_EVENT_VALUE_CHANGED, nullptr);

  for (uint8_t i = 0; i < PAGE_SLOT_COUNT; i++) {
    PageUi &slot = pageUi[i];
    slot.tile = lv_tileview_add_tile(pageStrip, i, 0, LV_DIR_HOR);
    lv_obj_remove_style_all(slot.tile);
    lv_obj_set_size(slot.tile, LCD_W, LCD_H);
    // lv_obj_remove_style_all() above wipes the x position the tileview
    // constructor just assigned (it's stored as a local style property), so
    // every tile ended up stacked at (0,0) instead of (i*LCD_W, 0). Restore it.
    lv_obj_set_pos(slot.tile, i * LCD_W, 0);
    lv_obj_clear_flag(slot.tile, LV_OBJ_FLAG_SCROLLABLE);

    slot.root = lv_obj_create(slot.tile);
    lv_obj_remove_style_all(slot.root);
    lv_obj_set_size(slot.root, LCD_W, LCD_H);
    lv_obj_set_pos(slot.root, 0, 0);
    lv_obj_set_style_bg_color(slot.root, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(slot.root, LV_OPA_COVER, 0);
    lv_obj_clear_flag(slot.root, LV_OBJ_FLAG_SCROLLABLE);

    slot.wallpaper = lv_img_create(slot.root);
    lv_img_set_src(slot.wallpaper, &cinemaWallpaper);
    lv_obj_set_pos(slot.wallpaper, 0, 0);
    lv_obj_set_style_img_opa(slot.wallpaper, LV_OPA_COVER, 0);
    lv_obj_set_style_img_recolor_opa(slot.wallpaper, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(slot.wallpaper, LV_OBJ_FLAG_CLICKABLE);

    slot.content = lv_obj_create(slot.root);
    lv_obj_remove_style_all(slot.content);
    lv_obj_set_pos(slot.content, 0, 42);
    lv_obj_set_size(slot.content, LCD_W, 250);
    lv_obj_set_style_bg_color(slot.content, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(slot.content, LV_OPA_COVER, 0);
    lv_obj_add_flag(slot.content, LV_OBJ_FLAG_SCROLL_CHAIN_HOR);

    slot.topBar = lv_obj_create(slot.root);
    lv_obj_remove_style_all(slot.topBar);
    lv_obj_set_pos(slot.topBar, 0, 0);
    lv_obj_set_size(slot.topBar, LCD_W, 42);
    lv_obj_set_style_bg_opa(slot.topBar, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(slot.topBar, LV_OBJ_FLAG_SCROLLABLE);

    slot.dots = lv_obj_create(slot.root);
    lv_obj_remove_style_all(slot.dots);
    lv_obj_set_size(slot.dots, 80, 12);
    lv_obj_clear_flag(slot.dots, LV_OBJ_FLAG_SCROLLABLE);
  }

  // Resolve all tile x positions before the first call to lv_obj_set_tile().
  // This is essential because startup normally selects Activities (tile 1),
  // not the tile at the strip's initial x=0 position.
  lv_obj_update_layout(pageStrip);

  bindPageUi(0);

  auto createDiagnosticLabel = [](const char *text, int x, int y, int width,
                                  lv_color_t colour) {
    lv_obj_t *label = lv_label_create(uiRoot);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(label, colour, 0);
    lv_obj_set_style_text_opa(label, LV_OPA_COVER, 0);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_width(label, width);
    lv_obj_clear_flag(label, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(label, LV_OBJ_FLAG_SCROLLABLE);
    return label;
  };

  cpuRamDiagnosticLabel = createDiagnosticLabel("CPU 0% RAM 0/0K", 3, 43, 184,
                                                lv_color_hex(0xFFD04A));
  fpsDiagnosticLabel = createDiagnosticLabel("FPS 0", 188, 43, 49,
                                             lv_color_hex(0xFF5BE1));
  lv_obj_set_style_text_align(fpsDiagnosticLabel, LV_TEXT_ALIGN_RIGHT, 0);
  accelerometerDiagnosticLabel = createDiagnosticLabel("ACC X0 Y0 Z0", 3, 55, 234,
                                                       lv_color_hex(0x42D9FF));
  splitDiagnosticLabel = createDiagnosticLabel("S 0", 3, LCD_H - 14, 92,
                                               lv_color_hex(0x36FF78));
  touchDiagnosticLabel = createDiagnosticLabel("T 0,0", 145, LCD_H - 14, 92,
                                               lv_color_hex(0xFF3C45));
  lv_obj_set_style_text_align(touchDiagnosticLabel, LV_TEXT_ALIGN_RIGHT, 0);

  for (TouchTrailPoint &point : touchTrail) {
    point.dot = lv_obj_create(uiRoot);
    lv_obj_remove_style_all(point.dot);
    lv_obj_set_size(point.dot, 6, 6);
    lv_obj_set_style_radius(point.dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(point.dot, lv_color_hex(0xFF9D2E), 0);
    lv_obj_set_style_bg_opa(point.dot, LV_OPA_COVER, 0);
    lv_obj_clear_flag(point.dot, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(point.dot, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(point.dot, LV_OBJ_FLAG_HIDDEN);
  }

  touchDot = lv_obj_create(uiRoot);
  lv_obj_remove_style_all(touchDot);
  lv_obj_set_size(touchDot, 44, 44);
  lv_obj_set_style_radius(touchDot, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_opa(touchDot, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_color(touchDot, lv_color_hex(0xFF2028), 0);
  lv_obj_set_style_border_width(touchDot, 2, 0);
  lv_obj_clear_flag(touchDot, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(touchDot, LV_OBJ_FLAG_SCROLLABLE);

  const int16_t tickPositions[4][4] = {
    {19, 0, 6, 10}, {19, 34, 6, 10}, {0, 19, 10, 6}, {34, 19, 10, 6}
  };
  for (uint8_t i = 0; i < 4; i++) {
    touchReticleTicks[i] = lv_obj_create(touchDot);
    lv_obj_remove_style_all(touchReticleTicks[i]);
    lv_obj_set_pos(touchReticleTicks[i], tickPositions[i][0], tickPositions[i][1]);
    lv_obj_set_size(touchReticleTicks[i], tickPositions[i][2], tickPositions[i][3]);
    lv_obj_set_style_bg_color(touchReticleTicks[i], lv_color_hex(0xFF2028), 0);
    lv_obj_set_style_bg_opa(touchReticleTicks[i], LV_OPA_COVER, 0);
    lv_obj_clear_flag(touchReticleTicks[i], LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(touchReticleTicks[i], LV_OBJ_FLAG_SCROLLABLE);
  }
  refreshDebugOverlayVisibility();
}

void setup() {
  esp_sleep_wakeup_cause_t bootWakeCause = esp_sleep_get_wakeup_cause();
  scheduledNtpWake = bootWakeCause == ESP_SLEEP_WAKEUP_TIMER;
  Serial.begin(USB_STUDIO_BAUD);
#if defined(ARDUINO_USB_CDC_ON_BOOT) && ARDUINO_USB_CDC_ON_BOOT
  Serial0.begin(USB_STUDIO_BAUD);
#endif
  delay(500);

  Serial.println();
  Serial.printf("OpenRemote %s - Rev 5 LVGL Runtime\n", OPENREMOTE_VERSION_TEXT);
  Serial.println(OPENREMOTE_FIRMWARE_MARKER);
  Serial.println("---------------------------------------");
  if (bootWakeCause == ESP_SLEEP_WAKEUP_EXT1) {
    Serial.printf("Deep wake status: 0x%llX\n", esp_sleep_get_ext1_wakeup_status());
  }
#if defined(ARDUINO_USB_CDC_ON_BOOT) && ARDUINO_USB_CDC_ON_BOOT
  Serial0.println();
  Serial0.printf("OpenRemote %s - Rev 5 LVGL Runtime (UART0)\n", OPENREMOTE_VERSION_TEXT);
  Serial0.println(OPENREMOTE_FIRMWARE_MARKER);
  Serial0.println("---------------------------------------");
#endif

  if (!allocateRuntimeStorage()) {
    while (true) delay(1000);
  }

  loadSettings();
  loadBatteryHistory();
  if (configureApplicationPowerMode(false)) {
    Serial.println("Power manager: DFS, tickless idle and BLE modem sleep ready");
  } else {
    Serial.println("Power manager: tickless, BLE modem sleep and safe 80 MHz idle ready");
  }
  lcdPowerOn();
  initBacklightPwm();
  IrSender.begin();
  IrReceiver.begin(PIN_IR_RX, DISABLE_LED_FEEDBACK);
  IrReceiver.stop();
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(400000);
  initialiseChargingState();

  // Match firmware 2.09's proven touch startup. The controller comes up with
  // the shared LCD rail and needs only an address probe; writing FT5x06 mode
  // registers here can preserve/replay stale contacts on this Rev 5 panel.
  touchFound = i2cDevicePresent(ADDR_TOUCH);
  initTca8418();
  initLIS3DH();
  sdReady = initSdStorage();
  loadIrdbMetadata();
  loadRuntimeConfig();
  // Refresh an existing bonded Android TV device immediately so newly added
  // HID commands are visible in WebConfig without first opening a BLE page.
  if (bleBonded) bleDeviceProvisionPending = true;
  restoreDeepSleepRuntimeState(bootWakeCause);
  Serial.printf("Touch 0x38: %s\n", touchFound ? "found" : "not found");
  Serial.printf("LIS3DH 0x19: %s\n", lis3dhReady ? "ready" : "not found");
  Serial.printf("SD storage: %s\n", sdReady ? "ready" : "unavailable");

  applyClockMode();
  applyBluetoothState();

  tft.init();
  tft.initDMA();
  tft.setSwapBytes(true);
  tft.fillScreen(TFT_BLACK);
  lcdControllerReady = true;
  ensureDisplayFlushBuffers();
  rebuildDisplayColourLut();
  applyDisplayControllerSettings();

  setupLvgl();
  setupUiRoot();
  rebuildPages();
  currentPage = activeDevice >= 0 ? pageCount - 1 :
                (activeActivity >= 0 ? 2 : 1);
  deviceReturnPage = activeActivity >= 0 ? 2 : 1;
  lastWakeMs = millis();
  lastTickMs = millis();
  nextStatusRefreshMs = millis() + STATUS_REFRESH_MS;
  nextBatteryHistoryCheckMs = millis() + 1000UL;
  renderAllPageSlots();
  // Allocate the Voice Search overlay before any BLE interaction. During a
  // physical hold, displaying it requires no heap allocation or object tree
  // construction in Chromecast's time-sensitive MIC_OPEN handshake window.
  createPhysicalVoiceOverlay();
  // The LCD controller may power up white. Keep its backlight off until LVGL
  // has restored and flushed the real first frame after cold or deep sleep.
  lv_refr_now(nullptr);
  if (!scheduledNtpWake) lcdBacklight(true);
  // A movement wake from deep sleep keeps the retained RTC time and avoids a
  // needless radio burst. Cold boots and the scheduled 3am timer refresh NTP.
  if (clockUseInternetTime &&
      (bootWakeCause != ESP_SLEEP_WAKEUP_EXT1 || time(nullptr) < 1700000000)) {
    requestInternetTimeSync();
  }
  if (scheduledNtpWake) {
    lastWakeMs = millis() - (uint32_t)deepSleepMinutes * 60UL * 1000UL;
    enterDisplaySleep();
  }
}

void loop() {
  unsigned long now = millis();
  uint32_t elapsed = now - lastTickMs;
  lastTickMs = now;
  lv_tick_inc(elapsed);

  // Service touch, scrolling and animation before storage, radio and command
  // work so input cadence remains smooth even when those subsystems are busy.
  if (!displaySleeping) {
    lv_timer_handler();
    servicePageStripChange();
  }

  if (dnsServerStarted) dnsServer.processNextRequest();
  now = millis();
  // UART traffic can wake automatic light sleep, but the application must
  // regain its PM locks before the USB parser writes a response or touches SD.
  if (bleConnectedIdleActive &&
      (Serial.available()
#if defined(ARDUINO_USB_CDC_ON_BOOT) && ARDUINO_USB_CDC_ON_BOOT
       || Serial0.available()
#endif
      )) {
    leaveBleConnectedIdle();
  }
  serviceUsbSerialImport(Serial, usbCdcSession);
#if defined(ARDUINO_USB_CDC_ON_BOOT) && ARDUINO_USB_CDC_ON_BOOT
  serviceUsbSerialImport(Serial0, uart0Session);
#endif
  serviceIrLearning(now);
  serviceKeypad(now);
  serviceHardwarePowerHold(now);
  serviceHeldIrRepeat(now);
  serviceButtonTest(now);
  serviceActivitySequence(now);
  serviceCommandFeedback(now);
  serviceButtonTestFeedback(now);
  serviceBluetooth(now);
  now = millis();
  serviceAtvvVoice(now);
  if (microphoneStopPending && !atvvAudioStarted) {
    microphoneStopPending = false;
    stopRealMicrophoneCapture();
    now = millis();
  }
  servicePhysicalVoiceOverlay();
  serviceAtvvDebug(now);
  if (!displaySleeping) {
    serviceDebugOverlay(now);
    updateSplitDiagnostic();
  }
  serviceInternetTime(now);
  serviceNetworkPower(now);
  serviceBatteryHistory(now);
  if (pendingRuntimeReload && (int32_t)(now - runtimeReloadAfterMs) >= 0) {
    pendingRuntimeReload = false;
    bool wasSleeping = displaySleeping;
    bool loaded = loadRuntimeConfig();
    if (!loaded && runtimeReloadCanRollback &&
        SD.exists(RUNTIME_CONFIG_BACKUP_PATH)) {
      Serial.println("Runtime sync: new config failed to load; restoring previous file");
      SD.remove(RUNTIME_CONFIG_PATH);
      if (SD.rename(RUNTIME_CONFIG_BACKUP_PATH, RUNTIME_CONFIG_PATH)) {
        loaded = loadRuntimeConfig();
      }
    }
    runtimeReloadCanRollback = false;
    if (loaded && wasSleeping) wakeDisplay();
    now = millis();
  }
  if (pendingNetworkApply) {
    pendingNetworkApply = false;
    if (wifiOn) startNetworkStack();
    else stopNetworkStack();
  }
  if (pendingBluetoothApply) {
    pendingBluetoothApply = false;
    applyBluetoothState();
  }
  if (runtimeSettingsSavePending &&
      (int32_t)(now - runtimeSettingsSaveAtMs) >= 0) {
    runtimeSettingsSavePending = !persistSettingsToRuntimeConfig();
    if (runtimeSettingsSavePending) runtimeSettingsSaveAtMs = millis() + 1000UL;
    now = millis();
  }

  serviceWebControlRequests(now);
  now = millis();
  serviceWifiScan(now);

  if (settingsView == SETTINGS_WIFI_QR && setupApStatusLabel &&
      (int32_t)(now - nextSetupApStatusRefreshMs) >= 0) {
    refreshSetupApStatusLabel();
    nextSetupApStatusRefreshMs = now + 1000UL;
  }

  bool stationUiNeedsReconnect =
    pages[currentPage].kind == PAGE_REMOTE_SETTINGS &&
    (settingsView == SETTINGS_WIFI || settingsView == SETTINGS_WIFI_QR) &&
    !setupApActive && !wifiScanPending && !wifiConnectPending && wifiOn &&
    selectedWifiSsid.length() && findWifiProfile(selectedWifiSsid) >= 0 &&
    WiFi.status() != WL_CONNECTED;
  if (stationUiNeedsReconnect) {
    networkShutdownAtMs = 0;
    if (!networkStackActive) startNetworkStack();
    else {
      String password = savedWifiPassword(selectedWifiSsid);
      WiFi.setSleep(false);
      WiFi.setAutoReconnect(true);
      WiFi.mode(WIFI_STA);
      WiFi.begin(selectedWifiSsid.c_str(), password.c_str());
    }
    wifiConnectPending = true;
    wifiConnectStartedMs = now;
    stationFallbackToSetupAp = settingsView == SETTINGS_WIFI_QR;
    pendingUiRefresh = true;
  }

  if (wifiConnectPending) {
    if (WiFi.status() == WL_CONNECTED || (uint32_t)(now - wifiConnectStartedMs) > 15000UL) {
      bool connected = WiFi.status() == WL_CONNECTED;
      wifiConnectPending = false;
      selectedWifiSsid = connected ? WiFi.SSID() : selectedWifiSsid;
      snprintf(webWifiStatusText, sizeof(webWifiStatusText),
               connected ? "Connected to %s" : "Could not connect to %s",
               selectedWifiSsid.c_str());
      if (webConfigQrPageActive()) {
        if (!connected && stationFallbackToSetupAp) {
          stationFallbackToSetupAp = false;
          startSetupAccessPoint();
        } else if (connected) {
          stationFallbackToSetupAp = false;
          requestWebServerListen(true);
        }
      } else if (connected && clockUseInternetTime) {
        requestInternetTimeSync();
      } else {
        scheduleNetworkShutdown();
      }
      pendingUiRefresh = settingsView == SETTINGS_WIFI ||
                         settingsView == SETTINGS_WIFI_QR;
    }
  }

  if (restartPending) {
    delay(600);
    ESP.restart();
  }
  if (hardRestartPending) {
    tft.waitDMA();
    delay(100);
    esp_rom_software_reset_system();
    while (true) delay(1000);
  }

  if (displaySleeping) {
    bool connectedActivityIdle = !webConfigQrPageActive() &&
      !webConfigTransferActive && !usbSdTransferActive() &&
      !ntpSyncPending && !wifiConnectPending &&
      bluetoothActivitySessionRequired();
    if (connectedActivityIdle) {
      enterBleConnectedIdle();
    }
    if (!connectedActivityIdle && !webConfigQrPageActive() &&
        !webConfigTransferActive && !usbSdTransferActive() &&
        !ntpSyncPending && !wifiConnectPending) {
      enterLowPowerWait();
      return;
    }
    if (!connectedActivityIdle ||
        (int32_t)(now - nextBleConnectedIdleMotionMs) >= 0) {
      nextBleConnectedIdleMotionMs = now + BLE_CONNECTED_IDLE_POLL_MS;
      float angle = bleIdleAccelerometerWake ? movementAngleDegrees() : 0.0f;
      if (raiseToWake && angle >= motionWakeAngleDegrees()) {
        if (bleIdleMotionAboveSinceMs == 0) bleIdleMotionAboveSinceMs = now;
        else if ((uint32_t)(now - bleIdleMotionAboveSinceMs) >=
                 BLE_CONNECTED_IDLE_MOTION_CONFIRM_MS) {
          Serial.printf("BLE idle motion wake: angle=%.1f threshold=%u deg\n",
                        angle, motionWakeAngleDegrees());
          wakeDisplay();
          return;
        }
      } else {
        bleIdleMotionAboveSinceMs = 0;
      }
    }
    if (webConfigQrPageActive() && nextDeepSleepAttemptMs &&
        (int32_t)(now - nextDeepSleepAttemptMs) >= 0) {
      nextDeepSleepAttemptMs = now + 5000UL;
      enterDeepPowerSleep(true);
    }
    delay(connectedActivityIdle ? BLE_CONNECTED_IDLE_POLL_MS : 30);
    return;
  }

  // The old IR heartbeat is disabled. Bound tiles call the real transmitter
  // directly, so GPIO5 is quiet unless the user sends a command.

  // LVGL may update lastWakeMs while handling a touch above. Refresh `now`
  // afterwards; otherwise subtracting the newer touch timestamp from the old
  // loop timestamp underflows and immediately sends the display to sleep.
  now = millis();
  bool pageUiSettled = !touchWasDown && !lvTouchDown &&
    !pageStripChangePending &&
    (!pageStrip || !lv_obj_is_scrolling(pageStrip));

  if (pendingDeviceOpen >= 0 && pageUiSettled) {
    int8_t deviceIndex = pendingDeviceOpen;
    pendingDeviceOpen = -1;
    openDevice((uint8_t)deviceIndex);
    now = millis();
  }

  if (pendingActivityOpen >= 0 && pageUiSettled) {
    int8_t activityIndex = pendingActivityOpen;
    pendingActivityOpen = -1;
    activateActivity((uint8_t)activityIndex);
    now = millis();
  }

  if (pendingUiRefresh && pageUiSettled) {
    pendingUiRefresh = false;
    bindPageUi(currentPage);
    renderCurrentPage();
    now = millis();
  }

  if (pageStripRebuildPending && pageUiSettled) {
    pageStripRebuildPending = false;
    renderAllPageSlots();
    now = millis();
  }

  if (brightnessOverlay && brightnessLastActivityMs != 0 &&
      (uint32_t)(now - brightnessLastActivityMs) >= BRIGHTNESS_PANEL_TIMEOUT_MS) {
    closeBrightnessPanel();
  }

  if (now >= nextStatusRefreshMs) {
    refreshStatusPill();
    nextStatusRefreshMs = now + STATUS_REFRESH_MS;
  }

  bool liveBatteryPage = pages[currentPage].kind == PAGE_REMOTE_SETTINGS &&
                         (settingsView == SETTINGS_ABOUT ||
                          settingsView == SETTINGS_BATTERY);
  if (liveBatteryPage && (int32_t)(now - nextBatteryPageRefreshMs) >= 0) {
    updateBatteryMetricLabels(currentBatteryMetrics());
    nextBatteryPageRefreshMs = now + 1000UL;
  }

  bool qrPageActive = pages[currentPage].kind == PAGE_REMOTE_SETTINGS &&
                      settingsView == SETTINGS_WIFI_QR;
  uint32_t activeSleepMs = (uint32_t)timeoutSeconds * 1000UL;
  if (qrPageActive) activeSleepMs += QR_PAGE_AWAKE_GRACE_MS;
  if (!activitySequenceActive && !usbSdTransferActive() &&
      (uint32_t)(now - lastWakeMs) > activeSleepMs) {
    enterDisplaySleep();
  }

  delay(5);
}

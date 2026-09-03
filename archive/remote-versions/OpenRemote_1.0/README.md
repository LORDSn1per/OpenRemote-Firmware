# OpenRemote 1.0 - Rev 5 LVGL Runtime

Open `OpenRemote_1.0.ino` from this folder in Arduino IDE. Keep `lv_conf.h`, `cinema_wallpaper_rgb565.h`, and the four `lv_font_openremote_*.c` files beside the INO file.

Required libraries:

- Arduino_GFX_Library
- LVGL 8.x

Suggested ESP32 board settings:

- Board: ESP32S3 Dev Module
- Flash size: 16 MB
- PSRAM: OPI PSRAM
- USB CDC On Boot: Enabled
- Upload speed: 115200 while bringing up new hardware

This build includes:

- Normal ILI9341 rotation with matching 180-degree touch-coordinate correction
- Working left/right page swipes
- Perceptual 0-100% active-low 12-bit PWM brightness control on GPIO9
- Non-black minimum brightness: 0% and 1% use the reliable 5% PWM floor
- Blue button backlights on GPIO46 follow the LCD awake/sleep state
- Synchronized 500 ms LCD and blue keypad LED fade before display sleep
- Unfiltered full-opacity RGB565 wallpaper rendering
- Tap-outside dismissal and four-second timeout for the brightness panel
- Custom glass slide-to-activate controls
- Rebalanced time and battery pill
- Raw touch-distance page swipes that work over cards and tiles
- True-white 1-bit Montserrat fonts with uniform solid pixels and no antialiasing haze
- Settings rows hand vertical swipes to the page instead of selecting row text
- Vertically scrollable device command pages with a fixed title bar
- Dynamic vertical-only device picker that grows down to the page-dot boundary
- Compact 12-hour clock with AM/PM and tighter battery spacing
- Light-green MAX17048 battery display with a debounced 2.2-second empty-to-full charging loop
- Shortened `Settings` page title
- 60-second sleep with accelerometer movement wake

Touch coordinates are printed to Serial at 115200 for calibration.

Verified compiling with ESP32 core 3.3.10, GFX Library for Arduino 1.6.6, and LVGL 8.3.11.

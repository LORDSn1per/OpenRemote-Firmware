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
    - Arduino_GFX_Library by moononournation
    - lvgl, preferably v8.x for this sketch
*/

#define LV_CONF_INCLUDE_SIMPLE

#include <Arduino.h>
#include <Wire.h>
#include <Arduino_GFX_Library.h>
#include <lvgl.h>
#include "cinema_wallpaper_rgb565.h"

// These thresholded 1-bpp Montserrat fonts use one solid colour for every
// illuminated glyph pixel. That avoids the pale antialiasing pixels visible
// around LVGL's standard 4-bpp fonts on the physical 240x320 LCD.
LV_FONT_DECLARE(lv_font_openremote_10);
LV_FONT_DECLARE(lv_font_openremote_12);
LV_FONT_DECLARE(lv_font_openremote_16);
LV_FONT_DECLARE(lv_font_openremote_20);

// ---------------------------------------------------------------------------
// Rev 5 pin map
// ---------------------------------------------------------------------------

static const int PIN_LCD_EN = 38;   // active-low
static const int PIN_LCD_BL = 9;    // active-low
static const int PIN_BUTTON_BL = 46; // SW_BL, active-high blue button LEDs
static const int PIN_IR_LED = 5;    // active-low
static const int PIN_CHARGE_STATUS = 1; // TP4056 CRG_STAT, active-low

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

static const int LCD_W = 240;
static const int LCD_H = 320;
static const uint32_t BACKLIGHT_PWM_HZ = 5000;
static const uint8_t BACKLIGHT_PWM_BITS = 12;
static const uint32_t BACKLIGHT_PWM_MAX = (1UL << BACKLIGHT_PWM_BITS) - 1UL;
// The Rev 5 backlight driver does not respond reliably to pulses below this
// point. Six counts is approximately the perceptual curve's 5% setting.
static const uint32_t BACKLIGHT_MIN_VISIBLE_DUTY = 6;
static const uint16_t BACKLIGHT_FADE_MS = 500;
static const uint8_t BACKLIGHT_FADE_STEPS = 25;
static const uint32_t BRIGHTNESS_PANEL_TIMEOUT_MS = 4000;
static const uint16_t STATUS_REFRESH_MS = 50;
static const uint16_t CHARGE_STATE_DEBOUNCE_MS = 300;
static const uint16_t CHARGE_ANIMATION_FILL_MS = 2200;
static const int16_t PAGE_SWIPE_THRESHOLD = 42;

Arduino_DataBus *bus = new Arduino_ESP32PAR8(
  PIN_LCD_DC, PIN_LCD_CS, PIN_LCD_WR, PIN_LCD_RD,
  PIN_LCD_D0, PIN_LCD_D1, PIN_LCD_D2, PIN_LCD_D3,
  PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7
);

Arduino_GFX *gfx = new Arduino_ILI9341(
  bus,
  PIN_LCD_RST,
  0,      // Normal orientation with the LCD installed in the remote
  false,
  LCD_W,
  LCD_H
);

// LVGL draw buffers. Two partial buffers are enough and avoid full-screen RAM use.
static lv_disp_draw_buf_t drawBuf;
static lv_color_t lvBuf1[LCD_W * 32];
static lv_color_t lvBuf2[LCD_W * 32];
static lv_disp_drv_t dispDrv;
static lv_indev_drv_t touchDrv;

// ---------------------------------------------------------------------------
// Fake Web Config data
// ---------------------------------------------------------------------------

struct DeviceCommand {
  const char *label;
};

struct Device {
  const char *name;
  const char *transport;
  const DeviceCommand *commands;
  uint8_t commandCount;
};

struct Activity {
  const char *name;
  uint16_t accent;
};

struct Tile {
  const char *label;
  uint8_t deviceIndex;
  uint8_t commandIndex;
  bool iconStyle;
};

struct ActivitySliderUi {
  lv_obj_t *card;
  lv_obj_t *thumb;
  uint8_t activityIndex;
};

const DeviceCommand hisenseCommands[] = {
  {"Power"}, {"Volume Up"}, {"Volume Down"}, {"Mute"}, {"Input"}, {"Guide"},
  {"Menu"}, {"Back"}, {"OK"}, {"HDMI 1"}, {"HDMI 2"}, {"Home"}
};

const DeviceCommand denonCommands[] = {
  {"Power"}, {"BD/DVD"}, {"TV Audio"}, {"Volume Up"}, {"Volume Down"}, {"Mute"},
  {"Movie"}, {"Music"}, {"Game"}, {"HDMI 1"}, {"HDMI 2"}, {"Surround"}
};

const DeviceCommand fetchCommands[] = {
  {"Power"}, {"Guide"}, {"Home"}, {"Menu"}, {"Back"}, {"OK"},
  {"Record"}, {"Play"}, {"Pause"}, {"Rewind"}, {"Forward"}, {"Info"}
};

const Device devices[] = {
  {"Hisense TV", "IR", hisenseCommands, sizeof(hisenseCommands) / sizeof(hisenseCommands[0])},
  {"Denon AVR", "IR", denonCommands, sizeof(denonCommands) / sizeof(denonCommands[0])},
  {"Fetch TV", "IR", fetchCommands, sizeof(fetchCommands) / sizeof(fetchCommands[0])}
};
static const uint8_t DEVICE_COUNT = sizeof(devices) / sizeof(devices[0]);

const Activity activities[] = {
  {"Fetch TV", 0xFD20},
  {"Chromecast", 0x07FF},
  {"Steamdeck", 0x07E0}
};
static const uint8_t ACTIVITY_COUNT = sizeof(activities) / sizeof(activities[0]);

const Tile fetchActivityTiles[] = {
  {"Guide", 2, 1, false}, {"Home", 2, 2, false}, {"Back", 2, 4, false},
  {"Vol +", 0, 1, true}, {"Mute", 0, 3, true}, {"Info", 2, 11, false}
};

const Tile chromecastActivityTiles[] = {
  {"Power", 0, 0, false}, {"Home", 2, 2, false}, {"Input", 0, 4, false},
  {"Vol +", 0, 1, true}, {"Vol -", 0, 2, true}, {"Back", 2, 4, false}
};

const Tile steamdeckActivityTiles[] = {
  {"Power", 1, 0, false}, {"Game", 1, 8, false}, {"HDMI 2", 1, 10, false},
  {"Vol +", 1, 3, true}, {"Vol -", 1, 4, true}, {"Mute", 1, 5, true}
};

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
int activeActivity = -1;
int activeDevice = -1;

bool wifiOn = true;
bool bluetoothOn = true;
bool ntpOn = true;
bool slideToUnlock = true;
bool raiseToWake = true;
uint8_t brightness = 100;  // User-facing percentage: 0 to 100.
uint8_t timeoutSeconds = 60;
const char *cityName = "Canberra";

bool touchFound = false;
bool lis3dhReady = false;
bool displaySleeping = false;
bool backlightPwmReady = false;
bool buttonBacklightPwmReady = false;
unsigned long lastWakeMs = 0;
unsigned long lastTickMs = 0;
unsigned long nextIrBlinkMs = 0;
unsigned long irOffAtMs = 0;
int16_t sleepBaseX = 0;
int16_t sleepBaseY = 0;
int16_t sleepBaseZ = 0;

lv_obj_t *screenRoot = nullptr;
lv_obj_t *wallpaper = nullptr;
lv_obj_t *topBar = nullptr;
lv_obj_t *content = nullptr;
lv_obj_t *dots = nullptr;
lv_obj_t *deviceModal = nullptr;
lv_obj_t *brightnessOverlay = nullptr;
lv_obj_t *brightnessPanel = nullptr;
lv_obj_t *touchDot = nullptr;
lv_obj_t *clockLabel = nullptr;
lv_obj_t *batteryFill = nullptr;
unsigned long nextStatusRefreshMs = 0;
unsigned long brightnessLastActivityMs = 0;
bool chargingState = false;
bool chargingCandidate = false;
unsigned long chargingCandidateSinceMs = 0;
unsigned long chargingAnimationStartMs = 0;
bool touchWasDown = false;
bool activityDragActive = false;
uint16_t touchStartX = 0;
uint16_t touchStartY = 0;
uint16_t touchLastX = 0;
uint16_t touchLastY = 0;
int8_t pendingPageDelta = 0;
ActivitySliderUi activitySliderUi[ACTIVITY_COUNT];

// ---------------------------------------------------------------------------
// Forward declarations used by LVGL callbacks
// ---------------------------------------------------------------------------

void renderCurrentPage();
void changePage(int delta);
void activateActivity(uint8_t index);
void openDevice(uint8_t index);
void showDevicePicker();
void toggleBrightnessPanel();
void closeBrightnessPanel();
void enterDisplaySleep();
void wakeDisplay();

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
  pinMode(PIN_CHARGE_STATUS, INPUT_PULLUP);
  digitalWrite(PIN_IR_LED, HIGH);
  buttonBacklight(true);
  digitalWrite(PIN_LCD_EN, LOW);
  delay(120);
  digitalWrite(PIN_LCD_BL, LOW);
}

uint32_t currentLcdOnDuty() {
  // Twelve-bit PWM provides sixteen times the low-end resolution of the old
  // eight-bit curve. Zero and one percent deliberately share the first
  // reliably visible duty level, so the slider can never blank the panel.
  float level = constrain(brightness, (uint8_t)0, (uint8_t)100) / 100.0f;
  if (brightness >= 100) return BACKLIGHT_PWM_MAX;
  if (brightness <= 1) return BACKLIGHT_MIN_VISIBLE_DUTY;
  return max(BACKLIGHT_MIN_VISIBLE_DUTY,
             (uint32_t)roundf(powf(level, 2.2f) * BACKLIGHT_PWM_MAX));
}

void applyBrightness() {
  if (backlightPwmReady) {
    // Rev 5 uses an active-low P-channel backlight driver.
    ledcWrite(PIN_LCD_BL, BACKLIGHT_PWM_MAX - currentLcdOnDuty());
  } else {
    digitalWrite(PIN_LCD_BL, brightness > 0 ? LOW : HIGH);
  }
}

void initBacklightPwm() {
  backlightPwmReady = ledcAttach(PIN_LCD_BL, BACKLIGHT_PWM_HZ, BACKLIGHT_PWM_BITS);
  buttonBacklightPwmReady = ledcAttach(PIN_BUTTON_BL, BACKLIGHT_PWM_HZ, BACKLIGHT_PWM_BITS);
  applyBrightness();
  buttonBacklight(true);
  Serial.printf("Backlight PWM: %s\n", backlightPwmReady ? "ready" : "failed");
  Serial.printf("Button LED PWM: %s\n", buttonBacklightPwmReady ? "ready" : "failed");
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

bool readTouch(uint16_t &x, uint16_t &y) {
  uint8_t data[5];
  if (!readBytes(ADDR_TOUCH, 0x02, data, 5)) return false;
  if ((data[0] & 0x0F) == 0) return false;

  uint16_t rawX = ((uint16_t)(data[1] & 0x0F) << 8) | data[2];
  uint16_t rawY = ((uint16_t)(data[3] & 0x0F) << 8) | data[4];

  if (rawX >= LCD_W) rawX = LCD_W - 1;
  if (rawY >= LCD_H) rawY = LCD_H - 1;

  // The LCD now uses its normal rotation (0). The touch controller's native
  // coordinates run in the opposite direction, so rotate both axes by 180
  // degrees to keep touches aligned with the displayed controls.
  x = (LCD_W - 1) - rawX;
  y = (LCD_H - 1) - rawY;
  return true;
}

void initLIS3DH() {
  uint8_t who = 0;
  lis3dhReady = readReg8(ADDR_LIS3DH, 0x0F, who) && who == 0x33;
  if (lis3dhReady) {
    writeReg8(ADDR_LIS3DH, 0x20, 0x57);
    writeReg8(ADDR_LIS3DH, 0x23, 0x88);
  }
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

float readBatteryPercent() {
  uint16_t soc = 0;
  if (!readReg16BE(ADDR_MAX17048, 0x04, soc)) return -1.0f;
  return (float)(soc >> 8) + ((float)(soc & 0xFF) / 256.0f);
}

// ---------------------------------------------------------------------------
// LVGL display/touch bridge
// ---------------------------------------------------------------------------

void lvFlush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *colour) {
  int32_t w = area->x2 - area->x1 + 1;
  int32_t h = area->y2 - area->y1 + 1;
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)colour, w, h);
  lv_disp_flush_ready(disp);
}

void lvTouchRead(lv_indev_drv_t *drv, lv_indev_data_t *data) {
  uint16_t x, y;
  if (touchFound && readTouch(x, y)) {
    static unsigned long lastTouchPrintMs = 0;
    if (millis() - lastTouchPrintMs > 300) {
      Serial.printf("LVGL touch x=%u y=%u\n", x, y);
      lastTouchPrintMs = millis();
    }
    data->state = LV_INDEV_STATE_PR;
    data->point.x = x;
    data->point.y = y;
    if (!touchWasDown) {
      touchWasDown = true;
      touchStartX = x;
      touchStartY = y;
    }
    touchLastX = x;
    touchLastY = y;
    if (brightnessOverlay) brightnessLastActivityMs = millis();
    if (touchDot) {
      lv_obj_clear_flag(touchDot, LV_OBJ_FLAG_HIDDEN);
      lv_obj_set_pos(touchDot, constrain((int)x - 5, 0, LCD_W - 10), constrain((int)y - 5, 0, LCD_H - 10));
      lv_obj_move_foreground(touchDot);
    }
    lastWakeMs = millis();
  } else {
    data->state = LV_INDEV_STATE_REL;
    if (touchWasDown) {
      int16_t dx = (int16_t)touchLastX - (int16_t)touchStartX;
      int16_t dy = (int16_t)touchLastY - (int16_t)touchStartY;

      // Queue page movement for loop(), outside LVGL's input callback. This
      // works even when the swipe begins over a card or tile.
      if (!activityDragActive && !brightnessOverlay && !deviceModal &&
          abs(dx) >= PAGE_SWIPE_THRESHOLD && abs(dx) > abs(dy)) {
        pendingPageDelta = (dx < 0) ? 1 : -1;
      }
      touchWasDown = false;
    }
    if (touchDot) {
      lv_obj_add_flag(touchDot, LV_OBJ_FLAG_HIDDEN);
    }
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
  lv_obj_set_style_text_font(label, &lv_font_openremote_12, 0);
  lv_obj_set_style_text_color(label, textPrimary(), 0);
  lv_obj_clear_flag(label, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(label, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_center(label);
  return btn;
}

void clearModalObjects() {
  if (deviceModal) {
    lv_obj_del(deviceModal);
    deviceModal = nullptr;
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

  if (activeDevice >= 0 && activeActivity >= 0) {
    pages[pageCount++] = {PAGE_DEVICE, devices[activeDevice].name};
  }

  if (currentPage >= pageCount) currentPage = pageCount - 1;
}

void drawDots() {
  if (!dots) return;
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
  if (enabled) {
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

bool updateChargingState() {
  unsigned long now = millis();
  bool rawCharging = digitalRead(PIN_CHARGE_STATUS) == LOW;

  if (rawCharging != chargingCandidate) {
    chargingCandidate = rawCharging;
    chargingCandidateSinceMs = now;
  }

  if (chargingCandidate != chargingState &&
      (uint32_t)(now - chargingCandidateSinceMs) >= CHARGE_STATE_DEBOUNCE_MS) {
    chargingState = chargingCandidate;
    if (chargingState) chargingAnimationStartMs = now;
  }
  return chargingState;
}

void refreshStatusPill() {
  if (clockLabel) {
    // A real NTP clock can replace this base time later. It advances normally
    // now so the runtime preview feels alive without requiring Wi-Fi.
    uint32_t minutes = (10UL * 60UL + 42UL + millis() / 60000UL) % (24UL * 60UL);
    uint32_t hour24 = minutes / 60UL;
    uint32_t hour12 = hour24 % 12UL;
    if (hour12 == 0) hour12 = 12;
    char timeText[12];
    snprintf(timeText, sizeof(timeText), "%lu:%02lu %s",
             (unsigned long)hour12, (unsigned long)(minutes % 60UL),
             hour24 >= 12 ? "PM" : "AM");
    lv_label_set_text(clockLabel, timeText);
  }

  if (batteryFill) {
    if (updateChargingState()) {
      // Conventional charging loop: fill smoothly from empty to full, reset
      // once, then immediately begin the next empty-to-full pass.
      uint32_t phase = (millis() - chargingAnimationStartMs) % CHARGE_ANIMATION_FILL_MS;
      uint8_t animatedWidth = 2 + (12UL * phase) / CHARGE_ANIMATION_FILL_MS;
      lv_obj_set_width(batteryFill, animatedWidth);
    } else {
      float percent = readBatteryPercent();
      if (percent < 0.0f) percent = 76.0f;
      percent = constrain(percent, 0.0f, 100.0f);
      lv_obj_set_width(batteryFill, max(2, (int)(13.0f * percent / 100.0f)));
    }
  }
}

void renderTopBar(const char *title, bool allowDevices) {
  lv_obj_clean(topBar);
  lv_obj_set_style_bg_opa(topBar, LV_OPA_TRANSP, 0);

  int titleX = 9;
  int titleWidth = allowDevices ? 91 : 124;
  if (allowDevices) {
    lv_obj_t *devBtn = lv_btn_create(topBar);
    lv_obj_set_pos(devBtn, 6, 5);
    lv_obj_set_size(devBtn, 32, 30);
    lv_obj_set_style_radius(devBtn, 15, 0);
    lv_obj_set_style_bg_color(devBtn, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(devBtn, LV_OPA_40, 0);
    lv_obj_set_style_border_color(devBtn, lv_color_white(), 0);
    lv_obj_set_style_border_opa(devBtn, LV_OPA_30, 0);
    lv_obj_set_style_border_width(devBtn, 1, 0);
    lv_obj_set_style_shadow_width(devBtn, 0, 0);
    lv_obj_set_style_color_filter_opa(devBtn, LV_OPA_TRANSP, LV_STATE_PRESSED);
    lv_obj_t *tv = lv_label_create(devBtn);
    lv_label_set_text(tv, "TV");
    lv_obj_set_style_text_font(tv, &lv_font_openremote_12, 0);
    lv_obj_set_style_text_color(tv, textPrimary(), 0);
    lv_obj_center(tv);
    lv_obj_add_event_cb(devBtn, [](lv_event_t *e) { showDevicePicker(); }, LV_EVENT_CLICKED, nullptr);
    titleX = 42;
  }

  lv_obj_t *titleLabel = lv_label_create(topBar);
  lv_label_set_text(titleLabel, title);
  lv_label_set_long_mode(titleLabel, LV_LABEL_LONG_DOT);
  lv_obj_set_pos(titleLabel, titleX, 9);
  lv_obj_set_width(titleLabel, titleWidth);
  lv_obj_set_style_text_align(titleLabel, allowDevices ? LV_TEXT_ALIGN_CENTER : LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_style_text_font(titleLabel, allowDevices ? &lv_font_openremote_16 : &lv_font_openremote_20, 0);
  lv_obj_set_style_text_color(titleLabel, textPrimary(), 0);

  if (!allowDevices) {
    lv_obj_t *hint = makeLabel(topBar, LV_SYMBOL_DOWN, 119, 13, &lv_font_openremote_10, textPrimary());
    lv_obj_set_style_text_opa(hint, LV_OPA_60, 0);
  }

  lv_obj_t *pill = lv_btn_create(topBar);
  lv_obj_set_pos(pill, 138, 5);
  lv_obj_set_size(pill, 96, 31);
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

  clockLabel = makeLabel(pill, "10:42 AM", 4, 8, &lv_font_openremote_12, textPrimary());
  lv_obj_set_width(clockLabel, 54);
  lv_obj_set_style_text_align(clockLabel, LV_TEXT_ALIGN_RIGHT, 0);

  lv_obj_t *battery = lv_obj_create(pill);
  lv_obj_remove_style_all(battery);
  lv_obj_set_pos(battery, 61, 9);
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
  lv_obj_remove_style_all(terminal);
  lv_obj_set_pos(terminal, 79, 12);
  lv_obj_set_size(terminal, 2, 6);
  lv_obj_set_style_bg_color(terminal, lv_color_white(), 0);
  lv_obj_set_style_bg_opa(terminal, LV_OPA_80, 0);

  refreshStatusPill();
  lv_obj_move_foreground(topBar);
}

// ---------------------------------------------------------------------------
// Pages
// ---------------------------------------------------------------------------

void switchEvent(lv_event_t *e) {
  bool *target = (bool *)lv_event_get_user_data(e);
  *target = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
}

void makeSettingRow(const char *name, const char *sub, int y, bool *switchTarget) {
  lv_obj_t *row = lv_obj_create(content);
  lv_obj_set_pos(row, 8, y);
  lv_obj_set_size(row, 224, 44);
  lv_obj_add_flag(row, LV_OBJ_FLAG_GESTURE_BUBBLE);
  lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(row, LV_OBJ_FLAG_CLICKABLE);
  stylePanel(row, lvRgb(34, 35, 39), lvRgb(54, 56, 62));
  makeLabel(row, name, 8, 2, &lv_font_openremote_16, textPrimary());
  makeLabel(row, sub, 8, 24, &lv_font_openremote_10, lvRgb(150, 150, 160));

  if (switchTarget) {
    lv_obj_t *sw = lv_switch_create(row);
    lv_obj_set_size(sw, 38, 22);
    lv_obj_align(sw, LV_ALIGN_RIGHT_MID, -4, 0);
    if (*switchTarget) lv_obj_add_state(sw, LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw, switchEvent, LV_EVENT_VALUE_CHANGED, switchTarget);
  }
}

void renderSettingsPage() {
  setCinematicBackground(false);
  configureContent(42, 250, false);
  lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(content, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_AUTO);
  lv_obj_set_style_pad_bottom(content, 20, 0);
  renderTopBar("Settings", false);
  makeSettingRow("Wi-Fi", wifiOn ? "Connected" : "Off", 8, &wifiOn);
  makeSettingRow("Bluetooth", bluetoothOn ? "On" : "Off", 58, &bluetoothOn);
  makeSettingRow("Time & Date", ntpOn ? cityName : "Manual", 108, &ntpOn);
  makeSettingRow("Wi-Fi Config", "Setup AP and QR later", 158, nullptr);
  makeSettingRow("Display", "Brightness, sleep, wake", 208, nullptr);
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

    int x = point.x - cardArea.x1 - 22;
    lv_obj_set_x(ui->thumb, constrain(x, 4, 176));
  }

  if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
    activityDragActive = false;
    if (lv_obj_get_x(ui->thumb) >= 164) {
      activateActivity(ui->activityIndex);
    } else {
      resetActivityThumb(ui);
    }
  }
}

void renderActivitiesPage() {
  setCinematicBackground(true);
  configureContent(0, LCD_H, true);
  renderTopBar("Activities", false);

  for (uint8_t i = 0; i < ACTIVITY_COUNT; i++) {
    lv_obj_t *card = lv_obj_create(content);
    lv_obj_set_pos(card, 8, 104 + i * 58);
    lv_obj_set_size(card, 224, 50);
    stylePanel(card, lvRgb(92, 46, 28), lv_color_white(), (lv_opa_t)58);
    lv_obj_set_style_border_opa(card, LV_OPA_10, 0);
    lv_obj_set_style_radius(card, 9, 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *thumb = lv_obj_create(card);
    lv_obj_remove_style_all(thumb);
    lv_obj_set_pos(thumb, 4, 3);
    lv_obj_set_size(thumb, 44, 44);
    lv_obj_set_style_radius(thumb, 8, 0);
    lv_obj_set_style_bg_color(thumb, lv_color_hex(activities[i].accent), 0);
    lv_obj_set_style_bg_opa(thumb, LV_OPA_90, 0);
    lv_obj_set_style_border_color(thumb, lv_color_white(), 0);
    lv_obj_set_style_border_opa(thumb, LV_OPA_30, 0);
    lv_obj_set_style_border_width(thumb, 1, 0);
    lv_obj_add_flag(thumb, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(thumb, LV_OBJ_FLAG_PRESS_LOCK);
    lv_obj_clear_flag(thumb, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *icon = lv_label_create(thumb);
    lv_label_set_text(icon, LV_SYMBOL_PLAY);
    lv_obj_set_style_text_font(icon, &lv_font_openremote_16, 0);
    lv_obj_set_style_text_color(icon, textPrimary(), 0);
    lv_obj_center(icon);
    lv_obj_clear_flag(icon, LV_OBJ_FLAG_CLICKABLE);

    activitySliderUi[i] = {card, thumb, i};
    lv_obj_add_event_cb(thumb, activitySliderEvent, LV_EVENT_ALL, &activitySliderUi[i]);

    lv_obj_t *name = makeLabel(card, activities[i].name, 58, 7, &lv_font_openremote_16, textPrimary());
    lv_obj_clear_flag(name, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t *instruction = makeLabel(card, "Slide to activate", 58, 28, &lv_font_openremote_10, textPrimary());
    lv_obj_set_style_text_opa(instruction, (lv_opa_t)166, 0);
    lv_obj_clear_flag(instruction, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *chevrons = makeLabel(card, LV_SYMBOL_RIGHT " " LV_SYMBOL_RIGHT,
                                   193, 17, &lv_font_openremote_12, textPrimary());
    lv_obj_set_style_text_opa(chevrons, LV_OPA_40, 0);
    lv_obj_clear_flag(chevrons, LV_OBJ_FLAG_CLICKABLE);
  }
}

const Tile *currentActivityTiles(uint8_t &count) {
  if (activeActivity == 1) {
    count = sizeof(chromecastActivityTiles) / sizeof(chromecastActivityTiles[0]);
    return chromecastActivityTiles;
  }
  if (activeActivity == 2) {
    count = sizeof(steamdeckActivityTiles) / sizeof(steamdeckActivityTiles[0]);
    return steamdeckActivityTiles;
  }
  count = sizeof(fetchActivityTiles) / sizeof(fetchActivityTiles[0]);
  return fetchActivityTiles;
}

void tileEvent(lv_event_t *e) {
  const char *label = (const char *)lv_event_get_user_data(e);
  Serial.printf("Tile pressed: %s\n", label);
  irLed(true);
  delay(25);
  irLed(false);
  lastWakeMs = millis();
}

void makeTile(uint8_t index, const char *label, bool iconStyle) {
  uint8_t col = index % 3;
  uint8_t row = index / 3;
  int x = 8 + col * 76;
  int y = 96 + row * 61;

  lv_obj_t *tile = makeButton(content, "", x, y, 68, 52, lvRgb(76, 48, 38));
  lv_obj_set_style_bg_opa(tile, LV_OPA_30, 0);
  lv_obj_set_style_border_color(tile, lv_color_white(), 0);
  lv_obj_set_style_border_opa(tile, LV_OPA_20, 0);
  lv_obj_add_event_cb(tile, tileEvent, LV_EVENT_CLICKED, (void *)label);

  if (iconStyle) {
    lv_obj_t *circle = lv_obj_create(tile);
    lv_obj_set_size(circle, 22, 22);
    lv_obj_align(circle, LV_ALIGN_TOP_MID, 0, -2);
    lv_obj_clear_flag(circle, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(circle, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(circle, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(circle, lvRgb(54, 180, 220), 0);
    lv_obj_set_style_border_width(circle, 0, 0);
    makeLabel(tile, label, 0, 25, &lv_font_openremote_10, textPrimary());
  } else {
    lv_obj_t *l = lv_label_create(tile);
    lv_label_set_text(l, label);
    lv_label_set_long_mode(l, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(l, 56);
    lv_obj_set_style_text_font(l, &lv_font_openremote_12, 0);
    lv_obj_set_style_text_color(l, textPrimary(), 0);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_clear_flag(l, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(l, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(l);
  }
}

void renderActivityPage() {
  setCinematicBackground(true);
  configureContent(0, LCD_H, true);
  const char *name = (activeActivity >= 0) ? activities[activeActivity].name : "Activity";
  renderTopBar(name, true);

  uint8_t count = 0;
  const Tile *tiles = currentActivityTiles(count);
  for (uint8_t i = 0; i < count && i < 12; i++) {
    makeTile(i, tiles[i].label, tiles[i].iconStyle);
  }
}

void renderDevicePage() {
  setCinematicBackground(true);
  configureContent(0, LCD_H, true);
  lv_obj_set_height(content, LCD_H - 20);
  lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(content, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_AUTO);
  lv_obj_set_style_pad_bottom(content, 12, 0);
  if (activeDevice < 0) activeDevice = 0;
  renderTopBar(devices[activeDevice].name, true);

  uint8_t count = min((uint8_t)12, devices[activeDevice].commandCount);
  for (uint8_t i = 0; i < count; i++) {
    makeTile(i, devices[activeDevice].commands[i].label, false);
  }
}

void renderCurrentPage() {
  clearModalObjects();
  lv_obj_clean(topBar);
  lv_obj_clean(content);

  switch (pages[currentPage].kind) {
    case PAGE_REMOTE_SETTINGS: renderSettingsPage(); break;
    case PAGE_ACTIVITIES: renderActivitiesPage(); break;
    case PAGE_ACTIVITY: renderActivityPage(); break;
    case PAGE_DEVICE: renderDevicePage(); break;
  }

  drawDots();
}

// ---------------------------------------------------------------------------
// Modal controls
// ---------------------------------------------------------------------------

void deviceChoiceEvent(lv_event_t *e) {
  int index = (int)(intptr_t)lv_event_get_user_data(e);
  openDevice(index);
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
  const int desiredHeight = 44 + DEVICE_COUNT * rowHeight;
  const int modalHeight = min(pageDotsTop - modalY, desiredHeight);

  deviceModal = lv_obj_create(screenRoot);
  lv_obj_set_pos(deviceModal, 8, modalY);
  lv_obj_set_size(deviceModal, 224, modalHeight);
  stylePanel(deviceModal, lvRgb(18, 22, 30), lvRgb(60, 180, 220), LV_OPA_COVER);
  lv_obj_clear_flag(deviceModal, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(deviceModal, LV_DIR_NONE);
  makeLabel(deviceModal, "Devices", 8, 4, &lv_font_openremote_16, textPrimary());

  lv_obj_t *list = lv_obj_create(deviceModal);
  lv_obj_remove_style_all(list);
  lv_obj_set_pos(list, 8, 30);
  lv_obj_set_size(list, 204, modalHeight - 42);
  lv_obj_set_scroll_dir(list, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_AUTO);
  lv_obj_set_style_pad_all(list, 0, 0);
  if (DEVICE_COUNT * rowHeight > modalHeight - 42) {
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

  lv_obj_t *slider = lv_slider_create(brightnessPanel);
  lv_obj_set_size(slider, 14, 204);
  lv_obj_align(slider, LV_ALIGN_CENTER, 0, 0);
  lv_slider_set_range(slider, 0, 100);
  lv_slider_set_value(slider, brightness, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(slider, lvRgb(75, 78, 86), LV_PART_MAIN);
  lv_obj_set_style_bg_color(slider, lv_color_white(), LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(slider, lv_color_white(), LV_PART_KNOB);
  lv_obj_set_style_pad_all(slider, 5, LV_PART_KNOB);
  lv_obj_add_event_cb(slider, brightnessEvent, LV_EVENT_VALUE_CHANGED, nullptr);

  brightnessLastActivityMs = millis();
  lv_obj_move_foreground(brightnessOverlay);
}

// ---------------------------------------------------------------------------
// Actions and gestures
// ---------------------------------------------------------------------------

void activateActivity(uint8_t index) {
  Serial.printf("Activate activity: %s\n", activities[index].name);
  activeActivity = index;
  activeDevice = -1;
  rebuildPages();
  currentPage = 2;
  renderCurrentPage();
}

void openDevice(uint8_t index) {
  Serial.printf("Open device page: %s\n", devices[index].name);
  activeDevice = index;
  rebuildPages();
  currentPage = pageCount - 1;
  renderCurrentPage();
}

void changePage(int delta) {
  int next = (int)currentPage + delta;
  if (next < 0) next = 0;
  if (next >= pageCount) next = pageCount - 1;
  if (next == currentPage) return;

  currentPage = next;

  if (activeDevice >= 0 && pages[currentPage].kind == PAGE_ACTIVITY) {
    activeDevice = -1;
    rebuildPages();
  }

  renderCurrentPage();
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

void enterDisplaySleep() {
  if (displaySleeping) return;
  Serial.println("Display sleep");
  captureSleepBaseline();
  fadeBacklightsToOff();
  lv_obj_add_flag(screenRoot, LV_OBJ_FLAG_HIDDEN);
  lv_refr_now(nullptr);
  gfx->fillScreen(0x0000);
  displaySleeping = true;
}

void wakeDisplay() {
  Serial.println("Movement wake");
  lcdBacklight(true);
  lv_obj_clear_flag(screenRoot, LV_OBJ_FLAG_HIDDEN);
  displaySleeping = false;
  lastWakeMs = millis();
  renderCurrentPage();
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
  lv_disp_draw_buf_init(&drawBuf, lvBuf1, lvBuf2, LCD_W * 32);

  lv_disp_drv_init(&dispDrv);
  dispDrv.hor_res = LCD_W;
  dispDrv.ver_res = LCD_H;
  dispDrv.flush_cb = lvFlush;
  dispDrv.draw_buf = &drawBuf;
  lv_disp_drv_register(&dispDrv);

  lv_indev_drv_init(&touchDrv);
  touchDrv.type = LV_INDEV_TYPE_POINTER;
  touchDrv.read_cb = lvTouchRead;
  lv_indev_drv_register(&touchDrv);
}

void setupUiRoot() {
  screenRoot = lv_obj_create(lv_scr_act());
  lv_obj_remove_style_all(screenRoot);
  lv_obj_set_size(screenRoot, LCD_W, LCD_H);
  lv_obj_set_style_bg_color(screenRoot, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(screenRoot, LV_OPA_COVER, 0);

  wallpaper = lv_img_create(screenRoot);
  lv_img_set_src(wallpaper, &cinemaWallpaper);
  lv_obj_set_pos(wallpaper, 0, 0);
  // Draw the stored RGB565 wallpaper exactly as supplied: no tint, recolor,
  // dark overlay, or transparency is applied at runtime.
  lv_obj_set_style_img_opa(wallpaper, LV_OPA_COVER, 0);
  lv_obj_set_style_img_recolor_opa(wallpaper, LV_OPA_TRANSP, 0);
  lv_obj_clear_flag(wallpaper, LV_OBJ_FLAG_CLICKABLE);

  content = lv_obj_create(screenRoot);
  lv_obj_remove_style_all(content);
  lv_obj_set_pos(content, 0, 42);
  lv_obj_set_size(content, LCD_W, 250);
  lv_obj_set_style_bg_color(content, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(content, LV_OPA_COVER, 0);

  topBar = lv_obj_create(screenRoot);
  lv_obj_remove_style_all(topBar);
  lv_obj_set_pos(topBar, 0, 0);
  lv_obj_set_size(topBar, LCD_W, 42);
  lv_obj_set_style_bg_opa(topBar, LV_OPA_TRANSP, 0);

  dots = lv_obj_create(screenRoot);
  lv_obj_remove_style_all(dots);
  lv_obj_set_size(dots, 80, 12);

  // Keep touch diagnostics on Serial. The old orange tracking dot caused
  // unnecessary redraws and is deliberately omitted from the finished UI.
  touchDot = nullptr;
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("OpenRemote 1.0 - Rev 5 LVGL Runtime");
  Serial.println("---------------------------------------");

  lcdPowerOn();
  initBacklightPwm();
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(400000);

  touchFound = i2cDevicePresent(ADDR_TOUCH);
  initLIS3DH();
  Serial.printf("Touch 0x38: %s\n", touchFound ? "found" : "not found");
  Serial.printf("LIS3DH 0x19: %s\n", lis3dhReady ? "ready" : "not found");

  if (!gfx->begin()) {
    Serial.println("LCD init failed");
  }

  setupLvgl();
  setupUiRoot();
  rebuildPages();
  currentPage = 1;
  lastWakeMs = millis();
  lastTickMs = millis();
  nextStatusRefreshMs = millis() + STATUS_REFRESH_MS;
  renderCurrentPage();
}

void loop() {
  unsigned long now = millis();
  uint32_t elapsed = now - lastTickMs;
  lastTickMs = now;
  lv_tick_inc(elapsed);

  if (displaySleeping) {
    if (movementDelta() > 180) {
      wakeDisplay();
    }
    delay(30);
    return;
  }

  lv_timer_handler();
  // IR heartbeat disabled for UI testing. Tiles still blink IR briefly when
  // pressed so command feedback can be tested without constant flashing.

  // LVGL may update lastWakeMs while handling a touch above. Refresh `now`
  // afterwards; otherwise subtracting the newer touch timestamp from the old
  // loop timestamp underflows and immediately sends the display to sleep.
  now = millis();

  if (pendingPageDelta != 0) {
    int8_t delta = pendingPageDelta;
    pendingPageDelta = 0;
    changePage(delta);
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

  if ((uint32_t)(now - lastWakeMs) > 60000UL) {
    enterDisplaySleep();
  }

  delay(5);
}

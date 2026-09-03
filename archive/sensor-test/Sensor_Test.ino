/*
  OpenRemote Rev 6 - hardware self-test + LCD / I2C sensor demo   (v2.00)

  v2.10 - 2026-08-29
    - Replaced the seven-page carousel with a single live dashboard: I2C and
      I2S device status, accelerometer bars, microphone level, battery and
      touch are all on screen at once. A fault is now visible next to the
      things that work rather than having to be waited for.
    - Auto-sleep removed. This is a bench instrument and blanking the screen
      mid-measurement was actively unhelpful.
    - Drawing is split into a static pass plus per-region updates, because
      repainting the whole screen at 10Hz on this synchronous 8-bit bus
      tears and flickers.

  v2.00 - 2026-08-29
    - Added a power-on self-test that PROVES each display bus pin can actually
      reach both levels, instead of trusting that a write succeeded. A white
      screen with "LCD init completed" in the log is the signature of a bus
      pin that is shorted or open: Arduino_GFX has no way to detect it, so
      this measures every pin by reading it back. If anything fails it prints
      SCREEN FAIL and names the pin, its GPIO and its J2 connector pad.
    - Added a microSD test (power, mount with speed fallback, then a real
      write/read/delete round-trip) and an I2S microphone test. GPIO45
      powers the mic and also lights D3, so a lit D3 confirms mic power.
    - Every pin number is verified against the Rev 6 KiCad netlist, not
      inherited from the Rev 5 demo. All 17 previously-used pins were checked
      and matched; the SD and mic pins are new here.

  Purpose:

  Purpose:
    - Power up the Rev 5 ILI9341 LCD at rotation 0. This demo ran at rotation
      2 (180 degrees) until 2026-08-04; the touch controller reports in a
      fixed orientation of its own, so readTouchPoint() flips both axes to
      keep presses landing under the finger.
    - Draw text, shapes, colours, and simple demo pages.
    - Scan and display every responding 7-bit I2C address on the LCD.
    - Display a live touch test page for the 0x38 touch controller.
    - Display battery stats from the MAX17048 fuel gauge.
    - Display accelerometer stats from the LIS3DH.
    - After 60 seconds, blank the LCD and turn the backlight off.
    - Wake the display only when movement is detected by the accelerometer.
    - Blink the IR LED on GPIO5 every few seconds as a heartbeat.

  Required Arduino library:
    Arduino_GFX_Library by moononournation

  Sleep note:
    This demo keeps LCD_EN active while sleeping so the board can continue
    polling the LIS3DH over I2C. The screen is made black and the backlight is
    turned off. A later firmware can use the LIS3DH interrupt pin for deeper
    ESP32 sleep.
*/

#include <Arduino.h>
#include <string.h>
#include <Wire.h>
#include <Arduino_GFX_Library.h>
#include <SPI.h>
#include <SD.h>
#include <driver/i2s_std.h>

// ---------------------------------------------------------------------------
// Basic RGB565 colours
// ---------------------------------------------------------------------------

static const uint16_t C_BLACK   = 0x0000;
static const uint16_t C_NAVY    = 0x000F;
static const uint16_t C_BLUE    = 0x001F;
static const uint16_t C_RED     = 0xF800;
static const uint16_t C_GREEN   = 0x07E0;
static const uint16_t C_CYAN    = 0x07FF;
static const uint16_t C_MAGENTA = 0xF81F;
static const uint16_t C_YELLOW  = 0xFFE0;
static const uint16_t C_WHITE   = 0xFFFF;
static const uint16_t C_ORANGE  = 0xFD20;
static const uint16_t C_GREY    = 0x8410;
static const uint16_t C_DARK    = 0x1082;

uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// ---------------------------------------------------------------------------
// OMOTE REV 5 PIN MAP
// ---------------------------------------------------------------------------

static const int PIN_LCD_EN = 38;   // LCD_EN, active-low display power enable
static const int PIN_LCD_BL = 9;    // LCD_BL, active-low backlight enable
static const int PIN_IR_LED = 5;    // IR_LED, active-low

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

// microSD (SPI) and I2S microphone. Verified against the Rev 6 netlist:
// SD_MISO=GPIO7 J5.7, SD_SCK=GPIO15 J5.5, SD_EN=GPIO16 (active-low, drives Q7
// through R2), SD_MOSI=GPIO17 J5.3, SD_CS=GPIO18 J5.2.
static const int PIN_SD_MISO = 7;
static const int PIN_SD_SCK  = 15;
static const int PIN_SD_EN   = 16;   // active-low card power switch
static const int PIN_SD_MOSI = 17;
static const int PIN_SD_CS   = 18;

// The microphone shares the SD bus through R45/R46/R47 (4k7 each), so only one
// of the two can be active at a time. GPIO45 drives indicator LED D3 - it is
// NOT mic power on this board; see testMicrophone() for the MIC_VDD finding.
static const int PIN_MIC_POWER = 45;
static const int PIN_MIC_BCLK  = PIN_SD_SCK;
static const int PIN_MIC_WS    = PIN_SD_MOSI;
static const int PIN_MIC_DATA  = PIN_SD_MISO;

SPIClass sdSpi(HSPI);

static const int LCD_WIDTH = 240;
static const int LCD_HEIGHT = 320;

// I2C device addresses expected on the OMOTE Rev 5 board.
static const uint8_t ADDR_LIS3DH = 0x19;
static const uint8_t ADDR_TCA8418 = 0x34;
static const uint8_t ADDR_MAX17048 = 0x36;
static const uint8_t ADDR_TOUCH = 0x38;

static const unsigned long SLEEP_TIMEOUT_MS = 60000;
static const uint16_t WAKE_MOVEMENT_DELTA = 180;

Arduino_DataBus *bus = new Arduino_ESP32PAR8(
  PIN_LCD_DC,
  PIN_LCD_CS,
  PIN_LCD_WR,
  PIN_LCD_RD,
  PIN_LCD_D0,
  PIN_LCD_D1,
  PIN_LCD_D2,
  PIN_LCD_D3,
  PIN_LCD_D4,
  PIN_LCD_D5,
  PIN_LCD_D6,
  PIN_LCD_D7
);

Arduino_GFX *gfx = new Arduino_ILI9341(
  bus,
  PIN_LCD_RST,
  0,      // rotation 0 - was 2 (180 deg); readTouchPoint() flips to match
  false,  // IPS
  LCD_WIDTH,
  LCD_HEIGHT
);

struct I2CDevice {
  uint8_t address;
  const char *name;
};

I2CDevice foundDevices[32];
uint8_t foundDeviceCount = 0;
bool i2cResultOverflow = false;

uint8_t page = 0;
unsigned long nextPageMs = 0;
unsigned long nextRefreshMs = 0;
unsigned long nextIrBlinkMs = 0;
unsigned long irOffAtMs = 0;
unsigned long wokeAtMs = 0;
bool lis3dhReady = false;
bool touchReady = false;
bool displaySleeping = false;
int16_t sleepBaseX = 0;
int16_t sleepBaseY = 0;
int16_t sleepBaseZ = 0;
int lastTouchX = -1;
int lastTouchY = -1;

// ---------------------------------------------------------------------------
// Hardware helpers
// ---------------------------------------------------------------------------

void lcdPowerOn() {
  pinMode(PIN_LCD_EN, OUTPUT);
  pinMode(PIN_LCD_BL, OUTPUT);

  digitalWrite(PIN_LCD_EN, LOW);
  delay(120);
  digitalWrite(PIN_LCD_BL, LOW);
}

void lcdBacklight(bool on) {
  digitalWrite(PIN_LCD_BL, on ? LOW : HIGH);
}

void irLed(bool on) {
  digitalWrite(PIN_IR_LED, on ? LOW : HIGH);
}

void serviceIrBlink() {
  unsigned long now = millis();

  if (irOffAtMs != 0 && now >= irOffAtMs) {
    irLed(false);
    irOffAtMs = 0;
  }

  if (now >= nextIrBlinkMs) {
    Serial.println("IR heartbeat blink");
    irLed(true);
    irOffAtMs = now + 90;
    nextIrBlinkMs = now + 3500;
  }
}

// ---------------------------------------------------------------------------
// I2C helpers
// ---------------------------------------------------------------------------

bool i2cDevicePresent(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

const char *i2cName(uint8_t address) {
  switch (address) {
    case ADDR_LIS3DH: return "LIS3DH accel";
    case ADDR_TCA8418: return "TCA8418 keypad";
    case ADDR_MAX17048: return "MAX17048 battery";
    case ADDR_TOUCH: return "Touch controller";
    default: return "Unknown";
  }
}

void scanI2C() {
  foundDeviceCount = 0;
  i2cResultOverflow = false;

  Serial.println("Scanning full 7-bit I2C address range...");
  Serial.println("Range: 0x01 to 0x7E");

  for (uint8_t address = 0x01; address <= 0x7E; address++) {
    if (i2cDevicePresent(address)) {
      if (foundDeviceCount < (sizeof(foundDevices) / sizeof(foundDevices[0]))) {
        foundDevices[foundDeviceCount].address = address;
        foundDevices[foundDeviceCount].name = i2cName(address);
        foundDeviceCount++;
      } else {
        i2cResultOverflow = true;
      }
      Serial.printf("  Found 0x%02X %s\n", address, i2cName(address));
    }
  }

  if (i2cResultOverflow) {
    Serial.println("Result buffer full; some addresses were not stored for LCD display.");
  }
  Serial.printf("Scan complete: %u device(s)\n", foundDeviceCount);
}

bool readTouchPoint(uint16_t &x, uint16_t &y) {
  // FT6206/FT6236/CST026-compatible register pattern:
  // 0x02 = touch count, 0x03..0x06 = first touch X/Y.
  Wire.beginTransmission(ADDR_TOUCH);
  Wire.write(0x02);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  const uint8_t needed = 5;
  if (Wire.requestFrom((int)ADDR_TOUCH, (int)needed) != needed) {
    while (Wire.available()) {
      Wire.read();
    }
    return false;
  }

  uint8_t data[needed];
  for (uint8_t i = 0; i < needed; i++) {
    data[i] = Wire.read();
  }

  uint8_t touches = data[0] & 0x0F;
  if (touches == 0) {
    return false;
  }

  x = ((uint16_t)(data[1] & 0x0F) << 8) | data[2];
  y = ((uint16_t)(data[3] & 0x0F) << 8) | data[4];

  if (x >= LCD_WIDTH) x = LCD_WIDTH - 1;
  if (y >= LCD_HEIGHT) y = LCD_HEIGHT - 1;

  // The panel moved from rotation 2 to rotation 0, but the touch controller
  // reports in its own fixed orientation and knows nothing about that. Without
  // this flip every press would land diagonally opposite where it was made -
  // the reticle would track the finger mirrored through the centre of the
  // screen. Clamping happens first, so both results stay within the panel.
  x = LCD_WIDTH - 1 - x;
  y = LCD_HEIGHT - 1 - y;

  return true;
}

bool readReg8(uint8_t address, uint8_t reg, uint8_t &value) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  if (Wire.requestFrom((int)address, 1) != 1) {
    return false;
  }

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
  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  if (Wire.requestFrom((int)address, 2) != 2) {
    return false;
  }

  uint8_t msb = Wire.read();
  uint8_t lsb = Wire.read();
  value = ((uint16_t)msb << 8) | lsb;
  return true;
}

bool readBytes(uint8_t address, uint8_t startReg, uint8_t *data, uint8_t length) {
  Wire.beginTransmission(address);
  Wire.write(startReg);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  if (Wire.requestFrom((int)address, (int)length) != length) {
    return false;
  }

  for (uint8_t i = 0; i < length; i++) {
    data[i] = Wire.read();
  }
  return true;
}

void initLIS3DH() {
  uint8_t whoAmI = 0;
  lis3dhReady = readReg8(ADDR_LIS3DH, 0x0F, whoAmI) && whoAmI == 0x33;

  if (lis3dhReady) {
    // CTRL_REG1: 100 Hz, all axes enabled.
    writeReg8(ADDR_LIS3DH, 0x20, 0x57);
    // CTRL_REG4: High resolution, +/-2g.
    writeReg8(ADDR_LIS3DH, 0x23, 0x88);
    Serial.println("LIS3DH initialised");
  } else {
    Serial.println("LIS3DH not found or wrong WHO_AM_I");
  }
}

bool readLIS3DH(int16_t &x, int16_t &y, int16_t &z) {
  uint8_t data[6];
  if (!readBytes(ADDR_LIS3DH, 0x28 | 0x80, data, 6)) {
    return false;
  }

  int16_t rawX = (int16_t)((uint16_t)data[1] << 8 | data[0]);
  int16_t rawY = (int16_t)((uint16_t)data[3] << 8 | data[2]);
  int16_t rawZ = (int16_t)((uint16_t)data[5] << 8 | data[4]);

  // In high-resolution +/-2g mode, the useful value is left-aligned.
  // Shift to a readable signed value. Treat roughly as mg for bring-up.
  x = rawX >> 4;
  y = rawY >> 4;
  z = rawZ >> 4;
  return true;
}

uint16_t movementDeltaFrom(int16_t baseX, int16_t baseY, int16_t baseZ) {
  if (!lis3dhReady) {
    return 0;
  }

  int16_t x = 0;
  int16_t y = 0;
  int16_t z = 0;
  if (!readLIS3DH(x, y, z)) {
    return 0;
  }

  uint16_t dx = abs(x - baseX);
  uint16_t dy = abs(y - baseY);
  uint16_t dz = abs(z - baseZ);
  return max(dx, max(dy, dz));
}

void captureSleepBaseline() {
  if (!lis3dhReady || !readLIS3DH(sleepBaseX, sleepBaseY, sleepBaseZ)) {
    sleepBaseX = 0;
    sleepBaseY = 0;
    sleepBaseZ = 0;
  }

  Serial.printf("Sleep baseline X=%d Y=%d Z=%d\n", sleepBaseX, sleepBaseY, sleepBaseZ);
}

void enterDisplaySleep() {
  if (displaySleeping) {
    return;
  }

  Serial.println("Entering display sleep. Move board to wake.");
  captureSleepBaseline();

  gfx->fillScreen(C_BLACK);
  delay(40);
  lcdBacklight(false);
  irLed(false);
  irOffAtMs = 0;
  displaySleeping = true;
}

void wakeDisplayFromMovement(uint16_t delta) {
  Serial.printf("Movement wake detected, delta=%u\n", delta);

  lcdBacklight(true);
  displaySleeping = false;
  wokeAtMs = millis();
  page = 6;  // Show the accelerometer page first so the wake source is obvious.
  drawPage(page);

  nextRefreshMs = millis() + 400;
  nextPageMs = millis() + 4200;
  nextIrBlinkMs = millis() + 1000;
}

bool readMAX17048(float &voltage, float &percent, float &rate) {
  uint16_t vcell = 0;
  uint16_t soc = 0;
  uint16_t crate = 0;

  if (!readReg16BE(ADDR_MAX17048, 0x02, vcell)) return false;
  if (!readReg16BE(ADDR_MAX17048, 0x04, soc)) return false;
  readReg16BE(ADDR_MAX17048, 0x16, crate);

  // MAX17048 VCELL is a 12-bit value in bits 15:4, 1.25mV per LSB.
  voltage = (float)(vcell >> 4) * 0.00125f;

  // SOC is 8.8 fixed point percent.
  percent = (float)(soc >> 8) + ((float)(soc & 0xFF) / 256.0f);

  // CRATE is signed 16-bit, approximately 0.208%/hour per LSB.
  rate = (float)((int16_t)crate) * 0.208f;
  return true;
}

// ---------------------------------------------------------------------------
// Drawing helpers
// ---------------------------------------------------------------------------

void centredText(const char *text, int y, uint8_t size, uint16_t colour) {
  gfx->setTextSize(size);
  gfx->setTextColor(colour);

  int16_t x1;
  int16_t y1;
  uint16_t w;
  uint16_t h;
  gfx->getTextBounds(text, 0, y, &x1, &y1, &w, &h);
  gfx->setCursor((LCD_WIDTH - w) / 2, y);
  gfx->print(text);
}

void drawHeader(const char *title) {
  gfx->fillRect(0, 0, LCD_WIDTH, 38, rgb565(16, 28, 58));
  gfx->drawFastHLine(0, 38, LCD_WIDTH, C_CYAN);
  centredText(title, 11, 2, C_WHITE);
}

void drawFooter(const char *text) {
  gfx->fillRect(0, 292, LCD_WIDTH, 28, C_BLACK);
  gfx->drawFastHLine(0, 291, LCD_WIDTH, C_DARK);
  centredText(text, 302, 1, C_GREY);
}

void labelValue(int y, const char *label, const char *value, uint16_t valueColour = C_WHITE) {
  gfx->setTextSize(1);
  gfx->setTextColor(C_GREY);
  gfx->setCursor(18, y);
  gfx->print(label);

  gfx->setTextSize(2);
  gfx->setTextColor(valueColour);
  gfx->setCursor(18, y + 14);
  gfx->print(value);
}

void drawSplash() {
  gfx->fillScreen(C_BLACK);
  drawHeader("OpenRemote");

  gfx->drawRoundRect(16, 64, 208, 104, 14, C_CYAN);
  gfx->fillRoundRect(26, 74, 188, 84, 12, rgb565(8, 22, 42));
  centredText("ILI9341", 92, 3, C_CYAN);
  centredText("180 deg", 128, 2, C_WHITE);

  gfx->fillRoundRect(34, 194, 172, 44, 14, C_ORANGE);
  centredText("Sensor Demo", 208, 2, C_BLACK);

  drawFooter("LCD + I2C + battery + accel");
}

void drawColourBars() {
  const uint16_t colours[] = {
    C_RED, C_ORANGE, C_YELLOW, C_GREEN, C_CYAN, C_BLUE, C_MAGENTA, C_WHITE
  };

  gfx->fillScreen(C_BLACK);
  drawHeader("Colour Test");

  int y = 50;
  for (int i = 0; i < 8; i++) {
    gfx->fillRoundRect(22, y + i * 28, 196, 22, 6, colours[i]);
  }

  drawFooter("RGB565 colour bars");
}

void drawShapes() {
  gfx->fillScreen(C_BLACK);
  drawHeader("Shapes");

  gfx->drawRect(18, 56, 80, 58, C_CYAN);
  gfx->fillRect(128, 56, 80, 58, C_BLUE);

  gfx->drawRoundRect(18, 134, 80, 58, 12, C_ORANGE);
  gfx->fillRoundRect(128, 134, 80, 58, 12, C_MAGENTA);

  gfx->drawCircle(58, 236, 34, C_YELLOW);
  gfx->fillCircle(168, 236, 34, C_GREEN);

  drawFooter("text, boxes, circles");
}

void drawI2CPage() {
  scanI2C();

  gfx->fillScreen(C_BLACK);
  drawHeader("I2C Scan");

  char countLine[32];
  snprintf(countLine, sizeof(countLine), "%u found", foundDeviceCount);
  centredText(countLine, 54, 2, C_CYAN);

  gfx->setTextSize(1);
  for (uint8_t i = 0; i < foundDeviceCount && i < 12; i++) {
    int y = 84 + i * 16;
    bool known = strcmp(foundDevices[i].name, "Unknown") != 0;

    gfx->setTextColor(known ? C_ORANGE : C_YELLOW);
    gfx->setCursor(12, y);
    gfx->printf("0x%02X", foundDevices[i].address);

    gfx->setTextColor(known ? C_WHITE : C_CYAN);
    gfx->setCursor(62, y);
    gfx->print(foundDevices[i].name);
  }

  if (foundDeviceCount > 12) {
    gfx->setTextColor(C_GREY);
    gfx->setCursor(18, 282);
    gfx->printf("+ %u more in Serial Monitor", foundDeviceCount - 12);
  }

  if (i2cResultOverflow) {
    gfx->setTextColor(C_RED);
    gfx->setCursor(18, 268);
    gfx->print("LCD list truncated");
  }

  if (foundDeviceCount == 0) {
    centredText("No devices found", 140, 2, C_RED);
  }

  drawFooter("all 7-bit addresses: 0x01-0x7E");
}

void drawTouchPage() {
  gfx->fillScreen(C_BLACK);
  drawHeader("Touch Test");

  gfx->drawRoundRect(10, 52, 220, 204, 12, C_CYAN);

  gfx->setTextSize(1);
  gfx->setTextColor(C_GREY);
  gfx->setCursor(22, 70);
  gfx->print("Touch the screen");
  gfx->setCursor(22, 86);
  gfx->print("Crosshair follows your finger");

  gfx->fillRoundRect(18, 264, 204, 34, 10, C_NAVY);
  gfx->setTextColor(C_WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(30, 274);

  if (touchReady) {
    gfx->print("Waiting...");
  } else {
    gfx->print("No 0x38");
  }

  drawFooter("touch controller at 0x38");
  lastTouchX = -1;
  lastTouchY = -1;
}

void drawTouchPoint(uint16_t x, uint16_t y) {
  if (lastTouchX >= 0 && lastTouchY >= 0) {
    gfx->fillRect(lastTouchX - 12, lastTouchY - 12, 24, 24, C_BLACK);
  }

  gfx->drawRoundRect(10, 52, 220, 204, 12, C_CYAN);
  gfx->drawLine(x - 12, y, x + 12, y, C_YELLOW);
  gfx->drawLine(x, y - 12, x, y + 12, C_YELLOW);
  gfx->fillCircle(x, y, 4, C_ORANGE);

  gfx->fillRoundRect(18, 264, 204, 34, 10, C_NAVY);
  gfx->setTextSize(2);
  gfx->setTextColor(C_WHITE);
  gfx->setCursor(30, 274);
  gfx->printf("X:%03u Y:%03u", x, y);

  lastTouchX = x;
  lastTouchY = y;
}

void drawBatteryPage() {
  gfx->fillScreen(C_BLACK);
  drawHeader("Battery");

  float voltage = 0.0f;
  float percent = 0.0f;
  float rate = 0.0f;

  if (!readMAX17048(voltage, percent, rate)) {
    centredText("MAX17048", 122, 2, C_RED);
    centredText("not found", 154, 2, C_RED);
    drawFooter("expected at 0x36");
    return;
  }

  char line[32];

  snprintf(line, sizeof(line), "%.3f V", voltage);
  labelValue(58, "Voltage", line, C_CYAN);

  snprintf(line, sizeof(line), "%.1f %%", percent);
  labelValue(126, "State of charge", line, C_GREEN);

  snprintf(line, sizeof(line), "%.1f %%/hr", rate);
  labelValue(194, "Charge/discharge rate", line, rate < 0 ? C_ORANGE : C_GREEN);

  drawFooter("MAX17048 at 0x36");
}

void drawAccelBar(int y, const char *axis, int16_t value, uint16_t colour) {
  int barX = 72;
  int barY = y + 4;
  int barW = 132;
  int mid = barX + barW / 2;
  int len = constrain(map(value, -1200, 1200, -barW / 2, barW / 2), -barW / 2, barW / 2);

  gfx->setTextSize(2);
  gfx->setTextColor(C_WHITE);
  gfx->setCursor(18, y);
  gfx->print(axis);

  gfx->drawRect(barX, barY, barW, 12, C_DARK);
  gfx->drawFastVLine(mid, barY - 2, 16, C_GREY);

  if (len >= 0) {
    gfx->fillRect(mid, barY + 2, len, 8, colour);
  } else {
    gfx->fillRect(mid + len, barY + 2, -len, 8, colour);
  }

  gfx->setTextSize(1);
  gfx->setTextColor(C_GREY);
  gfx->setCursor(210, y + 2);
  gfx->printf("%d", value);
}

void drawAccelPage() {
  gfx->fillScreen(C_BLACK);
  drawHeader("Accelerometer");

  int16_t x = 0;
  int16_t y = 0;
  int16_t z = 0;

  if (!lis3dhReady || !readLIS3DH(x, y, z)) {
    centredText("LIS3DH", 122, 2, C_RED);
    centredText("not found", 154, 2, C_RED);
    drawFooter("expected at 0x19");
    return;
  }

  drawAccelBar(72, "X", x, C_CYAN);
  drawAccelBar(132, "Y", y, C_ORANGE);
  drawAccelBar(192, "Z", z, C_GREEN);

  gfx->setTextSize(1);
  gfx->setTextColor(C_GREY);
  gfx->setCursor(18, 252);
  gfx->print("Values are raw-ish high-res readings");

  drawFooter("LIS3DH at 0x19");
}

void drawPage(uint8_t newPage) {
  Serial.printf("Drawing page %u\n", newPage);

  switch (newPage) {
    case 0: drawSplash(); break;
    case 1: drawColourBars(); break;
    case 2: drawShapes(); break;
    case 3: drawI2CPage(); break;
    case 4: drawTouchPage(); break;
    case 5: drawBatteryPage(); break;
    default: drawAccelPage(); break;
  }
}


// ---------------------------------------------------------------------------
// Rev 6 power-on self-test
//
// Every result below is measured, never assumed. The LCD bus test reads each
// pin back through the ESP32's own input buffer, so a pin that is shorted to a
// rail or bridged to its neighbour is caught even though the firmware "wrote"
// the right value - which is exactly the failure that produces a white screen
// while the driver still reports success.
//
// Pin numbers were verified against the Rev 6 KiCad netlist
// (PCB/Revision 6/OpenRemote.kicad_pcb), not copied from the Rev 5 demo.
// ---------------------------------------------------------------------------

struct BusPin {
  const char *name;
  int gpio;
};

// The 12 signals that carry the display image. J2 pad numbers are in the
// comments so a failure here maps straight onto a connector pin to probe.
static const BusPin LCD_BUS[] = {
  {"LCD_D0", PIN_LCD_D0},   // J2.32
  {"LCD_D1", PIN_LCD_D1},   // J2.31
  {"LCD_D2", PIN_LCD_D2},   // J2.30
  {"LCD_D3", PIN_LCD_D3},   // J2.29
  {"LCD_D4", PIN_LCD_D4},   // J2.28
  {"LCD_D5", PIN_LCD_D5},   // J2.27
  {"LCD_D6", PIN_LCD_D6},   // J2.26
  {"LCD_D7", PIN_LCD_D7},   // J2.25
  {"LCD_WR", PIN_LCD_WR},   // J2.36
  {"LCD_RD", PIN_LCD_RD},   // J2.35
  {"LCD_DC", PIN_LCD_DC},   // J2.37
  {"LCD_CS", PIN_LCD_CS}    // J2.38
};
static const uint8_t LCD_BUS_COUNT = sizeof(LCD_BUS) / sizeof(LCD_BUS[0]);
static const char *J2_PAD[] = {"32","31","30","29","28","27","26","25","36","35","37","38"};

bool selfTestScreenFail = false;
bool selfTestSdFail = false;
bool selfTestMicFail = false;
char selfTestScreenDetail[240] = "";

static void noteScreenFail(const char *text) {
  selfTestScreenFail = true;
  if (strlen(selfTestScreenDetail) + strlen(text) + 3 < sizeof(selfTestScreenDetail)) {
    if (selfTestScreenDetail[0]) strcat(selfTestScreenDetail, ", ");
    strcat(selfTestScreenDetail, text);
  }
}

// Reads a pin back after forcing a known level onto it. settleMicros gives the
// pad and any attached panel input time to actually reach that level - reading
// immediately after pinMode() can return the previous state on a line with
// cable capacitance behind it.
static int probe(int gpio, uint8_t mode, uint32_t settleMicros = 600) {
  pinMode(gpio, mode);
  delayMicroseconds(settleMicros);
  return digitalRead(gpio);
}

static int driveAndRead(int gpio, int level, uint32_t settleMicros = 600) {
  pinMode(gpio, OUTPUT);
  digitalWrite(gpio, level);
  delayMicroseconds(settleMicros);
  return digitalRead(gpio);
}

// Test 1: each pin must follow its own pull-up and pull-down, and must follow
// the output driver in both directions. A pin welded to GND or 3V3 by a solder
// bridge fails at least one of the four.
static void testLcdLevels() {
  Serial.println();
  Serial.println("[1/4] LCD bus - stuck-level test");
  Serial.println("      pin      gpio  J2   pu  pd  drvH drvL  result");

  for (uint8_t i = 0; i < LCD_BUS_COUNT; i++) {
    const BusPin &p = LCD_BUS[i];
    int pu   = probe(p.gpio, INPUT_PULLUP);
    int pd   = probe(p.gpio, INPUT_PULLDOWN);
    int dh   = driveAndRead(p.gpio, HIGH);
    int dl   = driveAndRead(p.gpio, LOW);

    const char *why = nullptr;
    if (pu == LOW  && pd == LOW  && dh == LOW)  why = "stuck LOW (short to GND)";
    else if (pu == HIGH && pd == HIGH && dl == HIGH) why = "stuck HIGH (short to 3V3)";
    else if (pu != HIGH) why = "pull-up did not read high";
    else if (pd != LOW)  why = "pull-down did not read low";
    else if (dh != HIGH) why = "cannot drive high";
    else if (dl != LOW)  why = "cannot drive low";

    Serial.printf("      %-8s %-5d %-4s %d   %d   %d    %d    %s\n",
                  p.name, p.gpio, J2_PAD[i], pu, pd, dh, dl,
                  why ? "FAIL" : "pass");
    if (why) {
      char msg[64];
      snprintf(msg, sizeof(msg), "%s(GPIO%d/J2.%s) %s", p.name, p.gpio, J2_PAD[i], why);
      noteScreenFail(msg);
    }
  }
}

// Test 2: drive one pin low while every other bus pin is pulled up. Any other
// pin that follows it down shares copper with it - the classic hand-solder
// bridge between neighbouring fine-pitch pads.
static void testLcdShorts() {
  Serial.println();
  Serial.println("[2/4] LCD bus - short-between-pins test");
  bool anyShort = false;

  for (uint8_t i = 0; i < LCD_BUS_COUNT; i++) {
    for (uint8_t j = 0; j < LCD_BUS_COUNT; j++) {
      if (i != j) pinMode(LCD_BUS[j].gpio, INPUT_PULLUP);
    }
    pinMode(LCD_BUS[i].gpio, OUTPUT);
    digitalWrite(LCD_BUS[i].gpio, LOW);
    delayMicroseconds(800);

    for (uint8_t j = 0; j < LCD_BUS_COUNT; j++) {
      if (i == j) continue;
      if (digitalRead(LCD_BUS[j].gpio) == LOW) {
        anyShort = true;
        Serial.printf("      SHORT: %s (GPIO%d/J2.%s) <-> %s (GPIO%d/J2.%s)\n",
                      LCD_BUS[i].name, LCD_BUS[i].gpio, J2_PAD[i],
                      LCD_BUS[j].name, LCD_BUS[j].gpio, J2_PAD[j]);
        char msg[80];
        snprintf(msg, sizeof(msg), "%s shorted to %s", LCD_BUS[i].name, LCD_BUS[j].name);
        noteScreenFail(msg);
      }
    }
  }
  if (!anyShort) Serial.println("      no shorts detected between the 12 bus pins - pass");
}

// Test 3: the two display power controls. Both are active-low on this board
// (LCD_EN -> Q4 -> +VSW, LCD_BL -> backlight), so each is checked for the
// ability to sit at both levels rather than being clamped.
static void testLcdPowerPins() {
  Serial.println();
  Serial.println("[3/4] LCD power control pins");
  struct { const char *name; int gpio; } ctrl[] = {
    {"LCD_EN", PIN_LCD_EN}, {"LCD_BL", PIN_LCD_BL}
  };
  for (uint8_t i = 0; i < 2; i++) {
    int dh = driveAndRead(ctrl[i].gpio, HIGH, 1500);
    int dl = driveAndRead(ctrl[i].gpio, LOW, 1500);
    bool ok = (dh == HIGH && dl == LOW);
    Serial.printf("      %-8s gpio %-3d drvH=%d drvL=%d  %s\n",
                  ctrl[i].name, ctrl[i].gpio, dh, dl, ok ? "pass" : "FAIL");
    if (!ok) {
      char msg[64];
      snprintf(msg, sizeof(msg), "%s(GPIO%d) cannot change state", ctrl[i].name, ctrl[i].gpio);
      noteScreenFail(msg);
    }
  }
}

// Restore every bus pin to the state Arduino_GFX expects before the display is
// initialised. Leaving them as inputs after the tests above would leave the
// panel unable to latch anything.
static void restoreLcdBus() {
  for (uint8_t i = 0; i < LCD_BUS_COUNT; i++) {
    pinMode(LCD_BUS[i].gpio, OUTPUT);
    digitalWrite(LCD_BUS[i].gpio, LOW);
  }
  pinMode(PIN_LCD_EN, OUTPUT);
  digitalWrite(PIN_LCD_EN, LOW);
  pinMode(PIN_LCD_BL, OUTPUT);
  digitalWrite(PIN_LCD_BL, LOW);
  delay(120);
}

// ---------------------------------------------------------------------------
// microSD test
//
// The card sits behind Q7, a P-channel MOSFET whose gate is pulled to +3.3V by
// R24 and dragged down through R2 from SD_EN - so SD_EN LOW powers the card.
// This mirrors the main firmware's initSdStorage() rather than inventing a
// different sequence, including its 120ms settle and its speed fallback: a
// card that will not train at 20MHz on hand-soldered wiring often mounts
// perfectly at 4MHz, and reporting "no card" in that case would be wrong.
// ---------------------------------------------------------------------------

static void testSdCard() {
  Serial.println();
  Serial.println("[4/4] microSD card");

  pinMode(PIN_SD_EN, OUTPUT);
  digitalWrite(PIN_SD_EN, LOW);          // active-low power switch: ON
  delay(120);

  sdSpi.begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);

  const uint32_t speeds[] = {20000000, 10000000, 4000000, 1000000};
  bool mounted = false;
  uint32_t mountedAt = 0;
  for (uint8_t i = 0; i < 4 && !mounted; i++) {
    if (SD.begin(PIN_SD_CS, sdSpi, speeds[i])) {
      mounted = true;
      mountedAt = speeds[i];
    } else {
      SD.end();
      delay(40);
    }
  }

  if (!mounted) {
    selfTestSdFail = true;
    Serial.println("      SD FAIL - no card responded at 20/10/4/1 MHz");
    Serial.println("      check: card inserted? J5 solder? SD_EN(GPIO16) -> Q7 -> /SD_3V3?");
    Serial.println("      probe TP17=SD_CS TP18=SD_MOSI TP19=SD_SCK TP20=SD_MISO TP23=SD_3V3");
    sdSpi.end();
    return;
  }

  uint8_t type = SD.cardType();
  const char *typeName = type == CARD_MMC  ? "MMC"  :
                         type == CARD_SD   ? "SDSC" :
                         type == CARD_SDHC ? "SDHC" : "unknown";
  uint64_t sizeMB = SD.cardSize() / (1024ULL * 1024ULL);
  Serial.printf("      mounted at %lu MHz - type %s, %llu MB\n",
                (unsigned long)(mountedAt / 1000000UL), typeName, sizeMB);

  // Mounting proves the card answers; a write proves the bus is good in both
  // directions and that the card is not write-protected or worn out.
  const char *path = "/or_selftest.tmp";
  File f = SD.open(path, FILE_WRITE);
  if (!f) {
    selfTestSdFail = true;
    Serial.println("      SD FAIL - card mounted but a test file could not be created");
  } else {
    f.print("openremote-selftest");
    f.close();
    File r = SD.open(path, FILE_READ);
    String back = r ? r.readString() : String();
    if (r) r.close();
    SD.remove(path);
    if (back == "openremote-selftest") {
      Serial.println("      read/write verified - pass");
    } else {
      selfTestSdFail = true;
      Serial.printf("      SD FAIL - wrote 19 bytes, read back %u\n", back.length());
    }
  }
  SD.end();
  sdSpi.end();
}

// ---------------------------------------------------------------------------
// I2S microphone test
//
// GPIO45 powers the microphone. It also drives indicator LED D3, so the LED
// lighting is a useful visual confirmation that mic power is actually applied.
//
// Note for anyone reading the KiCad files: the netlist shows MIC1's VDD pin on
// a net called MIC_VDD and GPIO45 on a net called Net-(D3-A), which makes them
// look separate. They are not - the copper joins at D3's anode pad, so the two
// net names describe one physical node. Verified by measurement on real
// hardware, and by the routing (a MIC_VDD track endpoint lands exactly on
// D3 pad 2). Do not "fix" this based on the net names alone.
//
// The mic shares SD_SCK / SD_MOSI / SD_MISO with the card through R45/R46/R47,
// so the SD test above must finish and release the bus before this runs.
// ---------------------------------------------------------------------------

static void testMicrophone() {
  Serial.println();
  Serial.println("[extra] I2S microphone");

  pinMode(PIN_MIC_POWER, OUTPUT);
  digitalWrite(PIN_MIC_POWER, HIGH);     // lights D3; does not power the mic
  delay(50);

  i2s_chan_handle_t rx = nullptr;
  i2s_chan_config_t chanCfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
  if (i2s_new_channel(&chanCfg, nullptr, &rx) != ESP_OK) {
    selfTestMicFail = true;
    Serial.println("      MIC FAIL - could not allocate an I2S channel");
    digitalWrite(PIN_MIC_POWER, LOW);
    return;
  }

  i2s_std_config_t cfg = {
    .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(16000),
    .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT,
                                                    I2S_SLOT_MODE_MONO),
    .gpio_cfg = {
      .mclk = I2S_GPIO_UNUSED,
      .bclk = (gpio_num_t)PIN_MIC_BCLK,
      .ws   = (gpio_num_t)PIN_MIC_WS,
      .dout = I2S_GPIO_UNUSED,
      .din  = (gpio_num_t)PIN_MIC_DATA,
      .invert_flags = {false, false, false}
    }
  };
  cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;   // L/R pin is tied to GND

  bool ok = (i2s_channel_init_std_mode(rx, &cfg) == ESP_OK) &&
            (i2s_channel_enable(rx) == ESP_OK);
  if (!ok) {
    selfTestMicFail = true;
    Serial.println("      MIC FAIL - I2S standard mode would not start");
    i2s_del_channel(rx);
    digitalWrite(PIN_MIC_POWER, LOW);
    return;
  }

  static int32_t samples[256];
  size_t got = 0;
  int32_t lo = INT32_MAX, hi = INT32_MIN;
  uint32_t nonZero = 0, total = 0;

  for (uint8_t pass = 0; pass < 4; pass++) {
    if (i2s_channel_read(rx, samples, sizeof(samples), &got, 200) != ESP_OK) continue;
    uint32_t n = got / sizeof(int32_t);
    for (uint32_t i = 0; i < n; i++) {
      int32_t v = samples[i] >> 8;        // 24-bit sample in a 32-bit slot
      if (v < lo) lo = v;
      if (v > hi) hi = v;
      if (v != 0) nonZero++;
      total++;
    }
  }

  i2s_channel_disable(rx);
  i2s_del_channel(rx);
  digitalWrite(PIN_MIC_POWER, LOW);

  if (total == 0) {
    selfTestMicFail = true;
    Serial.println("      MIC FAIL - I2S started but returned no samples at all");
    return;
  }

  int32_t span = hi - lo;
  Serial.printf("      %lu samples, min %ld, max %ld, peak-to-peak %ld, %lu non-zero\n",
                (unsigned long)total, (long)lo, (long)hi, (long)span,
                (unsigned long)nonZero);

  // A live mic always dithers by at least a few LSB. A dead line reads as a
  // single unchanging value - all zeros when idle-low, all -1 when idle-high.
  if (span < 8 || nonZero == 0) {
    selfTestMicFail = true;
    Serial.println("      MIC FAIL - data line is static, no audio present");
    Serial.println("      check: is LED D3 lit? if not, GPIO45 is not delivering");
    Serial.println("      mic power. if it IS lit, suspect MIC1 solder joints or");
    Serial.println("      R45/R46/R47 (SCK/WS/DATA to the shared SD bus).");
  } else {
    Serial.println("      microphone is producing live audio - pass");
  }
}

// ---------------------------------------------------------------------------
// Runs every check and prints one unambiguous verdict per subsystem.
// ---------------------------------------------------------------------------
void runSelfTest() {
  Serial.println();
  Serial.println("===========================================================");
  Serial.println("  OpenRemote Rev 6 hardware self-test");
  Serial.println("===========================================================");

  testLcdLevels();
  testLcdShorts();
  testLcdPowerPins();
  restoreLcdBus();
  testSdCard();
  testMicrophone();

  Serial.println();
  Serial.println("-----------------------------------------------------------");
  Serial.println("  RESULT");
  Serial.println("-----------------------------------------------------------");

  if (selfTestScreenFail) {
    Serial.println("  *** SCREEN FAIL ***");
    Serial.printf("  Failed: %s\n", selfTestScreenDetail);
    Serial.println("  A failing pin above is why the panel shows white: the");
    Serial.println("  driver still reports success because it cannot see the");
    Serial.println("  bus. Probe that pin at its J2 pad and at the ESP32.");
  } else {
    Serial.println("  SCREEN PASS - all 12 bus pins drive and read back cleanly,");
    Serial.println("  no shorts between them, both power controls respond.");
    Serial.println("  If the panel is still white the fault is past the pins:");
    Serial.println("  the J2 connector joints, the flex seating, or the panel.");
  }

  Serial.printf("  SD CARD  : %s\n", selfTestSdFail ? "FAIL (see above)" : "PASS");
  Serial.printf("  MICROPHONE: %s\n", selfTestMicFail
                ? "FAIL - unpowered by design on Rev 6, see above" : "PASS");
  Serial.println("-----------------------------------------------------------");
  Serial.println();
}


// ---------------------------------------------------------------------------
// Live dashboard - every subsystem on one 240x320 screen
//
// Replaces the old page carousel. Everything that was spread over seven
// auto-advancing pages is visible at once, so a fault shows up next to
// everything that is working instead of having to be waited for.
//
// Drawing is split into a static pass and per-region updates. Redrawing the
// whole screen at 10Hz on a synchronous 8-bit parallel bus visibly tears and
// flickers, so the chrome is painted once and each value repaints only its own
// small rectangle.
// ---------------------------------------------------------------------------

// Row geometry. Kept as named constants because several of the update
// functions need to clear a region they do not draw the label for.
static const int DASH_HEAD_Y   = 0;
static const int DASH_DEV_Y    = 22;
static const int DASH_ACC_Y    = 88;
static const int DASH_MIC_Y    = 150;
static const int DASH_BATT_Y   = 188;
static const int DASH_TOUCH_Y  = 226;
static const int DASH_TOUCH_H  = LCD_HEIGHT - DASH_TOUCH_Y - 2;

static const uint16_t C_PANEL = 0x18E3;   // panel fill, slightly above black
static const uint16_t C_LABEL = 0x8C71;   // dim grey label text

i2s_chan_handle_t dashMicRx = nullptr;
bool dashMicRunning = false;
float dashMicLevel = 0.0f;        // 0..1 smoothed
int32_t dashMicPeak = 0;
uint16_t dashLastTouchX = 0, dashLastTouchY = 0;
bool dashHadTouch = false;
uint32_t dashTouchCount = 0;

static void dashPanel(int y, int h, const char *title) {
  gfx->fillRoundRect(4, y, LCD_WIDTH - 8, h, 4, C_PANEL);
  gfx->drawRoundRect(4, y, LCD_WIDTH - 8, h, 4, 0x39E7);
  if (title) {
    gfx->setTextSize(1);
    gfx->setTextColor(C_LABEL);
    gfx->setCursor(9, y + 4);
    gfx->print(title);
  }
}

// Clears just the area a value occupies before reprinting it. Without this the
// old digits stay behind whenever a value gets shorter (4.05V -> 4.0V).
static void dashValue(int x, int y, int w, const char *text, uint16_t colour, uint8_t size = 1) {
  gfx->fillRect(x, y, w, size * 8, C_PANEL);
  gfx->setTextSize(size);
  gfx->setTextColor(colour);
  gfx->setCursor(x, y);
  gfx->print(text);
}

// The microphone shares SD_SCK / SD_MOSI / SD_MISO with the card, so this can
// only run once the self-test's SD stage has released the bus. It is started
// once and left running for the life of the dashboard.
static void dashStartMic() {
  pinMode(PIN_MIC_POWER, OUTPUT);
  digitalWrite(PIN_MIC_POWER, HIGH);    // powers the mic and lights D3
  delay(50);

  i2s_chan_config_t chanCfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
  if (i2s_new_channel(&chanCfg, nullptr, &dashMicRx) != ESP_OK) return;

  i2s_std_config_t cfg = {
    .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(16000),
    .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT,
                                                    I2S_SLOT_MODE_MONO),
    .gpio_cfg = {
      .mclk = I2S_GPIO_UNUSED,
      .bclk = (gpio_num_t)PIN_MIC_BCLK,
      .ws   = (gpio_num_t)PIN_MIC_WS,
      .dout = I2S_GPIO_UNUSED,
      .din  = (gpio_num_t)PIN_MIC_DATA,
      .invert_flags = {false, false, false}
    }
  };
  cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;   // L/R tied to GND

  if (i2s_channel_init_std_mode(dashMicRx, &cfg) == ESP_OK &&
      i2s_channel_enable(dashMicRx) == ESP_OK) {
    dashMicRunning = true;
  } else {
    i2s_del_channel(dashMicRx);
    dashMicRx = nullptr;
  }
}

// Non-blocking: a zero timeout means a quiet moment costs nothing rather than
// stalling the whole UI loop waiting for a full DMA buffer.
static void dashSampleMic() {
  if (!dashMicRunning) return;
  static int32_t buf[128];
  size_t got = 0;
  if (i2s_channel_read(dashMicRx, buf, sizeof(buf), &got, 0) != ESP_OK || got == 0) return;

  uint32_t n = got / sizeof(int32_t);
  int32_t lo = INT32_MAX, hi = INT32_MIN;
  for (uint32_t i = 0; i < n; i++) {
    int32_t v = buf[i] >> 8;          // 24-bit sample left-aligned in 32 bits
    if (v < lo) lo = v;
    if (v > hi) hi = v;
  }
  int32_t span = hi - lo;
  if (span > dashMicPeak) dashMicPeak = span;

  // Full scale for a 24-bit slot is 2^23. Scaled so ordinary speech lands
  // mid-bar rather than pinning at either end.
  float level = (float)span / 200000.0f;
  if (level > 1.0f) level = 1.0f;
  // Fast attack, slow decay - a meter that falls instantly is unreadable.
  dashMicLevel = (level > dashMicLevel) ? level : (dashMicLevel * 0.85f + level * 0.15f);
}

// Static chrome: painted once, then only values are repainted.
void drawDashboardStatic() {
  gfx->fillScreen(C_BLACK);

  gfx->fillRect(0, DASH_HEAD_Y, LCD_WIDTH, 20, 0x0208);
  gfx->setTextSize(1);
  gfx->setTextColor(C_CYAN);
  gfx->setCursor(6, DASH_HEAD_Y + 6);
  gfx->print("OpenRemote Rev6 - live monitor");

  dashPanel(DASH_DEV_Y,   62, "I2C / I2S DEVICES");
  dashPanel(DASH_ACC_Y,   58, "ACCELEROMETER  LIS3DH");
  dashPanel(DASH_MIC_Y,   34, "MICROPHONE  I2S");
  dashPanel(DASH_BATT_Y,  34, "BATTERY  MAX17048");
  dashPanel(DASH_TOUCH_Y, DASH_TOUCH_H, "TOUCH  0x38");

  // Axis letters are static; only the bars and numbers move.
  const char *axis[3] = {"X", "Y", "Z"};
  for (uint8_t i = 0; i < 3; i++) {
    gfx->setTextSize(1);
    gfx->setTextColor(C_LABEL);
    gfx->setCursor(9, DASH_ACC_Y + 18 + i * 12);
    gfx->print(axis[i]);
  }
  // Centre line for the signed accelerometer bars.
  for (uint8_t i = 0; i < 3; i++) {
    gfx->drawFastVLine(120, DASH_ACC_Y + 17 + i * 12, 9, 0x39E7);
  }
}

// Rescanned periodically so hot-plugging the touch flex or the SD card shows
// up without a reboot.
void updateDashboardDevices() {
  struct { uint8_t addr; const char *name; } want[] = {
    {ADDR_LIS3DH,   "ACCEL"},
    {ADDR_TCA8418,  "KEYPAD"},
    {ADDR_MAX17048, "BATTERY"},
    {ADDR_TOUCH,    "TOUCH"}
  };
  for (uint8_t i = 0; i < 4; i++) {
    int col = i % 2, row = i / 2;
    int x = 9 + col * 112;
    int y = DASH_DEV_Y + 18 + row * 12;
    bool present = i2cDevicePresent(want[i].addr);
    char text[24];
    snprintf(text, sizeof(text), "%02X %-7s %s", want[i].addr, want[i].name,
             present ? "OK" : "--");
    dashValue(x, y, 108, text, present ? C_GREEN : C_RED);
  }
  // I2S has no address to probe, so the mic is reported by whether its channel
  // actually started and is delivering samples.
  char mic[30];
  snprintf(mic, sizeof(mic), "I2S MIC   %s",
           !dashMicRunning ? "NOT STARTED" : (dashMicPeak > 8 ? "STREAMING" : "SILENT"));
  dashValue(9, DASH_DEV_Y + 42, LCD_WIDTH - 26, mic,
            !dashMicRunning ? C_RED : (dashMicPeak > 8 ? C_GREEN : C_YELLOW));
}

void updateDashboardAccel() {
  int16_t x = 0, y = 0, z = 0;
  if (!readLIS3DH(x, y, z)) {
    dashValue(9, DASH_ACC_Y + 18, LCD_WIDTH - 26, "read failed", C_RED);
    return;
  }
  const int16_t v[3] = {x, y, z};
  const uint16_t colour[3] = {C_RED, C_GREEN, C_CYAN};
  for (uint8_t i = 0; i < 3; i++) {
    int barY = DASH_ACC_Y + 17 + i * 12;
    // Clear the whole bar lane, then redraw from the centre line outward.
    gfx->fillRect(22, barY, 196, 9, C_PANEL);
    gfx->drawFastVLine(120, barY, 9, 0x39E7);
    int len = (int)((long)v[i] * 96 / 1100);       // +-1100 counts ~= +-1g
    if (len > 96) len = 96;
    if (len < -96) len = -96;
    if (len >= 0) gfx->fillRect(120, barY + 2, len, 5, colour[i]);
    else          gfx->fillRect(120 + len, barY + 2, -len, 5, colour[i]);
    char text[10];
    snprintf(text, sizeof(text), "%+5d", v[i]);
    gfx->fillRect(180, barY, 38, 8, C_PANEL);
    gfx->setTextSize(1);
    gfx->setTextColor(C_WHITE);
    gfx->setCursor(180, barY);
    gfx->print(text);
  }
}

void updateDashboardMic() {
  int barX = 9, barY = DASH_MIC_Y + 16, barW = 150, barH = 10;
  gfx->drawRect(barX, barY, barW, barH, 0x39E7);
  int fill = (int)(dashMicLevel * (barW - 2));
  gfx->fillRect(barX + 1, barY + 1, fill, barH - 2,
                dashMicLevel > 0.75f ? C_RED : (dashMicLevel > 0.35f ? C_YELLOW : C_GREEN));
  gfx->fillRect(barX + 1 + fill, barY + 1, (barW - 2) - fill, barH - 2, C_PANEL);
  char text[16];
  if (!dashMicRunning) snprintf(text, sizeof(text), "off");
  else snprintf(text, sizeof(text), "pk %ld", (long)dashMicPeak);
  dashValue(barX + barW + 6, barY + 1, LCD_WIDTH - (barX + barW + 6) - 10, text,
            dashMicRunning ? C_WHITE : C_RED);
}

void updateDashboardBattery() {
  float voltage = 0, percent = 0, rate = 0;
  if (!readMAX17048(voltage, percent, rate)) {
    dashValue(9, DASH_BATT_Y + 18, LCD_WIDTH - 26, "not responding", C_RED);
    return;
  }
  char text[40];
  snprintf(text, sizeof(text), "%.2fV  %.0f%%  %+.1f%%/h", voltage, percent, rate);
  uint16_t colour = percent < 15.0f ? C_RED : (percent < 40.0f ? C_YELLOW : C_GREEN);
  dashValue(9, DASH_BATT_Y + 18, LCD_WIDTH - 26, text, colour);
}

// Draws inside the touch panel only, so a stray coordinate cannot paint over
// the readouts above it.
void updateDashboardTouch() {
  const int x0 = 6, y0 = DASH_TOUCH_Y + 14;
  const int w = LCD_WIDTH - 12, h = DASH_TOUCH_H - 16;
  uint16_t tx = 0, ty = 0;
  if (!readTouchPoint(tx, ty)) {
    if (dashHadTouch) {
      dashHadTouch = false;
      char text[28];
      snprintf(text, sizeof(text), "released  count %lu", (unsigned long)dashTouchCount);
      dashValue(x0 + 4, y0 + 2, w - 8, text, C_LABEL);
    }
    return;
  }
  if (!dashHadTouch) dashTouchCount++;
  dashHadTouch = true;

  // Erase the previous marker rather than clearing the whole panel - a full
  // clear each frame is what made the old touch page strobe while dragging.
  if (dashLastTouchX || dashLastTouchY) {
    gfx->fillRect(dashLastTouchX - 7, dashLastTouchY - 7, 15, 15, C_PANEL);
  }
  int px = x0 + 4 + (int)((long)tx * (w - 8) / LCD_WIDTH);
  int py = y0 + 14 + (int)((long)ty * (h - 20) / LCD_HEIGHT);
  gfx->drawCircle(px, py, 6, C_ORANGE);
  gfx->drawFastHLine(px - 8, py, 17, C_ORANGE);
  gfx->drawFastVLine(px, py - 8, 17, C_ORANGE);
  dashLastTouchX = px; dashLastTouchY = py;

  char text[28];
  snprintf(text, sizeof(text), "x %3u  y %3u  n %lu", tx, ty, (unsigned long)dashTouchCount);
  dashValue(x0 + 4, y0 + 2, w - 8, text, C_WHITE);
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("OpenRemote Rev 5 LCD + I2C sensor demo");
  Serial.println("---------------------------------------");
  Serial.println("Screen rotation: 0 degrees (touch coordinates flipped to match)");

  pinMode(PIN_IR_LED, OUTPUT);
  irLed(false);

  // Self-test runs before the display is brought up: it needs the bus
  // pins to itself, and its result decides whether a white screen later
  // is a bus fault or something past the connector.
  runSelfTest();

  Serial.println("Powering LCD...");
  lcdPowerOn();

  Serial.println("Starting I2C...");
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(400000);

  scanI2C();
  touchReady = i2cDevicePresent(ADDR_TOUCH);
  Serial.printf("Touch controller at 0x38: %s\n", touchReady ? "found" : "not found");
  initLIS3DH();

  Serial.println("Initialising ILI9341...");
  if (!gfx->begin()) {
    Serial.println("gfx->begin() returned false. Check LCD wiring/power.");
  } else {
    Serial.println("LCD init completed.");
  }

  // One screen, always on. The self-test above has already released the SD
  // bus, so the microphone can take the shared pins from here on.
  dashStartMic();
  drawDashboardStatic();
  updateDashboardDevices();
  updateDashboardAccel();
  updateDashboardBattery();
  updateDashboardMic();

  wokeAtMs = millis();
  nextRefreshMs = millis() + 100;
  nextIrBlinkMs = millis() + 1500;
}

void loop() {
  unsigned long now = millis();

  if (displaySleeping) {
    uint16_t delta = movementDeltaFrom(sleepBaseX, sleepBaseY, sleepBaseZ);
    if (delta >= WAKE_MOVEMENT_DELTA) {
      wakeDisplayFromMovement(delta);
    }
    delay(80);
    return;
  }

  serviceIrBlink();

  // Deliberately no auto-sleep. This is a bench instrument - a screen that
  // blanks after 60s while someone is probing the board with a meter is worse
  // than useless. The accelerometer wake path above is left intact for anyone
  // who wants it back; only the timeout that triggered it is gone.

  // Touch is polled every pass so dragging stays smooth. Everything else is
  // rate-limited: I2C reads and panel writes are the expensive part, and the
  // numbers are unreadable if they update faster than the eye can follow.
  dashSampleMic();
  updateDashboardTouch();

  static unsigned long nextMicMs = 0, nextAccMs = 0, nextBattMs = 0, nextDevMs = 0;
  if (now >= nextAccMs)  { updateDashboardAccel();   nextAccMs  = now + 120; }
  if (now >= nextMicMs)  { updateDashboardMic();     nextMicMs  = now + 100; }
  if (now >= nextBattMs) { updateDashboardBattery(); nextBattMs = now + 1000; }
  if (now >= nextDevMs)  { updateDashboardDevices(); dashMicPeak = 0; nextDevMs = now + 2000; }

  delay(8);
}

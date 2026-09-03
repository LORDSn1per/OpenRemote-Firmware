#include <Arduino.h>
#include "driver/ledc.h"
#include "tft_hal_esp32.h"
#include "sleep_hal_esp32.h"

// The ESP32 can only do LOW_SPEED_MODE
#if(OMOTE_HARDWARE_REV >= 5)
#define LEDC_SPEED_MODE LEDC_LOW_SPEED_MODE
#else
#define LEDC_SPEED_MODE LEDC_HIGH_SPEED_MODE
#endif

// Set pins for 8-bit mode (ESP32-S3) or SPI (ESP32)
#if(OMOTE_HARDWARE_REV >= 5)
  const uint8_t SDA_GPIO = 20;
  const uint8_t SCL_GPIO = 19;

  const uint8_t LCD_BL_GPIO = 9;
  const uint8_t LCD_EN_GPIO = 38;
  const uint8_t LCD_CS_GPIO = 39;
  const uint8_t LCD_DC_GPIO = 40;
  const uint8_t LCD_WR_GPIO = 41;
  const uint8_t LCD_RD_GPIO = 42;
  const uint8_t LCD_D0_GPIO = 48;
  const uint8_t LCD_D1_GPIO = 47;
  const uint8_t LCD_D2_GPIO = 21;
  const uint8_t LCD_D3_GPIO = 14;
  const uint8_t LCD_D4_GPIO = 13;
  const uint8_t LCD_D5_GPIO = 12;
  const uint8_t LCD_D6_GPIO = 11;
  const uint8_t LCD_D7_GPIO = 10;
#else
  const uint8_t SDA_GPIO = 19;
  const uint8_t SCL_GPIO = 22;

  const uint8_t LCD_BL_GPIO = 4;
  const uint8_t LCD_EN_GPIO = 10;
  const uint8_t LCD_CS_GPIO = 5;
  const uint8_t LCD_DC_GPIO = 9;
  const uint8_t LCD_MOSI_GPIO = 23;
  const uint8_t LCD_SCK_GPIO = 18;
#endif

LGFX::LGFX(void) {
  {
    auto cfg = _bus_instance.config();
    cfg.freq_write = SPI_FREQUENCY;
    #if(OMOTE_HARDWARE_REV >= 5)
    cfg.pin_wr = LCD_WR_GPIO;
    cfg.pin_rd = LCD_RD_GPIO;
    cfg.pin_rs = LCD_DC_GPIO;
    cfg.pin_d0 = LCD_D0_GPIO;
    cfg.pin_d1 = LCD_D1_GPIO;
    cfg.pin_d2 = LCD_D2_GPIO;
    cfg.pin_d3 = LCD_D3_GPIO;
    cfg.pin_d4 = LCD_D4_GPIO;
    cfg.pin_d5 = LCD_D5_GPIO;
    cfg.pin_d6 = LCD_D6_GPIO;
    cfg.pin_d7 = LCD_D7_GPIO;
    #else
    cfg.freq_read  = 16000000;
    cfg.dma_channel = SPI_DMA_CH_AUTO;
    cfg.pin_sclk = LCD_SCK_GPIO;
    cfg.pin_mosi = LCD_MOSI_GPIO;
    cfg.pin_dc   = LCD_DC_GPIO;
    #endif
    _bus_instance.config(cfg);
    _panel_instance.setBus(&_bus_instance);
  }
  {
    auto cfg = _panel_instance.config();
    cfg.pin_cs           = LCD_CS_GPIO;
    cfg.pin_rst          = -1;
    cfg.pin_busy         = -1;
    cfg.memory_width     = SCR_WIDTH;
    cfg.memory_height    = SCR_HEIGHT;
    cfg.panel_width      = SCR_WIDTH;
    cfg.panel_height     = SCR_HEIGHT;
    cfg.offset_rotation  = 2;
    _panel_instance.config(cfg);
  }
  {
    auto cfg = _touch_instance.config();
    cfg.i2c_addr = 0x38;
    cfg.i2c_port = 0;
    cfg.pin_sda = SDA_GPIO;
    cfg.pin_scl = SCL_GPIO;
    cfg.freq = 400000;
    cfg.x_min = 0;
    cfg.x_max = SCR_WIDTH-1;
    cfg.y_min = 0;
    cfg.y_max = SCR_HEIGHT-1;
    _touch_instance.config(cfg);
    _panel_instance.setTouch(&_touch_instance);
  }
  setPanel(&_panel_instance);
}
LGFX tft;
byte backlightBrightness = 255;

// Quiet settling time given to the FT6206 after its power rail comes up, before
// anything starts driving the display. See the long note in init_tft().
// Datasheet-class figure for FT6x06 power-on to valid operation is ~300 ms.
static const uint32_t TOUCH_POWER_ON_SETTLE_MS = 300;

void init_tft(void) {
  // Configure the backlight PWM
  // Manual setup because ledcSetup() briefly turns on the backlight
  ledc_channel_config_t ledc_channel_left;
  ledc_channel_left.gpio_num = (gpio_num_t)LCD_BL_GPIO;
  ledc_channel_left.speed_mode = LEDC_SPEED_MODE;
  ledc_channel_left.channel = LEDC_CHANNEL_5;
  ledc_channel_left.intr_type = LEDC_INTR_DISABLE;
  ledc_channel_left.timer_sel = LEDC_TIMER_1;
  // LEDC channel duty, the range of duty setting is [0, (2**duty_resolution)]
  ledc_channel_left.duty = 0;
  // needs to be set to 0, otherwise log message "E (324) ledc: ledc_set_duty_with_hpoint(699): hpoint argument is invalid"
  // https://github.com/mudassar-tamboli/ESP32-OV7670-WebSocket-Camera/issues/13
  // LEDC channel hpoint value, the max value is 0xfffff
  ledc_channel_left.hpoint = 0;
  ledc_channel_left.flags.output_invert = 1; // Can't do this with ledcSetup()
  // hpoint and duty explained:
  // https://miro.medium.com/v2/resize:fit:1400/1*ViqSTFdH9COZ51iKYrIyMA.png
  ledc_channel_config(&ledc_channel_left);

  ledc_timer_config_t ledc_timer;
  ledc_timer.speed_mode = LEDC_SPEED_MODE;
  ledc_timer.duty_resolution = LEDC_TIMER_8_BIT;
  ledc_timer.timer_num = LEDC_TIMER_1;
  ledc_timer.freq_hz = 640;
  // https://github.com/mudassar-tamboli/ESP32-OV7670-WebSocket-Camera/issues/13
  // otherwise crash with "assert failed: ledc_clk_cfg_to_global_clk ledc.c:444 (false)"
  ledc_timer.clk_cfg = LEDC_USE_APB_CLK;
  esp_err_t err = ledc_timer_config(&ledc_timer);
  if (err != ESP_OK) {
    Serial.println("Error when calling ledc_timer_config!");
  }  

  #if (OMOTE_HARDWARE_REV == 1)
  // Slowly charge the VSW voltage to prevent a brownout
  // Workaround for hardware rev 1!
  Serial.println("Will slowly charge VSW voltage to prevent that screen is completely bright, with no content");
  for(int i = 0; i < 100; i++) {
    digitalWrite(LCD_EN_GPIO, HIGH);  // LCD Logic off
    delayMicroseconds(1);
    digitalWrite(LCD_EN_GPIO, LOW);   // LCD Logic on
  }
  #else
  Serial.println("Will immediately charge VSW voltage. If screen is completely bright, with no content, then this is the reason.");
  digitalWrite(LCD_EN_GPIO, LOW);
  #endif

  // https://github.com/CoretechR/OMOTE/issues/70#issuecomment-2016763291
  // The original 5 ms here was sized for the LCD driver only. But LCD_EN also
  // powers the FT6206 touch controller, and enterSleep() cuts that same rail -
  // so the touch controller is power-cycled on every sleep/wake.
  //
  // A FocalTech FT6x06 needs roughly 300 ms after power-up to boot and settle
  // its capacitive baseline. At 5 ms we were initialising it mid-boot and then
  // immediately driving the panel, so it established its zero-reference while
  // LCD DMA noise was already present - and, because waking is triggered by
  // picking the remote up, usually while a hand was still near the glass. A
  // baseline captured under those conditions is wrong, and the sensor stays
  // hypersensitive afterwards, which is what produces the bursts of phantom
  // touches shortly after a wake.
  //
  // Waiting here keeps the panel quiet (nothing drives the data lines until
  // tft.init() below), giving the controller a clean window to calibrate in.
  // Costs ~300 ms of wake latency, against a wake that already takes seconds.
  delay(TOUCH_POWER_ON_SETTLE_MS);

  // tft.init() can fail - most often after an esptool flash, where the chip
  // soft-resets and the panel is left in a state the init sequence does not
  // recover from. The symptom is a white screen that a manual reset clears.
  // The original code discarded this return value, so nothing noticed.
  // This panel has no reset line (cfg.pin_rst = -1), so the only way to recover
  // is to power-cycle it via LCD_EN and try again.
  bool lcdReady = tft.init();
  for (uint8_t attempt = 1; !lcdReady && attempt <= 3; attempt++) {
    Serial.printf("tft.init() failed - power cycling the panel and retrying (%u/3)\r\n", attempt);
    digitalWrite(LCD_EN_GPIO, HIGH);          // rail (LCD + touch controller) off
    delay(120);                               // let it fully discharge
    digitalWrite(LCD_EN_GPIO, LOW);           // rail back on
    delay(TOUCH_POWER_ON_SETTLE_MS);          // same quiet settle window as above
    lcdReady = tft.init();
  }
  if (!lcdReady) {
    Serial.println("WARNING: LCD did not initialise after 3 attempts - display may be blank");
  }

  tft.initDMA();
  tft.fillScreen(TFT_BLACK);
  // NOTE: do not pair this with LV_COLOR_16_SWAP=1 to try to skip the swap.
  // Tried 2026-08-02: the display came up as garbled, corrupted pixels. The
  // LovyanGFX Bus_Parallel8 path on this panel needs the swap done here.
  tft.setSwapBytes(true);
}

void update_backlightBrightness_HAL(void) {
  // A variable declared static inside a function is visible only inside that function, exists only once (not created/destroyed for each call) and is permanent. It is in a sense a private global variable.
  static int fadeInTimer = millis(); // fadeInTimer = time after setup
  if (millis() < fadeInTimer + backlightBrightness) {
    // after boot or wakeup, fade in backlight brightness
    // fade in lasts for <backlightBrightness> ms
    ledcWrite(LEDC_CHANNEL_5, millis() - fadeInTimer);
  } else {
    if (millis() - get_lastActivityTimestamp() > get_sleepTimeout_HAL() - 2000) {
      // less than 2000 ms until standby
      // dim backlight
      ledcWrite(LEDC_CHANNEL_5, get_backlightBrightness_HAL() * 0.3);
    } else {
      // normal mode, set full backlightBrightness
      // turn off PWM if backlight is at full brightness
      if(backlightBrightness < 255){
        ledcWrite(LEDC_CHANNEL_5, backlightBrightness);
      }
      else{
        ledc_stop(LEDC_SPEED_MODE, LEDC_CHANNEL_5, 255);
      }
    }
  }
}

uint8_t get_backlightBrightness_HAL() {
  return backlightBrightness;
};
void set_backlightBrightness_HAL(uint8_t aBacklightBrightness) {
  backlightBrightness = aBacklightBrightness;
};

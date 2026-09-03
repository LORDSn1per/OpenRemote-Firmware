#include "phantomTouchDiag_hal_esp32.h"

#if defined(PHANTOM_TOUCH_SOAK)

#include <Arduino.h>
#include <lvgl.h>
#include "tft_hal_esp32.h"     // pulls in LovyanGFX -> lgfx::i2c
#include "sleep_hal_esp32.h"

// Counters from the rejection filter in lvgl_hal_esp32.cpp, so a soak run can
// show how many of the ghosts it logged actually reached LVGL.
extern uint32_t phantomFilter_rawTouches;
extern uint32_t phantomFilter_suppressedTouches;

// The touch controller, as configured in LGFX::LGFX() in tft_hal_esp32.cpp.
static const int      FT_I2C_PORT = 0;
static const int      FT_I2C_ADDR = 0x38;
static const uint32_t FT_I2C_FREQ = 400000;

// FT5x06 register block. 0x00..0x0E covers the header plus two touch points:
//   0x00 DEVICE_MODE  0x01 GEST_ID   0x02 TD_STATUS (low nibble = point count)
//   0x03 P1_XH (7:6 event flag, 3:0 X high)   0x04 P1_XL
//   0x05 P1_YH (7:4 touch id,   3:0 Y high)   0x06 P1_YL
//   0x07 P1_WEIGHT    0x08 P1_MISC (7:4 area)
//   0x09..0x0E = same layout for point 2
static const uint8_t FT_BLOCK_START = 0x00;
static const uint8_t FT_BLOCK_LEN   = 15;

// Event flag values from bits 7:6 of the XH register.
static const char* eventFlagName(uint8_t xh) {
  switch ((xh >> 6) & 0x03) {
    case 0:  return "DOWN";
    case 1:  return "UP";
    case 2:  return "CONTACT";
    default: return "NONE";   // 3 = "no event" - a finger should never report this
  }
}

// --- experiment phases -------------------------------------------------------
// The strongest surviving hypothesis is that LCD_CAM + GDMA traffic is what
// pushes the touch controller over threshold. So rather than soak in one fixed
// condition, alternate between hammering the display with full-screen redraws
// and leaving it completely static, and count ghosts in each. If the ghost rate
// tracks the phase, DMA is implicated directly and measurably.
#if defined(PHANTOM_TOUCH_SWEEP)
static const uint32_t PHASE_MS = 180000;   // 3 minutes per threshold
#else
static const uint32_t PHASE_MS = 300000;   // 5 minutes per phase
#endif
static bool     dmaStressPhase = true;
static uint32_t phaseStartedMs = 0;
static uint32_t phaseGhosts    = 0;
static uint32_t phasePolls     = 0;

#if defined(PHANTOM_TOUCH_SWEEP)
// --- threshold sweep mode ----------------------------------------------------
// The FT6206's ID_G_THGROUP register sets how strong a capacitance delta has to
// be before the controller calls it a touch. Noise-induced ghosts are, by
// definition, weaker than a real finger - so there should be a threshold that
// sits above the noise floor but below a fingertip. This mode holds the display
// under constant DMA load (worst case) and steps the threshold, so the ghost
// rate at each setting can be measured rather than guessed.
static const uint8_t THGROUP_REG = 0x80;
static uint8_t originalThgroup = 0;
static const uint8_t sweepValues[] = {0x00, 0x30, 0x50, 0x70, 0x90, 0xB0, 0xD0};
static const uint8_t SWEEP_COUNT = sizeof(sweepValues) / sizeof(sweepValues[0]);
static uint8_t sweepIndex = 0;

// sweepValues[0] == 0x00 is a sentinel meaning "leave the factory default".
static uint8_t currentThreshold(void) {
  return sweepValues[sweepIndex] == 0x00 ? originalThgroup : sweepValues[sweepIndex];
}

static void applyThreshold(void) {
  uint8_t want = currentThreshold();
  bool wrote = lgfx::i2c::writeRegister8(FT_I2C_PORT, FT_I2C_ADDR, THGROUP_REG,
                                         want, 0, FT_I2C_FREQ).has_value();
  uint8_t readback = 0xEE;
  uint8_t reg = THGROUP_REG;
  lgfx::i2c::transactionWriteRead(FT_I2C_PORT, FT_I2C_ADDR, &reg, 1,
                                  &readback, 1, FT_I2C_FREQ);
  Serial.printf("PTD,THRESH,%lu,TH%02X,set=%02X wrote=%d readback=%02X\r\n",
                (unsigned long)millis(), want, want, wrote ? 1 : 0, readback);
}
#endif

// Log field describing the current experiment condition.
static const char* phaseLabel(void) {
#if defined(PHANTOM_TOUCH_SWEEP)
  static char buf[8];
  snprintf(buf, sizeof(buf), "TH%02X", currentThreshold());
  return buf;
#else
  return dmaStressPhase ? "DMA" : "IDLE";
#endif
}

// --- counters ----------------------------------------------------------------
static uint32_t totalPolls     = 0;
static uint32_t totalTouchRuns = 0;
static uint32_t totalI2cErrors = 0;
static uint32_t lastHeartbeatMs = 0;
static const uint32_t HEARTBEAT_MS = 60000;

// --- current touch run state -------------------------------------------------
static bool     runActive   = false;
static uint32_t runStartMs  = 0;
static uint32_t runPolls    = 0;
static uint16_t runMinX, runMaxX, runMinY, runMaxY;
static uint8_t  runMinW, runMaxW;

static void logRegisterBlock(const char* tag, const uint8_t* b) {
  Serial.printf("PTD,%s,%lu,%s,", tag, (unsigned long)millis(), phaseLabel());
  for (uint8_t i = 0; i < FT_BLOCK_LEN; i++) { Serial.printf("%02X", b[i]); }
  uint16_t x = ((b[3] & 0x0F) << 8) | b[4];
  uint16_t y = ((b[5] & 0x0F) << 8) | b[6];
  Serial.printf(",pts=%u,gest=%02X,ev=%s,id=%u,x=%u,y=%u,weight=%u,area=%u\r\n",
                b[2] & 0x0F, b[1], eventFlagName(b[3]), b[5] >> 4,
                x, y, b[7], b[8] >> 4);
}

void phantomDiag_init(void) {
  phaseStartedMs  = millis();
  lastHeartbeatMs = millis();
  Serial.println("PTD,BOOT,0,-,phantom touch soak build - every touch logged below is a GHOST if nobody is touching the screen");
  Serial.println("PTD,LEGEND,0,-,START/END=touch run, HB=heartbeat, PHASE=dma stress toggled, ERR=i2c failure");

#if defined(PHANTOM_TOUCH_SWEEP)
  // Capture the factory threshold so it can be used as the sweep's control
  // point and restored by a power cycle.
  uint8_t reg = THGROUP_REG;
  if (lgfx::i2c::transactionWriteRead(FT_I2C_PORT, FT_I2C_ADDR, &reg, 1,
                                      &originalThgroup, 1, FT_I2C_FREQ).has_value()) {
    Serial.printf("PTD,THDEFAULT,0,-,factory ID_G_THGROUP=0x%02X\r\n", originalThgroup);
  } else {
    originalThgroup = 0x16;   // documented FT6x06 default, used only if the read fails
    Serial.println("PTD,THDEFAULT,0,-,could not read ID_G_THGROUP, assuming 0x16");
  }
  applyThreshold();
#endif
}

void phantomDiag_poll(void) {
#if !defined(PHANTOM_TOUCH_FIELD)
  // Never let the remote sleep during the soak, or the experiment ends early.
  // NOT done in field mode: sleep/wake is precisely the path under suspicion,
  // and suppressing it is what hid this bug through every earlier test.
  setLastActivityTimestamp_HAL();
#endif

  uint32_t now = millis();
  totalPolls++;
  phasePolls++;

#if !defined(PHANTOM_TOUCH_FIELD)
  // --- phase switching -------------------------------------------------------
  if (now - phaseStartedMs >= PHASE_MS) {
    Serial.printf("PTD,PHASE,%lu,%s,ended: ghosts=%lu polls=%lu\r\n",
                  (unsigned long)now, phaseLabel(),
                  (unsigned long)phaseGhosts, (unsigned long)phasePolls);
#if defined(PHANTOM_TOUCH_SWEEP)
    // Keep the display under constant DMA load and step to the next threshold,
    // so the only variable between phases is the threshold itself.
    dmaStressPhase = true;
    sweepIndex = (sweepIndex + 1) % SWEEP_COUNT;
    applyThreshold();
#else
    dmaStressPhase = !dmaStressPhase;
#endif
    phaseStartedMs = now;
    phaseGhosts    = 0;
    phasePolls     = 0;
  }

  // --- DMA stress ------------------------------------------------------------
  // Force a full-screen repaint every cycle. This is the heaviest sustained
  // LCD_CAM/GDMA load the panel can be put under, which is exactly the
  // condition the ghost touches are suspected to need.
  if (dmaStressPhase) {
    lv_obj_invalidate(lv_scr_act());
  }
#endif  // !PHANTOM_TOUCH_FIELD

  // --- read the raw register block ------------------------------------------
  uint8_t reg = FT_BLOCK_START;
  uint8_t b[FT_BLOCK_LEN] = {0};
  bool ok = lgfx::i2c::transactionWriteRead(FT_I2C_PORT, FT_I2C_ADDR, &reg, 1,
                                            b, FT_BLOCK_LEN, FT_I2C_FREQ).has_value();
  if (!ok) {
    totalI2cErrors++;
    // Bus-level failures are themselves a finding - a rail glitch that upsets
    // the controller would plausibly show up here first.
    Serial.printf("PTD,ERR,%lu,%s,i2c read failed (total=%lu)\r\n",
                  (unsigned long)now, phaseLabel(),
                  (unsigned long)totalI2cErrors);
    return;
  }

  uint8_t points = b[2] & 0x0F;
  uint16_t x = ((b[3] & 0x0F) << 8) | b[4];
  uint16_t y = ((b[5] & 0x0F) << 8) | b[6];
  uint8_t  w = b[7];

  if (points > 0) {
    if (!runActive) {
      // A touch just began. Log the complete chip state at the instant it
      // appeared - this is the moment a ghost is born.
      runActive  = true;
      runStartMs = now;
      runPolls   = 0;
      runMinX = runMaxX = x;
      runMinY = runMaxY = y;
      runMinW = runMaxW = w;
      totalTouchRuns++;
      phaseGhosts++;
      logRegisterBlock("START", b);
    }
    runPolls++;
    if (x < runMinX) runMinX = x;
    if (x > runMaxX) runMaxX = x;
    if (y < runMinY) runMinY = y;
    if (y > runMaxY) runMaxY = y;
    if (w < runMinW) runMinW = w;
    if (w > runMaxW) runMaxW = w;
  } else if (runActive) {
    // Touch released. Duration and coordinate spread separate a real finger
    // (tens/hundreds of ms, jittery, meaningful weight) from a single-sample
    // electrical glitch.
    runActive = false;
    Serial.printf("PTD,END,%lu,%s,dur=%lums,polls=%lu,x=%u..%u,y=%u..%u,weight=%u..%u\r\n",
                  (unsigned long)now, phaseLabel(),
                  (unsigned long)(now - runStartMs), (unsigned long)runPolls,
                  runMinX, runMaxX, runMinY, runMaxY, runMinW, runMaxW);
  }

  // --- heartbeat -------------------------------------------------------------
  if (now - lastHeartbeatMs >= HEARTBEAT_MS) {
    lastHeartbeatMs = now;
    // Read back the controller's configuration registers. These are constant in
    // normal operation, so if the chip ever brown-outs and resets (the rail
    // transient theory), the values here should change or the read should fail.
    uint8_t cfgReg = 0xA3;
    uint8_t cfg[6] = {0};
    bool cfgOk = lgfx::i2c::transactionWriteRead(FT_I2C_PORT, FT_I2C_ADDR, &cfgReg, 1,
                                                 cfg, 6, FT_I2C_FREQ).has_value();
    unsigned long rawT = (unsigned long)phantomFilter_rawTouches;
    unsigned long supT = (unsigned long)phantomFilter_suppressedTouches;
    Serial.printf("PTD,HB,%lu,%s,polls=%lu,touchRuns=%lu,i2cErr=%lu,cfgOk=%d,"
                  "cipher=%02X,intmode=%02X,power=%02X,vendor=%02X,"
                  "filtRaw=%lu,filtSuppressed=%lu\r\n",
                  (unsigned long)now, phaseLabel(),
                  (unsigned long)totalPolls, (unsigned long)totalTouchRuns,
                  (unsigned long)totalI2cErrors, cfgOk ? 1 : 0,
                  cfg[0], cfg[1], cfg[2], cfg[5], rawT, supT);
  }
}

#endif

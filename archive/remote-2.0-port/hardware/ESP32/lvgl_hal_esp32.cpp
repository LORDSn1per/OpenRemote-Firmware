#include <lvgl.h>
#include "tft_hal_esp32.h"
#include "sleep_hal_esp32.h"
#include "applicationInternal/debugOverlay.h"
#include "phantomTouchDiag_hal_esp32.h"

// -----------------------
// https://docs.lvgl.io/8.3/porting/display.html?highlight=lv_disp_draw_buf_init#buffering-modes
// With two buffers, the rendering and refreshing of the display become parallel operations
// Second buffer needs 15.360 bytes more memory in heap.
#define useTwoBuffersForlvgl

// ---------------------------------------------------------------------------
// Phantom-touch rejection filter
//
// Measured on this hardware (OMOTE Rev5 / ESP32-S3 / ILI9341 8-bit parallel
// driven by LCD_CAM + GDMA), logging the FT6206's raw register block directly:
//
//   display held under continuous DMA load : 288 spurious touches / 7287 polls
//   display completely static              :   0 spurious touches / 9960 polls
//
// So the ghosts are caused by the parallel bus switching coupling into the
// touch sensor while the panel is being redrawn. They are NOT I2C corruption:
// LovyanGFX's Touch_FT5x06 already re-reads and compares before returning, and
// the bogus coordinates survive that check because the controller itself
// genuinely believes it is being touched.
//
// The captured ghosts have two properties that make them filterable:
//   * ~73% exist for exactly one poll and are gone by the next one.
//   * They only ever appear while the panel is actively being redrawn.
//   * Their reported position is structured, not random: X sits in a narrow
//     band around the middle of the sense axis (mean 106, stdev 5) while Y is
//     spread across the whole panel - the classic signature of noise coupling
//     into every sense line at once, with the drive line being scanned at that
//     instant deciding the reported Y.
//
// Hence: if the screen has been quiet, trust a touch immediately, so normal
// tapping keeps its usual latency. Only inside the window where ghosts can
// actually occur - shortly after a panel flush - require a second, spatially
// consistent sample before believing it.
//
// Set PHANTOM_TOUCH_FILTER=0 in platformio.ini to compile this out.
// ---------------------------------------------------------------------------
#ifndef PHANTOM_TOUCH_FILTER
#define PHANTOM_TOUCH_FILTER 1
#endif

// How long after a panel flush the display counts as "still busy". A
// full-screen LVGL redraw arrives as a burst of flushes, so this needs to span
// the gaps between them.
static const uint32_t TOUCH_DMA_BUSY_MS = 150;
// Max pixels a genuine finger can move between two consecutive polls (~45 ms
// apart) and still be considered the same, real contact.
static const int16_t  TOUCH_MAX_JUMP_PX = 30;

static uint32_t lastPanelFlushMs = 0;

#if PHANTOM_TOUCH_FILTER
static bool     touchEstablished  = false;  // a contact we have already accepted
static bool     havePendingSample = false;  // one unconfirmed sample is held
static uint16_t pendingX = 0, pendingY = 0;
#endif
// Defined unconditionally so the soak build can always link against them; they
// simply stay at zero when the filter is compiled out. (PHANTOM_TOUCH_FILTER is
// local to this translation unit, so it must not gate the symbols themselves.)
uint32_t phantomFilter_rawTouches        = 0;
uint32_t phantomFilter_suppressedTouches = 0;

// Display flushing
void my_disp_flush( lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p ){
  uint32_t w = ( area->x2 - area->x1 + 1 );
  uint32_t h = ( area->y2 - area->y1 + 1 );

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  #ifdef useTwoBuffersForlvgl
  tft.pushPixelsDMA((uint16_t*)&color_p->full, w * h);
  #else
  tft.pushColors((uint16_t*)&color_p->full, w * h, true);
  #endif
  tft.endWrite();

  lastPanelFlushMs = millis();
  // LVGL calls this once per flushed AREA, not once per frame - a full redraw
  // arrives as LVGL_DRAW_BUF_FRACTION separate calls. Only
  // count the final part of a refresh, otherwise the "FPS" readout is really a
  // flush rate and reads ~10x too high.
  if (lv_disp_flush_is_last(disp)) {
    debugOverlay_count_frame();
  }
  lv_disp_flush_ready( disp );
}

// Read the touchpad
void my_touchpad_read(lv_indev_drv_t * indev_driver, lv_indev_data_t * data) {
    uint16_t x, y;
    #if defined(PHANTOM_TOUCH_SOAK)
    // Read and log the raw controller state before LovyanGFX filters it.
    phantomDiag_poll();
    #endif
    if (tft.getTouch(&x, &y)) {
        #if PHANTOM_TOUCH_FILTER
        phantomFilter_rawTouches++;
        if (!touchEstablished) {
            bool panelBusy = (millis() - lastPanelFlushMs) < TOUCH_DMA_BUSY_MS;
            if (!panelBusy) {
                // Screen has been quiet, so no mechanism for a ghost here.
                // Accept straight away and keep tapping latency unchanged.
                touchEstablished = true;
            } else if (havePendingSample &&
                       abs((int)x - (int)pendingX) <= TOUCH_MAX_JUMP_PX &&
                       abs((int)y - (int)pendingY) <= TOUCH_MAX_JUMP_PX) {
                // Second consecutive sample in the same place while the panel
                // is busy - a real finger, not a one-poll glitch.
                touchEstablished = true;
            } else {
                // First (or an incoherent) sample during a redraw. Hold it back
                // and see whether it is still there on the next poll.
                havePendingSample = true;
                pendingX = x;
                pendingY = y;
                phantomFilter_suppressedTouches++;
                data->state = LV_INDEV_STATE_REL;
                debugOverlay_report_touch(0, 0, false);
                return;
            }
        }
        havePendingSample = false;
        #endif
        data->state = LV_INDEV_STATE_PR;
        data->point.x = x;
        data->point.y = y;
        setLastActivityTimestamp_HAL();
        debugOverlay_report_touch(x, y, true);

        // Uncomment this to show the touchpoint
        //tft.drawFastHLine(0, y, SCR_WIDTH, TFT_RED);
        //tft.drawFastVLine(x, 0, SCR_HEIGHT, TFT_RED);
    } else {
        #if PHANTOM_TOUCH_FILTER
        touchEstablished  = false;
        havePendingSample = false;
        #endif
        data->state = LV_INDEV_STATE_REL;
        debugOverlay_report_touch(0, 0, false);
    }
}

static lv_disp_draw_buf_t draw_buf;

// Draw buffer size as a fraction of the screen. Each flushed chunk costs a
// setAddrWindow + startWrite/endWrite + DMA setup, so fewer, larger chunks mean
// less fixed overhead per frame: at 1/10 a full redraw took 10 flush calls.
// Buffers come from internal DRAM (plain malloc), which is much faster for the
// renderer to write into than PSRAM - keep it that way.
#define LVGL_DRAW_BUF_FRACTION 5

void init_lvgl_HAL() {
  // first init TFT
  init_tft();

  // Larger buffers are faster but come out of internal DRAM, which WiFi and BLE
  // also draw on. Fall back to the old 1/10 size rather than handing LVGL a
  // NULL buffer, which would leave the screen blank with no obvious cause.
  uint32_t bufPixels = SCR_WIDTH * SCR_HEIGHT / LVGL_DRAW_BUF_FRACTION;
  lv_color_t * bufA = (lv_color_t *) malloc(sizeof(lv_color_t) * bufPixels);
  #ifdef useTwoBuffersForlvgl
  lv_color_t * bufB = (lv_color_t *) malloc(sizeof(lv_color_t) * bufPixels);
  #else
  lv_color_t * bufB = NULL;
  #endif

  #ifdef useTwoBuffersForlvgl
  bool allocOk = (bufA != NULL) && (bufB != NULL);
  #else
  bool allocOk = (bufA != NULL);
  #endif
  if (!allocOk) {
    Serial.printf("LVGL draw buffers of %lu px did not fit, falling back to 1/10 screen\r\n",
                  (unsigned long)bufPixels);
    free(bufA); free(bufB);
    bufPixels = SCR_WIDTH * SCR_HEIGHT / 10;
    bufA = (lv_color_t *) malloc(sizeof(lv_color_t) * bufPixels);
    #ifdef useTwoBuffersForlvgl
    bufB = (lv_color_t *) malloc(sizeof(lv_color_t) * bufPixels);
    #endif
  }
  Serial.printf("LVGL draw buffers: %lu px each (%lu KB total)\r\n",
                (unsigned long)bufPixels,
                (unsigned long)(bufPixels * sizeof(lv_color_t) * (bufB ? 2 : 1) / 1024));
  lv_disp_draw_buf_init(&draw_buf, bufA, bufB, bufPixels);

  // Initialize the display driver --------------------------------------------------------------------------
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init( &disp_drv );
  disp_drv.hor_res = SCR_WIDTH;
  disp_drv.ver_res = SCR_HEIGHT;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register( &disp_drv );

  // Initialize the touchscreen driver
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init( &indev_drv );
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register( &indev_drv );

  #if defined(PHANTOM_TOUCH_SOAK)
  phantomDiag_init();
  #endif
}

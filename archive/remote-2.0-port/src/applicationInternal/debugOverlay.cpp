#include <lvgl.h>
#include "applicationInternal/debugOverlay.h"
#include "applicationInternal/hardware/hardwarePresenter.h"
#include "applicationInternal/gui/guiBase.h"

static bool showFPS = false;
static bool showTouchDebug = false;

static lv_obj_t* FPSLabel = nullptr;
static lv_obj_t* TouchDot = nullptr;
static lv_obj_t* TouchCoordLabel = nullptr;

static volatile uint32_t frameCount = 0;
static uint32_t lastFrameCount = 0;
static unsigned long lastFpsSampleMs = 0;

// Touch trail: a ring buffer of small orange dots left behind at the last few
// touch points, fading out over TOUCH_TRAIL_FADE_MS.
static const uint8_t TOUCH_TRAIL_POINT_COUNT = 8;
static const uint32_t TOUCH_TRAIL_FADE_MS = 3000UL;
static const uint32_t TOUCH_TRAIL_MIN_SPACING_MS = 70UL;
struct TouchTrailPoint {
  lv_obj_t* dot;
  unsigned long createdMs;
  bool active;
};
static TouchTrailPoint touchTrail[TOUCH_TRAIL_POINT_COUNT] = {};
static uint8_t nextTouchTrailPoint = 0;
static unsigned long lastTouchTrailPointMs = 0;

static void addTouchTrailPoint(int16_t x, int16_t y, unsigned long now) {
  if (now - lastTouchTrailPointMs < TOUCH_TRAIL_MIN_SPACING_MS) { return; }
  TouchTrailPoint &point = touchTrail[nextTouchTrailPoint];
  nextTouchTrailPoint = (nextTouchTrailPoint + 1) % TOUCH_TRAIL_POINT_COUNT;
  if (point.dot == NULL || !lv_obj_is_valid(point.dot)) { return; }
  point.createdMs = now;
  point.active = true;
  lastTouchTrailPointMs = now;
  lv_obj_set_pos(point.dot, x - 3, y - 3);
  lv_obj_set_style_bg_opa(point.dot, LV_OPA_COVER, 0);
  lv_obj_clear_flag(point.dot, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(point.dot);
}

bool getShowFPS() {
  return showFPS;
}
void setShowFPS(bool aShowFPS) {
  showFPS = aShowFPS;
  lastFrameCount = frameCount;
  lastFpsSampleMs = millis();
  if (FPSLabel == NULL) { return; }
  if (showFPS) {
    lv_label_set_text(FPSLabel, "FPS 0");
    lv_obj_clear_flag(FPSLabel, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(FPSLabel, LV_OBJ_FLAG_HIDDEN);
  }
}

bool getShowTouchDebug() {
  return showTouchDebug;
}
void setShowTouchDebug(bool aShowTouchDebug) {
  showTouchDebug = aShowTouchDebug;
  if (!showTouchDebug) {
    if (TouchDot != NULL) { lv_obj_add_flag(TouchDot, LV_OBJ_FLAG_HIDDEN); }
    if (TouchCoordLabel != NULL) { lv_obj_add_flag(TouchCoordLabel, LV_OBJ_FLAG_HIDDEN); }
    for (TouchTrailPoint &point : touchTrail) {
      point.active = false;
      if (point.dot != NULL) { lv_obj_add_flag(point.dot, LV_OBJ_FLAG_HIDDEN); }
    }
  }
}

void debugOverlay_count_frame(void) {
  frameCount++;
}

void update_debugOverlay_fps(void) {
  if (!showFPS || FPSLabel == NULL) { return; }
  unsigned long now = millis();
  unsigned long elapsed = now - lastFpsSampleMs;
  if (elapsed == 0) { elapsed = 1; }
  uint32_t frames = frameCount;
  uint32_t fps = (uint32_t)(((uint64_t)(frames - lastFrameCount) * 1000UL) / elapsed);
  lastFrameCount = frames;
  lastFpsSampleMs = now;

  char text[14];
  snprintf(text, sizeof(text), "FPS %lu", (unsigned long)fps);
  lv_label_set_text(FPSLabel, text);
}

void debugOverlay_report_touch(int16_t x, int16_t y, bool pressed) {
  if (!showTouchDebug || TouchDot == NULL || TouchCoordLabel == NULL) { return; }
  if (pressed) {
    lv_obj_set_pos(TouchDot, x - 22, y - 22);
    lv_obj_clear_flag(TouchDot, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(TouchDot);

    char text[16];
    snprintf(text, sizeof(text), "T %d,%d", x, y);
    lv_label_set_text(TouchCoordLabel, text);
    lv_obj_clear_flag(TouchCoordLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(TouchCoordLabel);

    addTouchTrailPoint(x, y, millis());
  } else {
    lv_obj_add_flag(TouchDot, LV_OBJ_FLAG_HIDDEN);
  }
}

void debugOverlay_loop(void) {
  if (!showTouchDebug) { return; }
  unsigned long now = millis();
  for (TouchTrailPoint &point : touchTrail) {
    if (!point.active || point.dot == NULL || !lv_obj_is_valid(point.dot)) { continue; }
    uint32_t age = now - point.createdMs;
    if (age >= TOUCH_TRAIL_FADE_MS) {
      point.active = false;
      lv_obj_add_flag(point.dot, LV_OBJ_FLAG_HIDDEN);
      continue;
    }
    lv_opa_t opacity = (lv_opa_t)lv_map(age, 0, TOUCH_TRAIL_FADE_MS, LV_OPA_COVER, LV_OPA_10);
    lv_obj_set_style_bg_opa(point.dot, opacity, 0);
  }
}

void init_debugOverlay(void) {
  lv_obj_t* top = lv_layer_top();

  FPSLabel = lv_label_create(top);
  lv_label_set_text(FPSLabel, "FPS 0");
  lv_obj_set_style_text_font(FPSLabel, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(FPSLabel, lv_color_hex(0xFF5BE1), 0);
  // placed just below the status bar (battery/wifi/scene row) so it doesn't overlap it
  lv_obj_align(FPSLabel, LV_ALIGN_TOP_RIGHT, -2, tabviewTop + 2);
  lv_obj_clear_flag(FPSLabel, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(FPSLabel, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(FPSLabel, LV_OBJ_FLAG_HIDDEN);

  TouchCoordLabel = lv_label_create(top);
  lv_label_set_text(TouchCoordLabel, "T 0,0");
  lv_obj_set_style_text_font(TouchCoordLabel, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(TouchCoordLabel, lv_color_hex(0xFF3C45), 0);
  lv_obj_align(TouchCoordLabel, LV_ALIGN_BOTTOM_RIGHT, -2, -2);
  lv_obj_clear_flag(TouchCoordLabel, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(TouchCoordLabel, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(TouchCoordLabel, LV_OBJ_FLAG_HIDDEN);

  TouchDot = lv_obj_create(top);
  lv_obj_remove_style_all(TouchDot);
  lv_obj_set_size(TouchDot, 44, 44);
  lv_obj_set_style_radius(TouchDot, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_opa(TouchDot, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_color(TouchDot, lv_color_hex(0xFF2028), 0);
  lv_obj_set_style_border_width(TouchDot, 2, 0);
  lv_obj_clear_flag(TouchDot, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(TouchDot, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(TouchDot, LV_OBJ_FLAG_HIDDEN);

  for (TouchTrailPoint &point : touchTrail) {
    point.dot = lv_obj_create(top);
    lv_obj_remove_style_all(point.dot);
    lv_obj_set_size(point.dot, 6, 6);
    lv_obj_set_style_radius(point.dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(point.dot, lv_color_hex(0xFF9D2E), 0);
    lv_obj_set_style_bg_opa(point.dot, LV_OPA_COVER, 0);
    lv_obj_clear_flag(point.dot, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(point.dot, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(point.dot, LV_OBJ_FLAG_HIDDEN);
    point.active = false;
  }
}

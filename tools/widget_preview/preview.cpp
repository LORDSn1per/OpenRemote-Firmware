/*
  Renders the remote's widgets on a desktop with the firmware's own LVGL
  version and drawing code, so a design can be approved before flashing.

    tools/widget_preview/build.sh      -> PNGs in tools/widget_preview/out

  The power graph comes from remote/power_widget.h unchanged. The history is
  synthetic: a night in deep sleep, a day of use with BLE idle between, a
  charge, and a busy last few minutes. The battery face mirrors
  buildWidgetBatteryFace(); the battery column beside power in a custom
  widget is a close approximation of buildWidgetCell().
*/
#include <lvgl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "../../remote/power_widget.h"

static const int W = 240;
static const int H = 320;
static lv_color_t frame[W * H];
static lv_color_t drawBuffer[W * H];

static void flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *pixels) {
  for (int y = area->y1; y <= area->y2; y++) {
    for (int x = area->x1; x <= area->x2; x++) frame[y * W + x] = *pixels++;
  }
  lv_disp_flush_ready(drv);
}

static void savePng(const char *name) {
  const int scale = 2;
  char bmp[256];
  snprintf(bmp, sizeof(bmp), "out/%s.bmp", name);
  FILE *f = fopen(bmp, "wb");
  int ow = W * scale, oh = H * scale;
  int row = ow * 3;
  int size = 54 + row * oh;
  unsigned char header[54] = {'B', 'M'};
  memcpy(header + 2, &size, 4);
  int offset = 54, info = 40, planes = 1, bpp = 24;
  memcpy(header + 10, &offset, 4);
  memcpy(header + 14, &info, 4);
  memcpy(header + 18, &ow, 4);
  memcpy(header + 22, &oh, 4);
  memcpy(header + 26, &planes, 2);
  memcpy(header + 28, &bpp, 2);
  fwrite(header, 1, 54, f);
  std::vector<unsigned char> line(row);
  for (int y = oh - 1; y >= 0; y--) {
    for (int x = 0; x < ow; x++) {
      lv_color_t c = frame[(y / scale) * W + x / scale];
      uint32_t c32 = lv_color_to32(c);
      line[x * 3 + 0] = c32 & 0xFF;
      line[x * 3 + 1] = (c32 >> 8) & 0xFF;
      line[x * 3 + 2] = (c32 >> 16) & 0xFF;
    }
    fwrite(line.data(), 1, row, f);
  }
  fclose(f);
  char cmd[512];
  snprintf(cmd, sizeof(cmd), "sips -s format png out/%s.bmp --out out/%s.png >/dev/null && rm out/%s.bmp",
           name, name, name);
  system(cmd);
}

// ------------------------------------------------------- synthetic history
static std::vector<PowerPoint> history;
static uint32_t nowEpoch = 1793300000UL;
static uint32_t rng = 12345;
static float jitter(float spread) {
  rng = rng * 1103515245u + 12345u;
  return ((rng >> 8) % 1000 / 1000.0f - 0.5f) * 2.0f * spread;
}
static void add(uint32_t t, float mA, uint8_t cat, bool charging) {
  history.push_back({t, (uint16_t)(mA * 10.0f), cat, (uint8_t)charging});
}
// Fills [from, to) with one state, a sample every `step` seconds.
static void fill(uint32_t from, uint32_t to, float mA, float spread, uint8_t cat,
                 bool charging, uint32_t step) {
  for (uint32_t t = from; t < to; t += step) {
    float v = mA + jitter(spread);
    add(t, v < 0.05f ? 0.05f : v, cat, charging);
  }
}
static void makeHistory() {
  uint32_t n = nowEpoch;
  uint32_t t = n - 13 * 3600;
  fill(t, n - 34200, 0.25f, 0.0f, POWER_CAT_DEEP, false, 3600);           // overnight
  t = n - 34200;
  uint32_t charge0 = n - 4 * 3600, charge1 = n - 9000;
  int burst = 0;
  while (t < n - 720) {
    bool chg = t >= charge0 && t < charge1;
    uint32_t use = 60 + (burst * 97) % 180;
    fill(t, t + use, 118.0f, 14.0f, POWER_CAT_IN_USE, chg, 5);
    t += use;
    if (t >= n - 6 * 3600 && t < n - 6 * 3600 + 1800) {     // a spell of plain light sleep
      fill(t, t + 1800, 2.0f, 0.1f, POWER_CAT_LIGHT, chg, 300);
      t += 1800;
    }
    uint32_t idle = 900 + (burst * 523) % 2400;
    if (t + idle > n - 720) idle = n - 720 - t;
    fill(t, t + idle, 5.6f, 1.2f, POWER_CAT_LIGHT, chg, 5);
    t += idle;
    burst++;
  }
  // The last twelve minutes, second by second.
  fill(t, t + 180, 120.0f, 18.0f, POWER_CAT_IN_USE, false, 1); t += 180;
  fill(t, t + 25, 185.0f, 10.0f, POWER_CAT_IN_USE, false, 1); t += 25;    // Wi-Fi up
  fill(t, t + 215, 5.5f, 1.4f, POWER_CAT_LIGHT, false, 5); t += 215;
  fill(t, t + 50, 112.0f, 12.0f, POWER_CAT_IN_USE, false, 1); t += 50;
  fill(t, t + 160, 5.8f, 1.4f, POWER_CAT_LIGHT, false, 5); t += 160;
  fill(t, n + 1, 108.0f, 16.0f, POWER_CAT_IN_USE, false, 1);
}
static bool historyAt(uint32_t index, PowerPoint &out) {
  if (index >= history.size()) return false;
  out = history[index];
  return true;
}
static PowerSeries historySource() {
  return PowerSeries{(uint32_t)history.size(), historyAt, nowEpoch};
}

// ------------------------------------------------------------- page chrome
static lv_obj_t *screen() {
  lv_obj_t *scr = lv_obj_create(nullptr);
  lv_obj_set_style_bg_color(scr, lv_color_make(14, 30, 52), 0);
  lv_obj_set_style_bg_grad_color(scr, lv_color_make(6, 12, 22), 0);
  lv_obj_set_style_bg_grad_dir(scr, LV_GRAD_DIR_VER, 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t *title = lv_label_create(scr);
  lv_label_set_text(title, "Living Room");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
  lv_obj_set_style_text_color(title, lv_color_white(), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12);
  lv_obj_t *clock = lv_label_create(scr);
  lv_label_set_text(clock, "9:41");
  lv_obj_set_style_text_font(clock, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(clock, lv_color_make(150, 165, 182), 0);
  lv_obj_set_pos(clock, 10, 14);
  lv_scr_load(scr);
  return scr;
}

// styleWidgetContainer() with no wallpaper: the theme's glass.
static lv_obj_t *card(lv_obj_t *parent, int y, int h) {
  lv_obj_t *c = lv_obj_create(parent);
  lv_obj_remove_style_all(c);
  lv_obj_set_pos(c, 8, y);
  lv_obj_set_size(c, 224, h);
  lv_obj_set_style_radius(c, 12, 0);
  lv_obj_set_style_bg_color(c, lv_color_make(30, 42, 58), 0);
  lv_obj_set_style_bg_opa(c, (lv_opa_t)150, 0);
  lv_obj_set_style_border_color(c, lv_color_white(), 0);
  lv_obj_set_style_border_opa(c, LV_OPA_20, 0);
  lv_obj_set_style_border_width(c, 1, 0);
  lv_obj_set_style_clip_corner(c, true, 0);
  lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);
  return c;
}

static lv_obj_t *overlay(lv_obj_t *parent) {
  lv_obj_t *o = lv_obj_create(parent);
  lv_obj_remove_style_all(o);
  lv_obj_set_pos(o, 6, 42);
  lv_obj_set_size(o, 228, 268);
  lv_obj_set_style_radius(o, 14, 0);
  lv_obj_set_style_bg_color(o, lv_color_make(24, 43, 62), 0);
  lv_obj_set_style_bg_grad_color(o, lv_color_make(8, 18, 31), 0);
  lv_obj_set_style_bg_grad_dir(o, LV_GRAD_DIR_VER, 0);
  lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(o, lv_color_white(), 0);
  lv_obj_set_style_border_opa(o, LV_OPA_20, 0);
  lv_obj_set_style_border_width(o, 1, 0);
  lv_obj_set_style_clip_corner(o, true, 0);
  lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE);
  return o;
}

static void tapToClose(lv_obj_t *o) {
  powerLabel(o, "Tap to close", 0, 268 - 24, &lv_font_montserrat_10, powerMuted(), 228,
             LV_TEXT_ALIGN_CENTER);
}

static void divider(lv_obj_t *parent, int x, int y, int w, int h) {
  lv_obj_t *d = lv_obj_create(parent);
  lv_obj_remove_style_all(d);
  lv_obj_set_pos(d, x, y);
  lv_obj_set_size(d, w, h);
  lv_obj_set_style_bg_color(d, lv_color_white(), 0);
  lv_obj_set_style_bg_opa(d, LV_OPA_20, 0);
}

static lv_obj_t *box(lv_obj_t *parent, int x, int y, int w, int h) {
  lv_obj_t *b = lv_obj_create(parent);
  lv_obj_remove_style_all(b);
  lv_obj_set_pos(b, x, y);
  lv_obj_set_size(b, w, h);
  lv_obj_clear_flag(b, LV_OBJ_FLAG_SCROLLABLE);
  return b;
}

// --------------------------------------------------------- battery faces
static const int LEVEL = 73;

// A small cell glyph and a value, as buildWidgetCell() draws a battery column.
static void batteryCell(lv_obj_t *cell, int w, int h, bool slim) {
  lv_obj_set_flex_flow(cell, slim ? LV_FLEX_FLOW_ROW : LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(cell, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(cell, 2, 0);
  lv_obj_set_style_pad_column(cell, 6, 0);
  int gw = slim ? 24 : 46, gh = slim ? 12 : 22;
  lv_obj_t *g = lv_obj_create(cell);
  lv_obj_remove_style_all(g);
  lv_obj_set_size(g, gw, gh);
  lv_obj_set_style_radius(g, slim ? 3 : 5, 0);
  lv_obj_set_style_border_color(g, lv_color_white(), 0);
  lv_obj_set_style_border_opa(g, LV_OPA_40, 0);
  lv_obj_set_style_border_width(g, 2, 0);
  lv_obj_set_style_pad_all(g, 2, 0);
  lv_obj_t *f = lv_obj_create(g);
  lv_obj_remove_style_all(f);
  lv_obj_set_size(f, (gw - 8) * LEVEL / 100, gh - 8);
  lv_obj_set_style_radius(f, 2, 0);
  lv_obj_set_style_bg_color(f, lv_color_make(48, 209, 88), 0);
  lv_obj_set_style_bg_opa(f, LV_OPA_COVER, 0);
  lv_obj_t *v = lv_label_create(cell);
  lv_label_set_text(v, "73%");
  lv_obj_set_style_text_font(v, slim ? &lv_font_montserrat_16 : &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(v, lv_color_white(), 0);
  if (!slim) {
    lv_obj_t *s = lv_label_create(cell);
    lv_label_set_text(s, "Discharging");
    lv_obj_set_style_text_font(s, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(s, powerMuted(), 0);
  }
}

/*
  buildWidgetBatteryFace() with the "Until empty" statistic and the new
  percentage-inside-the-cell option.
*/
static void batteryFace(lv_obj_t *parent, int width, bool expanded) {
  int pad = expanded ? 16 : 10;
  int cellWidth = expanded ? 140 : 58;
  int cellHeight = expanded ? 62 : 28;
  int cellX = (width - cellWidth) / 2;
  int cellY = expanded ? 34 : 12;
  lv_obj_t *cell = lv_obj_create(parent);
  lv_obj_remove_style_all(cell);
  lv_obj_set_pos(cell, cellX, cellY);
  lv_obj_set_size(cell, cellWidth, cellHeight);
  lv_obj_set_style_radius(cell, expanded ? 12 : 7, 0);
  lv_obj_set_style_border_color(cell, lv_color_white(), 0);
  lv_obj_set_style_border_opa(cell, LV_OPA_40, 0);
  lv_obj_set_style_border_width(cell, 2, 0);
  lv_obj_set_style_pad_all(cell, 3, 0);
  lv_obj_clear_flag(cell, LV_OBJ_FLAG_SCROLLABLE);
  int innerWidth = cellWidth - 10;
  int innerHeight = cellHeight - 10;
  lv_obj_t *fill = lv_obj_create(cell);
  lv_obj_remove_style_all(fill);
  lv_obj_set_size(fill, (innerWidth * LEVEL) / 100, innerHeight);
  lv_obj_set_style_radius(fill, expanded ? 8 : 4, 0);
  lv_obj_set_style_bg_color(fill, lv_color_make(48, 209, 88), 0);
  // New: the level inside the cell, white over a dimmed fill so it reads
  // across both the filled and the empty part.
  lv_obj_set_style_bg_opa(fill, (lv_opa_t)150, 0);
  lv_obj_t *inside = lv_label_create(cell);
  lv_label_set_text(inside, "73%");
  lv_obj_set_style_text_font(inside, expanded ? &lv_font_montserrat_24 : &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(inside, lv_color_white(), 0);
  lv_obj_center(inside);

  lv_obj_t *nub = lv_obj_create(parent);
  lv_obj_remove_style_all(nub);
  lv_obj_set_pos(nub, cellX + cellWidth, cellY + cellHeight / 2 - (expanded ? 10 : 5));
  lv_obj_set_size(nub, expanded ? 6 : 4, expanded ? 20 : 10);
  lv_obj_set_style_radius(nub, 3, 0);
  lv_obj_set_style_bg_color(nub, lv_color_white(), 0);
  lv_obj_set_style_bg_opa(nub, LV_OPA_40, 0);

  powerLabel(parent, "1d 19h", pad, expanded ? 118 : cellY + cellHeight + 4,
             expanded ? &lv_font_montserrat_24 : &lv_font_montserrat_20, lv_color_white(),
             width - pad * 2, LV_TEXT_ALIGN_CENTER);
  powerLabel(parent, "Until empty", pad, expanded ? 186 : cellY + cellHeight + 32,
             &lv_font_montserrat_12, powerMuted(), width - pad * 2, LV_TEXT_ALIGN_CENTER);
  if (expanded) {
    powerLabel(parent, "No dock paired", pad, 214, &lv_font_montserrat_12, powerMuted(),
               width - pad * 2, LV_TEXT_ALIGN_CENTER);
  }
}

static void render(const char *name) {
  lv_obj_invalidate(lv_scr_act());
  lv_refr_now(nullptr);
  savePng(name);
}

int main() {
  makeHistory();
  powerSeriesSource = historySource;
  lv_init();
  static lv_disp_draw_buf_t buf;
  lv_disp_draw_buf_init(&buf, drawBuffer, nullptr, W * H);
  static lv_disp_drv_t drv;
  lv_disp_drv_init(&drv);
  drv.hor_res = W;
  drv.ver_res = H;
  drv.flush_cb = flush;
  drv.draw_buf = &buf;
  lv_disp_drv_register(&drv);

  // 1. A page: large power, slim power, and battery + power as a custom widget.
  {
    lv_obj_t *s = screen();
    buildPowerWidget(card(s, 52, 96), 224, 96, POWER_LAYOUT_LARGE, 3);
    buildPowerWidget(card(s, 156, 44), 224, 44, POWER_LAYOUT_SLIM, 3);
    lv_obj_t *c = card(s, 208, 96);
    batteryCell(box(c, 0, 0, 112, 96), 112, 96, false);
    divider(c, 112, 14, 1, 68);
    buildPowerWidget(box(c, 112, 0, 112, 96), 112, 96, POWER_LAYOUT_COLUMN, 3);
    render("1_page_large_slim_custom");
  }
  // 2. Slim custom (battery + power) and the battery widget with the
  //    percentage inside the cell and Until empty as the headline.
  {
    lv_obj_t *s = screen();
    lv_obj_t *c = card(s, 52, 44);
    batteryCell(box(c, 0, 0, 112, 44), 112, 44, true);
    divider(c, 112, 9, 1, 26);
    buildPowerWidget(box(c, 112, 0, 112, 44), 112, 44, POWER_LAYOUT_SLIM_CELL, 3);
    batteryFace(card(s, 104, 96), 224, false);
    buildPowerWidget(card(s, 208, 96), 224, 96, POWER_LAYOUT_LARGE, 4);
    render("2_page_slim_custom_battery");
  }
  // 3-6. Full screen at several ranges, and with the dropdown open.
  const uint8_t ranges[] = {3, 5, 0, 2};
  const char *names[] = {"3_full_1hour", "4_full_12hours", "5_full_1minute", "6_full_10minutes"};
  for (int i = 0; i < 4; i++) {
    lv_obj_t *s = screen();
    lv_obj_t *o = overlay(s);
    buildPowerWidget(o, 228, 268, POWER_LAYOUT_FULL, ranges[i]);
    tapToClose(o);
    render(names[i]);
  }
  {
    lv_obj_t *s = screen();
    lv_obj_t *o = overlay(s);
    PowerWidgetUi *ui = buildPowerWidget(o, 228, 268, POWER_LAYOUT_FULL, 3);
    tapToClose(o);
    lv_dropdown_open(ui->dropdown);
    render("7_full_dropdown_open");
  }
  // 8. The battery widget full screen with the percentage inside.
  {
    lv_obj_t *s = screen();
    lv_obj_t *o = overlay(s);
    batteryFace(o, 228, true);
    tapToClose(o);
    render("8_battery_full_percent_inside");
  }
  // 9. An expanded custom widget: battery row above a power row.
  {
    lv_obj_t *s = screen();
    lv_obj_t *o = overlay(s);
    int rowH = (268 - 6 - 28) / 2;
    lv_obj_t *top = box(o, 0, 6, 228, rowH);
    batteryCell(top, 228, rowH, false);
    divider(o, 18, 6 + rowH, 228 - 36, 1);
    buildPowerWidget(box(o, 0, 6 + rowH, 228, rowH), 228, rowH, POWER_LAYOUT_ROW, 3);
    tapToClose(o);
    render("9_custom_expanded_battery_power");
  }
  // 10. Custom colours, as WebConfig 3.47 would set them.
  {
    powerColours[POWER_CAT_IN_USE] = 0xFF2D55;
    powerColours[POWER_CAT_DEEP] = 0xFFD60A;
    powerLineColour = 0x00E5FF;
    lv_obj_t *s = screen();
    lv_obj_t *o = overlay(s);
    buildPowerWidget(o, 228, 268, POWER_LAYOUT_FULL, 5);
    tapToClose(o);
    render("10_full_custom_colours");
  }
  return 0;
}

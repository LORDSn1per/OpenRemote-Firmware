/*
  Power widget: an estimated-current graph with the remote's power states
  shaded behind it, plus the share of time and charge each state took.

  Plain LVGL 8.3 and C only, so the firmware and tools/widget_preview (which
  renders it on a desktop for design review) compile exactly the same
  drawing code. The firmware supplies the history through powerSeriesSource.

  Every figure here is an estimate. The MAX17048 measures no current; the
  firmware converts what it knows - screen, CPU clock, radio, how much of
  BLE idle the CPU spent light-sleeping - into milliamps with a fixed model.
*/
#pragma once

#include <lvgl.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

enum PowerCategory : uint8_t {
  POWER_CAT_IN_USE = 0,      // awake: screen on, or screen off but not sleeping
  POWER_CAT_LIGHT = 1,       // light sleep, including BLE-connected idle
  POWER_CAT_DEEP = 2,
  POWER_CAT_COUNT = 3,
  POWER_CAT_NONE = 0xFF
};

// One step of the history. It holds from t until the next point's t.
struct PowerPoint {
  uint32_t t;          // epoch seconds
  uint16_t mA10;       // estimated current, tenths of a milliamp
  uint8_t category;    // PowerCategory
  uint8_t charging;
};

struct PowerSeries {
  uint32_t count;
  bool (*at)(uint32_t index, PowerPoint &out);   // 0 is the oldest
  uint32_t now;                                  // epoch seconds; 0 = unknown
};

// Supplied by whoever owns the history (the firmware, or the preview tool).
static PowerSeries (*powerSeriesSource)() = nullptr;

static const uint8_t POWER_RANGE_COUNT = 9;
static const uint32_t POWER_RANGE_SECONDS[POWER_RANGE_COUNT] = {
  60, 300, 600, 3600, 6UL * 3600UL, 12UL * 3600UL,
  86400UL, 3UL * 86400UL, 7UL * 86400UL };
static const char *const POWER_RANGE_NAME[POWER_RANGE_COUNT] = {
  "1 min", "5 min", "10 min", "1 hour", "6 hours", "12 hours",
  "1 day", "3 days", "1 week" };
static const char *const POWER_RANGE_AXIS[POWER_RANGE_COUNT] = {
  "-1 min", "-5 min", "-10 min", "-1 h", "-6 h", "-12 h", "-1 day", "-3 days", "-7 days" };
static const char *const POWER_RANGE_OPTIONS =
  "1 minute\n5 minutes\n10 minutes\n1 hour\n6 hours\n12 hours\n1 day\n3 days\n1 week";
static const uint8_t POWER_RANGE_DEFAULT = 3;

static const uint16_t POWER_GRAPH_MAX_COLUMNS = 220;
// The vertical scale is logarithmic so deep sleep (a fraction of a milliamp)
// and the screen (over a hundred) are both readable on one graph.
static const float POWER_GRAPH_MIN_MA = 0.05f;
static const float POWER_GRAPH_MAX_MA = 400.0f;

/*
  The graph's colours, as 0xRRGGBB. These are the defaults; the firmware
  replaces them from WebConfig's Power widget settings, and faces built
  afterwards draw in the new ones.
*/
static const uint32_t POWER_DEFAULT_COLOURS[POWER_CAT_COUNT] = {0xFF9F0A, 0x0A84FF, 0x7C5CF0};
static const uint32_t POWER_DEFAULT_CHARGING_COLOUR = 0x30D158;
static const uint32_t POWER_DEFAULT_LINE_COLOUR = 0xFFFFFF;
static uint32_t powerColours[POWER_CAT_COUNT] = {0xFF9F0A, 0x0A84FF, 0x7C5CF0};
static uint32_t powerChargingColour = POWER_DEFAULT_CHARGING_COLOUR;
static uint32_t powerLineColour = POWER_DEFAULT_LINE_COLOUR;

static inline lv_color_t powerCategoryColour(uint8_t category) {
  return lv_color_hex(powerColours[category < POWER_CAT_COUNT ? category : POWER_CAT_DEEP]);
}

static inline const char *powerCategoryName(uint8_t category) {
  if (category == POWER_CAT_IN_USE) return "In use";
  if (category == POWER_CAT_LIGHT) return "Light sleep";
  return "Deep sleep";
}

struct PowerGraphModel {
  uint16_t columns;
  uint8_t range;
  float mA[POWER_GRAPH_MAX_COLUMNS];            // NAN where there is no data
  uint8_t category[POWER_GRAPH_MAX_COLUMNS];
  uint8_t charging[POWER_GRAPH_MAX_COLUMNS];
  float seconds[POWER_CAT_COUNT];               // time in each state, whole range
  float mAh[POWER_CAT_COUNT];                   // charge used in each state
  float coveredSeconds;
  float totalMah;
  float averageMa;                              // NAN with no data
  // How it is drawn.
  bool axisLabels;      // 1/10/100 mA labels and the time axis
  bool grid;
  uint8_t lineWidth;
};

/*
  Spreads the history over `columns` equal slices of the chosen range. Each
  column gets the time-weighted average current and the state that filled
  most of it; the totals are exact over the whole range.
*/
// Working space for computePowerGraph(), allocated through LVGL so the
// firmware's goes to PSRAM. As static arrays it was 6 KB of the ESP32-S3's
// internal RAM, which the Bluetooth stack runs short of (6.06).
struct PowerScratch {
  double energy[POWER_GRAPH_MAX_COLUMNS];
  float cover[POWER_GRAPH_MAX_COLUMNS];
  float catTime[POWER_GRAPH_MAX_COLUMNS][POWER_CAT_COUNT];
  float chargeTime[POWER_GRAPH_MAX_COLUMNS];
};
static PowerScratch *powerScratch = nullptr;

static void computePowerGraph(PowerGraphModel &m, const PowerSeries &series) {
  if (!powerScratch) powerScratch = (PowerScratch *)lv_mem_alloc(sizeof(PowerScratch));
  if (!powerScratch) return;
  double *energy = powerScratch->energy;
  float *cover = powerScratch->cover;
  float (*catTime)[POWER_CAT_COUNT] = powerScratch->catTime;
  float *chargeTime = powerScratch->chargeTime;
  uint16_t cols = m.columns > POWER_GRAPH_MAX_COLUMNS ? POWER_GRAPH_MAX_COLUMNS : m.columns;
  if (!cols) cols = 1;
  memset(powerScratch, 0, sizeof(*powerScratch));
  for (uint8_t c = 0; c < POWER_CAT_COUNT; c++) { m.seconds[c] = 0; m.mAh[c] = 0; }
  m.coveredSeconds = 0;
  m.totalMah = 0;

  uint32_t span = POWER_RANGE_SECONDS[m.range < POWER_RANGE_COUNT ? m.range : POWER_RANGE_DEFAULT];
  double end = series.now;
  double start = end - span;
  double colSpan = (double)span / cols;

  // Skip to the last point at or before the range start: a week of history
  // is tens of thousands of points, and a one-minute view needs a handful.
  uint32_t first = 0;
  if (series.count && series.at) {
    uint32_t lo = 0, hi = series.count;
    PowerPoint probe = {};
    while (lo + 1 < hi) {
      uint32_t mid = lo + (hi - lo) / 2;
      if (series.at(mid, probe) && probe.t <= start) lo = mid;
      else hi = mid;
    }
    first = lo;
  }
  PowerPoint p = {};
  PowerPoint next = {};
  bool haveNext = series.count && series.at && series.now && series.at(first, next);
  for (uint32_t i = first; haveNext; i++) {
    p = next;
    haveNext = i + 1 < series.count && series.at(i + 1, next);
    double segStart = p.t > start ? p.t : start;
    double segEnd = haveNext ? (next.t < end ? next.t : end) : end;
    if (segEnd <= segStart || p.category >= POWER_CAT_COUNT) continue;
    double mA = p.mA10 / 10.0;
    double s = segStart;
    while (s < segEnd) {
      int c = (int)((s - start) / colSpan);
      // On an exact column edge the division can round down to the column
      // just finished; step past it rather than pour the rest in there.
      while (c < cols - 1 && start + (c + 1) * colSpan <= s) c++;
      if (c >= cols) c = cols - 1;
      double colEnd = c == cols - 1 ? segEnd : start + (c + 1) * colSpan;
      double e = colEnd < segEnd ? colEnd : segEnd;
      double dt = e - s;
      energy[c] += mA * dt;
      cover[c] += (float)dt;
      catTime[c][p.category] += (float)dt;
      if (p.charging) chargeTime[c] += (float)dt;
      m.seconds[p.category] += (float)dt;
      m.mAh[p.category] += (float)(mA * dt / 3600.0);
      s = e;
    }
  }

  double totalEnergy = 0;
  for (uint16_t c = 0; c < cols; c++) {
    totalEnergy += energy[c];
    m.coveredSeconds += cover[c];
    // A column with under a fifth of its time known is treated as empty, so
    // the first moments of a fresh history do not draw a misleading step.
    if (cover[c] < colSpan * 0.2) {
      m.mA[c] = NAN;
      m.category[c] = POWER_CAT_NONE;
      m.charging[c] = 0;
      continue;
    }
    m.mA[c] = (float)(energy[c] / cover[c]);
    uint8_t best = 0;
    for (uint8_t k = 1; k < POWER_CAT_COUNT; k++) {
      if (catTime[c][k] > catTime[c][best]) best = k;
    }
    m.category[c] = best;
    m.charging[c] = chargeTime[c] * 2 > cover[c];
  }
  m.columns = cols;
  m.totalMah = (float)(totalEnergy / 3600.0);
  m.averageMa = m.coveredSeconds > 0 ? (float)(totalEnergy / m.coveredSeconds) : NAN;
}

static inline lv_coord_t powerGraphY(float mA, lv_coord_t top, lv_coord_t bottom) {
  if (mA < POWER_GRAPH_MIN_MA) mA = POWER_GRAPH_MIN_MA;
  if (mA > POWER_GRAPH_MAX_MA) mA = POWER_GRAPH_MAX_MA;
  float f = (log10f(mA) - log10f(POWER_GRAPH_MIN_MA)) /
            (log10f(POWER_GRAPH_MAX_MA) - log10f(POWER_GRAPH_MIN_MA));
  return (lv_coord_t)(bottom - f * (bottom - top));
}

static const int POWER_AXIS_LEFT = 22;     // room for "100"
static const int POWER_AXIS_BOTTOM = 13;   // room for the time labels

static void powerGraphDrawEvent(lv_event_t *e) {
  lv_obj_t *obj = lv_event_get_target(e);
  PowerGraphModel *m = (PowerGraphModel *)lv_event_get_user_data(e);
  if (!m) return;
  lv_draw_ctx_t *ctx = lv_event_get_draw_ctx(e);
  lv_area_t box;
  lv_obj_get_coords(obj, &box);
  lv_area_t plot = box;
  if (m->axisLabels) {
    plot.x1 += POWER_AXIS_LEFT;
    plot.y2 -= POWER_AXIS_BOTTOM;
  }
  lv_coord_t plotW = lv_area_get_width(&plot);
  lv_coord_t cols = m->columns ? m->columns : 1;
  auto colX = [&](int c) { return (lv_coord_t)(plot.x1 + (c * plotW) / cols); };

  // Nothing is drawn above the line: the state colours fill only the area
  // under it, and the rest of the graph is the card itself (6.08).

  lv_draw_label_dsc_t text;
  lv_draw_label_dsc_init(&text);
  text.font = &lv_font_montserrat_10;
  text.color = lv_color_make(150, 165, 182);

  if (m->grid) {
    lv_draw_line_dsc_t grid;
    lv_draw_line_dsc_init(&grid);
    grid.color = lv_color_white();
    grid.opa = (lv_opa_t)50;
    grid.width = 1;
    grid.dash_width = 2;
    grid.dash_gap = 3;
    const float marks[] = {1.0f, 10.0f, 100.0f};
    const char *names[] = {"1", "10", "100"};
    for (uint8_t i = 0; i < 3; i++) {
      lv_coord_t y = powerGraphY(marks[i], plot.y1, plot.y2);
      lv_point_t a = {plot.x1, y};
      lv_point_t b = {plot.x2, y};
      lv_draw_line(ctx, &grid, &a, &b);
      if (m->axisLabels) {
        lv_area_t label = {box.x1, (lv_coord_t)(y - 6), (lv_coord_t)(plot.x1 - 4), (lv_coord_t)(y + 6)};
        text.align = LV_TEXT_ALIGN_RIGHT;
        lv_draw_label(ctx, &text, &label, names[i], nullptr);
      }
    }
  }

  // The line, with the area beneath it filled in its state's colour.
  lv_draw_rect_dsc_t fill;
  lv_draw_rect_dsc_init(&fill);
  fill.bg_opa = (lv_opa_t)165;
  lv_draw_line_dsc_t line;
  lv_draw_line_dsc_init(&line);
  line.color = lv_color_hex(powerLineColour);
  line.width = m->lineWidth ? m->lineWidth : 2;
  line.round_start = 1;
  line.round_end = 1;
  bool havePrev = false;
  lv_point_t prev = {0, 0};
  bool anyData = false;
  for (int c = 0; c < cols; c++) {
    if (isnan(m->mA[c])) { havePrev = false; continue; }
    anyData = true;
    lv_coord_t x0 = colX(c);
    lv_coord_t x1 = (lv_coord_t)(colX(c + 1) - 1);
    if (x1 < x0) x1 = x0;
    lv_coord_t y = powerGraphY(m->mA[c], plot.y1, plot.y2);
    lv_area_t under = {x0, y, x1, plot.y2};
    fill.bg_color = powerCategoryColour(m->category[c]);
    lv_draw_rect(ctx, &fill, &under);
    lv_point_t here = {(lv_coord_t)((x0 + x1) / 2), y};
    if (havePrev) lv_draw_line(ctx, &line, &prev, &here);
    prev = here;
    havePrev = true;
  }

  // Charging: a strip along the foot of the graph, drawn over the fill so it
  // keeps its own colour.
  lv_draw_rect_dsc_t strip;
  lv_draw_rect_dsc_init(&strip);
  strip.bg_color = lv_color_hex(powerChargingColour);
  // An eighth of the graph, at least 4 px: about 11 px full screen, 6 on a
  // large tile (6.08 - 3 px was too thin to notice).
  lv_coord_t stripHeight = lv_area_get_height(&plot) / 8;
  if (stripHeight < 4) stripHeight = 4;
  strip.bg_opa = LV_OPA_COVER;
  for (int c = 0; c < cols;) {
    if (!m->charging[c]) { c++; continue; }
    int run = c + 1;
    while (run < cols && m->charging[run]) run++;
    lv_area_t a = {colX(c), (lv_coord_t)(plot.y2 - stripHeight + 1), (lv_coord_t)(colX(run) - 1), plot.y2};
    lv_draw_rect(ctx, &strip, &a);
    c = run;
  }

  if (m->axisLabels) {
    lv_area_t left = {plot.x1, (lv_coord_t)(plot.y2 + 2), (lv_coord_t)(plot.x1 + 60), box.y2};
    text.align = LV_TEXT_ALIGN_LEFT;
    lv_draw_label(ctx, &text, &left, POWER_RANGE_AXIS[m->range % POWER_RANGE_COUNT], nullptr);
    lv_area_t right = {(lv_coord_t)(plot.x2 - 60), (lv_coord_t)(plot.y2 + 2), plot.x2, box.y2};
    text.align = LV_TEXT_ALIGN_RIGHT;
    lv_draw_label(ctx, &text, &right, "now", nullptr);
  }

  if (!anyData) {
    lv_draw_label_dsc_t none;
    lv_draw_label_dsc_init(&none);
    none.font = &lv_font_montserrat_10;
    none.color = lv_color_make(150, 165, 182);
    none.align = LV_TEXT_ALIGN_CENTER;
    lv_area_t mid = {plot.x1, (lv_coord_t)((plot.y1 + plot.y2) / 2 - 6), plot.x2,
                     (lv_coord_t)((plot.y1 + plot.y2) / 2 + 6)};
    lv_draw_label(ctx, &none, &mid, "Collecting data", nullptr);
  }
}

// ------------------------------------------------------------ text helpers
static void formatPowerMa(char *out, size_t size, float mA) {
  if (isnan(mA)) snprintf(out, size, "--");
  else if (mA < 1.0f) snprintf(out, size, "%.2f", mA);
  else if (mA < 10.0f) snprintf(out, size, "%.1f", mA);
  else snprintf(out, size, "%.0f", mA);
}

// A charge with its unit: "0.52 mAh", "838 mAh", or "1.40 Ah" from 1000 up,
// which is what a week can reach and what fits the full-screen headline.
static void formatPowerMah(char *out, size_t size, float mAh) {
  if (isnan(mAh)) snprintf(out, size, "-- mAh");
  else if (mAh < 0.1f) snprintf(out, size, "%.2f mAh", mAh);
  else if (mAh < 10.0f) snprintf(out, size, "%.1f mAh", mAh);
  else if (mAh < 1000.0f) snprintf(out, size, "%.0f mAh", mAh);
  else snprintf(out, size, "%.2f Ah", mAh / 1000.0f);
}

static int powerCategoryPercent(const PowerGraphModel &m, uint8_t category) {
  if (m.coveredSeconds <= 0) return 0;
  return (int)lroundf(m.seconds[category] * 100.0f / m.coveredSeconds);
}

// ------------------------------------------------------------------ faces
enum PowerWidgetLayout : uint8_t {
  POWER_LAYOUT_SLIM,      // a slim tile on its own, 224 x 44
  POWER_LAYOUT_LARGE,     // a large tile, 224 x 96
  POWER_LAYOUT_FULL,      // the full-screen view, with the range dropdown
  POWER_LAYOUT_COLUMN,    // one column of a large custom widget
  POWER_LAYOUT_SLIM_CELL, // one column of a slim custom widget
  POWER_LAYOUT_ROW        // one row of an expanded custom widget
};

struct PowerWidgetUi {
  PowerGraphModel model;
  lv_obj_t *graph;
  lv_obj_t *value;       // average current
  lv_obj_t *detail;      // mAh and range, or "avg" and range
  lv_obj_t *dropdown;
  lv_obj_t *share[POWER_CAT_COUNT];    // "12%" per state
  lv_obj_t *shareMah[POWER_CAT_COUNT]; // full screen: [0] is "used in <range>"
  lv_obj_t *shareMahLabel[POWER_CAT_COUNT]; // full screen: mAh per state
  uint8_t layout;
  uint32_t refreshedMs;  // the firmware's refresh throttle
  uint8_t owner;         // the firmware's: which widget's setting this face shows
};

// Called after the full-screen menu changes the history length, so the owner
// can keep the choice (the firmware saves it as the widget's setting).
static void (*powerRangeChosenHook)(PowerWidgetUi *ui) = nullptr;

static lv_obj_t *powerLabel(lv_obj_t *parent, const char *text, int x, int y,
                            const lv_font_t *font, lv_color_t colour, int width = 0,
                            lv_text_align_t align = LV_TEXT_ALIGN_LEFT) {
  lv_obj_t *label = lv_label_create(parent);
  lv_label_set_text(label, text);
  lv_obj_set_style_text_font(label, font, 0);
  lv_obj_set_style_text_color(label, colour, 0);
  lv_obj_set_pos(label, x, y);
  if (width > 0) {
    lv_obj_set_width(label, width);
    lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(label, align, 0);
  }
  lv_obj_clear_flag(label, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(label, LV_OBJ_FLAG_SCROLLABLE);
  return label;
}

static inline lv_color_t powerMuted() { return lv_color_make(150, 165, 182); }

static void powerWidgetDeleted(lv_event_t *e) {
  PowerWidgetUi *ui = (PowerWidgetUi *)lv_event_get_user_data(e);
  if (ui) lv_mem_free(ui);
}

static void refreshPowerWidget(PowerWidgetUi *ui);

static void powerRangeChanged(lv_event_t *e) {
  PowerWidgetUi *ui = (PowerWidgetUi *)lv_event_get_user_data(e);
  if (!ui) return;
  uint16_t selected = lv_dropdown_get_selected(lv_event_get_target(e));
  ui->model.range = selected < POWER_RANGE_COUNT ? (uint8_t)selected : POWER_RANGE_DEFAULT;
  refreshPowerWidget(ui);
  if (powerRangeChosenHook) powerRangeChosenHook(ui);
}

static lv_obj_t *makePowerGraph(PowerWidgetUi *ui, lv_obj_t *parent, int x, int y, int w, int h) {
  lv_obj_t *graph = lv_obj_create(parent);
  lv_obj_remove_style_all(graph);
  lv_obj_set_pos(graph, x, y);
  lv_obj_set_size(graph, w, h);
  lv_obj_clear_flag(graph, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(graph, LV_OBJ_FLAG_SCROLLABLE);
  int plotW = w - (ui->model.axisLabels ? POWER_AXIS_LEFT : 0);
  ui->model.columns = (uint16_t)(plotW > POWER_GRAPH_MAX_COLUMNS ? POWER_GRAPH_MAX_COLUMNS
                                 : (plotW > 1 ? plotW : 1));
  lv_obj_add_event_cb(graph, powerGraphDrawEvent, LV_EVENT_DRAW_MAIN, &ui->model);
  ui->graph = graph;
  return graph;
}

// A coloured chip and a percentage, for the one-line breakdown.
static lv_obj_t *makePowerShare(PowerWidgetUi *ui, lv_obj_t *parent, uint8_t category,
                                const lv_font_t *font, bool named) {
  lv_obj_t *item = lv_obj_create(parent);
  lv_obj_remove_style_all(item);
  lv_obj_set_size(item, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(item, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(item, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(item, 3, 0);
  lv_obj_clear_flag(item, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(item, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t *chip = lv_obj_create(item);
  lv_obj_remove_style_all(chip);
  lv_obj_set_size(chip, 7, 7);
  lv_obj_set_style_radius(chip, 2, 0);
  lv_obj_set_style_bg_color(chip, powerCategoryColour(category), 0);
  lv_obj_set_style_bg_opa(chip, LV_OPA_COVER, 0);
  if (named) {
    const char *shortName = category == POWER_CAT_IN_USE ? "Use"
                          : category == POWER_CAT_LIGHT ? "Light" : "Deep";
    lv_obj_t *name = powerLabel(item, shortName, 0, 0, font, powerMuted());
    (void)name;
  }
  ui->share[category] = powerLabel(item, "--", 0, 0, font, lv_color_white());
  return item;
}

static lv_obj_t *makePowerShareRow(PowerWidgetUi *ui, lv_obj_t *parent, int x, int y, int w,
                                   const lv_font_t *font, bool named) {
  lv_obj_t *row = lv_obj_create(parent);
  lv_obj_remove_style_all(row);
  lv_obj_set_pos(row, x, y);
  lv_obj_set_size(row, w, LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(row, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
  for (uint8_t c = 0; c < POWER_CAT_COUNT; c++) makePowerShare(ui, row, c, font, named);
  return row;
}

/*
  Builds a power face of `layout` into parent (width x height) and returns
  its state, which lives until parent is deleted. range is the starting
  history length; the full-screen dropdown can change it while open.
*/
static PowerWidgetUi *buildPowerWidget(lv_obj_t *parent, int width, int height,
                                       uint8_t layout, uint8_t range) {
  PowerWidgetUi *ui = (PowerWidgetUi *)lv_mem_alloc(sizeof(PowerWidgetUi));
  if (!ui) return nullptr;
  memset(ui, 0, sizeof(*ui));
  ui->layout = layout;
  ui->model.range = range < POWER_RANGE_COUNT ? range : POWER_RANGE_DEFAULT;
  ui->model.lineWidth = 2;
  lv_obj_add_event_cb(parent, powerWidgetDeleted, LV_EVENT_DELETE, ui);
  const lv_color_t white = lv_color_white();

  if (layout == POWER_LAYOUT_SLIM) {
    ui->value = powerLabel(parent, "--", 10, 5, &lv_font_montserrat_16, white, 56);
    ui->detail = powerLabel(parent, "", 10, 25, &lv_font_montserrat_10, powerMuted(), 56);
    ui->model.lineWidth = 1;
    makePowerGraph(ui, parent, 70, 6, width - 70 - 8, height - 12);
  } else if (layout == POWER_LAYOUT_LARGE || layout == POWER_LAYOUT_ROW) {
    int pad = layout == POWER_LAYOUT_ROW ? 14 : 9;
    bool tall = height >= 110;
    ui->value = powerLabel(parent, "--", pad, 5, tall ? &lv_font_montserrat_20 : &lv_font_montserrat_16,
                           white, 100);
    ui->detail = powerLabel(parent, "", width / 2 - 10, tall ? 10 : 7, &lv_font_montserrat_12,
                            powerMuted(), width / 2 + 10 - pad, LV_TEXT_ALIGN_RIGHT);
    int graphTop = tall ? 34 : 26;
    int graphBottom = height - (tall ? 24 : 20);
    ui->model.grid = tall;
    makePowerGraph(ui, parent, pad, graphTop, width - pad * 2, graphBottom - graphTop);
    makePowerShareRow(ui, parent, pad, graphBottom + 4, width - pad * 2,
                      &lv_font_montserrat_10, true);
  } else if (layout == POWER_LAYOUT_FULL) {
    powerLabel(parent, "Power use", 12, 12, &lv_font_montserrat_18, white);
    ui->dropdown = lv_dropdown_create(parent);
    lv_dropdown_set_options(ui->dropdown, POWER_RANGE_OPTIONS);
    lv_dropdown_set_selected(ui->dropdown, ui->model.range);
    lv_obj_set_pos(ui->dropdown, width - 12 - 104, 6);
    lv_obj_set_size(ui->dropdown, 104, 32);
    lv_obj_set_style_text_font(ui->dropdown, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(ui->dropdown, white, 0);
    lv_obj_set_style_bg_color(ui->dropdown, lv_color_make(44, 58, 78), 0);
    lv_obj_set_style_bg_opa(ui->dropdown, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(ui->dropdown, lv_color_white(), 0);
    lv_obj_set_style_border_opa(ui->dropdown, (lv_opa_t)60, 0);
    lv_obj_set_style_border_width(ui->dropdown, 1, 0);
    lv_obj_set_style_radius(ui->dropdown, 8, 0);
    lv_obj_set_style_pad_ver(ui->dropdown, 8, 0);
    lv_obj_set_style_pad_hor(ui->dropdown, 10, 0);
    lv_obj_t *list = lv_dropdown_get_list(ui->dropdown);
    if (list) {
      lv_obj_set_style_text_font(list, &lv_font_montserrat_14, 0);
      // Nine lengths: tight enough rows that all of them fit below the button.
      lv_obj_set_style_text_line_space(list, 9, 0);
      lv_obj_set_style_pad_ver(list, 6, 0);
      lv_obj_set_style_max_height(list, 300, 0);
      lv_obj_set_style_text_color(list, white, 0);
      lv_obj_set_style_bg_color(list, lv_color_make(30, 42, 58), 0);
      lv_obj_set_style_bg_opa(list, LV_OPA_COVER, 0);
      lv_obj_set_style_border_color(list, lv_color_white(), 0);
      lv_obj_set_style_border_opa(list, (lv_opa_t)60, 0);
      lv_obj_set_style_bg_color(list, lv_color_make(10, 132, 255), LV_PART_SELECTED | LV_STATE_CHECKED);
      lv_obj_set_style_bg_color(list, lv_color_make(10, 132, 255), LV_PART_SELECTED | LV_STATE_PRESSED);
    }
    lv_obj_add_event_cb(ui->dropdown, powerRangeChanged, LV_EVENT_VALUE_CHANGED, ui);

    ui->value = powerLabel(parent, "--", 12, 46, &lv_font_montserrat_24, white, 104);
    powerLabel(parent, "average", 12, 74, &lv_font_montserrat_10, powerMuted());
    ui->detail = powerLabel(parent, "--", width - 12 - 104, 46, &lv_font_montserrat_24, white,
                            104, LV_TEXT_ALIGN_RIGHT);
    ui->shareMah[0] = powerLabel(parent, "used", width - 12 - 104, 74, &lv_font_montserrat_10,
                                 powerMuted(), 104, LV_TEXT_ALIGN_RIGHT);
    ui->model.axisLabels = true;
    ui->model.grid = true;
    makePowerGraph(ui, parent, 6, 90, width - 18, 104);

    // Three columns: state, share of time, charge used.
    int colW = (width - 12) / 3;
    for (uint8_t c = 0; c < POWER_CAT_COUNT; c++) {
      int x = 12 + c * colW;
      lv_obj_t *chip = lv_obj_create(parent);
      lv_obj_remove_style_all(chip);
      lv_obj_set_pos(chip, x, 203);
      lv_obj_set_size(chip, 8, 8);
      lv_obj_set_style_radius(chip, 2, 0);
      lv_obj_set_style_bg_color(chip, powerCategoryColour(c), 0);
      lv_obj_set_style_bg_opa(chip, LV_OPA_COVER, 0);
      powerLabel(parent, powerCategoryName(c), x + 12, 200, &lv_font_montserrat_10, powerMuted());
      ui->share[c] = powerLabel(parent, "--", x, 212, &lv_font_montserrat_16, white, colW - 4);
      lv_obj_t *mah = powerLabel(parent, "--", x, 230, &lv_font_montserrat_10, powerMuted(),
                                 colW - 4);
      ui->shareMahLabel[c] = mah;
    }
  } else if (layout == POWER_LAYOUT_COLUMN) {
    ui->value = powerLabel(parent, "--", 4, 8, &lv_font_montserrat_14, white, width - 8,
                           LV_TEXT_ALIGN_CENTER);
    ui->model.lineWidth = 1;
    makePowerGraph(ui, parent, 5, 30, width - 10, 42);
    ui->detail = powerLabel(parent, "", 2, 77, &lv_font_montserrat_10, powerMuted(), width - 4,
                            LV_TEXT_ALIGN_CENTER);
  } else {  // POWER_LAYOUT_SLIM_CELL
    ui->value = powerLabel(parent, "--", 2, 4, &lv_font_montserrat_12, white, width - 4,
                           LV_TEXT_ALIGN_CENTER);
    ui->model.lineWidth = 1;
    makePowerGraph(ui, parent, 5, 22, width - 10, 16);
  }
  refreshPowerWidget(ui);
  return ui;
}

static void refreshPowerWidget(PowerWidgetUi *ui) {
  if (!ui) return;
  PowerSeries series = powerSeriesSource ? powerSeriesSource() : PowerSeries{0, nullptr, 0};
  computePowerGraph(ui->model, series);
  const PowerGraphModel &m = ui->model;
  char ma[16];
  char mah[16];
  char text[48];
  formatPowerMa(ma, sizeof(ma), m.averageMa);
  formatPowerMah(mah, sizeof(mah), m.coveredSeconds > 0 ? m.totalMah : NAN);
  const char *range = POWER_RANGE_NAME[m.range % POWER_RANGE_COUNT];

  if (ui->layout == POWER_LAYOUT_FULL) {
    snprintf(text, sizeof(text), "%s mA", ma);
    lv_label_set_text(ui->value, text);
    snprintf(text, sizeof(text), "%s", mah);
    lv_label_set_text(ui->detail, text);
    snprintf(text, sizeof(text), "used in %s", range);
    lv_label_set_text(ui->shareMah[0], text);
    for (uint8_t c = 0; c < POWER_CAT_COUNT; c++) {
      snprintf(text, sizeof(text), "%d%%", powerCategoryPercent(m, c));
      lv_label_set_text(ui->share[c], text);
      char part[16];
      formatPowerMah(part, sizeof(part), m.mAh[c]);
      snprintf(text, sizeof(text), "%s", part);
      if (ui->shareMahLabel[c]) lv_label_set_text(ui->shareMahLabel[c], text);
    }
  } else {
    snprintf(text, sizeof(text), "%s mA", ma);
    if (ui->value) lv_label_set_text(ui->value, text);
    if (ui->detail) {
      if (ui->layout == POWER_LAYOUT_SLIM) snprintf(text, sizeof(text), "avg %s", range);
      else if (ui->layout == POWER_LAYOUT_COLUMN) snprintf(text, sizeof(text), "avg \xE2\x80\xA2 %s", range);
      else snprintf(text, sizeof(text), "%s \xE2\x80\xA2 %s", mah, range);
      lv_label_set_text(ui->detail, text);
    }
    for (uint8_t c = 0; c < POWER_CAT_COUNT; c++) {
      if (!ui->share[c]) continue;
      snprintf(text, sizeof(text), "%d%%", powerCategoryPercent(m, c));
      lv_label_set_text(ui->share[c], text);
    }
  }
  if (ui->graph) lv_obj_invalidate(ui->graph);
}

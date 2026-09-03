#pragma once

#include <stdint.h>

// FPS counter overlay --------------------------------------------------------
bool getShowFPS();
void setShowFPS(bool aShowFPS);
// called from the display flush callback (hardware/*/lvgl_hal_*.cpp) once per flushed area
void debugOverlay_count_frame(void);
// called every ~1000 ms from main.cpp's loop() to recompute and display the FPS value
void update_debugOverlay_fps(void);

// Touch debug overlay ---------------------------------------------------------
bool getShowTouchDebug();
void setShowTouchDebug(bool aShowTouchDebug);
// called from the touch indev read callback (hardware/*/lvgl_hal_*.cpp) with the raw touch state
void debugOverlay_report_touch(int16_t x, int16_t y, bool pressed);
// called every gui_loop() cycle to fade out and hide aged touch trail dots
void debugOverlay_loop(void);

// creates the (initially hidden) overlay objects on lv_layer_top(). Call once from init_gui().
void init_debugOverlay(void);

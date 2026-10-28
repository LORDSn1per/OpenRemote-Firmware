#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 0

#define LV_MEM_CUSTOM 1
#define LV_MEM_CUSTOM_INCLUDE "esp32-hal-psram.h"
#define LV_MEM_CUSTOM_ALLOC ps_malloc
#define LV_MEM_CUSTOM_FREE free
#define LV_MEM_CUSTOM_REALLOC ps_realloc

/* LVGL reads the millisecond clock itself (firmware 5.90) instead of being
   fed the loop's elapsed time once per pass. */
#define LV_TICK_CUSTOM 1
#define LV_TICK_CUSTOM_INCLUDE "Arduino.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())

/* Firmware 5.89. LVGL's defaults are 30 ms for both: at most 33 frames a
   second, and a finger sampled 33 times a second, which is what made a page
   swipe feel like it lagged behind the finger. A frame is only drawn when
   something changed, so a shorter period costs nothing on a still screen. */
#define LV_DISP_DEF_REFR_PERIOD 15
#define LV_INDEV_DEF_READ_PERIOD 10
#define LV_USE_LOG 0
#define LV_USE_ASSERT_NULL 0
#define LV_USE_ASSERT_MALLOC 0
#define LV_USE_ASSERT_STYLE 0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ 0

#define LV_USE_ANIMATION 1
#define LV_USE_SHADOW 1
#define LV_USE_OPA_SCALE 1
#define LV_USE_IMG 1
#define LV_USE_LABEL 1
#define LV_USE_BTN 1
#define LV_USE_SLIDER 1
#define LV_USE_SWITCH 1
#define LV_USE_BAR 1
#define LV_USE_ROLLER 1
#define LV_USE_TEXTAREA 1
#define LV_USE_KEYBOARD 1
#define LV_USE_QRCODE 1
#define LV_USE_PNG 1
#define LV_USE_SNAPSHOT 1
#define LV_IMG_CACHE_DEF_SIZE 1

#define LV_FONT_MONTSERRAT_10 1
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_22 1
#define LV_FONT_MONTSERRAT_24 1
// 48 is the hero glyph size for the pairing overlay's Bluetooth rune and
// its success tick. The built-in Montserrat faces carry the FontAwesome
// symbol range, so LV_SYMBOL_BLUETOOTH and LV_SYMBOL_OK come with it.
#define LV_FONT_MONTSERRAT_48 1
#define LV_FONT_DEFAULT &lv_font_montserrat_14

#endif

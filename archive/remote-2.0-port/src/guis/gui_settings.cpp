#include <lvgl.h>
#include "applicationInternal/hardware/hardwarePresenter.h"
#include "applicationInternal/memoryUsage.h"
#include "applicationInternal/debugOverlay.h"
#include "applicationInternal/gui/guiBase.h"
#include "applicationInternal/gui/guiRegistry.h"
#include "applicationInternal/omote_log.h"
#include "guis/gui_settings.h"

// LVGL declarations
LV_IMG_DECLARE(high_brightness);
LV_IMG_DECLARE(low_brightness);

lv_obj_t* objBattSettingsVoltage;
lv_obj_t* objBattSettingsPercentage;
//lv_obj_t* objBattSettingsIscharging;

// Display Backlight Slider Event handler
static void bl_slider_event_cb(lv_event_t* e){
  lv_obj_t* slider = lv_event_get_target(e);
  int32_t slider_value = lv_slider_get_value(slider);
  if (slider_value < 60)  {slider_value = 60;}
  if (slider_value > 255) {slider_value = 255;}
  set_backlightBrightness(slider_value);
}

#if(OMOTE_HARDWARE_REV >= 5)
// Keyboard Backlight Slider Event handler
static void kb_slider_event_cb(lv_event_t* e){
  lv_obj_t* slider = lv_event_get_target(e);
  int32_t slider_value = lv_slider_get_value(slider);
  if (slider_value < 0)  {slider_value = 0;}
  if (slider_value > 255) {slider_value = 255;}
  set_keyboardBrightness(slider_value);
}
#endif

// Wakeup by IMU Switch Event handler
static void WakeEnableSetting_event_cb(lv_event_t* e){
  set_wakeupByIMUEnabled(lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED));
}

// timout event handler
static void timout_event_cb(lv_event_t* e){
  lv_obj_t* drop = lv_event_get_target(e);
  uint16_t selected = lv_dropdown_get_selected(drop);
  switch (selected) {
    case 0: {set_sleepTimeout(  10000); break;}
    case 1: {set_sleepTimeout(  20000); break;}
    case 2: {set_sleepTimeout(  40000); break;}
    case 3: {set_sleepTimeout(  60000); break;}
    case 4: {set_sleepTimeout( 180000); break;}
    case 5: {set_sleepTimeout( 600000); break;}
    case 6: {set_sleepTimeout(3600000); break;}
  }
  omote_log_v("New timeout: %lu ms\r\n", get_sleepTimeout());
  setLastActivityTimestamp();
  // save preferences now, otherwise if you set a very big timeout and upload your firmware again, it never got saved
  save_preferences();
}

// motion threshold event handler
static void motion_threshold_event_cb(lv_event_t* e){
  lv_obj_t* drop = lv_event_get_target(e);
  uint16_t selected = lv_dropdown_get_selected(drop);
  switch (selected) {
    case 0: {set_motionThreshold(120); break;}
    case 1: {set_motionThreshold( 80); break;}
    case 2: {set_motionThreshold( 50); break;}
  }
  omote_log_v("New motion threshold: %lu ms\r\n", get_motionThreshold());
  save_preferences();
}

// show memory usage event handler
static void showMemoryUsage_event_cb(lv_event_t* e) {
  setShowMemoryUsage(lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED));
}

// show FPS counter event handler
static void showFPS_event_cb(lv_event_t* e) {
  bool enabled = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
  setShowFPS(enabled);
  set_debugFPSEnabled(enabled);
  save_preferences();
}

// show touch debug event handler
static void showTouchDebug_event_cb(lv_event_t* e) {
  bool enabled = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
  setShowTouchDebug(enabled);
  set_debugTouchEnabled(enabled);
  save_preferences();
}

// -----------------------------------------------------------------------------------------------
// Everything below is a faithful-looking but NON-FUNCTIONAL stand-in for settings pages that exist
// in OpenRemote_1.0 (Wi-Fi, Bluetooth, Clock, Buttons, Backup/Restore, About) but are not wired up
// to any real network/BLE/SD-card code yet in OpenRemote_2.0. Widgets are real OMOTE-styled LVGL
// controls so they can be tapped/dragged/opened for a UI test pass, they just don't do anything.
// -----------------------------------------------------------------------------------------------

// Fake Bluetooth "pair" button - only updates its own status label, no BLE pairing happens
static lv_obj_t* fakeBluetoothStatusLabel = NULL;
static void fakeBluetoothPair_event_cb(lv_event_t* e) {
  if (fakeBluetoothStatusLabel != NULL) {
    lv_label_set_text(fakeBluetoothStatusLabel, "Pairing... (fake)");
  }
}

// Fake "create backup" button - only updates its own status label, no SD card access happens
static lv_obj_t* fakeBackupStatusLabel = NULL;
static void fakeBackupCreate_event_cb(lv_event_t* e) {
  if (fakeBackupStatusLabel != NULL) {
    lv_label_set_text(fakeBackupStatusLabel, "Backup created (fake)");
  }
}

void create_tab_content_settings(lv_obj_t* tab) {

  // Add content to the settings tab
  // With a flex layout, setting groups/boxes will position themselves automatically
  lv_obj_set_layout(tab, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_scrollbar_mode(tab, LV_SCROLLBAR_MODE_ACTIVE);

  // Add a label, then a box for the display settings -----------------------------------------
  lv_obj_t* menuLabel = lv_label_create(tab);
  lv_label_set_text(menuLabel, "Display");

  lv_obj_t* menuBox = lv_obj_create(tab);
  lv_obj_set_size(menuBox, lv_pct(100), 141);
  lv_obj_set_style_bg_color(menuBox, color_primary, LV_PART_MAIN);
  lv_obj_set_style_border_width(menuBox, 0, LV_PART_MAIN);

  lv_obj_t* brightnessIcon = lv_img_create(menuBox);
  lv_img_set_src(brightnessIcon, &low_brightness);
  lv_obj_set_style_img_recolor(brightnessIcon, lv_color_white(), LV_PART_MAIN);
  lv_obj_set_style_img_recolor_opa(brightnessIcon, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_align(brightnessIcon, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_t* slider = lv_slider_create(menuBox);
  lv_slider_set_range(slider, 60, 255);
  lv_obj_set_style_bg_color(slider, lv_color_white(), LV_PART_KNOB);
  lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(slider, lv_color_lighten(color_primary, 50), LV_PART_MAIN);
  lv_slider_set_value(slider, get_backlightBrightness(), LV_ANIM_OFF);
  lv_obj_set_size(slider, lv_pct(66), 10);
  lv_obj_align(slider, LV_ALIGN_TOP_MID, 0, 3);
  brightnessIcon = lv_img_create(menuBox);
  lv_img_set_src(brightnessIcon, &high_brightness);
  lv_obj_set_style_img_recolor(brightnessIcon, lv_color_white(), LV_PART_MAIN);
  lv_obj_set_style_img_recolor_opa(brightnessIcon, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_align(brightnessIcon, LV_ALIGN_TOP_RIGHT, 0, -1);
  lv_obj_add_event_cb(slider, bl_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
  
  menuLabel = lv_label_create(menuBox);
  lv_label_set_text(menuLabel, "Lift to Wake");
  lv_obj_align(menuLabel, LV_ALIGN_TOP_LEFT, 0, 32);
  lv_obj_t* wakeToggle = lv_switch_create(menuBox);
  lv_obj_set_size(wakeToggle, 40, 22);
  lv_obj_align(wakeToggle, LV_ALIGN_TOP_RIGHT, 0, 29);
  lv_obj_set_style_bg_color(wakeToggle, lv_color_hex(0x505050), LV_PART_MAIN);
  lv_obj_add_event_cb(wakeToggle, WakeEnableSetting_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
  if (get_wakeupByIMUEnabled()) {
    lv_obj_add_state(wakeToggle, LV_STATE_CHECKED);
  } else {
    // lv_obj_clear_state(wakeToggle, LV_STATE_CHECKED);
  }

  menuLabel = lv_label_create(menuBox);
  lv_label_set_text(menuLabel, "Sensitivity");
  lv_obj_align(menuLabel, LV_ALIGN_TOP_LEFT, 0, 64);
  lv_obj_t* drop = lv_dropdown_create(menuBox);
  lv_dropdown_set_options(drop, "low\n"
                                "mid\n"
                                "high");
  // if you add more options here, do the same in timout_event_cb()
  switch (get_motionThreshold()) {
    case 120: {lv_dropdown_set_selected(drop, 0); break;}
    case  80: {lv_dropdown_set_selected(drop, 1); break;}
    case  50: {lv_dropdown_set_selected(drop, 2); break;}
  }
  lv_dropdown_set_selected_highlight(drop, true);
  lv_obj_align(drop, LV_ALIGN_TOP_RIGHT, 0, 61);
  lv_obj_set_size(drop, 70, 22);
  //lv_obj_set_style_text_font(drop, &lv_font_montserrat_12, LV_PART_MAIN);
  //lv_obj_set_style_text_font(lv_dropdown_get_list(drop), &lv_font_montserrat_12, LV_PART_MAIN);
  lv_obj_set_style_pad_top(drop, 1, LV_PART_MAIN);
  lv_obj_set_style_bg_color(drop, color_primary, LV_PART_MAIN);
  lv_obj_set_style_bg_color(lv_dropdown_get_list(drop), color_primary, LV_PART_MAIN);
  lv_obj_set_style_border_width(lv_dropdown_get_list(drop), 1, LV_PART_MAIN);
  lv_obj_set_style_border_color(lv_dropdown_get_list(drop), lv_color_hex(0x505050), LV_PART_MAIN);
  lv_obj_add_event_cb(drop, motion_threshold_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

  menuLabel = lv_label_create(menuBox);
  lv_label_set_text(menuLabel, "Timeout");
  lv_obj_align(menuLabel, LV_ALIGN_TOP_LEFT, 0, 96);
  drop = lv_dropdown_create(menuBox);
  lv_dropdown_set_options(drop, "10s\n"
                                "20s\n"
                                "40s\n"
                                "1m\n"
                                "3m\n"
                                "10m\n"
                                "1h"); // 1h for debug purposes, if you don't want the device to go to slepp
  // if you add more options here, do the same in timout_event_cb()
  switch (get_sleepTimeout()) {
    case   10000: {lv_dropdown_set_selected(drop, 0); break;}
    case   20000: {lv_dropdown_set_selected(drop, 1); break;}
    case   40000: {lv_dropdown_set_selected(drop, 2); break;}
    case   60000: {lv_dropdown_set_selected(drop, 3); break;}
    case  180000: {lv_dropdown_set_selected(drop, 4); break;}
    case  600000: {lv_dropdown_set_selected(drop, 5); break;}
    case 3600000: {lv_dropdown_set_selected(drop, 6); break;}
  }
  lv_dropdown_set_selected_highlight(drop, true);
  lv_obj_align(drop, LV_ALIGN_TOP_RIGHT, 0, 93);
  lv_obj_set_size(drop, 70, 22);
  //lv_obj_set_style_text_font(drop, &lv_font_montserrat_12, LV_PART_MAIN);
  //lv_obj_set_style_text_font(lv_dropdown_get_list(drop), &lv_font_montserrat_12, LV_PART_MAIN);
  lv_obj_set_style_pad_top(drop, 1, LV_PART_MAIN);
  lv_obj_set_style_bg_color(drop, color_primary, LV_PART_MAIN);
  lv_obj_set_style_bg_color(lv_dropdown_get_list(drop), color_primary, LV_PART_MAIN);
  lv_obj_set_style_border_width(lv_dropdown_get_list(drop), 1, LV_PART_MAIN);
  lv_obj_set_style_border_color(lv_dropdown_get_list(drop), lv_color_hex(0x505050), LV_PART_MAIN);
  lv_obj_add_event_cb(drop, timout_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

  // Wi-Fi (placeholder UI, mirrors OpenRemote_1.0's Wi-Fi page - not wired to a real network stack) --
  menuLabel = lv_label_create(tab);
  lv_label_set_text(menuLabel, "Wi-Fi");
  menuBox = lv_obj_create(tab);
  lv_obj_set_size(menuBox, lv_pct(100), 68);
  lv_obj_set_style_bg_color(menuBox, color_primary, LV_PART_MAIN);
  lv_obj_set_style_border_width(menuBox, 0, LV_PART_MAIN);

  menuLabel = lv_label_create(menuBox);
  lv_label_set_text(menuLabel, "Wi-Fi");
  lv_obj_align(menuLabel, LV_ALIGN_TOP_LEFT, 0, 3);
  lv_obj_t* wifiToggle = lv_switch_create(menuBox);
  lv_obj_set_size(wifiToggle, 40, 22);
  lv_obj_align(wifiToggle, LV_ALIGN_TOP_RIGHT, 0, 0);
  lv_obj_set_style_bg_color(wifiToggle, lv_color_hex(0x505050), LV_PART_MAIN);
  lv_obj_add_state(wifiToggle, LV_STATE_CHECKED);

  menuLabel = lv_label_create(menuBox);
  lv_label_set_text(menuLabel, "Network");
  lv_obj_align(menuLabel, LV_ALIGN_TOP_LEFT, 0, 35);
  menuLabel = lv_label_create(menuBox);
  lv_label_set_text(menuLabel, "Not connected");
  lv_obj_set_style_text_opa(menuLabel, LV_OPA_60, LV_PART_MAIN);
  lv_obj_align(menuLabel, LV_ALIGN_TOP_RIGHT, 0, 35);

  // Bluetooth (placeholder UI, mirrors OpenRemote_1.0's Bluetooth page - not wired to real BLE pairing) --
  menuLabel = lv_label_create(tab);
  lv_label_set_text(menuLabel, "Bluetooth");
  menuBox = lv_obj_create(tab);
  lv_obj_set_size(menuBox, lv_pct(100), 100);
  lv_obj_set_style_bg_color(menuBox, color_primary, LV_PART_MAIN);
  lv_obj_set_style_border_width(menuBox, 0, LV_PART_MAIN);

  menuLabel = lv_label_create(menuBox);
  lv_label_set_text(menuLabel, "Bluetooth");
  lv_obj_align(menuLabel, LV_ALIGN_TOP_LEFT, 0, 3);
  lv_obj_t* bluetoothToggle = lv_switch_create(menuBox);
  lv_obj_set_size(bluetoothToggle, 40, 22);
  lv_obj_align(bluetoothToggle, LV_ALIGN_TOP_RIGHT, 0, 0);
  lv_obj_set_style_bg_color(bluetoothToggle, lv_color_hex(0x505050), LV_PART_MAIN);

  fakeBluetoothStatusLabel = lv_label_create(menuBox);
  lv_label_set_text(fakeBluetoothStatusLabel, "Not paired");
  lv_obj_set_style_text_opa(fakeBluetoothStatusLabel, LV_OPA_60, LV_PART_MAIN);
  lv_obj_align(fakeBluetoothStatusLabel, LV_ALIGN_TOP_LEFT, 0, 35);

  lv_obj_t* bluetoothPairButton = lv_btn_create(menuBox);
  lv_obj_set_size(bluetoothPairButton, lv_pct(100), 30);
  lv_obj_align(bluetoothPairButton, LV_ALIGN_TOP_LEFT, 0, 62);
  lv_obj_set_style_bg_color(bluetoothPairButton, lv_color_hex(0x505050), LV_PART_MAIN);
  lv_obj_t* bluetoothPairLabel = lv_label_create(bluetoothPairButton);
  lv_label_set_text(bluetoothPairLabel, "Start pairing");
  lv_obj_center(bluetoothPairLabel);
  lv_obj_add_event_cb(bluetoothPairButton, fakeBluetoothPair_event_cb, LV_EVENT_CLICKED, NULL);

  // Clock (placeholder UI, mirrors OpenRemote_1.0's Clock page - status bar clock not implemented yet) --
  menuLabel = lv_label_create(tab);
  lv_label_set_text(menuLabel, "Clock");
  menuBox = lv_obj_create(tab);
  lv_obj_set_size(menuBox, lv_pct(100), 100);
  lv_obj_set_style_bg_color(menuBox, color_primary, LV_PART_MAIN);
  lv_obj_set_style_border_width(menuBox, 0, LV_PART_MAIN);

  menuLabel = lv_label_create(menuBox);
  lv_label_set_text(menuLabel, "Show in status bar");
  lv_obj_align(menuLabel, LV_ALIGN_TOP_LEFT, 0, 3);
  lv_obj_t* clockShowToggle = lv_switch_create(menuBox);
  lv_obj_set_size(clockShowToggle, 40, 22);
  lv_obj_align(clockShowToggle, LV_ALIGN_TOP_RIGHT, 0, 0);
  lv_obj_set_style_bg_color(clockShowToggle, lv_color_hex(0x505050), LV_PART_MAIN);

  menuLabel = lv_label_create(menuBox);
  lv_label_set_text(menuLabel, "Internet time sync");
  lv_obj_align(menuLabel, LV_ALIGN_TOP_LEFT, 0, 35);
  lv_obj_t* clockSyncToggle = lv_switch_create(menuBox);
  lv_obj_set_size(clockSyncToggle, 40, 22);
  lv_obj_align(clockSyncToggle, LV_ALIGN_TOP_RIGHT, 0, 32);
  lv_obj_set_style_bg_color(clockSyncToggle, lv_color_hex(0x505050), LV_PART_MAIN);

  menuLabel = lv_label_create(menuBox);
  lv_label_set_text(menuLabel, "City");
  lv_obj_align(menuLabel, LV_ALIGN_TOP_LEFT, 0, 67);
  lv_obj_t* cityDropdown = lv_dropdown_create(menuBox);
  lv_dropdown_set_options(cityDropdown, "Canberra\n"
                                        "Sydney\n"
                                        "Melbourne\n"
                                        "Brisbane\n"
                                        "Perth\n"
                                        "Adelaide\n"
                                        "Darwin\n"
                                        "Hobart\n"
                                        "UTC");
  lv_dropdown_set_selected_highlight(cityDropdown, true);
  lv_obj_align(cityDropdown, LV_ALIGN_TOP_RIGHT, 0, 64);
  lv_obj_set_size(cityDropdown, 120, 22);
  lv_obj_set_style_pad_top(cityDropdown, 1, LV_PART_MAIN);
  lv_obj_set_style_bg_color(cityDropdown, color_primary, LV_PART_MAIN);
  lv_obj_set_style_bg_color(lv_dropdown_get_list(cityDropdown), color_primary, LV_PART_MAIN);
  lv_obj_set_style_border_width(lv_dropdown_get_list(cityDropdown), 1, LV_PART_MAIN);
  lv_obj_set_style_border_color(lv_dropdown_get_list(cityDropdown), lv_color_hex(0x505050), LV_PART_MAIN);

  // Buttons (placeholder UI, mirrors OpenRemote_1.0's Buttons page - repeat timing is fixed for now) --
  menuLabel = lv_label_create(tab);
  lv_label_set_text(menuLabel, "Buttons");
  menuBox = lv_obj_create(tab);
  lv_obj_set_size(menuBox, lv_pct(100), 132);
  lv_obj_set_style_bg_color(menuBox, color_primary, LV_PART_MAIN);
  lv_obj_set_style_border_width(menuBox, 0, LV_PART_MAIN);

  menuLabel = lv_label_create(menuBox);
  lv_label_set_text(menuLabel, "Repeat while held");
  lv_obj_align(menuLabel, LV_ALIGN_TOP_LEFT, 0, 3);
  lv_obj_t* repeatToggle = lv_switch_create(menuBox);
  lv_obj_set_size(repeatToggle, 40, 22);
  lv_obj_align(repeatToggle, LV_ALIGN_TOP_RIGHT, 0, 0);
  lv_obj_set_style_bg_color(repeatToggle, lv_color_hex(0x505050), LV_PART_MAIN);
  lv_obj_add_state(repeatToggle, LV_STATE_CHECKED);

  menuLabel = lv_label_create(menuBox);
  lv_label_set_text(menuLabel, "Repeat delay");
  lv_obj_align(menuLabel, LV_ALIGN_TOP_LEFT, 0, 35);
  lv_obj_t* repeatDelaySlider = lv_slider_create(menuBox);
  lv_slider_set_range(repeatDelaySlider, 0, 100);
  lv_slider_set_value(repeatDelaySlider, 30, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(repeatDelaySlider, lv_color_white(), LV_PART_KNOB);
  lv_obj_set_style_bg_opa(repeatDelaySlider, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(repeatDelaySlider, lv_color_lighten(color_primary, 50), LV_PART_MAIN);
  lv_obj_set_size(repeatDelaySlider, lv_pct(100), 10);
  lv_obj_align(repeatDelaySlider, LV_ALIGN_TOP_MID, 0, 55);

  menuLabel = lv_label_create(menuBox);
  lv_label_set_text(menuLabel, "Repeat speed");
  lv_obj_align(menuLabel, LV_ALIGN_TOP_LEFT, 0, 78);
  lv_obj_t* repeatSpeedSlider = lv_slider_create(menuBox);
  lv_slider_set_range(repeatSpeedSlider, 0, 100);
  lv_slider_set_value(repeatSpeedSlider, 50, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(repeatSpeedSlider, lv_color_white(), LV_PART_KNOB);
  lv_obj_set_style_bg_opa(repeatSpeedSlider, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(repeatSpeedSlider, lv_color_lighten(color_primary, 50), LV_PART_MAIN);
  lv_obj_set_size(repeatSpeedSlider, lv_pct(100), 10);
  lv_obj_align(repeatSpeedSlider, LV_ALIGN_TOP_MID, 0, 98);

  #if(OMOTE_HARDWARE_REV >= 5)
  // Another setting for the keyboard ----------------------------------------------------------
  menuLabel = lv_label_create(tab);
  lv_label_set_text(menuLabel, "Keyboard");
  menuBox = lv_obj_create(tab);
  lv_obj_set_size(menuBox, lv_pct(100), 44);
  lv_obj_set_style_bg_color(menuBox, color_primary, LV_PART_MAIN);
  lv_obj_set_style_border_width(menuBox, 0, LV_PART_MAIN);
  
  brightnessIcon = lv_img_create(menuBox);
  lv_img_set_src(brightnessIcon, &low_brightness);
  lv_obj_set_style_img_recolor(brightnessIcon, lv_color_white(), LV_PART_MAIN);
  lv_obj_set_style_img_recolor_opa(brightnessIcon, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_align(brightnessIcon, LV_ALIGN_TOP_LEFT, 0, 0);
  slider = lv_slider_create(menuBox);
  lv_slider_set_range(slider, 0, 255);
  lv_obj_set_style_bg_color(slider, lv_color_white(), LV_PART_KNOB);
  lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(slider, lv_color_lighten(color_primary, 50), LV_PART_MAIN);
  lv_slider_set_value(slider, get_keyboardBrightness(), LV_ANIM_OFF);
  lv_obj_set_size(slider, lv_pct(66), 10);
  lv_obj_align(slider, LV_ALIGN_TOP_MID, 0, 3);
  brightnessIcon = lv_img_create(menuBox);
  lv_img_set_src(brightnessIcon, &high_brightness);
  lv_obj_set_style_img_recolor(brightnessIcon, lv_color_white(), LV_PART_MAIN);
  lv_obj_set_style_img_recolor_opa(brightnessIcon, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_align(brightnessIcon, LV_ALIGN_TOP_RIGHT, 0, -1);
  lv_obj_add_event_cb(slider, kb_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
  #endif

  // Another setting for the battery ----------------------------------------------------------
  menuLabel = lv_label_create(tab);
  lv_label_set_text(menuLabel, "Battery");
  menuBox = lv_obj_create(tab);
  lv_obj_set_size(menuBox, lv_pct(100), 77); // 125
  lv_obj_set_style_bg_color(menuBox, color_primary, LV_PART_MAIN);
  lv_obj_set_style_border_width(menuBox, 0, LV_PART_MAIN);
  
  objBattSettingsVoltage = lv_label_create(menuBox);
  lv_label_set_text(objBattSettingsVoltage, "Voltage:");
  lv_obj_align(objBattSettingsVoltage, LV_ALIGN_TOP_LEFT, 0, 0);
  objBattSettingsPercentage = lv_label_create(menuBox);
  lv_label_set_text(objBattSettingsPercentage, "Percentage:");
  lv_obj_align(objBattSettingsPercentage, LV_ALIGN_TOP_LEFT, 0, 32);
  // objBattSettingsIscharging = lv_label_create(menuBox);
  // lv_label_set_text(objBattSettingsIscharging, "Is charging:");
  // lv_obj_align(objBattSettingsIscharging, LV_ALIGN_TOP_LEFT, 0, 64);

  // Backup / Restore (placeholder UI, mirrors OpenRemote_1.0's Backup page - SD backups not implemented yet) --
  menuLabel = lv_label_create(tab);
  lv_label_set_text(menuLabel, "Backup / Restore");
  menuBox = lv_obj_create(tab);
  lv_obj_set_size(menuBox, lv_pct(100), 70);
  lv_obj_set_style_bg_color(menuBox, color_primary, LV_PART_MAIN);
  lv_obj_set_style_border_width(menuBox, 0, LV_PART_MAIN);

  lv_obj_t* backupButton = lv_btn_create(menuBox);
  lv_obj_set_size(backupButton, lv_pct(100), 30);
  lv_obj_align(backupButton, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_set_style_bg_color(backupButton, lv_color_hex(0x505050), LV_PART_MAIN);
  lv_obj_t* backupButtonLabel = lv_label_create(backupButton);
  lv_label_set_text(backupButtonLabel, "Create full backup");
  lv_obj_center(backupButtonLabel);
  lv_obj_add_event_cb(backupButton, fakeBackupCreate_event_cb, LV_EVENT_CLICKED, NULL);

  fakeBackupStatusLabel = lv_label_create(menuBox);
  lv_label_set_text(fakeBackupStatusLabel, "No backups found");
  lv_obj_set_style_text_font(fakeBackupStatusLabel, &lv_font_montserrat_10, LV_PART_MAIN);
  lv_obj_set_style_text_opa(fakeBackupStatusLabel, LV_OPA_60, LV_PART_MAIN);
  lv_obj_align(fakeBackupStatusLabel, LV_ALIGN_TOP_LEFT, 0, 38);

  // About ------------------------------------------------------------------------------------
  // Firmware/build fields below are real (compile-time), everything else on this tab is a placeholder.
  menuLabel = lv_label_create(tab);
  lv_label_set_text(menuLabel, "About");
  menuBox = lv_obj_create(tab);
  lv_obj_set_size(menuBox, lv_pct(100), 68);
  lv_obj_set_style_bg_color(menuBox, color_primary, LV_PART_MAIN);
  lv_obj_set_style_border_width(menuBox, 0, LV_PART_MAIN);

  menuLabel = lv_label_create(menuBox);
  lv_label_set_text(menuLabel, "Firmware");
  lv_obj_align(menuLabel, LV_ALIGN_TOP_LEFT, 0, 0);
  menuLabel = lv_label_create(menuBox);
  lv_label_set_text(menuLabel, "OpenRemote 2.0 (" __DATE__ ")");
  lv_obj_set_style_text_opa(menuLabel, LV_OPA_60, LV_PART_MAIN);
  lv_obj_align(menuLabel, LV_ALIGN_TOP_RIGHT, 0, 0);

  menuLabel = lv_label_create(menuBox);
  lv_label_set_text(menuLabel, "Hardware");
  lv_obj_align(menuLabel, LV_ALIGN_TOP_LEFT, 0, 32);
  menuLabel = lv_label_create(menuBox);
  #if(OMOTE_HARDWARE_REV >= 5)
  lv_label_set_text(menuLabel, "OMOTE Rev 5 / ESP32-S3");
  #else
  lv_label_set_text(menuLabel, "OMOTE Rev 4 / ESP32");
  #endif
  lv_obj_set_style_text_opa(menuLabel, LV_OPA_60, LV_PART_MAIN);
  lv_obj_align(menuLabel, LV_ALIGN_TOP_RIGHT, 0, 32);

  // Memory statistics ------------------------------------------------------------------------
  menuLabel = lv_label_create(tab);
  lv_label_set_text(menuLabel, "Memory usage");
  menuBox = lv_obj_create(tab);
  lv_obj_set_size(menuBox, lv_pct(100), 48);
  lv_obj_set_style_bg_color(menuBox, color_primary, LV_PART_MAIN);
  lv_obj_set_style_border_width(menuBox, 0, LV_PART_MAIN);
  
  menuLabel = lv_label_create(menuBox);
  lv_label_set_text(menuLabel, "Show mem usage");
  lv_obj_align(menuLabel, LV_ALIGN_TOP_LEFT, 0, 3);
  lv_obj_t* memoryUsageToggle = lv_switch_create(menuBox);
  lv_obj_set_size(memoryUsageToggle, 40, 22);
  lv_obj_align(memoryUsageToggle, LV_ALIGN_TOP_RIGHT, 0, 0);
  lv_obj_set_style_bg_color(memoryUsageToggle, lv_color_hex(0x505050), LV_PART_MAIN);
  lv_obj_add_event_cb(memoryUsageToggle, showMemoryUsage_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
  if (getShowMemoryUsage()) {
    lv_obj_add_state(memoryUsageToggle, LV_STATE_CHECKED);
  } else {
    // lv_obj_clear_state(memoryUsageToggle, LV_STATE_CHECKED);
  }

  // Debug overlay ------------------------------------------------------------------------
  menuLabel = lv_label_create(tab);
  lv_label_set_text(menuLabel, "Debug");
  menuBox = lv_obj_create(tab);
  lv_obj_set_size(menuBox, lv_pct(100), 80);
  lv_obj_set_style_bg_color(menuBox, color_primary, LV_PART_MAIN);
  lv_obj_set_style_border_width(menuBox, 0, LV_PART_MAIN);

  menuLabel = lv_label_create(menuBox);
  lv_label_set_text(menuLabel, "Show FPS");
  lv_obj_align(menuLabel, LV_ALIGN_TOP_LEFT, 0, 3);
  lv_obj_t* fpsToggle = lv_switch_create(menuBox);
  lv_obj_set_size(fpsToggle, 40, 22);
  lv_obj_align(fpsToggle, LV_ALIGN_TOP_RIGHT, 0, 0);
  lv_obj_set_style_bg_color(fpsToggle, lv_color_hex(0x505050), LV_PART_MAIN);
  lv_obj_add_event_cb(fpsToggle, showFPS_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
  if (getShowFPS()) {
    lv_obj_add_state(fpsToggle, LV_STATE_CHECKED);
  }

  menuLabel = lv_label_create(menuBox);
  lv_label_set_text(menuLabel, "Show touch debug");
  lv_obj_align(menuLabel, LV_ALIGN_TOP_LEFT, 0, 35);
  lv_obj_t* touchDebugToggle = lv_switch_create(menuBox);
  lv_obj_set_size(touchDebugToggle, 40, 22);
  lv_obj_align(touchDebugToggle, LV_ALIGN_TOP_RIGHT, 0, 32);
  lv_obj_set_style_bg_color(touchDebugToggle, lv_color_hex(0x505050), LV_PART_MAIN);
  lv_obj_add_event_cb(touchDebugToggle, showTouchDebug_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
  if (getShowTouchDebug()) {
    lv_obj_add_state(touchDebugToggle, LV_STATE_CHECKED);
  }
}

void notify_tab_before_delete_settings(void) {
  // remember to set all pointers to lvgl objects to NULL if they might be accessed from outside.
  // They must check if object is NULL and must not use it if so
  objBattSettingsVoltage = NULL;
  objBattSettingsPercentage = NULL;
  fakeBluetoothStatusLabel = NULL;
  fakeBackupStatusLabel = NULL;
}

void register_gui_settings(void){
  register_gui(std::string(tabName_settings), & create_tab_content_settings, & notify_tab_before_delete_settings);
}

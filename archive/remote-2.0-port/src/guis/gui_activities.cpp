#include <lvgl.h>
#include "applicationInternal/gui/guiBase.h"
#include "applicationInternal/gui/guiRegistry.h"
#include "applicationInternal/hardware/hardwarePresenter.h"
#include "applicationInternal/omote_log.h"
#include "guis/gui_activities.h"

// Fake activities standing in for real ones until activities are actually
// wired up to devices/macros. Slide a row's slider to (near) the end to
// "activate" it - nothing is actually sent, it just logs and snaps back.
struct FakeActivity {
  const char* name;
};

static const FakeActivity fakeActivities[3] = {
  {"Movie Night"},
  {"Gaming"},
  {"Music"},
};

static void snapSliderBackAnim_cb(void* obj, int32_t value) {
  lv_slider_set_value((lv_obj_t*)obj, value, LV_ANIM_OFF);
}

static void activitySlider_event_cb(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_RELEASED) { return; }
  lv_obj_t* slider = lv_event_get_target(e);
  int index = (int)(intptr_t)lv_event_get_user_data(e);
  int32_t value = lv_slider_get_value(slider);
  if (value >= 90 && index >= 0 && index < 3) {
    omote_log_i("Activity '%s' activated (fake - no devices controlled)\r\n", fakeActivities[index].name);
  }
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, slider);
  lv_anim_set_exec_cb(&a, snapSliderBackAnim_cb);
  lv_anim_set_values(&a, value, 0);
  lv_anim_set_time(&a, 180);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
  lv_anim_start(&a);
}

void create_tab_content_activities(lv_obj_t* tab) {
  lv_obj_set_layout(tab, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_scrollbar_mode(tab, LV_SCROLLBAR_MODE_ACTIVE);

  lv_obj_t* menuLabel = lv_label_create(tab);
  lv_label_set_text(menuLabel, "Activities");

  for (int i = 0; i < 3; i++) {
    lv_obj_t* menuBox = lv_obj_create(tab);
    lv_obj_set_size(menuBox, lv_pct(100), 68);
    lv_obj_set_style_bg_color(menuBox, color_primary, LV_PART_MAIN);
    lv_obj_set_style_border_width(menuBox, 0, LV_PART_MAIN);

    lv_obj_t* nameLabel = lv_label_create(menuBox);
    lv_label_set_text(nameLabel, fakeActivities[i].name);
    lv_obj_align(nameLabel, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t* hintLabel = lv_label_create(menuBox);
    lv_label_set_text(hintLabel, "Slide to activate");
    lv_obj_set_style_text_font(hintLabel, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_set_style_text_opa(hintLabel, LV_OPA_60, LV_PART_MAIN);
    lv_obj_align(hintLabel, LV_ALIGN_TOP_LEFT, 0, 18);

    lv_obj_t* slider = lv_slider_create(menuBox);
    lv_slider_set_range(slider, 0, 100);
    lv_obj_set_style_bg_color(slider, lv_color_white(), LV_PART_KNOB);
    lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider, lv_color_lighten(color_primary, 50), LV_PART_MAIN);
    lv_slider_set_value(slider, 0, LV_ANIM_OFF);
    lv_obj_set_size(slider, lv_pct(100), 12);
    lv_obj_align(slider, LV_ALIGN_TOP_MID, 0, 42);
    lv_obj_add_event_cb(slider, activitySlider_event_cb, LV_EVENT_RELEASED, (void*)(intptr_t)i);
  }
}

void notify_tab_before_delete_activities(void) {
  // no persistent lvgl object pointers to clear - nothing outside this tab references them
}

void register_gui_activities(void) {
  register_gui(std::string(tabName_activities), &create_tab_content_activities, &notify_tab_before_delete_activities);
}

#pragma once

#include <lvgl.h>

// Fake, non-functional stand-in for OpenRemote_1.0's Activities page: lists a
// few placeholder activities with a slide-to-activate slider each, styled
// with OpenRemote_2.0's own widgets. No devices are controlled.
const char * const tabName_activities = "Activities";
void register_gui_activities(void);

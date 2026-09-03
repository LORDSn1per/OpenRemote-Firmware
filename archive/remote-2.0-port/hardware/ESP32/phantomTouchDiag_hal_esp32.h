#pragma once

// Phantom-touch diagnostic instrumentation.
//
// Only compiled when -D PHANTOM_TOUCH_SOAK=1 (see the esp32-s3-soaktest env in
// platformio.ini). The normal firmware build is completely unaffected.
//
// Why this exists: LovyanGFX's Touch_FT5x06 driver discards the two fields that
// would most easily separate a real finger from an electrically-induced ghost -
// the per-point EVENT FLAG (bits 7:6 of the XH register) and the TOUCH WEIGHT /
// AREA registers. It also already does its own double-read retry, so a ghost
// that reaches LVGL is one the chip reported *consistently*. This module reads
// the raw register block itself and logs everything, so a ghost can be
// characterised rather than guessed at.

#include <stdint.h>

#if defined(PHANTOM_TOUCH_SOAK)

// Call once, after the touch controller is up (end of init_lvgl_HAL()).
void phantomDiag_init(void);

// Call every LVGL input-read cycle, before handing control to the normal touch
// read. Polls the FT5x06 register block directly and logs any reported touch.
void phantomDiag_poll(void);

#endif

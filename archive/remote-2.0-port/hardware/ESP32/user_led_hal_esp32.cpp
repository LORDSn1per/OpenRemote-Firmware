#include <Arduino.h>

#if(OMOTE_HARDWARE_REV >= 5)
const uint8_t USER_LED_GPIO = 45;
#else
const uint8_t USER_LED_GPIO = 2;
#endif

void init_userled_HAL(void) {
  pinMode(USER_LED_GPIO, OUTPUT);
  digitalWrite(USER_LED_GPIO, LOW);  
}

void update_userled_HAL(void) {
#if(OMOTE_HARDWARE_REV >= 5)
  // GPIO 45 is not a spare status LED on this hardware - it's the microphone
  // power/enable line (shares the SD bus rail, per OpenRemote_1.0's pin map).
  // Blinking it here toggles mic power once a second, which shows up as an
  // LED flicker on the mic circuit and is a plausible phantom-touch noise
  // source. Hold it low (mic off) instead of blinking.
  digitalWrite(USER_LED_GPIO, LOW);
#else
  digitalWrite(USER_LED_GPIO, millis() % 2000 > 1000);
#endif
}

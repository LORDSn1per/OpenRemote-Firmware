#include <Preferences.h>
#include "sleep_hal_esp32.h"
#include "tft_hal_esp32.h"
#include "keypad_keys_hal_esp32.h"

Preferences preferences;

std::string activeScene;
std::string activeGUIname;
int activeGUIlist;
int lastActiveGUIlistIndex;
bool debugFPSEnabled = false;
bool debugTouchEnabled = false;

void init_preferences_HAL(void) {
  // Restore settings from internal flash memory
  preferences.begin("settings", false);
  if (preferences.getBool("alreadySetUp")) {
    // from sleep.h
    set_wakeupByIMUEnabled_HAL(preferences.getBool("wkpByIMU"));
    set_sleepTimeout_HAL(preferences.getUInt("slpTimeout", DEFAULT_SLEEP_TIMEOUT));
    set_motionThreshold_HAL(preferences.getUInt("motionThreshold", DEFAULT_MOTION_THRESHOLD));
    // from tft.h
    set_backlightBrightness_HAL(preferences.getUInt("blBrightness", 255));
    // from keyboard.h
    #if(OMOTE_HARDWARE_REV >= 5)
    set_keyboardBrightness_HAL(preferences.getUInt("kbBrightness", 255));
    #endif
    // from debugOverlay.h (Settings > Debug > FPS / Touch)
    debugFPSEnabled = preferences.getBool("dbgFPS", false);
    debugTouchEnabled = preferences.getBool("dbgTouch", false);
    // from here
    activeScene = std::string(preferences.getString("currentScene").c_str());
    activeGUIname = std::string(preferences.getString("currentGUIname").c_str());
    activeGUIlist =(preferences.getInt("currentGUIlist"));
    lastActiveGUIlistIndex = (preferences.getInt("lastActiveIndex"));

    // Serial.printf("Preferences restored: blBrightness %d, kbBrightness %d, GUI %s, scene %s\r\n", get_backlightBrightness_HAL(), get_keyboardBrightness_HAL(), activeGUIname.c_str(), activeScene.c_str());
  } else {
    // Serial.printf("No preferences to restore\r\n");
  }
  preferences.end();
}

void save_preferences_HAL(void) {
  preferences.begin("settings", false);
  // from sleep.h
  preferences.putBool("wkpByIMU", get_wakeupByIMUEnabled_HAL());
  // from tft.h
  preferences.putUInt("slpTimeout", get_sleepTimeout_HAL());
  preferences.putUInt("motionThreshold", get_motionThreshold_HAL());
  preferences.putUInt("blBrightness", get_backlightBrightness_HAL());
  // from keyboard.h
  #if(OMOTE_HARDWARE_REV >= 5)
  preferences.putUInt("kbBrightness", get_keyboardBrightness_HAL());
  // Serial.printf("Preferences saved: blBrightness %d, kbBrightness %d, GUI %s, scene %s\r\n", get_backlightBrightness_HAL(), get_keyboardBrightness_HAL(), activeGUIname.c_str(), activeScene.c_str());
  #endif
  // from debugOverlay.h (Settings > Debug > FPS / Touch)
  preferences.putBool("dbgFPS", debugFPSEnabled);
  preferences.putBool("dbgTouch", debugTouchEnabled);
  // from here
  preferences.putString("currentScene", activeScene.c_str());
  preferences.putString("currentGUIname", activeGUIname.c_str());
  preferences.putInt("currentGUIlist", activeGUIlist);
  preferences.putInt("lastActiveIndex", lastActiveGUIlistIndex);
  if (!preferences.getBool("alreadySetUp")) {
    preferences.putBool("alreadySetUp", true);
  }
  preferences.end();
}

bool get_debugFPSEnabled_HAL() {
  return debugFPSEnabled;
}
void set_debugFPSEnabled_HAL(bool aDebugFPSEnabled) {
  debugFPSEnabled = aDebugFPSEnabled;
}
bool get_debugTouchEnabled_HAL() {
  return debugTouchEnabled;
}
void set_debugTouchEnabled_HAL(bool aDebugTouchEnabled) {
  debugTouchEnabled = aDebugTouchEnabled;
}

std::string get_activeScene_HAL() {
  return activeScene;
}
void set_activeScene_HAL(std::string anActiveScene) {
  activeScene = anActiveScene;
}
std::string get_activeGUIname_HAL(){
  return activeGUIname;
}
void set_activeGUIname_HAL(std::string anActiveGUIname) {
  activeGUIname = anActiveGUIname;
}
int get_activeGUIlist_HAL() {
  return activeGUIlist;
}
void set_activeGUIlist_HAL(int anActiveGUIlist) {
  activeGUIlist = anActiveGUIlist;
}
int get_lastActiveGUIlistIndex_HAL() {
  return lastActiveGUIlistIndex;
}
void set_lastActiveGUIlistIndex_HAL(int aGUIlistIndex) {
  lastActiveGUIlistIndex = aGUIlistIndex;
}

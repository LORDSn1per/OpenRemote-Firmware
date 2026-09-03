// PlatformIO entry point. The firmware itself lives in the .ino one level up,
// matching the layout of the OpenRemote remote firmware project so the two
// feel the same to work on. PlatformIO compiles this file; the .ino is pulled
// in wholesale below.
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Preferences.h>

#include "../OpenRemote_Dock.ino"

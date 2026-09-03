#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

// PlatformIO's Library Dependency Finder does not scan into a relatively
// included .ino, so every library used by Sensor_Test.ino has to be named here
// as well or it will not be linked. Same pattern as the main firmware's
// pio_src/main.cpp - see the SD.h build failure this fixes.

void drawPage(uint8_t newPage);

#include "../Sensor_Test.ino"

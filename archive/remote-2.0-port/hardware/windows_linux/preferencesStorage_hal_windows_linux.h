#pragma once

#include <string>

void init_preferences_HAL(void);
void save_preferences_HAL(void);

bool get_debugFPSEnabled_HAL();
void set_debugFPSEnabled_HAL(bool aDebugFPSEnabled);
bool get_debugTouchEnabled_HAL();
void set_debugTouchEnabled_HAL(bool aDebugTouchEnabled);

std::string get_activeScene_HAL();
void set_activeScene_HAL(std::string anActiveScene);
std::string get_activeGUIname_HAL();
void set_activeGUIname_HAL(std::string anActiveGUIname);
int get_activeGUIlist_HAL();
void set_activeGUIlist_HAL(int anActiveGUIlist);
int get_lastActiveGUIlistIndex_HAL();
void set_lastActiveGUIlistIndex_HAL(int aGUIlistIndex);

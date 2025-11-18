#pragma once
#include <vector>
#include "WebPortal.h"

void joystick_init();
void joystick_run_macro(const std::vector<MacroStep>& sequence, bool isRunning);
bool joystick_is_connected();
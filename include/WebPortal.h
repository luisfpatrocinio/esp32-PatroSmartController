#pragma once
#include <vector>
#include <Arduino.h>

struct MacroStep
{
    int button;
    int duration;
};

void web_init();
void web_stop();
void web_loop();
std::vector<MacroStep> web_get_sequence();
void web_load_default_sequence();
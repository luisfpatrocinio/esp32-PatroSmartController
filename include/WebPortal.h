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

// --- Wi-Fi Management Functions ---
// Attempts to load saved credentials and connect to a station Wi-Fi network.
void web_init_sta_mode();
// Disconnects from station mode.
void web_stop_sta_mode();
// Helper to check if the ESP32's Access Point is currently active.
bool web_is_in_ap_mode();
// Helper to check if the ESP32 is currently connected to an external Wi-Fi station.
bool web_is_in_sta_mode();

// --- Generic Server Functions ---
// Starts the web server (registers routes, calls begin())
void web_server_start();
// Processes only HTTP client requests (for STA mode)
void web_server_loop();
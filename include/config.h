#pragma once

// --- Hardware Definitions ---
// Use pins with internal PULLUP if possible
constexpr int BTN_MODE_PIN = 18;   // Button 1: Toggle Modes (Bluetooth <-> Config)
constexpr int BTN_ACTION_PIN = 19; // Button 2: Start/Stop or Save
constexpr int LED_PIN = 2;         // Onboard LED

// --- Wi-Fi Settings ---
constexpr char AP_SSID[] = "PatroSmart_Config";
// Leave empty for open network
constexpr char AP_PASS[] = ""; 

// --- Constants ---
constexpr int DEBOUNCE_DELAY = 50; // ms
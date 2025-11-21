#pragma once

// --- Hardware Definitions ---
constexpr int BTN_MODE_PIN = 18;   // Button 1: Toggles Modes
constexpr int BTN_ACTION_PIN = 19; // Button 2: Action
constexpr int LED_PIN = 2;         // Onboard LED

// --- Wi-Fi Configuration ---
constexpr char AP_SSID[] = "PatroSmart_Config";
constexpr char AP_PASS[] = ""; // Open network

// --- Constants ---
constexpr int DEBOUNCE_DELAY = 50; // ms

// --- NVS Keys for Preferences ---
constexpr const char* PREFERENCES_NAMESPACE_GENERAL = "patro_config"; // Namespace for macro
constexpr const char* PREFERENCES_NAMESPACE_WIFI = "patro_wifi";      // New namespace for Wi-Fi credentials
constexpr const char* MACRO_KEY = "macro_seq";                        // Key for macro
constexpr const char* WIFI_SSID_KEY = "wifi_ssid";                    // Key for STA SSID
constexpr const char* WIFI_PASS_KEY = "wifi_pass";                    // Key for STA Password
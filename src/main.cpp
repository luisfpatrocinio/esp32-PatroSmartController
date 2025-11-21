#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "InputManager.h"
#include "WebPortal.h"
#include "JoystickController.h"

// New System Modes:
// MODE_BLUETOOTH_IDLE: BLE ready, not running macro, not connected to STA
// MODE_BLUETOOTH_RUNNING: BLE running macro
// MODE_CONFIG_WIFI_AP: ESP32 is in AP mode, hosting the web configuration portal
// MODE_STA_CONNECTED_BLE: ESP32 is connected to an external Wi-Fi network AND BLE is active
enum SystemMode
{
    MODE_BLUETOOTH_IDLE,
    MODE_BLUETOOTH_RUNNING,
    MODE_CONFIG_WIFI_AP,
    MODE_STA_CONNECTED_BLE
};
SystemMode currentMode = MODE_BLUETOOTH_IDLE; // Initial state, will be updated in setup

void setup()
{
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    input_init();

    // Load macro and Wi-Fi credentials on boot
    web_load_default_sequence(); // Loads the saved macro from NVS

    // Attempt to connect to a saved external Wi-Fi network (Station Mode)
    web_init_sta_mode();

    // Wait briefly for STA connection to establish if credentials were valid
    unsigned long sta_connect_start = millis();
    while (!web_is_in_sta_mode() && millis() - sta_connect_start < 5000)
    { // Try for 5 seconds
        delay(100);
        Serial.print(".");
    }

    // Decide initial system mode based on STA connection attempt
    if (web_is_in_sta_mode())
    {
        currentMode = MODE_STA_CONNECTED_BLE;
        Serial.printf("\nStarting in STA-connected BLE mode. IP: %s\n", WiFi.localIP().toString().c_str());
        // Se conectou ao Wi-Fi, inicie o servidor web permanente.
        web_server_start();
    }
    else
    {
        currentMode = MODE_BLUETOOTH_IDLE;
        Serial.println("\nStarting in BLE (idle) mode. No STA connection or failed.");
    }

    joystick_init(); // Initialize BLE Gamepad
    Serial.println("--- PatroSmartController Initialized ---");
}

void loop()
{
    bool btnMode = input_is_mode_btn_pressed();
    bool btnAction = input_is_action_btn_pressed();

    switch (currentMode)
    {
    case MODE_BLUETOOTH_IDLE:
        digitalWrite(LED_PIN, LOW); // LED off
        if (btnAction)
        {
            currentMode = MODE_BLUETOOTH_RUNNING;
        }
        if (btnMode)
        {
            // Transition to Wi-Fi AP Config Mode
            web_init(); // This starts the ESP32's AP and web server
            currentMode = MODE_CONFIG_WIFI_AP;
        }
        break;

    case MODE_BLUETOOTH_RUNNING:
        if ((millis() / 500) % 2 == 0)
            digitalWrite(LED_PIN, HIGH);
        else
            digitalWrite(LED_PIN, LOW); // Slow blink to indicate macro running

        joystick_run_macro(web_get_sequence(), true); // Execute macro

        if (btnAction)
        {
            currentMode = MODE_BLUETOOTH_IDLE;
        } // Stop macro
        break;

    case MODE_CONFIG_WIFI_AP: // ESP32 is in Access Point mode, serving config pages
        if ((millis() / 100) % 2 == 0)
            digitalWrite(LED_PIN, HIGH);
        else
            digitalWrite(LED_PIN, LOW); // Fast blink for config mode

        web_loop(); // Process web server requests (DNS, HTTP)

        if (btnMode)
        {
            // Exit Config Mode
            web_stop(); // Stop AP and web server. Also disconnects STA if connected.

            // After exiting config, check if STA is now connected.
            if (web_is_in_sta_mode())
            { // Check if a station connection was established
                currentMode = MODE_STA_CONNECTED_BLE;
            }
            else
            {
                currentMode = MODE_BLUETOOTH_IDLE;
            }
            Serial.printf("Exiting config mode. New state: %s\n", (currentMode == MODE_STA_CONNECTED_BLE ? "STA_CONNECTED_BLE" : "BLUETOOTH_IDLE"));
        }
        break;

    case MODE_STA_CONNECTED_BLE:     // ESP32 is connected to an external Wi-Fi network AND BLE is active
        digitalWrite(LED_PIN, HIGH); // LED solid ON to indicate STA connection

        // Run web server loop to handle any incoming requests
        web_server_loop();

        // Allow transition to Config Mode (AP) from STA mode
        if (btnMode)
        {
            Serial.println("Entering config mode from STA. Disconnecting STA and starting AP...");
            web_stop_sta_mode(); // Explicitly disconnect STA
            web_init();          // Start the AP and web server for config
            currentMode = MODE_CONFIG_WIFI_AP;
        }

        // Allow starting/stopping macro
        if (btnAction)
        {
            currentMode = MODE_BLUETOOTH_RUNNING;
        }
        break;
    }
    delay(10); // Small delay to prevent watchdog timer from resetting the ESP32
}
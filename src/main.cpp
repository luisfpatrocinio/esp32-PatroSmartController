#include <Arduino.h>
#include "config.h"
#include "InputManager.h"
#include "WebPortal.h"
#include "JoystickController.h"

enum SystemMode
{
    MODE_BLUETOOTH_IDLE,
    MODE_BLUETOOTH_RUNNING,
    MODE_CONFIG_WIFI
};
SystemMode currentMode = MODE_BLUETOOTH_IDLE;

void setup()
{
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    input_init();
    joystick_init();
    Serial.println("--- PatroSmartController Iniciado ---");
}

void loop()
{
    bool btnMode = input_is_mode_btn_pressed();
    bool btnAction = input_is_action_btn_pressed();

    switch (currentMode)
    {
    case MODE_BLUETOOTH_IDLE:
        digitalWrite(LED_PIN, LOW);
        if (btnAction)
        {
            currentMode = MODE_BLUETOOTH_RUNNING;
        }
        if (btnMode)
        {
            web_init();
            currentMode = MODE_CONFIG_WIFI;
        }
        break;

    case MODE_BLUETOOTH_RUNNING:
        if ((millis() / 500) % 2 == 0)
            digitalWrite(LED_PIN, HIGH);
        else
            digitalWrite(LED_PIN, LOW);

        joystick_run_macro(web_get_sequence(), true);

        if (btnAction)
        {
            currentMode = MODE_BLUETOOTH_IDLE;
        }
        break;

    case MODE_CONFIG_WIFI:
        if ((millis() / 100) % 2 == 0)
            digitalWrite(LED_PIN, HIGH);
        else
            digitalWrite(LED_PIN, LOW);

        web_loop();

        if (btnMode)
        {
            web_stop();
            currentMode = MODE_BLUETOOTH_IDLE;
        }
        break;
    }
    delay(10);
}
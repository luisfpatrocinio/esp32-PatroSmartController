#include <Arduino.h>
#include "InputManager.h"
#include "config.h"

void input_init()
{
    pinMode(BTN_MODE_PIN, INPUT_PULLUP);
    pinMode(BTN_ACTION_PIN, INPUT_PULLUP);
}

bool read_button(int pin)
{
    if (digitalRead(pin) == LOW)
    {
        delay(DEBOUNCE_DELAY);
        if (digitalRead(pin) == LOW)
            return true;
    }
    return false;
}

bool input_is_mode_btn_pressed()
{
    static bool lastState = false;
    bool currentState = read_button(BTN_MODE_PIN);
    if (currentState && !lastState)
    {
        lastState = true;
        return true;
    }
    else if (!currentState)
    {
        lastState = false;
    }
    return false;
}

bool input_is_action_btn_pressed()
{
    static bool lastState = false;
    bool currentState = read_button(BTN_ACTION_PIN);
    if (currentState && !lastState)
    {
        lastState = true;
        return true;
    }
    else if (!currentState)
    {
        lastState = false;
    }
    return false;
}
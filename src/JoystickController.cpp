#include "JoystickController.h"
#include <BleGamepad.h>
#include <Arduino.h>

BleGamepad bleGamepad("PatroSmartController", "LFP", 100);

void joystick_init() { bleGamepad.begin(); }
bool joystick_is_connected() { return bleGamepad.isConnected(); }

void joystick_run_macro(const std::vector<MacroStep> &sequence, bool isRunning)
{
    static int stepIndex = 0;
    static unsigned long lastStepTime = 0;
    static bool isPressing = false;

    if (!isRunning || !bleGamepad.isConnected() || sequence.empty())
    {
        isPressing = false;
        stepIndex = 0;
        return;
    }

    unsigned long now = millis();
    MacroStep currentStep = sequence[stepIndex];

    if (!isPressing)
    {
        bleGamepad.press(currentStep.button);
        isPressing = true;
        lastStepTime = now;
    }
    else
    {
        if (now - lastStepTime >= currentStep.duration)
        {
            bleGamepad.release(currentStep.button);
            isPressing = false;
            lastStepTime = now;
            stepIndex++;
            if (stepIndex >= sequence.size())
                stepIndex = 0;
            delay(30);
        }
    }
}
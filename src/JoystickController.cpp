#include "JoystickController.h"
#include <BleGamepad.h>
#include <Arduino.h>

BleGamepad bleGamepad("PatroSmartController", "LFP", 100);

void joystick_init() { bleGamepad.begin(); }
bool joystick_is_connected() { return bleGamepad.isConnected(); }

void joystick_run_macro(const std::vector<MacroStep> &sequence, bool isRunning)
{
    // Macro machine states
    enum class MacroState
    {
        IDLE,
        PRESSING,
        WAITING_BETWEEN_STEPS
    };

    static MacroState state = MacroState::IDLE;
    static int stepIndex = 0;
    static unsigned long stateStartTime = 0;
    const int DELAY_BETWEEN_STEPS_MS = 50; // Tempo de espera entre os passos da macro

    // Condições de reset: macro pausada, desconectado, ou sequência vazia
    if (!isRunning || !bleGamepad.isConnected() || sequence.empty())
    {
        if (state != MacroState::IDLE)
        {
            // A biblioteca não tem releaseAll(), então liberamos os botões manualmente.
            // Para este projeto, apenas o botão atual estaria pressionado,
            // mas por segurança, podemos liberar todos os botões usados.
            for (const auto &step : sequence)
            {
                bleGamepad.release(step.button);
            }
            state = MacroState::IDLE;
            stepIndex = 0;
        }
        return;
    }

    unsigned long now = millis();
    MacroStep currentStep = sequence[stepIndex];

    switch (state)
    {
    case MacroState::IDLE:
        // Inicia o primeiro passo
        bleGamepad.press(currentStep.button);
        state = MacroState::PRESSING;
        stateStartTime = now;
        break;

    case MacroState::PRESSING:
        // Verifica se o tempo de pressionamento já passou
        if (now - stateStartTime >= currentStep.duration)
        {
            bleGamepad.release(currentStep.button);
            state = MacroState::WAITING_BETWEEN_STEPS;
            stateStartTime = now;
        }
        break;

    case MacroState::WAITING_BETWEEN_STEPS:
        // Verifica se o tempo de espera entre os passos já passou
        if (now - stateStartTime >= DELAY_BETWEEN_STEPS_MS)
        {
            // Avança para o próximo passo
            stepIndex++;
            if (stepIndex >= sequence.size())
            {
                stepIndex = 0; // Reinicia a macro
            }
            // Prepara para pressionar o próximo botão no próximo ciclo
            state = MacroState::IDLE;
        }
        break;
    }
}
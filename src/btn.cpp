#include <btn.h>

BTN::BTN() {}

void BTN::begin(MCU* mcu) {
    pinMode(PIN_RESET_BUTTON, INPUT_PULLUP);
    _mcu = mcu;
}

void BTN::onSinglePress(
    std::function<void()> onSinglePress
) {
    bool isPressed = digitalRead(PIN_RESET_BUTTON) == LOW;

    if (isPressed && !_wasPressed) {
        _pressStartTime = millis();
        _wasPressed = true;
    }

    if (!isPressed && _wasPressed) {
        unsigned long pressDuration = millis() - _pressStartTime;

        if (pressDuration < RESET_BUTTON_PRESS_TIMEOUT) {
            onSinglePress();
        }

        _wasPressed = false;
    }
}

void BTN::onLongPress(
    std::function<void()> onLongPress
) {
    bool isPressed = digitalRead(PIN_RESET_BUTTON) == LOW;
    if (isPressed && !_wasPressed) {
        _pressStartTime = millis();
        _wasPressed = true;
    }
    if (!isPressed && _wasPressed) {
        _wasPressed = false;
    }
    if (isPressed && (millis() - _pressStartTime >= RESET_BUTTON_PRESS_TIMEOUT)) {
        onLongPress();
        _wasPressed = false;
    }
}

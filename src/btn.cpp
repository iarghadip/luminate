#include <btn.h>

BTN::BTN() {}

void BTN::begin(
    MCU* mcu
) {
    _mcu = mcu;
    _mcu->log("BTN::begin(): Initializing BTN...");
    pinMode(PIN_BUTTON_RESET, INPUT_PULLUP);
    pinMode(PIN_SWITCH_DLS, INPUT_PULLUP);
    _wasEnabled = digitalRead(PIN_SWITCH_DLS) == LOW;
}

bool BTN::isToogleEnabled() {
    return _wasEnabled;
}

void BTN::onToggle(
    std::function<void(bool status)> onToggle
) {
    bool isEnabled = digitalRead(PIN_SWITCH_DLS) == LOW;
    if (isEnabled != _wasEnabled) {
        _wasEnabled = isEnabled;
        onToggle(isEnabled);
    }
}

void BTN::onSinglePress(
    std::function<void()> onSinglePress
) {
    bool isPressed = digitalRead(PIN_BUTTON_RESET) == LOW;
    if (isPressed && !_wasPressed) {
        _pressStartTime = millis();
        _wasPressed = true;
    }
    if (!isPressed && _wasPressed) {
        unsigned long pressDuration = millis() - _pressStartTime;
        if (pressDuration < RESET_BUTTON_PRESS_TIMEOUT) {
            _mcu->log("BTN::onSinglePress(): Button was pressed.");
            onSinglePress();
        }
        _wasPressed = false;
    }
}

void BTN::onLongPress(
    std::function<void()> onLongPress
) {
    bool isPressed = digitalRead(PIN_BUTTON_RESET) == LOW;
    if (isPressed && !_wasPressed) {
        _pressStartTime = millis();
        _wasPressed = true;
    }
    if (!isPressed && _wasPressed) {
        _wasPressed = false;
    }
    if (isPressed && (millis() - _pressStartTime >= RESET_BUTTON_PRESS_TIMEOUT)) {
        _mcu->log("BTN::onLongPress(): Button was long pressed.");
        onLongPress();
        _wasPressed = false;
    }
}

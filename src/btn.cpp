#include <btn.h>

/**
 * @brief Default constructor.
 */
BTN::BTN() {}

/**
 * @brief Initializes the button (on GPIO 0).
 */
void BTN::begin() {
    pinMode(PIN_RESET_BUTTON, INPUT_PULLUP);
}

/**
 * @brief Monitors the button and calls the callback on a short (single) press.
 * 
 * This function triggers the callback only when the button is released quickly 
 * (i.e., within the long-press timeout threshold).
 * 
 * @param onSinglePress Function to call on a short press.
 */
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

/**
 * @brief Monitors the button and calls the callback if held for longer than 5 seconds.
 * 
 * @param onLongPress Function to call on a long press.
 */
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
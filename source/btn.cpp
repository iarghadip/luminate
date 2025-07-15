#include <btn.h>

/**
 * @brief The BTN class represents a push button with long press and short press detection.
 * 
 * This class allows the user to configure callbacks for single (short) press and long press events.
 * It works with a provided MCU object for hardware abstraction.
 */
BTN::BTN() {}

/**
 * @brief Initializes the button and associates it with an MCU instance.
 * 
 * This method sets up the button GPIO using the MCU class and prepares internal state.
 * 
 * @param mcu Pointer to an MCU instance that provides hardware-level access.
 */
void BTN::begin(
    MCU* mcu
) {
    _mcu = mcu;
    _mcu->log("BTN::begin(): Initializing BTN...");
    pinMode(PIN_BUTTON_RESET, INPUT_PULLUP);
    pinMode(PIN_SWITCH_DLS, INPUT_PULLUP);
}

/**
 * @brief Monitors the toggle switch and invokes a callback on state change.
 *
 * This method continuously checks the state of the toggle switch connected to PIN_SWITCH_DLS.
 * When the switch state changes (from ON to OFF or OFF to ON), the provided callback function
 * is called with the new state.
 *
 * @param onToggle A callback function that receives the current state of the switch.
 *                 - true:  Switch is ON (circuit closed, pin LOW)
 *                 - false: Switch is OFF (circuit open, pin HIGH)
 *
 * @note This function should be called repeatedly (e.g., in the main loop) to detect state changes.
 */
void BTN::onToggle(
    std::function<void(bool status)> onToggle
) {
    bool isEnabled = digitalRead(PIN_SWITCH_DLS) == LOW;
    if (!_wasInitialized || isEnabled != _wasEnabled) {
        _wasInitialized = true;
        _wasEnabled = isEnabled;
        _mcu->log("BTN::onToggle(): Switch was toggled.");
        _mcu->log("BTN::onToggle(): isEnabled: " + _sBool(isEnabled));
        onToggle(isEnabled);
    }
}

/**
 * @brief Registers a callback to be executed on a short button press.
 * 
 * A short press is considered when the button is pressed and released 
 * within the long-press timeout (e.g., less than 5 seconds).
 * 
 * @param onSinglePress Callback function to invoke on a short press event.
 */
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

/**
 * @brief Registers a callback to be executed on a long button press.
 * 
 * A long press is detected when the button remains pressed for more than 5 seconds.
 * 
 * @param onLongPress Callback function to invoke on a long press event.
 */
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

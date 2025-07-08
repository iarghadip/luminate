#include <led.h>

/**
 * @brief Constructs a LED object.
 * 
 * Initializes internal brightness tracking and prepares the object for configuration.
 */
LED::LED() {}

/**
 * @brief Initializes LED control pins and configures PWM channels.
 * 
 * This should be called in the main setup routine. It sets the GPIO pins
 * for output and assigns each pin as its own PWM channel for simplicity.
 * 
 * @param mcu Pointer to the MCU instance that abstracts hardware I/O.
 */
void LED::begin(
    MCU* mcu
) {
    _mcu = mcu;
    _mcu->log("LED::begin(): Initializing LED...");
    pinMode(PIN_LED_WSL, OUTPUT);
    pinMode(PIN_LED_COOL, OUTPUT);
    pinMode(PIN_LED_WARM, OUTPUT);
    ledcSetup(PWM_CHANNEL_LED_COOL, PWM_FREQUENCY, PWM_RESOLUTION_BITS);
    ledcSetup(PWM_CHANNEL_LED_WARM, PWM_FREQUENCY, PWM_RESOLUTION_BITS);
    ledcAttachPin(PIN_LED_COOL, PWM_CHANNEL_LED_COOL);
    ledcAttachPin(PIN_LED_WARM, PWM_CHANNEL_LED_WARM);
    renderLumination(0.0f, 0.0f);
}

/**
 * @brief Sets the Wi-Fi status LED pin to HIGH or LOW.
 * 
 * @param value Digital output value (`HIGH` or `LOW`).
 */
void LED::toggleWSL(
    uint8_t value
) {
    digitalWrite(PIN_LED_WSL, value);
}

/**
 * @brief Smoothly transitions the cool and warm LEDs to new brightness levels.
 * 
 * This function computes a series of steps between the current and target brightness
 * values and updates the PWM output for both cool and warm LED channels frame-by-frame,
 * creating a smooth fading effect.
 * 
 * @param coolLED Target brightness percentage for the cool white LED (0–100).
 * @param warmLED Target brightness percentage for the warm white LED (0–100).
 * 
 * @note This method blocks execution while the transition occurs. It is best used in
 *       task/thread contexts where blocking is acceptable.
 */
void LED::renderLumination(
    float coolLED,
    float warmLED
) {
    int totalSteps = max(
        abs(_oldCoolBrightness - coolLED),
        abs(_oldWarmBrightness - warmLED)
    );
    if (totalSteps > 0) {
        _mcu->log("LED::renderLumination(): Updating light lumination...");
        _mcu->log("LED::renderLumination(): coolLED: " + String(coolLED));
        _mcu->log("LED::renderLumination(): warmLED: " + String(warmLED));
        _mcu->log("LED::renderLumination(): _oldCoolBrightness: " + String(_oldCoolBrightness));
        _mcu->log("LED::renderLumination(): _oldWarmBrightness: " + String(_oldWarmBrightness));
        for (int currentStep = 0; currentStep <= totalSteps; currentStep++) {
            _luminate(
                PWM_CHANNEL_LED_COOL,
                _oldCoolBrightness,
                coolLED,
                currentStep,
                totalSteps
            );
            _luminate(
                PWM_CHANNEL_LED_WARM,
                _oldWarmBrightness,
                warmLED,
                currentStep,
                totalSteps
            );
            delay(GENERAL_SIO_INTERVAL);
        }
        _oldCoolBrightness = coolLED;
        _oldWarmBrightness = warmLED;
    }
}

/**
 * @brief Applies brightness to a given LED using PWM.
 * 
 * Converts the brightness percentage (0–100) into an 8-bit PWM value
 * and writes it to the specified pin/channel using `ledcWrite()`.
 * 
 * @param lightPin The GPIO pin (also used as the PWM channel).
 * @param percentage Brightness percentage (0–100).
 */
void LED::_luminate(
    int pwmChannel,
    float oldBrightness,
    float newBrightness,
    int currentStep,
    int totalSteps
) {
    ledcWrite(
        pwmChannel,
        static_cast<uint8_t>(
            (constrain(
                oldBrightness +
                (newBrightness - oldBrightness) *
                ((float)currentStep / totalSteps),
                0.0f,
                100.0f
            ) / 100.0f) * 255.0f
        )
    );
}

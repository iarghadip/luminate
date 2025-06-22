#include <led.h>

/**
 * @brief Constructs a LED object.
 * 
 * Initializes the LED object and prepares it for use.
 */
LED::LED() {}

/**
 * @brief Initializes the light pins and configures PWM for brightness control.
 * 
 * Call this method during setup() to prepare the led for use. It configures
 * the GPIO pins as outputs and attaches them to PWM channels using the same 
 * pin number as the channel ID (for simplicity).
 */
void LED::begin() {
    pinMode(PIN_LED_COOL, OUTPUT);
    pinMode(PIN_LED_WARM, OUTPUT);
    ledcSetup(PWM_CHANNEL_LED_COOL, PWM_FREQUENCY, PWM_RESOLUTION_BITS);
    ledcSetup(PWM_CHANNEL_LED_WARM, PWM_FREQUENCY, PWM_RESOLUTION_BITS);
    ledcAttachPin(PIN_LED_COOL, PWM_CHANNEL_LED_COOL);
    ledcAttachPin(PIN_LED_WARM, PWM_CHANNEL_LED_WARM);
}

/**
 * @brief Smoothly transitions the cool and warm LEDs to the specified brightness levels.
 * 
 * This function gradually interpolates the brightness of the cool and warm white LEDs
 * from their previous values to the new target values. The transition is done over a 
 * number of steps determined by the maximum change in brightness between the old and new 
 * levels. PWM is used to adjust the brightness of each LED channel incrementally.
 * 
 * The function ensures a smooth visual transition by updating the LED states frame-by-frame
 * with a fixed delay (`BRIGHTNESS_RENDER_INTERVAL`) between each step.
 * 
 * @param coolLED Target brightness for the cool white LED (range: 0–100).
 * @param warmLED Target brightness for the warm white LED (range: 0–100).
 * 
 * @note The function blocks during the transition and should be called from within 
 *       a task context where blocking is acceptable.
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
        for (int currentStep = 0; currentStep <= totalSteps; currentStep++) {
            _luminate(
                PWM_CHANNEL_LED_COOL,
                _lerp(
                    _oldCoolBrightness,
                    coolLED,
                    currentStep,
                    totalSteps
                )
            );
            _luminate(
                PWM_CHANNEL_LED_WARM,
                _lerp(
                    _oldWarmBrightness,
                    warmLED,
                    currentStep,
                    totalSteps
                )
            );
            delay(BRIGHTNESS_RENDER_INTERVAL);
        }
        _oldCoolBrightness = coolLED;
        _oldWarmBrightness = warmLED;
    }
}

/**
 * @brief Linearly interpolates between two values.
 * 
 * @param from The starting value.
 * @param to The target value.
 * @param step Current step number (0 to steps).
 * @param steps Total number of steps.
 * @return Interpolated float value.
 */
inline float LED::_lerp(
    float from,
    float to,
    int step,
    int steps
) {
    return from + (to - from) * ((float)step / steps);
}

/**
 * @brief Sets the PWM duty cycle for the specified light channel.
 * 
 * Converts a brightness percentage (0–100) to an 8-bit PWM duty cycle and applies 
 * it using @c ledcWrite.
 * 
 * @param lightChannel The PWM channel (same as GPIO pin in this setup).
 * @param percentage Brightness percentage (0–100).
 */
void LED::_luminate(
    int lightChannel,
    float percentage
) {
    ledcWrite(
        lightChannel,
        static_cast<uint8_t>((
            constrain(percentage, 0.0f, 100.0f) / 100.0f
        ) * 255.0f)
    );
}
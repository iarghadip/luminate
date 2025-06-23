#include <led.h>

LED::LED() {}

void LED::begin(
    MCU* mcu
) {
    pinMode(PIN_LED_COOL, OUTPUT);
    pinMode(PIN_LED_WARM, OUTPUT);
    ledcSetup(PWM_CHANNEL_LED_COOL, PWM_FREQUENCY, PWM_RESOLUTION_BITS);
    ledcSetup(PWM_CHANNEL_LED_WARM, PWM_FREQUENCY, PWM_RESOLUTION_BITS);
    ledcAttachPin(PIN_LED_COOL, PWM_CHANNEL_LED_COOL);
    ledcAttachPin(PIN_LED_WARM, PWM_CHANNEL_LED_WARM);
    _mcu = mcu;
}

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

inline float LED::_lerp(
    float from,
    float to,
    int step,
    int steps
) {
    return from + (to - from) * ((float)step / steps);
}

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

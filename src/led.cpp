#include <led.h>

LED::LED() {}

void LED::begin(
    MCU* mcu
) {
    _mcu = mcu;
    _mcu->log("LED::begin(): Initializing LED...");
    pinMode(PIN_LED_COOL, OUTPUT);
    pinMode(PIN_LED_WARM, OUTPUT);
    ledcSetup(PWM_CHANNEL_LED_COOL, PWM_FREQUENCY, PWM_RESOLUTION_BITS);
    ledcSetup(PWM_CHANNEL_LED_WARM, PWM_FREQUENCY, PWM_RESOLUTION_BITS);
    ledcAttachPin(PIN_LED_COOL, PWM_CHANNEL_LED_COOL);
    ledcAttachPin(PIN_LED_WARM, PWM_CHANNEL_LED_WARM);
    renderLumination(0.0f, 0.0f);
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

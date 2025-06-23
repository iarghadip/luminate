#include <fan.h>

FAN::FAN() {}

void FAN::begin(
    MCU* mcu
) {
    pinMode(PIN_BLDC_FAN, OUTPUT);
    ledcSetup(PWM_CHANNEL_BLDC_FAN, PWM_FREQUENCY, PWM_RESOLUTION_BITS);
    ledcAttachPin(PIN_BLDC_FAN, PWM_CHANNEL_BLDC_FAN);
    _mcu = mcu;
}

void FAN::adjust(
    float temperature
) {
    ledcWrite(
        PWM_CHANNEL_BLDC_FAN,
        static_cast<uint8_t>((
            constrain((
                ((temperature - 25.0f) / 75.0f) * 100.0f
            ), 0.0f, 100.0f) / 100.0f
        ) * 255.0f)
    );
}

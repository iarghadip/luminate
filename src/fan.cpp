#include <fan.h>

FAN::FAN() {}

void FAN::begin(
    MCU* mcu
) {
    _mcu = mcu;
    _mcu->log("FAN::begin(): Initializing FAN...");
    pinMode(PIN_BLDC_FAN, OUTPUT);
    ledcSetup(PWM_CHANNEL_BLDC_FAN, PWM_FREQUENCY, PWM_RESOLUTION_BITS);
    ledcAttachPin(PIN_BLDC_FAN, PWM_CHANNEL_BLDC_FAN);
    _mcu->log("FAN::begin(): Initialization completed.");
}

void FAN::adjust(
    float temperature
) {
    _mcu->log("FAN::adjust(): Adjusting fan speed...");
    float percentage = constrain((temperature - 25.0f) / 10.0f, 0.0f, 1.0f);
    uint32_t duty = static_cast<uint32_t>(percentage * 255.0f);
    _mcu->log("FAN::adjust(): temperature: " + String(temperature));
    _mcu->log("FAN::adjust(): percentage: " + String(percentage));
    _mcu->log("FAN::adjust(): duty: " + String(duty));
    ledcWrite(PWM_CHANNEL_BLDC_FAN, duty);
    _mcu->log("FAN::adjust(): duty was updated.");
}

#include <fan.h>

/**
 * @brief Constructs a FAN object.
 * 
 * Initializes the FAN object and prepares it for use.
 */
FAN::FAN() {}

/**
 * @brief Initializes the GPIO pin(s) used by the fan.
 * 
 * Call this function in your setup routine to configure the fan's GPIO pins.
 */
void FAN::begin() {
    pinMode(PIN_BLDC_FAN, OUTPUT);
    ledcSetup(PWM_CHANNEL_BLDC_FAN, PWM_FREQUENCY, PWM_RESOLUTION_BITS);
    ledcAttachPin(PIN_BLDC_FAN, PWM_CHANNEL_BLDC_FAN);
}

/**
 * @brief Adjust fan speed based on temperature.
 * 
 * Below 25°C: fan off.  
 * From 25°C to 100°C: PWM increases linearly from 0% to 100%.
 * 
 * @param temperature Temperature in °C, expected from -100 to 100.
 */
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
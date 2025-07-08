#include <fan.h>

/**
 * @brief Constructs a FAN object with default internal state.
 * 
 * Initializes the object but does not configure hardware. 
 * Call begin() to initialize the GPIO.
 */
FAN::FAN() {}

/**
 * @brief Initializes the GPIO pin(s) used by the fan.
 * 
 * This function should be called during setup. It configures the necessary 
 * GPIO using the provided MCU instance for output control.
 * 
 * @param mcu Pointer to the MCU abstraction providing hardware access.
 */
void FAN::begin(
    MCU* mcu
) {
    _mcu = mcu;
    _mcu->log("FAN::begin(): Initializing FAN...");
    pinMode(PIN_BLDC_FAN, OUTPUT);
    ledcSetup(PWM_CHANNEL_BLDC_FAN, PWM_FREQUENCY, PWM_RESOLUTION_BITS);
    ledcAttachPin(PIN_BLDC_FAN, PWM_CHANNEL_BLDC_FAN);
}

/**
 * @brief Adjusts fan speed based on the input temperature.
 * 
 * Behavior:
 * - Below 25°C: Fan remains off.
 * - Between 25°C and 100°C: PWM duty cycle increases linearly from 0% to 100%.
 * - Above 100°C: Fan remains at 100%.
 * 
 * @param temperature Current temperature in degrees Celsius (-100°C to 100°C expected).
 */
void FAN::adjust(
    float temperature
) {
    _mcu->log("FAN::adjust(): Updating fan speed...");
    float percentage = constrain((temperature - 25.0f) / 10.0f, 0.0f, 1.0f);
    uint32_t duty = static_cast<uint32_t>(percentage * 255.0f);
    _mcu->log("FAN::adjust(): temperature: " + String(temperature));
    _mcu->log("FAN::adjust(): percentage: " + String(percentage * 100.0f));
    _mcu->log("FAN::adjust(): duty: " + String(duty));
    ledcWrite(PWM_CHANNEL_BLDC_FAN, duty);
}

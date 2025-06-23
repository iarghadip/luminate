#ifndef fan_h
#define fan_h

// Libraries from module wrappers
#include <mcu.h>

/**
 * @class FAN
 * @brief Controls a GPIO-based fan based on temperature input.
 * 
 * The FAN class manages a fan (or similar cooling device) through GPIO using an MCU wrapper.
 * It allows initialization and dynamic speed adjustment using a linear PWM profile based on temperature.
 */
class FAN {
    public:
        /**
         * @brief Constructs a FAN object with default internal state.
         * 
         * Initializes the object but does not configure hardware. 
         * Call begin() to initialize the GPIO.
         */
        FAN();

        /**
         * @brief Initializes the GPIO pin(s) used by the fan.
         * 
         * This function should be called during setup. It configures the necessary 
         * GPIO using the provided MCU instance for output control.
         * 
         * @param mcu Pointer to the MCU abstraction providing hardware access.
         */
        void begin(
            MCU* mcu
        );

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
        void adjust(
            float temperature
        );

    private:
        MCU* _mcu; // Pointer to the MCU instance controlling the fan GPIO/PWM.
};

#endif // fan_h

#ifndef fan_h
#define fan_h

// Libraries from system framework
#include <Arduino.h>

/**
 * @class FAN
 * @brief Represents a cooling device controlled by GPIO pins.
 * 
 * The FAN class manages one or more GPIO-controlled cooling devices. It allows
 * initialization and toggling of the device state based on a given temperature threshold.
 */
class FAN {
    public:
        /**
         * @brief Constructs a FAN object with default pin configuration.
         * 
         * This constructor initializes the internal state of the FAN class.
         */
        FAN();

        /**
         * @brief Initializes the GPIO pin(s) used by the fan.
         * 
         * Call this function in your setup routine to configure the fan's GPIO pins.
         */
        void begin();

        /**
         * @brief Adjust fan speed based on temperature.
         * 
         * Below 25°C: fan off.  
         * From 25°C to 100°C: PWM increases linearly from 0% to 100%.
         * 
         * @param temperature Temperature in °C, expected from -100 to 100.
         */
        void adjust(
            float temperature
        );

    private:
        // No private members
};

#endif ///< fan_h
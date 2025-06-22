#ifndef dls_h
#define dls_h

// Libraries from system framework
#include <Arduino.h>
#include <Wire.h>

// Libraries from module wrappers
#include <mcu.h>

/**
 * @brief Provides an interface to a digital light sensor.
 * 
 * The DLS class allows initializing the sensor, reading brightness levels,
 * and registering callbacks to respond to changes in ambient light.
 */
class DLS {
    public:
        /**
         * @brief Constructs a DLS object.
         * 
         * Prepares the instance for initialization and use.
         */
        DLS();

        /**
         * @brief Initializes the BH1750 digital light sensor.
         * 
         * Sets up I2C communication and starts the sensor in continuous high-resolution mode.
         */
        void begin();

        /**
         * @brief Reads the current ambient brightness level from the BH1750 sensor.
         * 
         * Converts the lux reading to a percentage, where 100 lux corresponds to 100%.
         * The resulting percentage is constrained to a minimum threshold to ensure 
         * that brightness never drops below a specified level.
         * 
         * @param minimumBrightness The lowest allowable brightness percentage (e.g., 5.0).
         * @return Brightness as a percentage (minimumBrightness to 100.0).
         */
        float read(
            float minimumBrightness = 0.0f
        );

        /**
         * @brief Registers a callback function to be called when the brightness changes.
         * 
         * The callback is triggered only when the brightness value differs from the last recorded value.
         * 
         * @param onChange Function that receives the updated brightness percentage.
         */
        void onBrightnessChange(
            std::function<void(float)> onChange
        );
    
    private:
        MCU _mcu; ///< Object representing the mcu wrapper.
        unsigned long _lastRead = 0; ///< Timestamp of the last sensor reading in ms.
        float _oldBrightness = 0.0f; ///< The previously recorded brightness.

        /**
         * @brief Checks if the BH1750 DLS module is connected on the I2C bus.
         * 
         * Initiates an I2C transmission to the device address (0x23) and checks
         * for an acknowledgment (ACK) from the DLS module. If no ACK is received,
         * it prints an error message and returns false.
         * 
         * @note This function does not attempt to read or write any data beyond 
         * the address check. It is useful for confirming device presence during initialization.
         * 
         * @return true if the DLS module is connected and acknowledged on the bus.
         * @return false if the device is not responding or not present.
         */
        bool _isConnected();
};

#endif ///< dls_h
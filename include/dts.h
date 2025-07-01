#ifndef dts_h
#define dts_h

// Libraries from module wrappers
#include <mcu.h>

/**
 * @brief Provides an interface to a digital temperature sensor (LM75A).
 * 
 * The DTS class enables initialization, temperature reading (in degrees Celsius or percentage), and 
 * callback registration for temperature change detection.
 */
class DTS {
    public:
        /**
         * @brief Constructs a DTS object.
         * 
         * Prepares the instance for initialization and use with an LM75A temperature sensor.
         */
        DTS();

        /**
         * @brief Initializes the LM75A digital temperature sensor.
         * 
         * Sets up I2C communication using the provided MCU instance and 
         * prepares the sensor for continuous temperature measurement.
         * 
         * @param mcu Pointer to the MCU instance handling I2C communication.
         */
        void begin(
            MCU* mcu
        );

        /**
         * @brief Reads the current ambient temperature from the sensor.
         * 
         * Reads the temperature value from the LM75A sensor via I2C.
         * The result can be constrained to a minimum threshold to ensure the temperature 
         * does not fall below a defined floor (useful for certain applications).
         * 
         * @param minimumTemperature Minimum allowed temperature value (e.g., 5.0).
         * @return Temperature in degrees Celsius, constrained to [minimumTemperature, 100.0].
         */
        float read(
            float minimumTemperature = 0.0f
        );

        /**
         * @brief Registers a callback to be triggered when the temperature changes.
         * 
         * Compares the current and previously recorded temperature. If different,
         * updates the cached value and invokes the provided callback.
         * 
         * @param onChange Callback function that receives the new temperature value (in degrees Celsius).
         */
        void onTemperatureChange(
            std::function<void(float)> onChange
        );

    private:
        MCU* _mcu; // Pointer to the MCU instance managing.
        unsigned long _lastRead = 0; // Timestamp of the last temperature.
        float _oldTemperature = 0.0f; // Previously recorded temperature value.

        /**
         * @brief Checks whether the LM75A sensor is connected on the I2C bus.
         * 
         * Sends an I2C transmission to the LM75A's address (0x48) to confirm device presence.
         * 
         * @note No data is read or written beyond the address check.
         * 
         * @return true if the sensor responds to the I2C address.
         * @return false if the device is not connected or not responding.
         */
        bool _isConnected();
};

#endif // dts_h

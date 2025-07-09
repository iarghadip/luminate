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
            float minimumTemperature
        );

        /**
         * @brief Registers a callback to be triggered when the temperature changes.
         *
         * This method reads the current temperature and compares it to the previously cached value.
         * If a change in temperature is detected (i.e., the new value differs from the previous one),
         * the cached value is updated and the provided callback function is invoked with the new temperature.
         *
         * @param minimumTemperature The minimum threshold for temperature measurement. Temperature values below this
         *        threshold are ignored.
         * @param onChange Callback function to be called when a temperature change is detected.
         *        The function receives the new temperature value as a float (in degrees Celsius).
         *
         * @note The callback is only triggered if the temperature value has changed since the last invocation.
         * @note This function logs the detected temperature change for debugging purposes.
         *
         * @see read(float minimumTemperature)
         *
         * @code
         * // Example usage:
         * dts.onTemperatureChange(15.0f, [](float newTemp) {
         *     Serial.print("Temperature changed to: ");
         *     Serial.println(newTemp);
         * });
         * @endcode
         */
        void onTemperatureChange(
            float minimumTemperature,
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

        /**
         * @brief Sends a register address or command byte to the DTS temperature sensor over I2C.
         *
         * This function begins an I2C transmission to the device at address 0x48,
         * writes a single byte (typically the register address to be read from),
         * and ends the transmission without a repeated start.
         *
         * It is typically used to select the temperature register (0x00) before initiating a read.
         *
         * @param command The register address or command byte to send to the sensor.
         * @return true if the transmission was acknowledged by the sensor; false otherwise.
         */
        bool _sendCommand(
            uint8_t command
        );
};

#endif // dts_h

#ifndef dls_h
#define dls_h

// Libraries from module wrappers
#include <mcu.h>

/**
 * @brief Provides an interface to a digital light sensor (BH1750).
 * 
 * The DLS class enables initialization, brightness reading (in percentage), and 
 * callback registration for ambient light change detection.
 */
class DLS {
    public:
        /**
         * @brief Constructs a DLS object.
         * 
         * Prepares the instance for initialization and use with a BH1750 sensor.
         */
        DLS();

        /**
         * @brief Initializes the BH1750 digital light sensor.
         * 
         * Sets up I2C communication using the provided MCU instance and 
         * configures the sensor in continuous high-resolution mode.
         * 
         * @param mcu Pointer to the MCU instance handling I2C communication.
         */
        void begin(
            MCU* mcu
        );

        /**
         * @brief Reads the current ambient brightness level from the sensor.
         * 
         * Converts the lux value to a percentage (relative to 100 lux). The result is 
         * constrained to a minimum threshold to ensure the brightness doesn't fall below 
         * a defined floor (useful for dimming applications).
         * 
         * @param minimumBrightness Minimum allowed brightness percentage (e.g., 5.0).
         * @return Brightness as a percentage (from minimumBrightness up to 100.0).
         */
        float read(
            float minimumBrightness = 0.0f
        );

        /**
         * @brief Registers a callback function for brightness change events.
         * 
         * The provided function is invoked only when a change in brightness 
         * is detected compared to the previous value.
         * 
         * @param onChange Callback receiving the updated brightness percentage.
         */
        void onBrightnessChange(
            std::function<void(float)> onChange
        );

    private:
        MCU* _mcu; // Pointer to the MCU instance managing sensor communication.
        unsigned long _lastRead = 0; // Timestamp of the last brightness reading (in milliseconds).
        float _oldBrightness = 0.0f; // Previously recorded brightness percentage.

        /**
         * @brief Checks whether the BH1750 sensor is connected on the I2C bus.
         * 
         * Sends an I2C transmission to the BH1750's address (0x23) to confirm device presence.
         * 
         * @note No data is read or written beyond the address check.
         * 
         * @return true if the sensor responds to the I2C address.
         * @return false if the device is not connected or not responding.
         */
        bool _isConnected();
};

#endif // dls_h

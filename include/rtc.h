#ifndef rtc_h
#define rtc_h

// Libraries from system framework
#include <Arduino.h>
#include <Wire.h>

/**
 * @brief The RTC class represents a DS3231 RTC-based rtc device.
 * 
 * Provides functionalities to calibrate time, retrieve formatted date/time strings,
 * and handle temperature change notifications.
 */
class RTC {
    public:
        /**
         * @brief Enumeration defining the types of formats supported by RTC.
         */
        enum Format {
            DATE_TIME, ///< The output will have both date and time.
            DATE_ONLY, ///< The output will have only date.
            TIME_ONLY ///< The output will have only time.
        };

        /**
         * @brief Default constructor for RTC class.
         * Initializes the rtc object.
         */
        RTC();

        /**
         * @brief Initializes the I2C bus for the RTC module.
         * 
         * Uses predefined pins and frequency constants for SDA, SCL, and bus speed.
         */
        void begin();

        /**
         * @brief Sets the RTC date and time using a datetime string and day of week.
         * 
         * The datetime string should follow the format: "YY-MM-DDTHH:MM:SS".
         * The values are parsed and written to the appropriate DS3231 registers.
         * 
         * @param datetime A String representing the date and time (e.g., "24-06-04T13:45:30").
         * @param dayOfWeek An integer representing the day of the week (1 = Sunday, 7 = Saturday).
         */
        void calibrate(
            String datetime,
            int dayOfWeek
        );

       /**
         * @brief Retrieves the current date and/or time from the RTC.
         * 
         * Reads from the DS3231's time registers and returns a formatted string.
         * Values are cached for performance with a fallback retry mechanism.
         * 
         * @param format The desired format: DATE_TIME, DATE_ONLY, or TIME_ONLY.
         * @return A formatted string with date and/or time, or "NaN" if read fails.
         */
        String read(
            Format format = DATE_TIME
        );

        /**
         * @brief Reads the current temperature from the DS3231 RTC sensor.
         * 
         * Reads two bytes from the temperature registers, combines them,
         * and converts to a floating-point temperature value in Celsius.
         * 
         * @return The temperature in degrees Celsius.
         */
        float temperature();

        /**
         * @brief Calls a callback function when the temperature changes.
         * 
         * Reads the current temperature and compares it to the last known value.
         * If different, updates the stored temperature and invokes the callback.
         * 
         * @param onChange Function to call with the new temperature value.
         */
        void onTemperatureChange(
            std::function<void(float)> onChange
        );

    private:
        struct Cache {
            String date; ///< Cached date.
            String time; ///< Cached time.
            unsigned long msDate; ///< Date cache time.
            unsigned long msTime; ///< Time cache time.
        };

        Cache _cache; ///< Cached datetime.
        float _oldTemperature = 0.0; ///< The previous temperature recorded by the rtc.

        /**
         * @brief Clears the internal date/time cache timestamps.
         * 
         * Resets the millisecond timestamps used for caching the most recent date/time read.
         */
        void _resetCache();

        /**
         * @brief Parses the input byte `w` and returns a formatted string based on the provided range and suffix.
         * 
         * @param w The byte value to be checked and formatted.
         * @param x The lower bound of the range (inclusive).
         * @param y The upper bound of the range (inclusive).
         * @param z The string suffix to append if `w` is within the range.
         * @return A formatted string with a leading zero if `w` is less than 10 and within the range [x, y], followed by `z`.
         *         Returns an empty string if `w` is out of the range.
         */
        String _parse(byte w, byte x, byte y, String z);

        /**
         * @brief Converts a decimal byte value to BCD (Binary-Coded Decimal).
         * 
         * @param val The decimal value (0–99).
         * @return The BCD representation.
         */
        byte _decToBcd(byte val);

        /**
         * @brief Converts a BCD (Binary-Coded Decimal) byte to decimal.
         * 
         * @param val The BCD value.
         * @return The decimal equivalent.
         */
        byte _bcdToDec(byte val);

        /**
         * @brief Checks if the DS3231 RTC module is connected on the I2C bus.
         * 
         * Initiates an I2C transmission to the device address (0x68) and checks
         * for an acknowledgment (ACK) from the RTC module. If no ACK is received,
         * it prints an error message and returns false.
         * 
         * @note This function does not attempt to read or write any data beyond 
         * the address check. It is useful for confirming device presence during initialization.
         * 
         * @return true if the RTC module is connected and acknowledged on the bus.
         * @return false if the device is not responding or not present.
         */
        bool _isConnected();
};

#endif
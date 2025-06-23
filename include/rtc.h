#ifndef rtc_h
#define rtc_h

// Libraries from module wrappers
#include <mcu.h>

/**
 * @class RTC
 * @brief Provides access to a DS3231 real-time clock with time, date, and temperature handling.
 * 
 * The RTC class supports setting and retrieving the current date/time, reading temperature,
 * and notifying the user on temperature change. It also supports formatted output and caching.
 */
class RTC {
    public:
        /**
         * @brief Enumeration for date/time formatting options.
         */
        enum Format {
            DATE_TIME, // Output includes both date and time.
            DATE_ONLY, // Output includes only date.
            TIME_ONLY // Output includes only time.
        };

        /**
         * @brief Default constructor.
         * 
         * Initializes internal state for the RTC interface.
         */
        RTC();

        /**
         * @brief Initializes the I2C bus connection to the DS3231 RTC module.
         * 
         * Uses fixed pin/frequency definitions via the provided MCU instance.
         * 
         * @param mcu Pointer to the MCU abstraction layer handling I2C communication.
         */
        void begin(
            MCU* mcu
        );

        /**
         * @brief Calibrates the RTC with a new datetime and day of the week.
         * 
         * The datetime must follow the format: "YY-MM-DDTHH:MM:SS".
         * The values are parsed and written to the appropriate RTC registers.
         * 
         * @param datetime A datetime string (e.g., "24-06-04T13:45:30").
         * @param dayOfWeek Day of the week (1 = Sunday, 7 = Saturday).
         */
        void calibrate(
            String datetime,
            int dayOfWeek
        );

        /**
         * @brief Retrieves the current date and/or time in formatted string.
         * 
         * Reads from the RTC and formats the output as per the specified format.
         * Uses a caching mechanism for performance and includes retry logic on failure.
         * 
         * @param format One of DATE_TIME, DATE_ONLY, or TIME_ONLY.
         * @return A formatted string or "NaN" if the read fails.
         */
        String read(
            Format format = DATE_TIME
        );

        /**
         * @brief Reads the current temperature from the DS3231's internal sensor.
         * 
         * @return Temperature in degrees Celsius.
         */
        float temperature();

        /**
         * @brief Registers a callback to be triggered when temperature changes.
         * 
         * Compares current and previously recorded temperature. If different,
         * updates the cache and invokes the provided callback.
         * 
         * @param onChange Callback function that receives the new temperature value.
         */
        void onTemperatureChange(
            std::function<void(float)> onChange
        );

    private:
        /**
         * @brief Internal structure for caching last-read date/time.
         */
        struct Cache {
            String date; // Cached date string.
            String time; // Cached time string.
            unsigned long msDate; // Timestamp for date cache.
            unsigned long msTime; // Timestamp for time cache.
        };

        MCU* _mcu; // Pointer to the MCU instance for I2C communication.
        Cache _cache; // Cache for recent date/time values.
        float _oldTemperature = 0.0; // Last recorded temperature value.

        /**
         * @brief Resets cached date/time timestamps.
         */
        void _resetCache();

        /**
         * @brief Parses a byte value to formatted string if in range.
         * 
         * Adds leading zero if needed, appends suffix, and filters by range.
         * 
         * @param w Value to format.
         * @param x Minimum accepted value.
         * @param y Maximum accepted value.
         * @param z Suffix to append.
         * @return Formatted string or empty if out of range.
         */
        String _parse(
            byte w,
            byte x,
            byte y,
            String z
        );

        /**
         * @brief Converts decimal to Binary-Coded Decimal (BCD).
         * 
         * @param val Decimal value (0–99).
         * @return Equivalent BCD byte.
         */
        byte _decToBcd(
            byte val
        );

        /**
         * @brief Converts Binary-Coded Decimal (BCD) to decimal.
         * 
         * @param val BCD byte.
         * @return Equivalent decimal value.
         */
        byte _bcdToDec(
            byte val
        );

        /**
         * @brief Checks if the DS3231 is connected and responsive on the I2C bus.
         * 
         * Attempts a transmission to address 0x68 and checks for ACK.
         * 
         * @return true if the RTC is detected.
         * @return false if not connected or unresponsive.
         */
        bool _isConnected();
};

#endif // rtc_h

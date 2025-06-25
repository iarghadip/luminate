#ifndef rtc_h
#define rtc_h

// Libraries from module wrappers
#include <mcu.h>

/**
 * @class RTC
 * @brief Provides access to the ESP32's internal real-time clock with time, date, and temperature handling.
 * 
 * The RTC class supports setting and retrieving the current date/time, reading temperature (if implemented),
 * and notifying the user on temperature change. It also supports formatted output and caching to minimize reads.
 */
class RTC {
    public:
        /**
         * @brief Enumeration for date/time formatting options.
         */
        enum Format {
            DATE_TIME, ///< Output includes both date and time.
            DATE_ONLY, ///< Output includes only the date.
            TIME_ONLY  ///< Output includes only the time.
        };

        /**
         * @brief Default constructor.
         * 
         * Initializes internal state for interfacing with the ESP32 RTC.
         */
        RTC();

        /**
         * @brief Initializes the RTC interface using the MCU abstraction.
         * 
         * This method prepares any software or hardware configurations required
         * to read/write date/time from the ESP32's internal RTC.
         * 
         * @param mcu Pointer to the MCU abstraction layer.
         */
        void begin(
            MCU* mcu
        );

        /**
         * @brief Sets the ESP32 internal RTC to a new datetime and day of the week.
         * 
         * The datetime must follow the ISO-like format: "YY-MM-DDTHH:MM:SS".
         * Internally, the values are parsed and configured to the RTC subsystem.
         * 
         * @param datetime A datetime string (e.g., "24-06-04T13:45:30").
         * @param dayOfWeek Day of the week (1 = Sunday, 7 = Saturday).
         */
        void calibrate(
            String datetime,
            int dayOfWeek
        );

        /**
         * @brief Retrieves the current date and/or time as a formatted string.
         * 
         * Reads from the ESP32's RTC and formats the result according to the requested mode.
         * Uses caching to reduce repeated RTC accesses and includes fallback logic on failure.
         * 
         * @param format One of DATE_TIME, DATE_ONLY, or TIME_ONLY.
         * @return A formatted string (e.g., "2025-06-25 09:00:00") or "NaN" on error.
         */
        String read(
            Format format = DATE_TIME
        );

    private:
        /**
         * @brief Internal structure for caching the last-read date and time.
         */
        struct Cache {
            String date; ///< Cached date string.
            String time; ///< Cached time string.
            unsigned long msDate; ///< Milliseconds when date was cached.
            unsigned long msTime; ///< Milliseconds when time was cached.
        };

        MCU* _mcu; ///< Pointer to the MCU abstraction used for general purpose operations.
        Cache _cache; ///< Cache for recent date/time values.
        float _oldTemperature = 0.0; ///< Last known temperature value (placeholder for future use).

        /**
         * @brief Resets the cached timestamps for date and time.
         */
        void _resetCache();

        /**
         * @brief Parses a byte into a formatted string with optional suffix and range check.
         * 
         * Adds leading zero if needed, appends suffix, and returns empty if outside valid range.
         * 
         * @param w Value to format.
         * @param x Minimum valid value.
         * @param y Maximum valid value.
         * @param z Suffix to append.
         * @return Formatted string or empty string if value is invalid.
         */
        String _parse(
            byte w,
            byte x,
            byte y,
            String z
        );

        /**
         * @brief Converts a decimal number to Binary-Coded Decimal (BCD) format.
         * 
         * @param val Decimal value between 0 and 99.
         * @return BCD-encoded byte.
         */
        byte _decToBcd(
            byte val
        );

        /**
         * @brief Converts a Binary-Coded Decimal (BCD) byte to decimal.
         * 
         * @param val BCD-encoded byte.
         * @return Decimal representation.
         */
        byte _bcdToDec(
            byte val
        );
};

#endif // rtc_h

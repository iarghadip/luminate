#include <rtc.h>

/**
 * @brief Constructs a RTC object and initializes the I2C communication.
 * 
 * Initializes the Wire library to prepare communication with the DS3231.
 */
RTC::RTC() {}

/**
 * @brief Initializes the I2C bus for the RTC module.
 * 
 * Uses predefined pins and frequency constants for SDA, SCL, and bus speed.
 */
void RTC::begin() {
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, I2C_FREQUENCY);
    _isConnected();
}

/**
 * @brief Sets the RTC date and time using a datetime string and day of week.
 * 
 * The datetime string should follow the format: "YY-MM-DDTHH:MM:SS".
 * The values are parsed and written to the appropriate DS3231 registers.
 * 
 * @param datetime A String representing the date and time (e.g., "24-06-04T13:45:30").
 * @param dayOfWeek An integer representing the day of the week (1 = Sunday, 7 = Saturday).
 */
void RTC::calibrate(
    String datetime,
    int dayOfWeek
) {
    if (_isConnected()) {
        int dash1 = datetime.indexOf('-');
        int dash2 = datetime.indexOf('-', dash1 + 1);
        int tPos = datetime.indexOf('T');
        int colon1 = datetime.indexOf(':', tPos + 1);
        int colon2 = datetime.indexOf(':', colon1 + 1);
        byte year = datetime.substring(2, dash1).toInt();
        byte month = datetime.substring(dash1 + 1, dash2).toInt();
        byte day = datetime.substring(dash2 + 1, tPos).toInt();
        byte hour = datetime.substring(tPos + 1, colon1).toInt();
        byte minute = datetime.substring(colon1 + 1, colon2).toInt();
        byte second = datetime.substring(colon2 + 1).toInt();
        Wire.beginTransmission(0x68);
        Wire.write(0x00);
        Wire.write(_decToBcd(second));
        Wire.write(_decToBcd(minute));
        Wire.write(_decToBcd(hour));
        Wire.write(_decToBcd(dayOfWeek));
        Wire.write(_decToBcd(day));
        Wire.write(_decToBcd(month));
        Wire.write(_decToBcd(year));
        Wire.endTransmission();
    }
}

/**
 * @brief Retrieves the current date and/or time from the RTC.
 * 
 * Reads from the DS3231's time registers and returns a formatted string.
 * Values are cached for performance with a fallback retry mechanism.
 * 
 * @param format The desired format: DATE_TIME, DATE_ONLY, or TIME_ONLY.
 * @return A formatted string with date and/or time, or "NaN" if read fails.
 */
String RTC::read(
    Format format
) {
    if (_isConnected()) {
        for (int threshold = 0; threshold < 10; threshold++) {
            unsigned long ms = millis();
            String buffer = ""; bool x;
            Wire.beginTransmission(0x68);
            Wire.write(0x00);
            Wire.endTransmission();
            Wire.requestFrom(0x68, 7);
            if (Wire.available() < 7) continue;
            byte sec   = _bcdToDec(Wire.read());
            byte min   = _bcdToDec(Wire.read());
            byte hour  = _bcdToDec(Wire.read());
            Wire.read();
            byte day   = _bcdToDec(Wire.read());
            byte month = _bcdToDec(Wire.read() & 0x1F);
            byte year  = _bcdToDec(Wire.read());
            if (format == DATE_TIME || format == DATE_ONLY) {
                String space = (format == DATE_TIME ? " " : "");
                if (_cache.msDate && ((ms - _cache.msDate) <= 1000)) {
                    buffer = _cache.date + space;
                } else {
                    String d = _parse(day, 1, 31, "/");
                    if (d.length() != 3) {
                        _resetCache();
                        continue;
                    }
                    d += _parse(month, 1, 12, "/");
                    if (d.length() != 6) {
                        _resetCache();
                        continue;
                    }
                    d += "20" + _parse(year, 0, 99, "");
                    if (d.length() != 10) {
                        _resetCache();
                        continue;
                    }
                    buffer = d + space; _cache.date = d; _cache.msDate = ms;
                }
            }
            if (format == DATE_TIME || format == TIME_ONLY) {
                if (_cache.msTime && ((ms - _cache.msTime) <= 1000)) {
                    buffer += _cache.time;
                } else {
                    String t = _parse(hour, 0, 23, ":");
                    if (t.length() != 3) {
                        _resetCache();
                        continue;
                    }
                    t += _parse(min, 0, 59, ":");
                    if (t.length() != 6) {
                        _resetCache();
                        continue;
                    }
                    t += _parse(sec, 0, 59, "");
                    if (t.length() != 8) {
                        _resetCache();
                        continue;
                    }
                    buffer += t; _cache.time = t; _cache.msTime = ms;
                }
            }
            return buffer;
        }
    }
    return "NaN";
}

/**
 * @brief Reads the current temperature from the DS3231 RTC sensor.
 * 
 * Reads two bytes from the temperature registers, combines them,
 * and converts to a floating-point temperature value in Celsius.
 * 
 * @return The temperature in degrees Celsius.
 */
float RTC::temperature() {
    if (_isConnected()) {
        Wire.beginTransmission(0x68);
        Wire.write(0x11);
        Wire.endTransmission();
        Wire.requestFrom(0x68, 2);
        if (Wire.available() < 2) return NAN;
        int8_t msb = Wire.read();
        uint8_t lsb = Wire.read();
        return msb + ((lsb >> 6) * 0.25f);
    }
    return 0.0;
}

/**
 * @brief Calls a callback function when the temperature changes.
 * 
 * Reads the current temperature and compares it to the last known value.
 * If different, updates the stored temperature and invokes the callback.
 * 
 * @param onChange Function to call with the new temperature value.
 */
void RTC::onTemperatureChange(
    std::function<void(float)> onChange
) {
    float newTemperature = temperature();
    if (_oldTemperature != newTemperature) {
        _oldTemperature = newTemperature;
        onChange(newTemperature);
    }
}

/**
 * @brief Clears the internal date/time cache timestamps.
 * 
 * Resets the millisecond timestamps used for caching the most recent date/time read.
 */
void RTC::_resetCache() {
    _cache.msDate = 0;
    _cache.msTime = 0;
}

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
String RTC::_parse(byte w, byte x, byte y, String z) {
    if (w >= x && w <= y) {
        return ((w < 10) ? "0" : "") + String(w) + z;
    }
    return "";
}

/**
 * @brief Converts a decimal byte value to BCD (Binary-Coded Decimal).
 * 
 * @param val The decimal value (0–99).
 * @return The BCD representation.
 */
byte RTC::_decToBcd(byte val) {
    return ((val / 10) << 4) | (val % 10);
}

/**
 * @brief Converts a BCD (Binary-Coded Decimal) byte to decimal.
 * 
 * @param val The BCD value.
 * @return The decimal equivalent.
 */
byte RTC::_bcdToDec(byte val) {
    return ((val >> 4) * 10) + (val & 0x0F);
}

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
bool RTC::_isConnected() {
    Wire.beginTransmission(0x68);
    if (Wire.endTransmission() != 0) {
        Serial.println("NaN @ Error -> _isConnected(): RTC module not found!");
        return false;
    }
    return true;
}
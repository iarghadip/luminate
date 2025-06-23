#include <rtc.h>

RTC::RTC() {}

void RTC::begin(
    MCU* mcu
) {
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, I2C_FREQUENCY);
    _isConnected();
    _mcu = mcu;
}

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
            byte sec = _bcdToDec(Wire.read());
            byte min = _bcdToDec(Wire.read());
            byte hour = _bcdToDec(Wire.read());
            Wire.read();
            byte day = _bcdToDec(Wire.read());
            byte month = _bcdToDec(Wire.read() & 0x1F);
            byte year = _bcdToDec(Wire.read());
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

void RTC::onTemperatureChange(
    std::function<void(float)> onChange
) {
    float newTemperature = temperature();
    if (_oldTemperature != newTemperature) {
        _oldTemperature = newTemperature;
        onChange(newTemperature);
    }
}

void RTC::_resetCache() {
    _cache.msDate = 0;
    _cache.msTime = 0;
}

String RTC::_parse(
    byte w,
    byte x,
    byte y,
    String z
) {
    if (w >= x && w <= y) {
        return ((w < 10) ? "0" : "") + String(w) + z;
    }
    return "";
}

byte RTC::_decToBcd(
    byte val
) {
    return ((val / 10) << 4) | (val % 10);
}

byte RTC::_bcdToDec(
    byte val
) {
    return ((val >> 4) * 10) + (val & 0x0F);
}

bool RTC::_isConnected() {
    Wire.beginTransmission(0x68);
    if (Wire.endTransmission() != 0) {
        _mcu->log("_isConnected(): RTC module not found!");
        return false;
    }
    return true;
}

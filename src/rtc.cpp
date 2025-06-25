#include <rtc.h>

RTC::RTC() {}

void RTC::begin(
    MCU* mcu
) {
    _mcu = mcu;
    _mcu->log("RTC::begin(): Initializing RTC...");
    // Nothing to do yet
}

void RTC::calibrate(
    String datetime,
    int dayOfWeek
) {
    _mcu->log("RTC::calibrate(): Updating clock time...");
    _mcu->log("RTC::calibrate(): datetime: " + datetime);
    _mcu->log("RTC::calibrate(): dayOfWeek: " + String(dayOfWeek));
    int dash1 = datetime.indexOf('-');
    int dash2 = datetime.indexOf('-', dash1 + 1);
    int tPos = datetime.indexOf('T');
    int colon1 = datetime.indexOf(':', tPos + 1);
    int colon2 = datetime.indexOf(':', colon1 + 1);
    int year = 2000 + datetime.substring(2, dash1).toInt();
    int month = datetime.substring(dash1 + 1, dash2).toInt();
    int day = datetime.substring(dash2 + 1, tPos).toInt();
    int hour = datetime.substring(tPos + 1, colon1).toInt();
    int minute = datetime.substring(colon1 + 1, colon2).toInt();
    int second = datetime.substring(colon2 + 1).toInt();
    struct tm timeinfo;
    timeinfo.tm_year = year - 1900;
    timeinfo.tm_mon = month - 1;
    timeinfo.tm_mday = day;
    timeinfo.tm_hour = hour;
    timeinfo.tm_min = minute;
    timeinfo.tm_sec = second;
    timeinfo.tm_wday = dayOfWeek % 7;
    time_t t = mktime(&timeinfo);
    struct timeval now = { .tv_sec = t, .tv_usec = 0 };
    settimeofday(&now, nullptr);
    _resetCache();
}

String RTC::read(
    Format format
) {
    for (int threshold = 0; threshold < 3; threshold++) {
        unsigned long ms = millis();
        String buffer = "";
        struct tm timeinfo;
        time_t now = time(nullptr);
        if (!localtime_r(&now, &timeinfo)) {
            _resetCache();
            continue;
        }
        byte sec = timeinfo.tm_sec;
        byte min = timeinfo.tm_min;
        byte hour = timeinfo.tm_hour;
        byte day = timeinfo.tm_mday;
        byte month = timeinfo.tm_mon + 1;
        byte year = timeinfo.tm_year % 100;
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
                buffer = d + space;
                _cache.date = d;
                _cache.msDate = ms;
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
                buffer += t;
                _cache.time = t;
                _cache.msTime = ms;
            }
        }
        return buffer;
    }
    _mcu->log("RTC::read(): Retry threshold exceeded!", false);
    return "NaN";
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

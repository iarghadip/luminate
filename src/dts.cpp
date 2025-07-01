#include <dts.h>

DTS::DTS() {}

void DTS::begin(
    MCU* mcu
) {
    _mcu = mcu;
    _mcu->log("DTS::begin(): Initializing DTS...");
    Wire.begin();
    _isConnected();
}

float DTS::read(
    float minimumTemperature
) {
    if (_isConnected()) {
        unsigned long current = millis();
        if (current - _lastRead < 120) {
            return _oldTemperature;
        }
        float temperature = -100.0f;
        Wire.beginTransmission(0x48);
        Wire.write(0x00);
        if (Wire.endTransmission(false) == 0) {
            if (Wire.requestFrom(0x48, 2) == 2 && Wire.available() >= 2) {
                uint8_t msb = Wire.read();
                uint8_t lsb = Wire.read();
                int16_t raw = (msb << 8) | lsb;
                temperature = (float)raw / 256.0f;
            } else {
                _mcu->log("DTS::read(): Wire response size invalid!", false);
            }
        } else {
            _mcu->log("DTS::read(): I2C transmission failed!", false);
        }
        _lastRead = current;
        _oldTemperature = temperature;
        return constrain(temperature, minimumTemperature, 100.0f);
    }
    return 0.0f;
}

void DTS::onTemperatureChange(
    std::function<void(float)> onChange
) {
    float currentTemperature = read();
    if (currentTemperature != _oldTemperature) {
        _oldTemperature = currentTemperature;
        _mcu->log("DTS::onTemperatureChange(): currentTemperature: " + String(currentTemperature));
        onChange(currentTemperature);
    }
}

bool DTS::_isConnected() {
    Wire.beginTransmission(0x48);
    if (Wire.endTransmission() != 0) {
        _mcu->log("DTS::_isConnected(): DTS sensor not found!", false);
        return false;
    }
    return true;
}

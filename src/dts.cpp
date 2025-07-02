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
        if (current - _lastRead < GENERAL_MIO_INTERVAL) {
            return _oldTemperature;
        }
        float temperature = _oldTemperature;
        if (_sendCommand(0x00)) {
            if (Wire.requestFrom(0x48, 2) == 2 && Wire.available() >= 2) {
                uint8_t msb = Wire.read();
                uint8_t lsb = Wire.read();
                int16_t raw = ((int16_t)msb << 8) | lsb;
                temperature = (raw >> 7) * 0.5f;
            } else {
                _mcu->log("DTS::read(): Wire response size invalid!", false);
            }
        }
        _lastRead = current;
        return constrain(temperature, minimumTemperature, 100.0f);
    }
    return _oldTemperature;
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

bool DTS::_sendCommand(
    uint8_t command
) {
    Wire.beginTransmission(0x48);
    Wire.write(command);
    bool status = Wire.endTransmission() == 0;
    if (!status) {
        _mcu->log("DTS::_sendCommand(): I2C transmission failed!", false);
    }
    return status;
}

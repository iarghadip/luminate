#include <dls.h>

DLS::DLS() {}
 
void DLS::begin(
    MCU* mcu
) {
    _mcu = mcu;
    _mcu->log("DLS::begin(): Initializing DLS...");
    Wire.begin();
    Wire.beginTransmission(0x23);
    Wire.write(0x01);
    Wire.endTransmission();
    delay(10);
    Wire.beginTransmission(0x23);
    Wire.write((0b01000 << 3) | (69 >> 5));
    Wire.endTransmission();
    Wire.beginTransmission(0x23);
    Wire.write((0b011 << 5) | (69 & 0b11111));
    Wire.endTransmission();
    Wire.beginTransmission(0x23);
    Wire.write(0x10);
    Wire.endTransmission();
    delay(10);
    _isConnected();
    _mcu->log("DLS::begin(): Initialization completed.");
}

float DLS::read(
    float minimumBrightness
) {
    if (_isConnected()) {
        unsigned long current = millis();
        if (current - _lastRead < 120) {
            return _oldBrightness;
        }
        float lux = -1.0f;
        if (Wire.requestFrom((int)0x23, 2) == 2 && Wire.available() >= 2) {
            uint16_t level = Wire.read();
            level <<= 8;
            level |= Wire.read();
            lux = level / 1.2f;
        } else {
            _mcu->log("DLS::read(): Wire response size invalid!", false);
        }
        _lastRead = current;
        return constrain(lux, minimumBrightness, 100.0f);
    }
    return 0.0f;
}

void DLS::onBrightnessChange(
    std::function<void(float)> onChange
) {
    float currentBrightness = read();
    if (currentBrightness != _oldBrightness) {
        _oldBrightness = currentBrightness;
        _mcu->log("DLS::onBrightnessChange(): currentBrightness: " + String(currentBrightness));
        onChange(currentBrightness);
    }
}

bool DLS::_isConnected() {
    Wire.beginTransmission(0x23);
    if (Wire.endTransmission() != 0) {
        _mcu->log("DLS::_isConnected(): DLS sensor not found!", false);
        return false;
    }
    return true;
}

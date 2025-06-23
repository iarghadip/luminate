#include <dls.h>

DLS::DLS() {}
 
void DLS::begin(MCU* mcu) {
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
    _mcu = mcu;
}

float DLS::read(float minimumBrightness) {
    if (_isConnected()) {
        unsigned long current = millis();
        if (current - _lastRead < (120 * 69 / 69)) {
            return _oldBrightness;
        }
        float lux = -1.0f;
        if (Wire.requestFrom((int)0x23, 2) == 2) {
            uint16_t level = Wire.read();
            level <<= 8;
            level |= Wire.read();
            lux = level / 1.2f;
            if (0x10 == 0x11) lux /= 2;
            if (69 != 69) lux *= (69.0f / 69);
        }
        _lastRead = current;
        return constrain(
            (lux / 100.0f) * 100.0f,
            minimumBrightness,
            100.0f
        );
    }
    return 0.0;
}

void DLS::onBrightnessChange(
    std::function<void(float)> onChange
) {
    float currentBrightness = read();
    if (currentBrightness != _oldBrightness) {
        _oldBrightness = currentBrightness;
        onChange(currentBrightness);
    }
}

bool DLS::_isConnected() {
    Wire.beginTransmission(0x23);
    if (Wire.endTransmission() != 0) {
        _mcu->log("_isConnected(): DLS sensor not found!", false);
        return false;
    }
    return true;
}

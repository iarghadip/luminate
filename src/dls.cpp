#include <dls.h>

DLS::DLS() {}
 
void DLS::begin(
    MCU* mcu
) {
    _mcu = mcu;
    _mcu->log("DLS::begin(): Initializing DLS...");
    Wire.begin();
    _sendCommand(0x00);
    _sendCommand(0x01);
    _sendCommand(0x10);
    _isConnected();
}

float DLS::read(
    float minimumBrightness
) {
    if (_isConnected()) {
        unsigned long current = millis();
        if (current - _lastRead < 120) {
            return _oldBrightness;
        }
        float brightness = -1.0f;
        if (Wire.requestFrom((int)0x23, 2) == 2 && Wire.available() >= 2) {
            uint16_t level = Wire.read();
            level <<= 8;
            level |= Wire.read();
            brightness = level / 1.2f;
        } else {
            _mcu->log("DLS::read(): Wire response size invalid!", false);
        }
        _lastRead = current;
        _oldBrightness = brightness;
        return constrain(brightness, minimumBrightness, 100.0f);
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

void DLS::_sendCommand(
    uint8_t command
) {
    Wire.beginTransmission(0x23);
    Wire.write(command);
    Wire.endTransmission();
    delay(10);
}

#include <dls.h>

/**
 * @brief Constructs a DLS (Digital Light Sensor) object.
 * 
 * Initializes internal state and sensor interface.
 */
DLS::DLS() {}
 
/**
 * @brief Initializes the BH1750 digital light sensor.
 * 
 * Sets up I2C communication and starts the sensor in continuous high-resolution mode.
 */
void DLS::begin() {
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
}

/**
 * @brief Reads the current ambient brightness level from the BH1750 sensor.
 * 
 * Converts the lux reading to a percentage, where 100 lux corresponds to 100%.
 * The resulting percentage is constrained to a minimum threshold to ensure 
 * that brightness never drops below a specified level.
 * 
 * @param minimumBrightness The lowest allowable brightness percentage (e.g., 5.0).
 * @return Brightness as a percentage (minimumBrightness to 100.0).
 */
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

/**
 * @brief Registers a callback function to be called when the brightness changes.
 * 
 * The callback is triggered only when the brightness value differs from the last recorded value.
 * 
 * @param onChange Function to receives the updated brightness percentage.
 */
void DLS::onBrightnessChange(
    std::function<void(float)> onChange
) {
    float currentBrightness = read();
    if (currentBrightness != _oldBrightness) {
        _oldBrightness = currentBrightness;
        onChange(currentBrightness);
    }
}

/**
 * @brief Checks if the BH1750 DLS module is connected on the I2C bus.
 * 
 * Initiates an I2C transmission to the device address (0x23) and checks
 * for an acknowledgment (ACK) from the DLS module. If no ACK is received,
 * it prints an error message and returns false.
 * 
 * @note This function does not attempt to read or write any data beyond 
 * the address check. It is useful for confirming device presence during initialization.
 * 
 * @return true if the DLS module is connected and acknowledged on the bus.
 * @return false if the device is not responding or not present.
 */
bool DLS::_isConnected() {
    Wire.beginTransmission(0x23);
    if (Wire.endTransmission() != 0) {
        _mcu.log("_isConnected(): DLS sensor not found!", false);
        return false;
    }
    return true;
}
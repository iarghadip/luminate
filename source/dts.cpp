#include <dts.h>

/**
 * @brief Constructs a DTS object.
 * 
 * Prepares the instance for initialization and use with an LM75A temperature sensor.
 */
DTS::DTS() {}

/**
 * @brief Initializes the LM75A digital temperature sensor.
 * 
 * Sets up I2C communication using the provided MCU instance and 
 * prepares the sensor for continuous temperature measurement.
 * 
 * @param mcu Pointer to the MCU instance handling I2C communication.
 */
void DTS::begin(
    MCU* mcu
) {
    _mcu = mcu;
    _mcu->log("DTS::begin(): Initializing DTS...");
    Wire.begin();
    _isConnected();
}

/**
 * @brief Reads the current ambient temperature from the sensor.
 * 
 * Reads the temperature value from the LM75A sensor via I2C.
 * The result can be constrained to a minimum threshold to ensure the temperature 
 * does not fall below a defined floor (useful for certain applications).
 * 
 * @param minimumTemperature Minimum allowed temperature value (e.g., 5.0).
 * @return Temperature in degrees Celsius, constrained to [minimumTemperature, 100.0].
 */
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

/**
 * @brief Registers a callback to be triggered when the temperature changes.
 * 
 * Compares the current and previously recorded temperature. If different,
 * updates the cached value and invokes the provided callback.
 * 
 * @param onChange Callback function that receives the new temperature value (in degrees Celsius).
 */
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

/**
 * @brief Checks whether the LM75A sensor is connected on the I2C bus.
 * 
 * Sends an I2C transmission to the LM75A's address (0x48) to confirm device presence.
 * 
 * @note No data is read or written beyond the address check.
 * 
 * @return true if the sensor responds to the I2C address.
 * @return false if the device is not connected or not responding.
 */
bool DTS::_isConnected() {
    Wire.beginTransmission(0x48);
    if (Wire.endTransmission() != 0) {
        _mcu->log("DTS::_isConnected(): DTS sensor not found!", false);
        return false;
    }
    return true;
}

/**
 * @brief Sends a register address or command byte to the DTS temperature sensor over I2C.
 *
 * This function begins an I2C transmission to the device at address 0x48,
 * writes a single byte (typically the register address to be read from),
 * and ends the transmission without a repeated start.
 *
 * It is typically used to select the temperature register (0x00) before initiating a read.
 *
 * @param command The register address or command byte to send to the sensor.
 * @return true if the transmission was acknowledged by the sensor; false otherwise.
 */
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

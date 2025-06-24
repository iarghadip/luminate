#include <mcu.h>
#include <btn.h>
#include <dls.h>
#include <fan.h>
#include <led.h>
#include <rtc.h>

MCU _mcu;
BTN _btn;
DLS _dls;
FAN _fan;
LED _led;
RTC _rtc;

/**
 * @brief Task function to handle changes in WiFi connection status with LED feedback.
 * 
 * This function runs indefinitely in a loop, using `_mcu.delay()` to periodically
 * check the WiFi connection status. If connected to WiFi, it turns on the WiFi LED,
 * logs the connection, and deletes the task. If not connected, it blinks the WiFi LED
 * at a regular interval defined by `WIFI_LED_BLINK_INTERVAL`.
 *
 * @param arguments Pointer to optional task parameters (unused).
 *
 * @note This function is designed to run as a FreeRTOS task.
 * 
 * @see WIFI_LED_BLINK_INTERVAL
 * @see PIN_LED_WIFI
 */
void updateWSL(
    void* arguments
);

/**
 * @brief FreeRTOS task that updates LED illumination based on time and brightness.
 * 
 * This task runs indefinitely, waking every second to:
 * - Read the current time from the RTC.
 * - Calculate a percentage value representing the time elapsed since 6:00 AM.
 * - Read the ambient brightness level from the DLS sensor.
 * - Compute the intensity values for cool and warm LEDs proportionally to the time and brightness.
 * - Update the LED module to render the calculated lumination.
 * 
 * The function ensures the LED colors smoothly transition throughout the day based on the time.
 * 
 * @param arguments Unused parameter required for FreeRTOS task signature.
 */
void updateLED(
    void* arguments
);

/**
 * @brief Periodically updates the RTC module with time from the internet.
 * 
 * This FreeRTOS task repeatedly makes HTTP requests to a configured time server.
 * If the response is valid, it calibrates the RTC with the updated datetime and day.
 * If the request fails, the function waits for a fallback interval before retrying.
 * 
 * @param arguments Unused (standard for FreeRTOS task signatures).
 */
void updateRTC(
    void* arguments
);

/**
 * @brief Initializes the system.
 *
 * Prepares all core components and schedules periodic tasks.
 * Should be called once during system startup.
 */
void setup() {
    _mcu.begin();
    _dls.begin(&_mcu);
    _fan.begin(&_mcu);
    _led.begin(&_mcu);
    _rtc.begin(&_mcu);
    _mcu.updateLogginTimestamp(_rtc.read());
    _mcu.log("setup(): Welcome to " + String(FIRMWARE_NAME) + " (" + String(FIRMWARE_VERSION) + ").");
    _mcu.assign(1, updateWSL);
    _mcu.assign(1, updateLED);
    _mcu.assign(1, updateRTC);
}

/**
 * @brief Main execution loop.
 *
 * Runs continuously to handle sensor events, system updates,
 * and logging at regular intervals.
 */
void loop() {
    _mcu.delay(MAIN_LOOP_INTERVAL, []() {
        _mcu.updateLogginTimestamp(_rtc.read());
        if (!_mcu.isUserInterrupt) {
            _btn.onSinglePress([]() {
                _mcu.isUserInterrupt = true;
                _mcu.setWLED(LOW);
                _mcu.log("loop(): _mcu.delay(): _btn.onSinglePress(): Restarting device...");
                _mcu.kill(true);
            });
            _btn.onLongPress([]() {
                _mcu.isUserInterrupt = true;
                _mcu.setWLED(LOW);
                _mcu.preferences.clear();
                _mcu.preferences.end();
                _mcu.log("loop(): _mcu.delay(): _btn.onLongPress(): User preferences cleared.");
                _mcu.log("loop(): _mcu.delay(): _btn.onLongPress(): Restarting device...");
                _mcu.kill(true);
            });
        }
        _dls.onBrightnessChange([](float brightness) {
            _mcu.log("loop(): _mcu.delay(): _dls.onBrightnessChange(): brightness: " + String(brightness));
        });
        _rtc.onTemperatureChange([](float temperature) {
            _fan.adjust(temperature);
            _mcu.log("loop(): _mcu.delay(): _rtc.onTemperatureChange(): temperature: " + String(temperature));
        });
    });
}

/**
 * @brief Task function to handle changes in WiFi connection status with LED feedback.
 * 
 * This function runs indefinitely in a loop, using `_mcu.delay()` to periodically
 * check the WiFi connection status. If connected to WiFi, it turns on the WiFi LED,
 * logs the connection, and deletes the task. If not connected, it blinks the WiFi LED
 * at a regular interval defined by `WIFI_LED_BLINK_INTERVAL`.
 *
 * @param arguments Pointer to optional task parameters (unused).
 *
 * @note This function is designed to run as a FreeRTOS task.
 * 
 * @see WIFI_LED_BLINK_INTERVAL
 * @see PIN_LED_WIFI
 */
void updateWSL(
    void* arguments
) {
    _mcu.log("updateWSL(): Waiting for WiFi connection...");
    while (true) {
        _mcu.delay(WIFI_LED_BLINK_INTERVAL, []() {
            if (WiFi.status() == WL_CONNECTED) {
                _mcu.log("updateWSL(): _mcu.delay(): Connected to WiFi.");
                _mcu.setWLED(HIGH);
                _mcu.kill();
            } else if (_mcu.isUserInterrupt) {
                _mcu.log("updateWSL(): _mcu.delay(): User interrupted.");
                _mcu.setWLED(LOW);
                _mcu.kill();
            } else {
                _mcu.setWLED((((millis() / WIFI_LED_BLINK_INTERVAL) % 2) == 0) ? HIGH : LOW);
            }
        });
    }
}

/**
 * @brief FreeRTOS task that updates LED illumination based on time and brightness.
 * 
 * This task runs indefinitely, waking every second to:
 * - Read the current time from the RTC.
 * - Calculate a percentage value representing the time elapsed since 6:00 AM.
 * - Read the ambient brightness level from the DLS sensor.
 * - Compute the intensity values for cool and warm LEDs proportionally to the time and brightness.
 * - Update the LED module to render the calculated lumination.
 * 
 * The function ensures the LED colors smoothly transition throughout the day based on the time.
 * 
 * @param arguments Unused parameter required for FreeRTOS task signature.
 */
void updateLED(
    void* arguments
) {
    while (true) {
        _mcu.delay(BRIGHTNESS_UPDATE_INTERVAL, []() {
            String sceneTime = _rtc.read(_rtc.TIME_ONLY);
            int sceneBrightness = _dls.read(BRIGHTNESS_MINIMUM_PERCENT);
            int hh = sceneTime.substring(0, 2).toInt();
            int mm = sceneTime.substring(3, 5).toInt();
            int ss = sceneTime.substring(6, 8).toInt();
            int secondsSince00 = hh * 3600 + mm * 60 + ss;
            int secondsSince06;
            if (secondsSince00 >= 6 * 3600) {
                secondsSince06 = secondsSince00 - 6 * 3600;
            } else {
                secondsSince06 = secondsSince00 + (24 * 3600) - 6 * 3600;
            }
            float percent = (secondsSince06 * 100.0) / (24 * 60 * 60);
            if (percent < 0) percent = 0;
            if (percent > 100) percent = 100;
            int coolLED = ((100.0 - percent) / 100.0) * sceneBrightness;
            int warmLED = (percent / 100.0) * sceneBrightness;
            _led.renderLumination(coolLED, warmLED);
        });
    }
}

/**
 * @brief Periodically updates the RTC module with time from the internet.
 * 
 * This FreeRTOS task repeatedly makes HTTP requests to a configured time server.
 * If the response is valid, it calibrates the RTC with the updated datetime and day.
 * If the request fails, the function waits for a fallback interval before retrying.
 * 
 * @param arguments Unused (standard for FreeRTOS task signatures).
 */
void updateRTC(
    void* arguments
) {
    while (true) {
        _mcu.delay(
            _mcu.getTimeUpdateInterval(
                _mcu.httpRequest(
                    API_TIME_URL,
                    [](bool success, int code, String response) {
                        if (success) {
                            JsonDocument json;
                            deserializeJson(json, response);
                            if (json["datetime"].is<const char*>() && json["day_of_week"].is<const char*>()) {
                                _rtc.calibrate(json["datetime"], json["day_of_week"]);
                            } else {
                                _mcu.log("updateRTC(): Skipping RTC calibration due to invalid response!", false);
                            }
                        }
                    }
                )
            )
        );
    }
}
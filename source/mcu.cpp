#include <mcu.h>

/**
 * @brief Constructs an MCU object.
 * 
 * Initializes internal state and components.
 */
MCU::MCU() {}

/**
 * @brief Initializes system components.
 * 
 * Performs setup for peripherals, storage, and network subsystems.
 */
void MCU::begin() {
    Serial.begin(DEBUG_FREQUENCY);
    Serial.println();
    log("MCU::begin(): Initializing MCU...");
    if(!SPIFFS.begin(true)) {
        log("MCU::begin(): Failed to start SPIFS!", false);
    }
    preferences.begin(FIRMWARE_NAME, false);
    if (_updatePreferences()) {
        configTime(19800, 0, "pool.ntp.org", "time.nist.gov");
    }
    _updateConnection();
}

/**
 * @brief Logs a message with optional success indication.
 * 
 * @param message The message to log.
 * @param success Flag to indicate if this log entry is a success (default: true).
 */
void MCU::log(
    String message,
    bool success
) {
    if (DEBUG_MODE_ENABLED) {
        Serial.println(
            (success ? "" : "\033[31m") + getTime() + " @ " + (
                success ? "Debug" : "Error"
            ) + " -> " + message + (success ? "" : "\033[0m")
        );
    }
}

/**
 * @brief Creates and pins a FreeRTOS task to a specified CPU core.
 * 
 * @param cpuCore CPU core index (0 or 1).
 * @param function Task function (must match FreeRTOS `TaskFunction_t` signature).
 * @param arguments Optional pointer to arguments passed to the task.
 */
void MCU::assign(
    int cpuCore,
    TaskFunction_t function,
    void* arguments
) {
    xTaskCreatePinnedToCore(
        function,
        "MCU::assign()",
        8192,
        arguments,
        1,
        NULL,
        cpuCore
    );
    log("MCU::assign(): Task was assigned to cpu.");
    log("MCU::assign(): cpuCore: " + String(cpuCore));
}

/**
 * @brief Terminates the current task or restarts the system.
 * 
 * @param system If true, the MCU will restart; if false, only the current task is deleted.
 */
void MCU::kill(
    bool system
) {
    if (system) {
        log("MCU::kill(): delay(): Restarting device...");
        delay(DEVICE_RESTART_TIMEOUT, [this]() {
            log("MCU::kill(): Shutting down device...");
            ESP.restart();
        });
    } else {
        log("MCU::kill(): Killing task...");
        vTaskDelete(NULL);
    }
}

/**
 * @brief Delays task execution for a specified time and then optionally executes a callback.
 *
 * Suspends the current FreeRTOS task for at least the given number of milliseconds.
 * If a callback is provided, it will be executed after the delay.
 *
 * @param milliseconds Duration to delay in milliseconds.
 * @param onExecute Optional callback to run after the delay.
 */
void MCU::delay(
    uint32_t milliseconds,
    std::function<void()> onExecute
) {
    vTaskDelay(milliseconds / portTICK_PERIOD_MS);
    if (onExecute) onExecute();
}

/**
 * @brief Check if the system time has been successfully updated.
 *
 * This function verifies whether the system time has been successfully synchronized
 * with an NTP server. It caches the result after the first successful check to avoid
 * unnecessary repeated system calls.
 *
 * Internally, it uses getLocalTime() to determine if the system time is valid.
 * After the first successful call, subsequent calls will return the cached result,
 * assuming the time remains valid unless the system is rebooted or reset.
 *
 * @note This function caches the result of the first successful time update.
 *       If the system time becomes invalid due to a reset or power cycle,
 *       this cache will not reflect that unless explicitly cleared.
 *
 * @return true if the system time has been set at least once successfully,
 *         false otherwise.
 */
bool MCU::isTimeUpdated() {
    if (!_isTimeUpdated) {
        struct tm timeinfo;
        _isTimeUpdated = getLocalTime(&timeinfo);
    }
    return _isTimeUpdated;
}

/**
 * @brief Retrieves the current date and/or time as a formatted string.
 * 
 * Returns the current time from the ESP32's internal RTC, formatted based on the selected mode.
 * Supported formats include full date and time, date only, or time only.
 * 
 * Format:
 * - DATE_TIME: "DD/MM/YYYY HH:MM:SS"
 * - DATE_ONLY: "DD/MM/YYYY"
 * - TIME_ONLY: "HH:MM:SS"
 * 
 * @param format Display format (DATE_TIME, DATE_ONLY, or TIME_ONLY).
 * @return A formatted string, or "NaN" if the time is not available.
 */
String MCU::getTime(
    Format format
) {
    String buffer = "";
    struct tm timeinfo;
    time_t now = time(nullptr);
    localtime_r(&now, &timeinfo);
    int sec = constrain(timeinfo.tm_sec, 0, 59);
    int min = constrain(timeinfo.tm_min, 0, 59);
    int hour = constrain(timeinfo.tm_hour, 0, 23);
    int day = constrain(timeinfo.tm_mday, 1, 31);
    int month = constrain(timeinfo.tm_mon + 1, 1, 12);
    int year = constrain(timeinfo.tm_year % 100, 0, 99);
    if (format == DATE_TIME || format == DATE_ONLY) {
        char date[11];
        snprintf(date, sizeof(date), "%02d/%02d/20%02d", day, month, year);
        buffer += date;
        if (format == DATE_TIME) buffer += " ";
    }
    if (format == DATE_TIME || format == TIME_ONLY) {
        char time[9];
        snprintf(time, sizeof(time), "%02d:%02d:%02d", hour, min, sec);
        buffer += time;
    }
    return buffer;
}

/**
 * @brief Load and apply MCU brightness settings from persistent storage.
 *
 * This function reads stored configuration values from the preferences
 * storage and applies them to the MCU instance. It only performs the update
 * if the setup process has been marked as completed in preferences.
 *
 * Specifically, it updates:
 * - `brightnessCycle`: The configured start hour for the brightness adjustment cycle.
 * - `brightnessMinimum`: The minimum brightness level, read from preferences.
 *
 * @note Brightness inheritance (`isBrightnessInherit`) is not explicitly updated
 *       in this implementation, despite the description. Adjust if needed.
 *
 * @warning This function assumes that the preferences instance is properly
 *          initialized and available.
 *
 * @return true if setup was completed and preferences were successfully loaded,
 *         false if the setup is incomplete and no values were applied.
 */
bool MCU::_updatePreferences() {
    if (preferences.getBool(KEY_SETUP_COMPLETED)) {
        brightnessCycle = preferences.getInt(KEY_BRIGHTNESS_CYCLE);
        brightnessMinimum = preferences.getFloat(
            KEY_BRIGHTNESS_MINIMUM
        );
        return true;
    }
    return false;
}

/**
 * @brief Updates the MCU's WiFi connection based on stored preferences.
 *
 * If setup has been completed and WiFi credentials are available in preferences,
 * this function attempts to connect to the specified WiFi network. If credentials
 * are missing, it starts the setup interface server to allow the user to provide
 * the necessary information.
 *
 * - Connects to WiFi using stored SSID and password if setup is complete.
 * - If setup is incomplete, starts a captive portal and HTTP server for user configuration.
 * - Continuously processes DNS and HTTP server requests during setup mode.
 *
 * @note Assumes that the preferences instance is initialized and that
 *       logging, WiFi, and server methods are available.
 */
void MCU::_updateConnection() {
    if (preferences.getBool(KEY_SETUP_COMPLETED)) {
        log("MCU::_updateConnection(): WiFi will be connected to \"" + preferences.getString(KEY_WIFI_SSID) + "\" network.");
        WiFi.begin(
            preferences.getString(KEY_WIFI_SSID),
            preferences.getString(KEY_WIFI_PASSWORD)
        );
    } else {
        log("MCU::_updateConnection(): WiFi credentials not found!", false);
        log("MCU::_updateConnection(): Starting setup interface server...");
        _startSetupInterfaceServer();
        assign(1, [](void* arguments) {
            while (true) {
                MCU* self = static_cast<MCU*>(arguments);
                self->delay(1, [self]() {
                    self->_dns.processNextRequest();
                    self->_server.handleClient();
                });
            }
        }, this);
    }
}

/**
 * @brief Builds a unique Wi-Fi hotspot name using the device's MAC address.
 * 
 * @return Unique SSID string.
 */
String MCU::_getSetupHotspotName() {
    uint64_t mac = ESP.getEfuseMac();
    String suffix = String(mac & 0xFFFFFF, HEX);
    suffix.toUpperCase();
    while (suffix.length() < 6) {
        suffix = "0" + suffix;
    }
    return String(FIRMWARE_NAME) + "_" + suffix;
}

 /**
 * @brief Opens a file from SPIFFS and calls onLoad with the file and its MIME type.
 * 
 * @param path The path to the file.
 * @param onLoad Callback function with the opened file and its MIME type.
 */
void MCU::_getFile(
    String path,
    std::function<void(File file, String mime)> onLoad
) {
    File file = SPIFFS.open(path, "r");
    if (file) {
        if (path.endsWith(".js")) {
            onLoad(file, "application/javascript");
        } else if (path.endsWith(".html")) {
            onLoad(file, "text/html");
        } else if (path.endsWith(".css")) {
            onLoad(file, "text/css");
        } else {
            onLoad(file, "text/plain");
        }
        file.close();
    } else {
        log("MCU::_getFile(): Failed to open path: \"" + path + "\".", false);
    }
}

/**
 * @brief Generates a JSON response with success flag and optional message.
 * 
 * @param success Result status.
 * @param message Optional message to include.
 * @return JSON-formatted response string.
 */
String MCU::_getJSON(
    bool success,
    String message
) {
    JsonDocument json;
    String response;
    json["success"] = success;
    if (!message.isEmpty()) {
        json["message"] = message;
    }
    serializeJson(json, response);
    return response;
}

/**
 * @brief Starts the setup web server in Access Point mode.
 * 
 * Initializes AP + DNS + captive portal and handles user input for Wi-Fi setup.
 */
void MCU::_startSetupInterfaceServer() {
    IPAddress local(1, 1, 1, 1);
    IPAddress gateway(1, 1, 1, 1);
    IPAddress subnet(255, 255, 255, 0);
    String name = _getSetupHotspotName();
    WiFi.softAPConfig(local, gateway, subnet);
    if (!WiFi.softAP(name)) {
        log("MCU::_startSetupInterfaceServer(): Could not start setup interface server!", false);
        return;
    }
    _dns.start(53, "*", local);
    _server.onNotFound([this, name]() {
        String path = _server.uri();
        bool asset = path == "/index.js" || path == "/index.css";
        log("_startSetupInterfaceServer(): onNotFound(): path: " + path);
        _getFile(asset ? path : "/index.html", [this](File file, String mime) {
            log("_startSetupInterfaceServer(): onNotFound(): _getFile(): mime: " + mime);
            _server.streamFile(file, mime);
        });
    });
    _server.on("/setup/save", HTTP_POST, [this]() {
        if (_server.hasArg(KEY_SETUP_COMPLETED)) {
            String wSsid = _server.arg(KEY_WIFI_SSID);
            String wPassword = _server.arg(KEY_WIFI_PASSWORD);
            String bCycle = _server.arg(KEY_BRIGHTNESS_CYCLE);
            String bMinimum = _server.arg(KEY_BRIGHTNESS_MINIMUM);
            log("MCU::_startSetupInterfaceServer(): _server.on(\"/setup/save\"): Saving WiFi credentials...");
            log("MCU::_startSetupInterfaceServer(): _server.on(\"/setup/save\"): wSsid: " + wSsid);
            log("MCU::_startSetupInterfaceServer(): _server.on(\"/setup/save\"): wPassword: " + wPassword);
            log("MCU::_startSetupInterfaceServer(): _server.on(\"/setup/save\"): bCycle: " + bCycle);
            log("MCU::_startSetupInterfaceServer(): _server.on(\"/setup/save\"): bMinimum: " + bMinimum);
            preferences.putString(KEY_WIFI_SSID, wSsid);
            preferences.putString(KEY_WIFI_PASSWORD, wPassword);
            preferences.putInt(KEY_BRIGHTNESS_CYCLE, bCycle.toInt());
            preferences.putFloat(KEY_BRIGHTNESS_MINIMUM, bMinimum.toFloat());
            preferences.putBool(KEY_SETUP_COMPLETED, true);
            _server.send(200, "application/json", _getJSON(true));
            kill(true);
        } else {
            log("MCU::_startSetupInterfaceServer(): _server.on(\"/setup/save\"): Missing required parameters.");
            _server.send(400, "application/json", _getJSON(false, "Missing required parameters!"));
        }
    });
    _server.begin();
    log("MCU::_startSetupInterfaceServer(): AP started with SSID.");
    log("MCU::_startSetupInterfaceServer(): name: " + name);
    log("MCU::_startSetupInterfaceServer(): DNS and server routes configured.");
}

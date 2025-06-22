#include <mcu.h>

/**
 * @brief Default constructor for the MCU class.
 * 
 * Initializes the MCU object and prepares it for use.
 */
MCU::MCU() {}

/**
 * @brief Initializes the MCU system.
 * 
 * Sets up any required subsystems or configurations.
 */
void MCU::begin() {
    Serial.begin(DEBUG_FREQUENCY);
    Serial.println();
    pinMode(PIN_LED_WIFI, OUTPUT);
    preferences.begin(FIRMWARE_NAME, false);
    _reConnect();
    if(!SPIFFS.begin(true)) {
        log("begin(): Failed to start SPIFS.");
    }
}

/**
 * @brief Logs a message to the storage system.
 * 
 * @param message The message to log.
 * @param success Indicates whether the message represents a successful event. Defaults to true.
 */
void MCU::log(
    String message,
    bool success
) {
    if (DEBUG_MODE_ENABLED) {
        Serial.println(
            _rtc.read() + " @ " + (
                success ? "Debug" : "Error"
            ) + " -> " + message
        );
    }
}

/**
 * @brief Makes an HTTP GET request to the specified URL.
 * 
 * This function performs an HTTP GET request to the provided URL.
 * 
 * @param url The URL to make the HTTP request to.
 * @param onSuccess Callback function to be executed upon successful HTTP request.
 *                  It takes three parameters: success status, HTTP status code, and response body.
 *                  Default is an empty function.
 * @return true if the HTTP request was successful, false otherwise.
 */
bool MCU::httpRequest(
    String url,
    std::function<void(bool, int, String)> onSuccess,
    JsonDocument requestBody
) {
    log("httpRequest(): url: " + url);
    if (WiFi.status() == WL_CONNECTED) {
        int httpCode;
        bool isSuccess;
        _client.begin(url);
        _client.setReuse(true);
        _client.setTimeout(HTTP_REQUEST_TIMEOUT);
        _client.addHeader("User-Agent", "ESP32HTTPClient");
        if (requestBody.isNull()) {
            httpCode = _client.GET();
            isSuccess = httpCode == HTTP_CODE_OK;
            log("httpRequest(): Requested in GET method.");
        } else {
            String requestString;
            serializeJson(requestBody, requestString);
            _client.addHeader("Content-Type", "application/json");
            httpCode = _client.POST(requestString);
            isSuccess = httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_FOUND || httpCode == HTTPC_ERROR_READ_TIMEOUT;
            log("httpRequest(): Requested in POST method.");
            log("httpRequest(): requestString: " + requestString);
        }
        String response = _client.getString();
        _client.end();
        log("httpRequest(): isSuccess: " + _sBool(isSuccess), isSuccess);
        log("httpRequest(): httpCode: " + String(httpCode));
        log("httpRequest(): response: " + response);
        _setStandardRequestIntervalFactor(isSuccess);
        onSuccess(isSuccess, httpCode, response);
        return isSuccess;
    }
    log("httpRequest(): Failed because WiFi is not connected.", false);
    _setStandardRequestIntervalFactor(false);
    _reConnect();
    onSuccess(false, 0, "");
    return false;
}

/**
 * @brief Gets the standard request interval in milliseconds.
 * 
 * Returns the request interval value based on the success state of the previous operation.
 * 
 * @param isSuccess A boolean indicating whether the previous operation was successful.
 * If `true`, returns the fixed `TIME_UPDATE_INTERVAL`. If `false`, returns a calculated
 * interval based on `_standardRequestIntervalFactor` and `HTTP_REQUEST_INTERVAL`.
 * 
 * @return int The appropriate request interval in milliseconds.
 */
int MCU::getTimeUpdateInterval(
    bool isSuccess
) {
    if (isSuccess) return TIME_UPDATE_INTERVAL;
    return _standardRequestIntervalFactor * HTTP_REQUEST_INTERVAL;
}

/**
 * @brief Sets the state of the WiFi status LED.
 * 
 * This function writes a digital value (HIGH or LOW) to the WiFi LED pin (`PIN_LED_WIFI`),
 * allowing the LED to be turned on or off based on the provided value.
 * 
 * @param value The digital output value to set on the WiFi LED pin. 
 *              Use `HIGH` to turn the LED on, and `LOW` to turn it off.
 * 
 * @see digitalWrite
 * @see PIN_LED_WIFI
 */
void MCU::setWLED(
    uint8_t value
) {
    digitalWrite(PIN_LED_WIFI, value);
}

/**
 * @brief Creates and assigns a FreeRTOS task to a specific CPU core.
 * 
 * This function wraps the FreeRTOS xTaskCreatePinnedToCore API to create a task
 * pinned to the specified CPU core. The task is created with a fixed stack size of 8192 bytes,
 * priority 1, and no parameters or task handle returned.
 * 
 * @param cpuCore The CPU core number to which the task will be pinned.
 * @param function The task function to be executed by the created task.
 */
void MCU::assign(
    int cpuCore,
    TaskFunction_t function,
    void* arguments
) {
    xTaskCreatePinnedToCore(
        function,
        "_mcu.assign()",
        8192,
        arguments,
        1,
        NULL,
        cpuCore
    );
    log("assign(): Task assigned to cpu cpuCore: " + String(cpuCore) + ".");
}

/**
 * @brief Delays the current task for the specified number of milliseconds.
 *
 * This function wraps `vTaskDelay`, pausing the current task for the given number of milliseconds.
 *
 * @param milliseconds The delay duration in milliseconds.
 *
 * @note This function must be called from within a FreeRTOS task context.
 */
void MCU::delay(
    uint32_t milliseconds
) {
    vTaskDelay(milliseconds / portTICK_PERIOD_MS);
}

/**
 * @brief Delays the current task and then executes a callback.
 *
 * This function wraps `vTaskDelay`, pausing the current task for the given number of milliseconds.
 * After the delay completes, the specified callback function is executed.
 *
 * @param milliseconds The delay duration in milliseconds.
 * @param onExecute A `std::function<void()>` callback to run after the delay.
 *
 * @note This function must be called from within a FreeRTOS task context.
 */
void MCU::delay(
    uint32_t milliseconds,
    std::function<void()> onExecute
) {
    vTaskDelay(milliseconds / portTICK_PERIOD_MS);
    onExecute();
}

/**
 * @brief Terminates the current task or restarts the system.
 * 
 * Depending on the `system` parameter, this function either restarts the entire ESP32
 * device or deletes the currently executing FreeRTOS task.
 * 
 * @param system If true, the ESP32 system will restart via ESP.restart().  
 *               If false, only the calling FreeRTOS task will be terminated using vTaskDelete(NULL).
 * 
 * @note Use with caution. Restarting the system interrupts all tasks and operations.
 * @see ESP.restart
 * @see vTaskDelete
 */
void MCU::kill(
    bool system
) {
    if (system) {
        delay(DEVICE_RESTART_TIMEOUT, [this]() {
            log("kill(): delay(): Restarting device...");
        });
        ESP.restart();
    } else {
        log("kill(): Killing task...");
        vTaskDelete(NULL);
    }
}

/**
 * @brief Adjusts the standard request interval factor.
 *
 * Modifies the `_standardRequestIntervalFactor` variable based on the `decrease` parameter.
 * If `decrease` is true, decrements `_standardRequestIntervalFactor` unless it's already at its minimum.
 * If `decrease` is false, increments `_standardRequestIntervalFactor` unless it's already at its maximum.
 * Logs the change and current factor value.
 *
 * @param decrease Boolean flag: true to decrease factor, false to increase.
 */
void MCU::_setStandardRequestIntervalFactor(
    bool decrease
) {
    if (decrease) {
        if (_standardRequestIntervalFactor > 1) {
            _standardRequestIntervalFactor--;
            log("_setStandardRequestInterval(): Decremented standard request interval.");
        }
    } else {
        if (_standardRequestIntervalFactor < 10) {
            _standardRequestIntervalFactor++;
            log("_setStandardRequestInterval(): Incremented standard request interval.");
        }
    }
    log("_setStandardRequestInterval(): _standardRequestIntervalFactor: " + String(_standardRequestIntervalFactor));
    log("_setStandardRequestInterval(): decrease: " + _sBool(decrease));
}

/**
 * @brief Attempts to reconnect to WiFi using stored credentials.
 * 
 * This method retrieves the WiFi SSID and password from non-volatile storage (Preferences).
 * If both the SSID and password are found and non-empty, it initiates a connection
 * using `WiFi.begin()`.
 * 
 * @note This function does not block or wait for the connection to complete. Use 
 *       `WiFi.status()` to monitor connection status externally.
 * 
 * @see preferences.getString
 * @see WiFi.begin
 */
void MCU::_reConnect() {
    if (preferences.isKey(KEY_WIFI_SSID) && preferences.isKey(KEY_WIFI_PASSWORD)) {
        log("_reConnect(): WiFi is connecting to " + preferences.getString(KEY_WIFI_SSID));
        WiFi.begin(
            preferences.getString(KEY_WIFI_SSID),
            preferences.getString(KEY_WIFI_PASSWORD)
        );
    } else if (!_isServerRunning) {
        log("_reConnect(): WiFi credentials not found.", false);
        log("_reConnect(): Starting setup interface server...");
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
    } else {
        log("_reConnect(): Waiting for the user to configure the device...");
    }
}

/**
 * @brief Loads and processes the HTML setup page from SPIFFS.
 *
 * This function reads the contents of the `/index.html` file stored
 * in SPIFFS, replaces placeholder tokens (`{SETUP_INTERFACE_TITLE}`, `{KEY_WIFI_SSID}`,
 * `{KEY_WIFI_PASSWORD}`) with actual runtime values, and returns the
 * resulting HTML string.
 *
 * @return A String containing the processed HTML. If the file fails to open,
 *         an error message string is returned instead.
 *
 * @note This function logs an error and returns a plain text string if
 *       `/index.html` cannot be opened.
 */
String MCU::_getHTML() {
    String body = "";
    File file = SPIFFS.open("/index.html", "r");
    if (!file) {
        log("_getHTML(): Failed to open /index.html.", false);
        return "_getHTML(): Failed to open /index.html.";
    }
    while (file.available()) {
        body += file.readString();
    }
    file.close();
    log("_getHTML(): Successfully loaded setup HTML.");
    body.replace("{SETUP_INTERFACE_TITLE}", _getSetupHotspotName() + " v(" + FIRMWARE_VERSION + ")");
    body.replace("{KEY_WIFI_SSID}", KEY_WIFI_SSID);
    body.replace("{KEY_WIFI_PASSWORD}", KEY_WIFI_PASSWORD);
    return body;
}

/**
 * @brief Generate a JSON response string with success status and an optional message.
 *
 * This function creates a JSON response string containing a "success" key with a boolean value
 * and optionally a "message" key if a non-empty message is provided.
 *
 * @param success A boolean indicating the success status of the operation.
 * @param message An optional string containing a message to include in the response.
 * @return A String object containing the JSON-formatted response.
 *
 * @note The JSON is generated using the ArduinoJson library. The result will include:
 *       - "success": true or false
 *       - "message": (if provided) a descriptive string
 *
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
 * @brief Generates a unique setup name using the device's MAC address.
 * 
 * The function extracts the lower 24 bits of the device's MAC address,
 * converts it to a 6-digit uppercase hexadecimal string (padded with leading zeros if needed),
 * and appends it to the firmware name to create a unique identifier.
 * 
 * @return String A unique setup name.
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
 * @brief Initializes the setup interface server in Access Point (AP) mode.
 * 
 * This function sets up the ESP32 as a Wi-Fi access point with a fixed IP configuration
 * and starts a minimal web server for user configuration. It also initializes a DNS server 
 * to redirect all domains to the local web server, enabling captive portal behavior.
 * 
 * The server handles two types of requests:
 * - `onNotFound`: Serves the setup HTML page for all unknown routes.
 * - `"/save"` (HTTP POST): Accepts Wi-Fi SSID and password from the user, saves them to 
 *   non-volatile storage using the `preferences` API, and then reboots the device.
 * 
 * Logging is performed throughout the process for debugging purposes.
 * 
 * @note If the AP fails to start, the function logs an error and exits early.
 * 
 * @see WiFi.softAPConfig
 * @see WiFi.softAP
 * @see Preferences::putString
 */
void MCU::_startSetupInterfaceServer() {
    IPAddress local(1, 1, 1, 1);
    IPAddress gateway(1, 1, 1, 1);
    IPAddress subnet(255, 255, 255, 0);
    String hotspotName = _getSetupHotspotName();
    WiFi.softAPConfig(local, gateway, subnet);
    if (!WiFi.softAP(hotspotName)) {
        log("_startSetupInterfaceServer(): Could not start setup interface server!", false);
        return;
    }
    _dns.start(53, "*", local);
    _server.onNotFound([this]() {
        _server.send(200, "text/html", _getHTML());
    });
    _server.on("/wifi/save", HTTP_POST, [this]() {
        if (_server.hasArg(KEY_WIFI_SSID) && _server.hasArg(KEY_WIFI_PASSWORD)) {
            String ssid = _server.arg(KEY_WIFI_SSID);
            String password = _server.arg(KEY_WIFI_PASSWORD);
            log("_startSetupInterfaceServer(): _server.on(\"/wifi/save\"): Saving WiFi credentials...");
            log("_startSetupInterfaceServer(): _server.on(\"/wifi/save\"): ssid: " + ssid);
            log("_startSetupInterfaceServer(): _server.on(\"/wifi/save\"): password: " + password);
            preferences.putString(KEY_WIFI_SSID, ssid);
            preferences.putString(KEY_WIFI_PASSWORD, password);
            _server.send(200, "application/json", _getJSON(true));
            kill(true);
        } else {
            log("_startSetupInterfaceServer(): _server.on(\"/wifi/save\"): Missing required parameters.");
            _server.send(400, "application/json", _getJSON(false, "Missing required parameters!"));
        }
    });
    _server.begin();
    _isServerRunning = true;
    log("_startSetupInterfaceServer(): AP started with SSID.");
    log("_startSetupInterfaceServer(): hotspotName: " + hotspotName);
    log("_startSetupInterfaceServer(): DNS and server routes configured.");
}
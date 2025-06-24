#include <mcu.h>

MCU::MCU() {}

void MCU::begin() {
    Serial.begin(DEBUG_FREQUENCY);
    Serial.println();
    log("MCU::begin(): Initializing MCU...");
    pinMode(PIN_LED_WIFI, OUTPUT);
    preferences.begin(FIRMWARE_NAME, false);
    _reConnect();
    if(!SPIFFS.begin(true)) {
        log("MCU::begin(): Failed to start SPIFS!", false);
    }
}

void MCU::log(
    String message,
    bool success
) {
    if (DEBUG_MODE_ENABLED) {
        Serial.println(
            _timestamp + " @ " + (
                success ? "Debug" : "Error"
            ) + " -> " + message
        );
    }
}

bool MCU::httpRequest(
    String url,
    std::function<void(bool, int, String)> onSuccess,
    JsonDocument requestBody
) {
    log("MCU::httpRequest(): url: " + url);
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
            log("MCU::httpRequest(): Requested in GET method.");
        } else {
            String requestString;
            serializeJson(requestBody, requestString);
            _client.addHeader("Content-Type", "application/json");
            httpCode = _client.POST(requestString);
            isSuccess = httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_FOUND || httpCode == HTTPC_ERROR_READ_TIMEOUT;
            log("MCU::httpRequest(): Requested in POST method.");
            log("MCU::httpRequest(): requestString: " + requestString);
        }
        String response = _client.getString();
        _client.end();
        log("MCU::httpRequest(): Response fully received and client closed.");
        log("MCU::httpRequest(): isSuccess: " + _sBool(isSuccess), isSuccess);
        log("MCU::httpRequest(): httpCode: " + String(httpCode));
        log("MCU::httpRequest(): response: " + response);
        _setStandardRequestIntervalFactor(isSuccess);
        onSuccess(isSuccess, httpCode, response);
        return isSuccess;
    }
    log("MCU::httpRequest(): Failed because WiFi is not connected.", false);
    _setStandardRequestIntervalFactor(false);
    _reConnect();
    onSuccess(false, 0, "");
    return false;
}

int MCU::getTimeUpdateInterval(
    bool isSuccess
) {
    if (isSuccess) return TIME_UPDATE_INTERVAL;
    return _standardRequestIntervalFactor * HTTP_REQUEST_INTERVAL;
}

void MCU::setWLED(
    uint8_t value
) {
    digitalWrite(PIN_LED_WIFI, value);
}

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

void MCU::delay(
    uint32_t milliseconds
) {
    vTaskDelay(milliseconds / portTICK_PERIOD_MS);
}

void MCU::delay(
    uint32_t milliseconds,
    std::function<void()> onExecute
) {
    vTaskDelay(milliseconds / portTICK_PERIOD_MS);
    onExecute();
}

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

void MCU::updateLogginTimestamp(
    String timestamp
) {
    _timestamp = timestamp;
}

void MCU::_setStandardRequestIntervalFactor(
    bool decrease
) {
    if (decrease) {
        if (_standardRequestIntervalFactor > 1) {
            _standardRequestIntervalFactor--;
            log("MCU::_setStandardRequestInterval(): Decremented standard request interval.");
        }
    } else {
        if (_standardRequestIntervalFactor < 10) {
            _standardRequestIntervalFactor++;
            log("MCU::_setStandardRequestInterval(): Incremented standard request interval.");
        }
    }
    log("MCU::_setStandardRequestInterval(): _standardRequestIntervalFactor: " + String(_standardRequestIntervalFactor));
    log("MCU::_setStandardRequestInterval(): decrease: " + _sBool(decrease));
}

void MCU::_reConnect() {
    if (preferences.isKey(KEY_WIFI_SSID) && preferences.isKey(KEY_WIFI_PASSWORD)) {
        log("MCU::_reConnect(): WiFi will be connected to \"" + preferences.getString(KEY_WIFI_SSID) + "\" network.");
        WiFi.begin(
            preferences.getString(KEY_WIFI_SSID),
            preferences.getString(KEY_WIFI_PASSWORD)
        );
    } else if (!_isServerRunning) {
        log("MCU::_reConnect(): WiFi credentials not found!", false);
        log("MCU::_reConnect(): Starting setup interface server...");
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
        log("MCU::_reConnect(): Waiting for the user to configure the device...");
    }
}

String MCU::_getSetupHotspotName() {
    uint64_t mac = ESP.getEfuseMac();
    String suffix = String(mac & 0xFFFFFF, HEX);
    suffix.toUpperCase();
    while (suffix.length() < 6) {
        suffix = "0" + suffix;
    }
    return String(FIRMWARE_NAME) + "_" + suffix;
}

String MCU::_getHTML(
    String title
) {
    String body = "";
    File file = SPIFFS.open("/index.html", "r");
    if (!file) {
        log("MCU::_getHTML(): Failed to open /index.html.", false);
        return "MCU::_getHTML(): Failed to open /index.html.";
    }
    while (file.available()) {
        body += file.readString();
    }
    file.close();
    log("MCU::_getHTML(): Successfully loaded setup HTML.");
    body.replace("{SETUP_INTERFACE_TITLE}", title);
    body.replace("{KEY_WIFI_SSID}", KEY_WIFI_SSID);
    body.replace("{KEY_WIFI_PASSWORD}", KEY_WIFI_PASSWORD);
    body.replace("{FIRMWARE_VERSION}", FIRMWARE_VERSION);
    return body;
}

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
        _server.send(200, "text/html", _getHTML(name));
    });
    _server.on("/wifi/save", HTTP_POST, [this]() {
        if (_server.hasArg(KEY_WIFI_SSID) && _server.hasArg(KEY_WIFI_PASSWORD)) {
            String ssid = _server.arg(KEY_WIFI_SSID);
            String password = _server.arg(KEY_WIFI_PASSWORD);
            log("MCU::_startSetupInterfaceServer(): _server.on(\"/wifi/save\"): Saving WiFi credentials...");
            log("MCU::_startSetupInterfaceServer(): _server.on(\"/wifi/save\"): ssid: " + ssid);
            log("MCU::_startSetupInterfaceServer(): _server.on(\"/wifi/save\"): password: " + password);
            preferences.putString(KEY_WIFI_SSID, ssid);
            preferences.putString(KEY_WIFI_PASSWORD, password);
            _server.send(200, "application/json", _getJSON(true));
            kill(true);
        } else {
            log("MCU::_startSetupInterfaceServer(): _server.on(\"/wifi/save\"): Missing required parameters.");
            _server.send(400, "application/json", _getJSON(false, "Missing required parameters!"));
        }
    });
    _server.begin();
    _isServerRunning = true;
    log("MCU::_startSetupInterfaceServer(): AP started with SSID.");
    log("MCU::_startSetupInterfaceServer(): name: " + name);
    log("MCU::_startSetupInterfaceServer(): DNS and server routes configured.");
}

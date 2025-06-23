#include <mcu.h>

MCU::MCU() {}

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
        "_mcu.assign()",
        8192,
        arguments,
        1,
        NULL,
        cpuCore
    );
    log("assign(): Task assigned to cpu cpuCore: " + String(cpuCore) + ".");
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
        delay(DEVICE_RESTART_TIMEOUT, [this]() {
            log("kill(): delay(): Restarting device...");
        });
        ESP.restart();
    } else {
        log("kill(): Killing task...");
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

String MCU::_getSetupHotspotName() {
    uint64_t mac = ESP.getEfuseMac();
    String suffix = String(mac & 0xFFFFFF, HEX);
    suffix.toUpperCase();
    while (suffix.length() < 6) {
        suffix = "0" + suffix;
    }
    return String(FIRMWARE_NAME) + "_" + suffix;
}

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

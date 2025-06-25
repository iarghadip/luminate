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
            getTime() + " @ " + (
                success ? "Debug" : "Error"
            ) + " -> " + message
        );
    }
}

int MCU::getTimeUpdateInterval(
    bool isSuccess
) {
    _setStandardRequestIntervalFactor(isSuccess);
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

bool MCU::setTime() {
    log("MCU::setTime(): Updating clock time...");
    configTime(19800, 0, "pool.ntp.org", "time.nist.gov");
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        log("MCU::setTime(): Failed to update clock!", false);
        return false;
    }
    log("MCU::setTime(): Clock was updated.");
    return true;
}

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

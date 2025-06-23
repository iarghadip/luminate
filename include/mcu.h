#ifndef mcu_h
#define mcu_h

// Libraries from system framework
#include <Arduino.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <SPIFFS.h>
#include <WebServer.h>
#include <WiFi.h>
#include <Wire.h>

/**
 * @class MCU
 * @brief Provides utilities for system setup, HTTP communication, task management, and logging.
 * 
 * The MCU class wraps core functionalities such as delayed execution, HTTP requests, 
 * Wi-Fi configuration, file access via SPIFFS, and task control using FreeRTOS.
 */
class MCU {
    public:
        Preferences preferences; // Preferences instance for persistent key-value storage.
        bool isUserInterrupt = false; // Tracks user button interrupt state.

        /**
         * @brief Constructs an MCU object.
         * 
         * Initializes internal state and components.
         */
        MCU();

        /**
         * @brief Initializes system components.
         * 
         * Performs setup for peripherals, storage, and network subsystems.
         */
        void begin();

        /**
         * @brief Logs a message with optional success indication.
         * 
         * @param message The message to log.
         * @param success Flag to indicate if this log entry is a success (default: true).
         */
        void log(
            String message,
            bool success = true
        );

        /**
         * @brief Performs an HTTP GET request to the specified URL.
         * 
         * Optionally accepts a callback to handle the result.
         * 
         * @param url Target URL for the HTTP request.
         * @param onSuccess Optional callback with signature: (success, HTTP status code, response body).
         * @param requestBody JSON document to include in the request (if needed).
         * @return true if request succeeded, false otherwise.
         */
        bool httpRequest(
            String url,
            std::function<void(bool, int, String)> onSuccess = {},
            JsonDocument requestBody = JsonDocument()
        );

        /**
         * @brief Returns an appropriate time interval based on the success of the last request.
         * 
         * @param isSuccess Whether the last operation succeeded.
         * @return Time interval in milliseconds.
         */
        int getTimeUpdateInterval(
            bool isSuccess
        );

        /**
         * @brief Sets the Wi-Fi status LED pin to HIGH or LOW.
         * 
         * @param value Digital output value (`HIGH` or `LOW`).
         */
        void setWLED(
            uint8_t value
        );

        /**
         * @brief Creates and pins a FreeRTOS task to a specified CPU core.
         * 
         * @param cpuCore CPU core index (0 or 1).
         * @param function Task function (must match FreeRTOS `TaskFunction_t` signature).
         * @param arguments Optional pointer to arguments passed to the task.
         */
        void assign(
            int cpuCore,
            TaskFunction_t function,
            void* arguments = NULL
        );

        /**
         * @brief Delays task execution for the specified duration.
         * 
         * @param milliseconds Time to delay, in milliseconds.
         */
        void delay(
            uint32_t milliseconds
        );

        /**
         * @brief Delays task execution, then runs a callback.
         * 
         * @param milliseconds Delay duration in milliseconds.
         * @param onExecute Callback to execute after the delay.
         */
        void delay(
            uint32_t milliseconds,
            std::function<void()> onExecute
        );

        /**
         * @brief Terminates the current task or restarts the system.
         * 
         * @param system If true, the MCU will restart; if false, only the current task is deleted.
         */
        void kill(
            bool system = false
        );

        /**
         * @brief Updates the internal timestamp with a new value.
         * 
         * Stores or replaces the current timestamp used for tracking or logging purposes.
         * 
         * @param timestamp A string representing the new timestamp.
         */
        void updateLogginTimestamp(
            String timestamp
        );

    private:
        DNSServer _dns; // Internal DNS server for captive portal.
        HTTPClient _client; // HTTP client for outgoing requests.
        WebServer _server; // Web server for setup interface.
        String _timestamp = "NaN"; // Stores the most recent timestamp as a string.
        int _standardRequestIntervalFactor = 1; // Multiplier for retry timing logic.
        bool _isServerRunning = false; // Indicates if setup interface server is running.

        /**
         * @brief Converts a boolean to its string representation.
         * 
         * @param x Boolean value.
         * @return "true" or "false".
         */
        #define _sBool(x) String((x) ? "true" : "false")

        /**
         * @brief Adjusts the internal retry interval factor.
         * 
         * @param decrease If true, decrease the factor; otherwise, increase it.
         */
        void _setStandardRequestIntervalFactor(
            bool decrease
        );

        /**
         * @brief Attempts to reconnect to Wi-Fi using stored credentials.
         * 
         * Reads SSID and password from `Preferences` and calls `WiFi.begin()`.
         */
        void _reConnect();

        /**
         * @brief Loads and token-replaces the HTML setup page from SPIFFS.
         * 
         * Replaces `{TITLE}`, `{KEY_WIFI_SSID}`, `{KEY_WIFI_PASSWORD}` in `/index.html`.
         * 
         * @return Processed HTML string, or error message if the file fails to open.
         */
        String _getHTML();

        /**
         * @brief Generates a JSON response with success flag and optional message.
         * 
         * @param success Result status.
         * @param message Optional message to include.
         * @return JSON-formatted response string.
         */
        String _getJSON(
            bool success,
            String message = ""
        );

        /**
         * @brief Builds a unique Wi-Fi hotspot name using the device's MAC address.
         * 
         * @return Unique SSID string.
         */
        String _getSetupHotspotName();

        /**
         * @brief Starts the setup web server in Access Point mode.
         * 
         * Initializes AP + DNS + captive portal and handles user input for Wi-Fi setup.
         */
        void _startSetupInterfaceServer();
};

#endif // mcu_h

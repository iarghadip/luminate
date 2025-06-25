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
        /**
         * @brief Enumeration for date/time formatting options.
         */
        enum Format {
            DATE_TIME, ///< Output includes both date and time.
            DATE_ONLY, ///< Output includes only the date.
            TIME_ONLY  ///< Output includes only the time.
        };

        Preferences preferences; // Preferences instance for persistent key-value storage.
        bool isUserInterrupt = false; // Tracks user button interrupt state.
        bool isTimeUpdated = false; // Indicates if clock time is up to date.

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
         * @brief Synchronizes the ESP32 internal RTC using an NTP server.
         * 
         * Connects to a predefined NTP server (e.g., pool.ntp.org) and sets the internal 
         * system time using `configTime()` and `settimeofday()`. 
         * 
         * Requires an active Wi-Fi connection.
         * 
         * @return true if synchronization was successful, false on failure.
         */
        bool setTime();

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
        String getTime(
            Format format = DATE_TIME
        );

    private:
        DNSServer _dns; // Internal DNS server for captive portal.
        HTTPClient _client; // HTTP client for outgoing requests.
        WebServer _server; // Web server for setup interface.
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
         * @brief Builds a unique Wi-Fi hotspot name using the device's MAC address.
         * 
         * @return Unique SSID string.
         */
        String _getSetupHotspotName();

        /**
         * @brief Loads and token-replaces the HTML setup page from SPIFFS.
         * 
         * Reads the contents of `/index.html` from SPIFFS and replaces template tokens
         * with runtime values. The following placeholders are replaced:
         * - `{TITLE}` → the provided `title` argument
         * - `{KEY_WIFI_SSID}` → stored Wi-Fi SSID from preferences
         * - `{KEY_WIFI_PASSWORD}` → stored Wi-Fi password from preferences
         * 
         * @param title The title to inject into the HTML (replaces `{TITLE}`).
         * @return A processed HTML string, or a plain text error message if the file cannot be opened.
         */
        String _getHTML(
            String title
        );

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
         * @brief Starts the setup web server in Access Point mode.
         * 
         * Initializes AP + DNS + captive portal and handles user input for Wi-Fi setup.
         */
        void _startSetupInterfaceServer();
};

#endif // mcu_h

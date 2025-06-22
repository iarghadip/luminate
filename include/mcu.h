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

// Libraries from module wrappers
#include <rtc.h>

/**
 * @brief Provides utility functions for delayed execution and logging.
 * 
 * The MCU class allows scheduling functions and logging messages with optional success flags.
 */
class MCU {
    public:
        Preferences preferences; ///< Prederences instance.
        bool isUserInterrupt = false; ///< Stores user button press interrupts.

        /**
         * @brief Constructs a MCU object.
         * 
         * Initializes internal state and dependencies.
         */
        MCU();

        /**
         * @brief Initializes the MCU system.
         * 
         * Sets up any required subsystems or configurations.
         */
        void begin();

        /**
         * @brief Logs a message to the storage system.
         * 
         * @param message The message to log.
         * @param success Indicates whether the message represents a successful event. Defaults to true.
         */
        void log(
            String message,
            bool success = true
        );

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
        bool httpRequest(
            String url,
            std::function<void(bool, int, String)> onSuccess = {},
            JsonDocument requestBody = JsonDocument()
        );

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
        int getTimeUpdateInterval(
            bool isSuccess
        );

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
        void setWLED(
            uint8_t value
        );

        /**
         * @brief Creates and assigns a FreeRTOS task to a specific CPU core.
         * 
         * This function wraps the FreeRTOS `xTaskCreatePinnedToCore` API to create a task
         * pinned to the specified CPU core. The task is created with a fixed stack size of 8192 bytes,
         * priority 1, and no task handle is returned.
         * 
         * @param cpuCore The CPU core number (typically 0 or 1 on ESP32) to which the task will be pinned.
         * @param function The task function to be executed. It must match the FreeRTOS TaskFunction_t signature: `void (*)(void*)`.
         * @param arguments Optional argument pointer to be passed to the task function. Defaults to `NULL`.
         */
        void assign(
            int cpuCore,
            TaskFunction_t function,
            void* arguments = NULL
        );

        /**
         * @brief Delays the current task for the specified number of milliseconds.
         *
         * This function wraps `vTaskDelay`, pausing the current task for the given number of milliseconds.
         *
         * @param milliseconds The delay duration in milliseconds.
         *
         * @note This function must be called from within a FreeRTOS task context.
         */
        void delay(
            uint32_t milliseconds
        );

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
        void delay(
            uint32_t milliseconds,
            std::function<void()> onExecute
        );

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
        void kill(
            bool system = false
        );

    private:
        DNSServer _dns; ///< DNS Server instance.
        HTTPClient _client; ///< HTTP client instance.
        WebServer _server; ///< Web Server instance.
        RTC _rtc; ///< Object representing the rtc wrapper.
        int _standardRequestIntervalFactor = 1; ///< Standard request interval factor.
        bool _isServerRunning = false; ///< Stores setup interface server running status.

        /**
         * @brief Converts a boolean value to its string representation.
         * 
         * @param x The boolean value to be converted.
         * @return A String object representing "true" if the value is true, otherwise "false".
         */
        #define _sBool(x) String((x) ? "true" : "false")

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
        void _setStandardRequestIntervalFactor(
            bool decrease
        );

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
        void _reConnect();

        /**
         * @brief Loads and processes the HTML setup page from SPIFFS.
         *
         * This function reads the contents of the `/index.html` file stored
         * in SPIFFS, replaces placeholder tokens (`{TITLE}`, `{KEY_WIFI_SSID}`,
         * `{KEY_WIFI_PASSWORD}`) with actual runtime values, and returns the
         * resulting HTML string.
         *
         * @return A String containing the processed HTML. If the file fails to open,
         *         an error message string is returned instead.
         *
         * @note This function logs an error and returns a plain text string if
         *       `/index.html` cannot be opened.
         */
        String _getHTML();

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
        String _getJSON(
            bool success,
            String message = ""
        );

        /**
         * @brief Generates a unique setup name using the device's MAC address.
         * 
         * The function extracts the lower 24 bits of the device's MAC address,
         * converts it to a 6-digit uppercase hexadecimal string (padded with leading zeros if needed),
         * and appends it to the firmware name to create a unique identifier.
         * 
         * @return String A unique setup name.
         */
        String _getSetupHotspotName();

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
        void _startSetupInterfaceServer();
};

#endif ///< mcu_h
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
        int brightnessCycle = 6; // Preferences instance for persistent key-value storage.
        bool isBrightnessInherit; // Tracks user button interrupt state.
        float brightnessMinimum = 5.0f; // Indicates if clock time is up to date.

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
         * @brief Terminates the current task or restarts the system.
         * 
         * @param system If true, the MCU will restart; if false, only the current task is deleted.
         */
        void kill(
            bool system = false
        );

        /**
         * @brief Delays task execution for a specified time and then optionally executes a callback.
         *
         * Suspends the current FreeRTOS task for at least the given number of milliseconds.
         * If a callback is provided, it will be executed after the delay.
         *
         * @param milliseconds Duration to delay in milliseconds.
         * @param onExecute Optional callback to run after the delay.
         */
        void delay(uint32_t milliseconds, std::function<void()> onExecute = {});

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

        /**
         * @brief Converts a boolean to its string representation.
         * 
         * @param x Boolean value.
         * @return "true" or "false".
         */
        #define _sBool(x) String((x) ? "true" : "false")

        /**
         * @brief Updates MCU settings from persistent preferences.
         *
         * Reads stored values from the preferences storage to update
         * the brightness cycle hour, brightness inheritance flag,
         * and minimum brightness value if setup has been completed.
         *
         * - Updates `brightnessCycle` with the stored cycle start hour.
         * - If brightness inheritance is enabled, updates `isBrightnessInherit`
         *   and `brightnessMinimum` from preferences.
         *
         * @note This function assumes that the preferences instance
         *       has been properly initialized.
         */
        void _updatePreferences();

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
        void _updateConnection();

        /**
         * @brief Initialize and synchronize the ESP32 system time with NTP servers.
         *
         * Configures the ESP32's internal RTC to synchronize with specified NTP servers
         * ("pool.ntp.org" and "time.nist.gov") using the current GMT offset.
         * After calling this function, the ESP32 will periodically auto-sync its time with the NTP servers.
         *
         * Requires an active Wi-Fi connection.
         * Use getLocalTime() to retrieve the updated time from the internal RTC.
         *
         * @note
         * - Manual periodic updates are not required; ESP32 will auto-sync in the background.
         * - Call this function again only if you need to reconfigure NTP servers or after Wi-Fi reconnection.
         *
         * @see configTime
         * @see getLocalTime
         */
        void _updateTime();

        /**
         * @brief Builds a unique Wi-Fi hotspot name using the device's MAC address.
         * 
         * @return Unique SSID string.
         */
        String _getSetupHotspotName();

        /**
         * @brief Opens a file from SPIFFS and calls onLoad with the file and its MIME type.
         * 
         * @param path The path to the file.
         * @param onLoad Callback function with the opened file and its MIME type.
         */
        void _getFile(
            String path,
            std::function<void(File file, String mime)> onLoad
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

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
        float brightnessMinimum = 0.0f; // Indicates if clock time is up to date.

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
        bool isTimeUpdated();

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
        bool _isTimeUpdated = false; // Indicates if clock time is up to date.

        /**
         * @brief Converts a boolean to its string representation.
         * 
         * @param x Boolean value.
         * @return "true" or "false".
         */
        #define _sBool(x) String((x) ? "true" : "false")

        /**
         * @brief Loads and applies MCU brightness settings from persistent storage.
         *
         * Reads configuration values from preferences and updates the MCU instance.
         * Only performs the update if the setup process has been completed.
         *
         * The following settings are updated:
         * - `brightnessCycle`: The configured start hour for the brightness adjustment cycle.
         * - `brightnessMinimum`: The minimum brightness level, as stored in preferences.
         *
         * @note The `isBrightnessInherit` property is not modified by this function.
         * @warning Assumes the preferences instance is properly initialized and accessible.
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

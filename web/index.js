/**
 * @fileoverview Manages form submission, input validation, UI updates, and nested square generation for Luminate Wi-Fi setup.
 */

document.addEventListener("DOMContentLoaded", () => {
    /** @type {?number} Timeout ID for resetting message text */
    let messageResetTimeoutId = null;

    /** @type {boolean} Tracks if the user is on the Preferences screen */
    let isOnPreferencesScreen = false;

    /** @type {HTMLFormElement} Main form element */
    const form = document.querySelector("form");
    /** @type {HTMLButtonElement} Submit button element */
    const submitButton = form.querySelector("button[type=submit]");

    /**
     * Displays a message and optionally resets it after a delay.
     * @param {string} text Message text to display.
     * @param {boolean} [resetAfterDelay=true] Whether to reset the message after a delay.
     */
    const displayMessage = (text, resetAfterDelay = true) => {
        const messageElement = document.getElementById("message");
        if (messageResetTimeoutId !== null) {
            clearTimeout(messageResetTimeoutId);
            messageResetTimeoutId = null;
        }
        if (resetAfterDelay) {
            messageResetTimeoutId = setTimeout(() => {
                messageElement.textContent = isOnPreferencesScreen
                    ? "What are your preferences?"
                    : "Connect Luminate to Wi-Fi?";
                messageResetTimeoutId = null;
            }, 2500);
        }
        messageElement.textContent = text;
    };

    /**
     * Safely parses a JSON string, applying corrections for common formatting errors.
     * @param {string} response JSON string to parse.
     * @returns {Object} Parsed object.
     */
    const safeParseJSON = (response) => {
        try {
            return JSON.parse(response);
        } catch {
            return JSON.parse(
                response
                    .replace(/'/g, '"')
                    .replace(/,\s*([}\]])/g, "$1")
                    .trim()
            );
        }
    };

    /**
     * Validates Wi-Fi SSID and password.
     * @param {string} ssid Wi-Fi SSID.
     * @param {string} password Wi-Fi password.
     * @returns {boolean} Whether the inputs are valid.
     */
    const validateWiFiCredentials = (ssid, password) =>
        typeof ssid === "string" &&
        typeof password === "string" &&
        ssid.length >= 1 &&
        ssid.length <= 32 &&
        password.length >= 8 &&
        password.length <= 63;

    /**
     * Validates brightness cycle and minimum values.
     * @param {string} cycle Brightness cycle (0–23).
     * @param {string} minimum Minimum brightness (0–50).
     * @returns {boolean} Whether the inputs are valid.
     */
    const validateBrightnessPreferences = (cycle, minimum) => {
        if (cycle === "" || minimum === "") return false;
        const cycleNum = Number(cycle);
        const minimumNum = Number(minimum);
        return (
            Number.isInteger(cycleNum) && cycleNum >= 0 && cycleNum <= 23 &&
            !isNaN(minimumNum) && minimumNum >= 0 && minimumNum <= 50
        );
    };

    /**
     * Enables or disables all form inputs and buttons.
     * @param {boolean} disabled True to disable, false to enable.
     */
    const toggleFormInputs = (disabled) => {
        form.querySelectorAll("input, button").forEach(el => el.disabled = disabled);
    };

    /**
     * Reads all form input values.
     * @returns {{ssid: string, password: string, cycle: string, minimum: string}} The input values.
     */
    const readFormInputs = () => ({
        ssid: form.querySelector("input[name='{KEY_WIFI_SSID}']").value,
        password: form.querySelector("input[name='{KEY_WIFI_PASSWORD}']").value,
        cycle: form.querySelector("input[name='{KEY_BRIGHTNESS_CYCLE}']").value,
        minimum: form.querySelector("input[name='{KEY_BRIGHTNESS_MINIMUM}']").value,
    });

    /**
     * Switches the UI to the Preferences screen.
     */
    const showPreferencesScreen = () => {
        isOnPreferencesScreen = true;
        document.getElementById("message").textContent = "What are your preferences?";
        document.getElementById("screen-1").style.display = "none";
        document.getElementById("screen-2").style.display = "block";
        submitButton.querySelector(".text").textContent = "Save Preferences";
    };

    /**
     * Handles form submission, validating inputs and communicating with the server.
     * @param {Event} event Form submission event.
     */
    const handleSubmit = (event) => {
        event.preventDefault();
        const { ssid, password, cycle, minimum } = readFormInputs();

        if (!isOnPreferencesScreen) {
            if (validateWiFiCredentials(ssid, password)) {
                submitButton.querySelector(".text").textContent = "Saving...";
                setTimeout(showPreferencesScreen, 1000);
            } else {
                displayMessage("WiFi credentials are invalid!");
            }
            return;
        }

        if (!validateWiFiCredentials(ssid, password)) {
            displayMessage("WiFi credentials are invalid!");
            return;
        }

        if (!validateBrightnessPreferences(cycle, minimum)) {
            displayMessage("Brightness values are invalid!");
            return;
        }

        submitButton.querySelector(".text").textContent = "Saving...";
        toggleFormInputs(true);

        setTimeout(() => {
            fetch("/wifi/save", {
                method: "POST",
                headers: { "Content-Type": "application/x-www-form-urlencoded" },
                body: new URLSearchParams({
                    '{KEY_WIFI_SSID}': ssid,
                    '{KEY_WIFI_PASSWORD}': password,
                    '{KEY_BRIGHTNESS_CYCLE}': cycle,
                    '{KEY_BRIGHTNESS_MINIMUM}': minimum,
                }),
            })
            .then(response => response.text())
            .then(responseText => {
                const data = safeParseJSON(responseText);
                if (data.success) {
                    displayMessage(`Luminate is connected to ${ssid}.`);
                    submitButton.querySelector(".text").textContent = "Completed";
                } else {
                    throw new Error(data.message || `Could not connect to ${ssid}!`);
                }
                console.log("response:", data);
            })
            .catch(error => {
                displayMessage(error.message || "Something went wrong!");
                submitButton.querySelector(".text").textContent = `Save ${isOnPreferencesScreen ? "Preferences" : "Network"}`;
                toggleFormInputs(false);
            });
        }, 1000);
    };

    /**
     * Generates nested square elements inside the container.
     */
    const createNestedSquares = () => {
        let current = document.createElement("div");
        current.className = "square black";
        document.querySelector(".container").appendChild(current);
        let isBlack = false;
        for (let i = 0; i < 30; i++) {
            const next = document.createElement("div");
            next.className = isBlack ? "square black" : "square";
            current.appendChild(next);
            current = next;
            isBlack = !isBlack;
        }
    };

    // --- Initialization ---
    form.addEventListener("submit", handleSubmit);
    createNestedSquares();
    document.getElementById("screen-2").style.display = "none";
});
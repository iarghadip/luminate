#ifndef btn_h
#define btn_h

// Libraries from module wrappers
#include <mcu.h>

/**
 * @brief The BTN class represents a push button with long press and short press detection.
 * 
 * This class allows the user to configure callbacks for single (short) press and long press events.
 * It works with a provided MCU object for hardware abstraction.
 */
class BTN {
    public:
        /**
         * @brief Default constructor.
         */
        BTN();

        /**
         * @brief Initializes the button and associates it with an MCU instance.
         * 
         * This method sets up the button GPIO using the MCU class and prepares internal state.
         * 
         * @param mcu Pointer to an MCU instance that provides hardware-level access.
         */
        void begin(
            MCU* mcu
        );

        /**
         * @brief Returns the current enabled state of the toggle switch.
         *
         * This function returns the last known state of the toggle switch as tracked by the BTN class.
         * It reflects whether the switch is currently considered enabled (ON) or disabled (OFF).
         *
         * @return true if the toggle switch is enabled (ON), false if it is disabled (OFF).
         */
        bool isToogleEnabled();

        /**
         * @brief Monitors the toggle switch and invokes a callback on state change.
         *
         * This method continuously checks the state of the toggle switch connected to PIN_SWITCH_DLS.
         * When the switch state changes (from ON to OFF or OFF to ON), the provided callback function
         * is called with the new state.
         *
         * @param onToggle A callback function that receives the current state of the switch.
         *                 - true:  Switch is ON (circuit closed, pin LOW)
         *                 - false: Switch is OFF (circuit open, pin HIGH)
         *
         * @note This function should be called repeatedly (e.g., in the main loop) to detect state changes.
         */
        void onToggle(
            std::function<void(bool status)> onToggle
        );

        /**
         * @brief Registers a callback to be executed on a short button press.
         * 
         * A short press is considered when the button is pressed and released 
         * within the long-press timeout (e.g., less than 5 seconds).
         * 
         * @param onSinglePress Callback function to invoke on a short press event.
         */
        void onSinglePress(
            std::function<void()> onSinglePress
        );

        /**
         * @brief Registers a callback to be executed on a long button press.
         * 
         * A long press is detected when the button remains pressed for more than 5 seconds.
         * 
         * @param onLongPress Callback function to invoke on a long press event.
         */
        void onLongPress(
            std::function<void()> onLongPress
        );

    private:
        MCU* _mcu; // Pointer to the MCU instance for GPIO interaction.
        unsigned long _pressStartTime = 0; // Timestamp when the button was initially pressed.
        bool _wasPressed = false; // Internal state flag to track if the button was previously pressed.
        bool _wasEnabled; // Tracks the previous enabled state of the switch (for edge detection).
};

#endif // btn_h

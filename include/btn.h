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
};

#endif // btn_h

#ifndef btn_h
#define btn_h

// Libraries from system framework
#include <Arduino.h>

/**
 * @brief The BTN class represents a push button with long press detection.
 */
class BTN {
    public:
        /**
         * @brief Default constructor.
         */
        BTN();

        /**
         * @brief Initializes the button (on GPIO 0).
         */
        void begin();

        /**
         * @brief Monitors the button and calls the callback on a short (single) press.
         * 
         * This function triggers the callback only when the button is released quickly 
         * (i.e., within the long-press timeout threshold).
         * 
         * @param onSinglePress Function to call on a short press.
         */
        void onSinglePress(
            std::function<void()> onSinglePress
        );

        /**
         * @brief Monitors the button and calls the callback if held for longer than 5 seconds.
         * 
         * @param onLongPress Function to call on a long press.
         */
        void onLongPress(
            std::function<void()> onLongPress
        );

    private:
        unsigned long _pressStartTime = 0;
        bool _wasPressed = false;
};

#endif // btn_h
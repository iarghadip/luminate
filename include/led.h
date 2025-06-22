#ifndef led_h
#define led_h

// Libraries from system framework
#include <Arduino.h>

/**
 * @brief Provides utility functions for delayed execution and logging.
 * 
 * The LED class allows scheduling functions and logging messages with optional success flags.
 * It manages PWM-controlled cool and warm led, enabling smooth transitions in brightness.
 */
class LED {
    public:
        /**
         * @brief Constructs a LED object.
         * 
         * Initializes internal state and dependencies.
         */
        LED();

        /**
         * @brief Initializes the light pins and configures PWM for brightness control.
         * 
         * Call this method during setup() to prepare the led for use. It configures
         * the GPIO pins as outputs and attaches them to PWM channels using the same 
         * pin number as the channel ID (for simplicity).
         */
        void begin();

        /**
         * @brief Smoothly transitions the cool and warm LEDs to the specified brightness levels.
         * 
         * This function gradually interpolates the brightness of the cool and warm white LEDs
         * from their previous values to the new target values. The transition is done over a 
         * number of steps determined by the maximum change in brightness between the old and new 
         * levels. PWM is used to adjust the brightness of each LED channel incrementally.
         * 
         * The function ensures a smooth visual transition by updating the LED states frame-by-frame
         * with a fixed delay (`BRIGHTNESS_RENDER_INTERVAL`) between each step.
         * 
         * @param coolLED Target brightness for the cool white LED (range: 0–100).
         * @param warmLED Target brightness for the warm white LED (range: 0–100).
         * 
         * @note The function blocks during the transition and should be called from within 
         *       a task context where blocking is acceptable.
         */
        void renderLumination(
            float coolLED,
            float warmLED
        );

    private:
        int _oldCoolBrightness; ///< Stores the most recently applied brightness for the cool light.
        int _oldWarmBrightness; ///< Stores the most recently applied brightness for the warm light.

        /**
         * @brief Linearly interpolates between two values.
         * 
         * @param from The starting value.
         * @param to The target value.
         * @param step Current step number (0 to steps).
         * @param steps Total number of steps.
         * @return Interpolated float value.
         */
        inline float _lerp(
            float from,
            float to,
            int step,
            int steps
        );

        /**
         * @brief Sets the PWM duty cycle for the specified light channel.
         * 
         * Converts a brightness percentage (0–100) to an 8-bit PWM duty cycle and applies 
         * it using @c ledcWrite.
         * 
         * @param lightChannel The PWM channel (same as GPIO pin in this setup).
         * @param percentage Brightness percentage (0–100).
         */
        void _luminate(
            int lightPin,
            float percentage
        );
};

#endif ///< led_h

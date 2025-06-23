#ifndef led_h
#define led_h

// Libraries from module wrappers
#include <mcu.h>

/**
 * @class LED
 * @brief Controls dual-channel (cool and warm) PWM-based LEDs with smooth brightness transitions.
 * 
 * The LED class manages two separate PWM-controlled LED channels (cool and warm) and provides
 * smooth brightness interpolation. It also wraps initialization using an MCU interface.
 */
class LED {
    public:
        /**
         * @brief Constructs a LED object.
         * 
         * Initializes internal brightness tracking and prepares the object for configuration.
         */
        LED();

        /**
         * @brief Initializes LED control pins and configures PWM channels.
         * 
         * This should be called in the main setup routine. It sets the GPIO pins
         * for output and assigns each pin as its own PWM channel for simplicity.
         * 
         * @param mcu Pointer to the MCU instance that abstracts hardware I/O.
         */
        void begin(
            MCU* mcu
        );

        /**
         * @brief Smoothly transitions the cool and warm LEDs to new brightness levels.
         * 
         * This function computes a series of steps between the current and target brightness
         * values and updates the PWM output for both cool and warm LED channels frame-by-frame,
         * creating a smooth fading effect.
         * 
         * @param coolLED Target brightness percentage for the cool white LED (0–100).
         * @param warmLED Target brightness percentage for the warm white LED (0–100).
         * 
         * @note This method blocks execution while the transition occurs. It is best used in
         *       task/thread contexts where blocking is acceptable.
         */
        void renderLumination(
            float coolLED,
            float warmLED
        );

    private:
        MCU* _mcu; // Pointer to the MCU instance used for GPIO and PWM control.
        int _oldCoolBrightness; // Most recent brightness applied to the cool white LED.
        int _oldWarmBrightness; // Most recent brightness applied to the warm white LED.

        /**
         * @brief Performs linear interpolation between two values.
         * 
         * Calculates an intermediate value between `from` and `to`, based on the current step.
         * 
         * @param from Starting value.
         * @param to Target value.
         * @param step Current step number (0-based).
         * @param steps Total number of interpolation steps.
         * @return Interpolated value as a float.
         */
        inline float _lerp(
            float from,
            float to,
            int step,
            int steps
        );

        /**
         * @brief Applies brightness to a given LED using PWM.
         * 
         * Converts the brightness percentage (0–100) into an 8-bit PWM value
         * and writes it to the specified pin/channel using `ledcWrite()`.
         * 
         * @param lightPin The GPIO pin (also used as the PWM channel).
         * @param percentage Brightness percentage (0–100).
         */
        void _luminate(
            int lightPin,
            float percentage
        );
};

#endif // led_h

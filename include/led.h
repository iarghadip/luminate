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
         * @brief Sets the Wi-Fi status LED pin to HIGH or LOW.
         * 
         * @param value Digital output value (`HIGH` or `LOW`).
         */
        void toggleWSL(
            uint8_t value
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
         * @brief Applies brightness to a given LED using PWM.
         * 
         * Converts the brightness percentage (0–100) into an 8-bit PWM value
         * and writes it to the specified pin/channel using `ledcWrite()`.
         * 
         * @param lightPin The GPIO pin (also used as the PWM channel).
         * @param percentage Brightness percentage (0–100).
         */
        void _luminate(
            int pwmChannel,
            float oldBrightness,
            float newBrightness,
            int currentStep,
            int totalSteps
        );
};

#endif // led_h

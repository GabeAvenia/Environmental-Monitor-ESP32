/**
 * @file LedManager.h
 * @brief Manager for controlling the onboard NeoPixel LED
 * @author Gabriel Avenia
 * @date May 2025
 *
 * @defgroup led_management LED Management
 * @brief Components for controlling the system's LED indicators
 * @{
 */

 #pragma once

 #include <Arduino.h>
 #include <Adafruit_NeoPixel.h>
 #include "error/ErrorHandler.h"
 #include "Constants.h"
 
 /**
  * @brief Manager for controlling the onboard NeoPixel LED
  * 
  * This class provides a unified interface for controlling the system's LED
  * indicator, including state indication, error notification, and visual feedback.
  */
 class LedManager {
 private:
     /**
      * @brief Adafruit NeoPixel controller
      */
     Adafruit_NeoPixel pixel;
     
     /**
      * @brief Error handler for logging
      */
     ErrorHandler* errorHandler;
     
     /**
      * @brief Pin definitions
      * @{
      */
     const int neopixelPin;      ///< Data pin for NeoPixel
     const int neopixelPowerPin; ///< Power pin for NeoPixel
     const int numPixels;        ///< Number of NeoPixels in chain
     /** @} */
     
     /**
      * @brief Color constants
      * @{
      */
     const uint32_t COLOR_OFF = Constants::LED::COLOR_OFF;         ///< Off (black)
     const uint32_t COLOR_GREEN = Constants::LED::COLOR_GREEN;     ///< Normal operation (green)
     const uint32_t COLOR_YELLOW = Constants::LED::COLOR_YELLOW;   ///< Setup mode (yellow)
     const uint32_t COLOR_BLUE = Constants::LED::COLOR_BLUE;       ///< Identify mode (blue)
     const uint32_t COLOR_RED = Constants::LED::COLOR_RED;         ///< Error indication (red)
     const uint32_t COLOR_ORANGE = Constants::LED::COLOR_ORANGE;   ///< Warning indication (orange)
     /** @} */
     
     /**
      * @brief LED brightness levels
      * @{
      */
     const uint8_t DIM_BRIGHTNESS = Constants::LED::DIM_BRIGHTNESS;      ///< 30% brightness for idle
     const uint8_t FULL_BRIGHTNESS = Constants::LED::FULL_BRIGHTNESS;    ///< 100% brightness for active
     /** @} */
     
     /**
      * @brief State tracking flags
      * @{
      */
     bool initialized = false;           ///< Whether LED has been initialized
     bool identifying = false;           ///< Whether identify mode is active
     unsigned long identifyStartTime = 0; ///< When identify mode started
     /** @} */
     
     /**
      * @brief Reading pulse effect tracking
      * @{
      */
     bool pulseActive = false;           ///< Whether pulse effect is active
     unsigned long pulseStartTime = 0;    ///< When pulse started
     const unsigned long PULSE_DURATION = Constants::LED::PULSE_DURATION_MS;  ///< 100ms pulse duration
     /** @} */
     
     /**
      * @brief Error indication tracking
      * @{
      */
     bool errorIndicationActive = false;           ///< Whether error indication is active
     bool fatalErrorActive = false;                ///< Whether in fatal error mode
     unsigned long errorIndicationStartTime = 0;    ///< When error indication started
     const unsigned long WARNING_ERROR_DURATION = Constants::LED::WARNING_ERROR_DURATION_MS; ///< Duration of warning/error indication
     uint32_t errorIndicationColor = COLOR_OFF;    ///< Color of current error indication
     /** @} */
     
     /**
      * @brief Set a solid color on the NeoPixel
      * 
      * @param color The color value (32-bit, format: 0x00RRGGBB)
      * @param brightness The brightness level (0-255)
      */
     void setColor(uint32_t color, uint8_t brightness = 255);
     
 public:
     /**
      * @brief Constructor for LedManager
      * 
      * @param err Pointer to the error handler for logging
      * @param pin NeoPixel data pin (default: Constants::Pins::NEOPIXEL_DATA)
      * @param powerPin NeoPixel power pin (default: Constants::Pins::NEOPIXEL_PWR)
      * @param numLeds Number of NeoPixels (default: 1)
      */
     LedManager(ErrorHandler* err, int pin = Constants::Pins::NEOPIXEL_DATA, 
                int powerPin = Constants::Pins::NEOPIXEL_PWR, int numLeds = 1);
     
     /**
      * @brief Destructor - ensures LED is turned off
      */
     ~LedManager();
     
     /**
      * @brief Initialize the LED manager
      * 
      * Sets up the NeoPixel hardware and initializes the LED to setup mode.
      * 
      * @return true if initialization succeeded, false otherwise
      */
     bool begin();
     
     /**
      * @brief Set LED to setup mode (solid yellow)
      * 
      * Used during system initialization to indicate setup in progress.
      */
     void setSetupMode();
     
     /**
      * @brief Set LED to normal operation mode (solid green)
      * 
      * Used during normal operation to indicate system is functioning properly.
      * Will not override fatal error state if active.
      */
     void setNormalMode();
     
     /**
      * @brief Pulse LED to indicate sensor reading
      * 
      * Briefly flashes the LED brighter to indicate sensor activity.
      * Will not override fatal error or warning/error indications.
      */
     void indicateReading();
     
     /**
      * @brief Start identification mode (flashing blue)
      * 
      * Flashes the LED blue to help identify a specific unit.
      * Will not override fatal error state if active.
      */
     void startIdentify();
     
     /**
      * @brief Indicate a warning (orange LED for 2 seconds)
      * 
      * Shows an orange indicator to indicate a warning condition.
      * Will not override fatal error state if active.
      */
     void indicateWarning();
     
     /**
      * @brief Indicate an error (red LED for 2 seconds)
      * 
      * Shows a red indicator to indicate an error condition.
      * Will not override fatal error state if active.
      */
     void indicateError();
     
     /**
      * @brief Indicate a fatal error (permanent red LED)
      * 
      * Shows a permanent red indicator to indicate a fatal error condition.
      * This state overrides all other states and remains until system restart.
      */
     void indicateFatalError();
     
     /**
      * @brief Main update function for LED animations
      * 
      * Must be called regularly from loop() or a task to update LED
      * animations and state transitions.
      */
     void update();
     
     /**
      * @brief Check if identification sequence is currently active
      * 
      * @return true if identify mode is active, false otherwise
      */
     bool isIdentifying() const;
     
     /**
      * @brief Check if a fatal error is currently being indicated
      * 
      * @return true if in fatal error state
      */
     bool isFatalError() const;
 };
 
 /** @} */ // End of led_management group
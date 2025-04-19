#pragma once

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "error/ErrorHandler.h"

/**
 * @brief Simple manager for controlling the onboard NeoPixel LED
 */
class LedManager {
private:
    Adafruit_NeoPixel pixel;
    ErrorHandler* errorHandler;
    
    // Pin definitions
    const int neopixelPin;
    const int neopixelPowerPin;
    const int numPixels;
    
    // Color constants
    const uint32_t COLOR_OFF = 0x000000;
    const uint32_t COLOR_GREEN = 0x00FF00;
    const uint32_t COLOR_YELLOW = 0xFFFF00;
    const uint32_t COLOR_BLUE = 0x0000FF;
    const uint32_t COLOR_RED = 0xFF0000;       // For error indication
    const uint32_t COLOR_ORANGE = 0xFF8000;    // For warning indication
    
    // LED brightness levels
    const uint8_t DIM_BRIGHTNESS = 30;    // 30% brightness for idle
    const uint8_t FULL_BRIGHTNESS = 40;   // 100% brightness for active
    
    // State tracking
    bool initialized = false;
    bool identifying = false;
    unsigned long identifyStartTime = 0;
    
    // Reading pulse effect tracking
    bool pulseActive = false;
    unsigned long pulseStartTime = 0;
    const unsigned long PULSE_DURATION = 100;
    
    // Error indication tracking
    bool errorIndicationActive = false;
    bool fatalErrorActive = false;           // Track if we're in fatal error mode
    unsigned long errorIndicationStartTime = 0;
    const unsigned long WARNING_ERROR_DURATION = 2000;  // 2 seconds for temporary warnings/errors
    uint32_t errorIndicationColor = COLOR_OFF;
    
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
     * @param pin NeoPixel data pin (default: 39 for Adafruit QT Py ESP32-S3)
     * @param powerPin NeoPixel power pin (default: 38 for Adafruit QT Py ESP32-S3)
     * @param numLeds Number of NeoPixels (default: 1)
     */
    LedManager(ErrorHandler* err, int pin = 39, int powerPin = 38, int numLeds = 1);
    
    /**
     * @brief Destructor
     */
    ~LedManager();
    
    /**
     * @brief Initialize the LED manager
     * 
     * @return true if initialization succeeded, false otherwise
     */
    bool begin();
    
    /**
     * @brief Set LED to setup mode (solid yellow)
     */
    void setSetupMode();
    
    /**
     * @brief Set LED to normal operation mode (solid green)
     */
    void setNormalMode();
    
    /**
     * @brief Start a LED pulse to indicate sensor reading
     */
    void indicateReading();
    
    /**
     * @brief Start identification mode (flashing blue)
     */
    void startIdentify();
    
    /**
     * @brief Indicate a warning (orange LED for 2 seconds)
     */
    void indicateWarning();
    
    /**
     * @brief Indicate an error (red LED for 2 seconds)
     */
    void indicateError();
    
    /**
     * @brief Indicate a fatal error (permanent red LED)
     */
    void indicateFatalError();
    
    /**
     * @brief Main update function, must be called regularly from loop()
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
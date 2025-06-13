#include "LedManager.h"

LedManager::LedManager(ErrorHandler* err, int pin, int powerPin, int numLeds)
    : errorHandler(err),
      neopixelPin(pin),
      neopixelPowerPin(powerPin),
      numPixels(numLeds),
      pixel(numLeds, pin, NEO_GRB + NEO_KHZ800) {
}

LedManager::~LedManager() {
    // Turn off the LED
    setColor(COLOR_OFF, 0);
    
    // Disable power to NeoPixel if power pin is defined
    if (neopixelPowerPin >= 0) {
        digitalWrite(neopixelPowerPin, LOW);
    }
}

bool LedManager::begin() {
    // Setup the power pin if available
    if (neopixelPowerPin >= 0) {
        pinMode(neopixelPowerPin, OUTPUT);
        digitalWrite(neopixelPowerPin, HIGH);  // Enable power to NeoPixel
    }
    
    // Initialize the NeoPixel
    pixel.begin();
    pixel.setBrightness(FULL_BRIGHTNESS);
    pixel.show();  // Initialize all pixels to 'off'
    
    // Log initialization
    if (errorHandler) {
        errorHandler->logError(INFO, "NeoPixel initialized on pin " + String(neopixelPin));
    }
    
    // Set the setup color (yellow)
    setSetupMode();
    
    initialized = true;
    return true;
}

void LedManager::setColor(uint32_t color, uint8_t brightness) {
    if (!initialized) return;
    
    pixel.setBrightness(brightness);
    pixel.setPixelColor(0, color);
    pixel.show();
}

void LedManager::setSetupMode() {
    setColor(COLOR_YELLOW, FULL_BRIGHTNESS);
}

void LedManager::setNormalMode() {
    // Only set normal mode if not in fatal error state
    if (fatalErrorActive) return;
    
    setColor(COLOR_GREEN, DIM_BRIGHTNESS);
}

void LedManager::indicateReading() {
    // Don't override fatal errors or temporary error/warning indications
    if (!initialized || fatalErrorActive || errorIndicationActive) return;
    
    pulseActive = true;
    pulseStartTime = millis();
    
    // Flash from dim green to bright green and back
    setColor(COLOR_GREEN, FULL_BRIGHTNESS);
}

void LedManager::startIdentify() {
    // Don't override fatal errors
    if (!initialized || fatalErrorActive) return;
    
    identifying = true;
    identifyStartTime = millis();
    
    // Start with LED on
    setColor(COLOR_BLUE, FULL_BRIGHTNESS);
}

void LedManager::indicateWarning() {
    if (!initialized || fatalErrorActive) return;  // Don't override fatal errors
    
    // Cancel any identification or pulse in progress
    identifying = false;
    pulseActive = false;
    
    // Set warning indication (orange)
    errorIndicationActive = true;
    errorIndicationStartTime = millis();
    errorIndicationColor = COLOR_ORANGE;
    setColor(COLOR_ORANGE, FULL_BRIGHTNESS);
}

void LedManager::indicateError() {
    if (!initialized || fatalErrorActive) return;  // Don't override fatal errors
    
    // Cancel any identification or pulse in progress
    identifying = false;
    pulseActive = false;
    
    // Set error indication (red)
    errorIndicationActive = true;
    errorIndicationStartTime = millis();
    errorIndicationColor = COLOR_RED;
    setColor(COLOR_RED, FULL_BRIGHTNESS);
}

void LedManager::indicateFatalError() {
    if (!initialized) return;
    
    // Cancel any other indications in progress
    identifying = false;
    pulseActive = false;
    errorIndicationActive = false;
    
    // Set fatal error indication (permanent red)
    fatalErrorActive = true;
    setColor(COLOR_RED, FULL_BRIGHTNESS);
    
    if (errorHandler) {
        errorHandler->logError(ERROR, "LED set to fatal error mode (permanently red)");
    }
}

bool LedManager::isFatalError() const {
    return fatalErrorActive;
}

void LedManager::update() {
    if (!initialized) return;
    
    unsigned long currentTime = millis();
    
    // Don't do any other updates if we're in fatal error mode
    if (fatalErrorActive) {
        // Just ensure the LED stays red
        setColor(COLOR_RED, FULL_BRIGHTNESS);
        return;
    }
    
    // Handle identification mode
    if (identifying) {
        unsigned long elapsed = currentTime - identifyStartTime;
        int flashPhase = (elapsed / 250) % 2;  // 2Hz = 500ms period, 250ms per state
        int flashCount = elapsed / 500;        // Complete cycles
        
        if (flashCount >= 10) {
            // Identification complete after 10 flashes
            identifying = false;
            setNormalMode();  // Return to normal mode
        } else {
            // Set the LED based on flash phase
            if (flashPhase == 0) {
                setColor(COLOR_BLUE, FULL_BRIGHTNESS);
            } else {
                setColor(COLOR_OFF, 0);
            }
        }
        
        return;  // Exit early - identification takes precedence (except for fatal errors)
    }
    
    // Handle error/warning indication timeout
    if (errorIndicationActive) {
        unsigned long errorElapsed = currentTime - errorIndicationStartTime;
        
        if (errorElapsed >= WARNING_ERROR_DURATION) {
            // Error indication complete, return to normal mode
            errorIndicationActive = false;
            setNormalMode();
        } else {
            // Ensure the error color is still showing
            setColor(errorIndicationColor, FULL_BRIGHTNESS);
        }
        
        return; // Exit early - error indication takes precedence over reading pulse
    }
    
    // Handle reading pulse
    if (pulseActive) {
        unsigned long pulseElapsed = currentTime - pulseStartTime;
        
        if (pulseElapsed >= PULSE_DURATION) {
            // Pulse complete, return to dim green
            pulseActive = false;
            setColor(COLOR_GREEN, DIM_BRIGHTNESS);
        }
    }
}

bool LedManager::isIdentifying() const {
    return identifying;
}
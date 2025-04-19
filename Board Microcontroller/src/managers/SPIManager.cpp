#include "SPIManager.h"

SPIManager::SPIManager(ErrorHandler* err) 
    : errorHandler(err),
      initialized(false),
      mosiPin(37),  // GPIO37 (MOSI)
      misoPin(35),  // GPIO35 (MISO)
      sckPin(36),   // GPIO36 (SCK)
      defaultSettings(1000000, MSBFIRST, SPI_MODE0) {
}

int SPIManager::ssPinMapping[MAX_SS_PINS] = {
    SS_PIN_A0,  // Index 0 -> GPIO18 (A0)
    SS_PIN_A1,  // Index 1 -> GPIO17 (A1)
    SS_PIN_A2,  // Index 2 -> GPIO9  (A2)
    SS_PIN_A3   // Index 3 -> GPIO8  (A3)
};

SPIManager::~SPIManager() {
    // Nothing to clean up
}

bool SPIManager::begin(int mosi, int miso, int sck) {
    if (initialized) {
        // Already initialized
        return true;
    }
    
    mosiPin = mosi;
    misoPin = miso;
    sckPin = sck;
    
    // Initialize SPI
    SPI.begin(sckPin, misoPin, mosiPin);
    
    initialized = true;
    errorHandler->logError(INFO, "SPI initialized with pins MOSI:" + String(mosiPin) + 
                          " MISO:" + String(misoPin) + 
                          " SCK:" + String(sckPin));
    
    // Don't auto-register any SS pin
    return true;
}

bool SPIManager::isInitialized() const {
    return initialized;
}

bool SPIManager::registerSSPin(int ssPin) {
    // Map logical index to physical pin if needed
    int physicalPin = ssPin;
    if (ssPin >= 0 && ssPin < MAX_SS_PINS) {
        physicalPin = ssPinMapping[ssPin];
        errorHandler->logError(INFO, "Mapping logical SS index " + String(ssPin) + 
                             " to physical pin " + String(physicalPin));
    }
    
    // Check if pin is already registered
    for (int pin : ssPins) {
        if (pin == physicalPin) {
            errorHandler->logError(INFO, "SS pin " + String(physicalPin) + " already registered");
            return true; // Already registered, so technically successful
        }
    }
    
    // Configure the pin as output
    pinMode(physicalPin, OUTPUT);
    digitalWrite(physicalPin, HIGH); // Inactive state
    
    // Add to the list
    ssPins.push_back(physicalPin);
    
    errorHandler->logError(INFO, "Registered SS pin " + String(physicalPin));
    return true;
}

bool SPIManager::beginTransaction(int ssPin, SPISettings settings) {
    if (!initialized) {
        errorHandler->logError(ERROR, "SPI not initialized");
        return false;
    }
    
    // Map logical index to physical pin if needed
    int physicalPin = ssPin;
    if (ssPin >= 0 && ssPin < MAX_SS_PINS) {
        physicalPin = ssPinMapping[ssPin];
    }
    
    // Make sure the SS pin is registered
    bool pinFound = false;
    for (int pin : ssPins) {
        if (pin == physicalPin) {
            pinFound = true;
            break;
        }
    }
    
    if (!pinFound) {
        // Auto-register the pin if not found
        registerSSPin(ssPin); // This will handle the mapping
        physicalPin = (ssPin >= 0 && ssPin < MAX_SS_PINS) ? ssPinMapping[ssPin] : ssPin;
    }
    
    // Begin transaction
    SPI.beginTransaction(settings);
    digitalWrite(physicalPin, LOW); // Active state
    
    return true;
}

void SPIManager::endTransaction(int ssPin) {
    // Map logical index to physical pin if needed
    int physicalPin = ssPin;
    if (ssPin >= 0 && ssPin < MAX_SS_PINS) {
        physicalPin = ssPinMapping[ssPin];
    }
    
    digitalWrite(physicalPin, HIGH); // Inactive state
    SPI.endTransaction();
}

uint8_t SPIManager::transfer(uint8_t data) {
    return SPI.transfer(data);
}

void SPIManager::transfer(uint8_t* buffer, size_t size) {
    SPI.transfer(buffer, size);
}

bool SPIManager::testDevice(int ssPin) {
    // This is a generic test that may not work for all devices.
    // Some devices require specific commands to respond correctly.
    
    if (!initialized) {
        errorHandler->logError(ERROR, "SPI not initialized");
        return false;
    }
    
    bool success = beginTransaction(ssPin);
    if (!success) {
        return false;
    }
    
    // Transfer a byte and check if we get something back
    // This may not be reliable for all devices, but it's a starting point
    uint8_t response = transfer(0xFF); // Send dummy byte
    
    endTransaction(ssPin);
    
    errorHandler->logError(INFO, "SPI test on SS pin " + String(ssPin) + 
                          " returned response: 0x" + String(response, HEX));
    
    // For a real test, we would need device-specific logic here
    // For now, just return true as a placeholder
    return true;
}

SPIClass& SPIManager::getSPI() {
    return SPI;
}
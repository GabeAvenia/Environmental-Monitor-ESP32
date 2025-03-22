#include "SPIManager.h"

SPIManager::SPIManager(ErrorHandler* err) 
    : errorHandler(err),
      initialized(false),
      mosiPin(37),  // GPIO37 (MOSI)
      misoPin(35),  // GPIO35 (MISO)
      sckPin(36),   // GPIO36 (SCK)
      defaultSettings(1000000, MSBFIRST, SPI_MODE0) {
}

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
    errorHandler->logInfo("SPI initialized with pins MOSI:" + String(mosiPin) + 
                          " MISO:" + String(misoPin) + 
                          " SCK:" + String(sckPin));
    return true;
}

bool SPIManager::isInitialized() const {
    return initialized;
}

bool SPIManager::registerSSPin(int ssPin) {
    // Check if pin is already registered
    for (int pin : ssPins) {
        if (pin == ssPin) {
            errorHandler->logWarning("SS pin " + String(ssPin) + " already registered");
            return true; // Already registered, so technically successful
        }
    }
    
    // Configure the pin as output
    pinMode(ssPin, OUTPUT);
    digitalWrite(ssPin, HIGH); // Inactive state
    
    // Add to the list
    ssPins.push_back(ssPin);
    
    errorHandler->logInfo("Registered SS pin " + String(ssPin));
    return true;
}

bool SPIManager::beginTransaction(int ssPin, SPISettings settings) {
    if (!initialized) {
        errorHandler->logError(ERROR, "SPI not initialized");
        return false;
    }
    
    // Make sure the SS pin is registered
    bool pinFound = false;
    for (int pin : ssPins) {
        if (pin == ssPin) {
            pinFound = true;
            break;
        }
    }
    
    if (!pinFound) {
        // Auto-register the pin if not found
        registerSSPin(ssPin);
    }
    
    // Begin transaction
    SPI.beginTransaction(settings);
    digitalWrite(ssPin, LOW); // Active state
    
    return true;
}

void SPIManager::endTransaction(int ssPin) {
    digitalWrite(ssPin, HIGH); // Inactive state
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
    
    errorHandler->logInfo("SPI test on SS pin " + String(ssPin) + 
                          " returned response: 0x" + String(response, HEX));
    
    // For a real test, we would need device-specific logic here
    // For now, just return true as a placeholder
    return true;
}

SPIClass& SPIManager::getSPI() {
    return SPI;
}
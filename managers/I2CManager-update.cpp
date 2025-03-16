#include "I2CManager.h"

I2CManager::I2CManager(ErrorHandler* err) 
    : errorHandler(err), initialized(false), sdaPin(41), sclPin(40) {
    wire = &Wire1; // Use Wire1 instead of Wire
}

I2CManager::~I2CManager() {
    // Nothing to clean up for now
}

bool I2CManager::begin(int sda, int scl) {
    if (initialized) {
        // Already initialized
        return true;
    }
    
    sdaPin = sda;
    sclPin = scl;
    
    // Use Wire1 with the specified pins
    wire->begin(sda, scl);
    initialized = true;
    
    errorHandler->logInfo("I2C initialized on Wire1 with pins SDA:" + String(sda) + " SCL:" + String(scl));
    return true;
}

bool I2CManager::isInitialized() const {
    return initialized;
}

bool I2CManager::scanBus(std::vector<int>& addresses) {
    if (!initialized) {
        errorHandler->logError(ERROR, "I2C not initialized before scan");
        return false;
    }
    
    addresses.clear();
    
    errorHandler->logInfo("Scanning I2C bus on Wire1...");
    for (int address = 1; address < 127; address++) {
        if (devicePresent(address)) {
            addresses.push_back(address);
            errorHandler->logInfo("Found I2C device at address 0x" + String(address, HEX));
        }
    }
    
    if (addresses.empty()) {
        errorHandler->logWarning("No I2C devices found on Wire1");
        return false;
    }
    
    errorHandler->logInfo("Found " + String(addresses.size()) + " I2C devices on Wire1");
    return true;
}

TwoWire* I2CManager::getWire() {
    return wire;
}

bool I2CManager::devicePresent(int address) {
    if (!initialized) {
        errorHandler->logError(ERROR, "I2C not initialized");
        return false;
    }
    
    wire->beginTransmission(address);
    byte error = wire->endTransmission();
    
    return (error == 0);
}

int I2CManager::getSdaPin() const {
    return sdaPin;
}

int I2CManager::getSclPin() const {
    return sclPin;
}

#include "I2CManager.h"

I2CManager::I2CManager(ErrorHandler* err) : errorHandler(err) {
    wire = &Wire1; // Use Wire1 instead of Wire
}

I2CManager::~I2CManager() {
    // Nothing to clean up for now
}

bool I2CManager::begin(int sda, int scl, uint32_t frequency) {
    // Use Wire1 with pins 41 and 40
    wire->begin(sda, scl);
    wire->setClock(frequency);
    errorHandler->logInfo("I2C initialized on Wire1 with pins SDA:" + String(sda) + " SCL:" + String(scl));
    return true;
}

bool I2CManager::scanBus(std::vector<int>& addresses) {
    addresses.clear();
    
    errorHandler->logInfo("Scanning I2C bus on Wire1...");
    for (int address = 1; address < 127; address++) {
        wire->beginTransmission(address);
        byte error = wire->endTransmission();
        
        if (error == 0) {
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
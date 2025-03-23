#include "I2CManager.h"

I2CManager::I2CManager(ErrorHandler* err) 
    : errorHandler(err), 
      initialized0(false), 
      initialized1(false),
      sda0Pin(7),   // GPIO7 (A3/SDA)
      scl0Pin(6),   // GPIO6 (A2/SCL)
      sda1Pin(40),  // GPIO40 (SCL1, marked as MTDO)
      scl1Pin(41)   // GPIO41 (SDA1, marked as MTDI)
{
    wire0 = &Wire1;    // Primary I2C bus
    wire1 = &Wire;   // Secondary I2C bus
}

I2CManager::~I2CManager() {
    // Nothing to clean up for now
}

bool I2CManager::begin() {
    bool success0 = beginPort(I2CPort::I2C0);
    bool success1 = beginPort(I2CPort::I2C1);
    
    return success0 || success1; // Success if at least one bus initialized
}

bool I2CManager::beginPort(I2CPort port) {
    switch (port) {
        case I2CPort::I2C0:
            if (initialized0) {
                return true; // Already initialized
            }
            
            // Initialize I2C0 with the specified pins
            wire0->begin(sda0Pin, scl0Pin);
            initialized0 = true;
            
            errorHandler->logInfo("I2C0 initialized with pins SDA:" + String(sda0Pin) + " SCL:" + String(scl0Pin));
            return true;
            
        case I2CPort::I2C1:
            if (initialized1) {
                return true; // Already initialized
            }
            
            // Initialize I2C1 with the specified pins
            wire1->begin(sda1Pin, scl1Pin);
            initialized1 = true;
            
            errorHandler->logInfo("I2C1 initialized with pins SDA:" + String(sda1Pin) + " SCL:" + String(scl1Pin));
            return true;
            
        default:
            errorHandler->logError(ERROR, "Invalid I2C port specified");
            return false;
    }
}

bool I2CManager::isPortInitialized(I2CPort port) const {
    switch (port) {
        case I2CPort::I2C0:
            return initialized0;
        case I2CPort::I2C1:
            return initialized1;
        default:
            return false;
    }
}

bool I2CManager::scanBus(I2CPort port, std::vector<int>& addresses) {
    TwoWire* wire = getWire(port);
    if (!wire || !isPortInitialized(port)) {
        errorHandler->logError(ERROR, "I2C port " + portToString(port) + " not initialized before scan");
        return false;
    }
    
    addresses.clear();
    
    errorHandler->logInfo("Scanning I2C port " + portToString(port) + "...");
    for (int address = 1; address < 127; address++) {
        if (devicePresent(port, address)) {
            addresses.push_back(address);
            errorHandler->logInfo("Found I2C device at address 0x" + String(address, HEX) + 
                              " on port " + portToString(port));
        }
    }
    
    if (addresses.empty()) {
        errorHandler->logWarning("No I2C devices found on port " + portToString(port));
        return false;
    }
    
    errorHandler->logInfo("Found " + String(addresses.size()) + " I2C devices on port " + 
                      portToString(port));
    return true;
}

TwoWire* I2CManager::getWire(I2CPort port) {
    switch (port) {
        case I2CPort::I2C0:
            return wire0;
        case I2CPort::I2C1:
            return wire1;
        default:
            errorHandler->logError(ERROR, "Invalid I2C port requested");
            return nullptr;
    }
}

bool I2CManager::devicePresent(I2CPort port, int address) {
    TwoWire* wire = getWire(port);
    if (!wire || !isPortInitialized(port)) {
        errorHandler->logError(ERROR, "I2C port " + portToString(port) + " not initialized");
        return false;
    }
    
    wire->beginTransmission(address);
    byte error = wire->endTransmission();
    
    return (error == 0);
}

I2CPort I2CManager::stringToPort(const String& portName) {
    if (portName.equalsIgnoreCase("I2C0")) {
        return I2CPort::I2C0;
    } else if (portName.equalsIgnoreCase("I2C1")) {
        return I2CPort::I2C1;
    } else {
        // Default to I2C0 if unrecognized
        return I2CPort::I2C0;
    }
}

String I2CManager::portToString(I2CPort port) {
    switch (port) {
        case I2CPort::I2C0:
            return "I2C0";
        case I2CPort::I2C1:
            return "I2C1";
        default:
            return "UNKNOWN";
    }
}
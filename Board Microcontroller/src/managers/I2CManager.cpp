#include "I2CManager.h"

I2CManager::I2CManager(ErrorHandler* err) : errorHandler(err) {
    // Register default Wire instances
    // Main I2C bus - traditional Arduino pins
    registerWire(I2CPort::I2C0, &Wire1, 7, 6);  // GPIO7 (SDA), GPIO6 (SCL)
    
    // STEMMA QT / Qwiic connector
    registerWire(I2CPort::I2C1, &Wire, 41, 40); // GPIO41 (SDA), GPIO40 (SCL)
}

I2CManager::~I2CManager() {
    // Nothing to clean up
}

bool I2CManager::registerWire(I2CPort port, TwoWire* wire, int sdaPin, int sclPin, uint32_t clockFreq) {
    if (!wire) {
        errorHandler->logError(ERROR, "Null Wire instance provided for port " + portToString(port));
        return false;
    }
    
    wireBuses[port] = WireConfig(wire, sdaPin, sclPin, clockFreq);
    errorHandler->logError(INFO, "Registered Wire instance for port " + portToString(port) + 
                         " with pins SDA:" + String(sdaPin) + " SCL:" + String(sclPin));
    return true;
}

bool I2CManager::begin() {
    // Initialize all registered buses
    bool atLeastOneSuccess = false;
    
    for (auto& pair : wireBuses) {
        I2CPort port = pair.first;
        bool success = beginPort(port);
        atLeastOneSuccess |= success;
    }
    
    return atLeastOneSuccess;
}

bool I2CManager::beginPort(I2CPort port) {
    auto it = wireBuses.find(port);
    if (it == wireBuses.end()) {
        errorHandler->logError(ERROR, "No Wire instance registered for port " + portToString(port));
        return false;
    }
    
    WireConfig& config = it->second;
    
    // Skip if already initialized
    if (config.initialized) {
        return true;
    }
    
    // Initialize the Wire instance with the specified pins
    config.wire->begin(config.sdaPin, config.sclPin);
    
    // Set clock frequency if specified and different from default
    if (config.clockFrequency != 50000) {
        config.wire->setClock(config.clockFrequency);
    }
    
    config.initialized = true;
    errorHandler->logError(INFO, "I2C port " + portToString(port) + " initialized with pins SDA:" + 
                         String(config.sdaPin) + " SCL:" + String(config.sclPin));
    return true;
}

bool I2CManager::isPortInitialized(I2CPort port) const {
    auto it = wireBuses.find(port);
    if (it == wireBuses.end()) {
        return false;
    }
    
    return it->second.initialized;
}

TwoWire* I2CManager::getWire(I2CPort port) {
    auto it = wireBuses.find(port);
    if (it == wireBuses.end()) {
        errorHandler->logError(ERROR, "No Wire instance registered for port " + portToString(port));
        return nullptr;
    }
    
    return it->second.wire;
}

const WireConfig* I2CManager::getWireConfig(I2CPort port) const {
    auto it = wireBuses.find(port);
    if (it == wireBuses.end()) {
        return nullptr;
    }
    
    return &(it->second);
}

bool I2CManager::scanBus(I2CPort port, std::vector<int>& addresses) {
    TwoWire* wire = getWire(port);
    if (!wire || !isPortInitialized(port)) {
        errorHandler->logError(ERROR, "I2C port " + portToString(port) + " not initialized before scan");
        return false;
    }
    
    addresses.clear();
    
    errorHandler->logError(INFO, "Scanning I2C port " + portToString(port) + "...");
    for (int address = 1; address < 127; address++) {
        if (devicePresent(port, address)) {
            addresses.push_back(address);
            errorHandler->logError(INFO, "Found I2C device at address 0x" + String(address, HEX) + 
                              " on port " + portToString(port));
        }
    }
    
    if (addresses.empty()) {
        errorHandler->logError(WARNING, "No I2C devices found on port " + portToString(port));
        return false;
    }
    
    errorHandler->logError(INFO, "Found " + String(addresses.size()) + " I2C devices on port " + 
                      portToString(port));
    return true;
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
    // Handle existing port names
    if (portName.equalsIgnoreCase("I2C0")) {
        return I2CPort::I2C0;
    } else if (portName.equalsIgnoreCase("I2C1")) {
        return I2CPort::I2C1;
    } 
    // #TODO Handle multiplexed ports if format is like "I2C_MUX_0", "I2C_MUX_1", etc.
    if (portName.startsWith("I2C_MUX_")) {
        String channelStr = portName.substring(8);
        int channel = channelStr.toInt();
        return static_cast<I2CPort>(static_cast<int>(I2CPort::I2C_MULTIPLEXED_START) + channel);
    }
    
    // Default to I2C0 if unrecognized
    return I2CPort::I2C0;
}

String I2CManager::portToString(I2CPort port) {
    int portValue = static_cast<int>(port);
    
    // Handle built-in ports
    switch (port) {
        case I2CPort::I2C0: return "I2C0";
        case I2CPort::I2C1: return "I2C1";

        default: break;
    }
    
    // Handle multiplexed ports
    if (portValue >= static_cast<int>(I2CPort::I2C_MULTIPLEXED_START)) {
        int channel = portValue - static_cast<int>(I2CPort::I2C_MULTIPLEXED_START);
        return "I2C_MUX_" + String(channel);
    }
    
    return "UNKNOWN";
}
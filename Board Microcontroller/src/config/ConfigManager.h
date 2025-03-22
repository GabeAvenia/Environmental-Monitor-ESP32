#pragma once

#include <Arduino.h>
#include <vector>
#include <LittleFS.h>
#include "../error/ErrorHandler.h"
#include "../managers/I2CManager.h"

// Structure for sensor configurations
struct SensorConfig {
    String name;
    String type;
    int address;         // I2C address or SPI SS pin
    bool isSPI;          // True if this is an SPI sensor
    I2CPort i2cPort;     // Which I2C bus to use (only for I2C sensors)
    uint32_t pollingRate; // Polling rate in milliseconds
    
    // Equality operator for comparing configurations
    bool operator==(const SensorConfig& other) const {
        return name == other.name && 
               type == other.type && 
               address == other.address && 
               isSPI == other.isSPI &&
               i2cPort == other.i2cPort &&
               pollingRate == other.pollingRate;
    }
    
    // Inequality operator
    bool operator!=(const SensorConfig& other) const {
        return !(*this == other);
    }
};

// Callback for configuration changes
typedef void (*ConfigChangeCallback)(const String& newConfig);

class ConfigManager {
private:
    ErrorHandler* errorHandler;
    String boardId;
    std::vector<SensorConfig> sensorConfigs;
    std::vector<ConfigChangeCallback> changeCallbacks;
    
    // Private helper methods
    bool loadConfigFromFile();
    bool createDefaultConfig();
    void notifyConfigChanged(const String& newConfig);

public:
    // Constructor
    ConfigManager(ErrorHandler* err);
    
    // Public methods
    bool begin();
    String getBoardIdentifier();
    bool setBoardIdentifier(String identifier);
    
    // Sensor configuration methods
    std::vector<SensorConfig> getSensorConfigs();
    bool updateSensorConfigs(const std::vector<SensorConfig>& configs);
    
    // Configuration JSON methods
    String getConfigJson();
    bool updateConfigFromJson(const String& jsonConfig);
    
    // Change notification
    void registerChangeCallback(ConfigChangeCallback callback);
};
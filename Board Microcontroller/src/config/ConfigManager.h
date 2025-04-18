#pragma once

#include <Arduino.h>
#include <vector>
#include <LittleFS.h>
#include <ArduinoJson.h>
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
    String additional;   // Additional sensor-specific settings
    
    // Equality operator for comparing configurations
    bool operator==(const SensorConfig& other) const {
        return name == other.name && 
               type == other.type && 
               address == other.address && 
               isSPI == other.isSPI &&
               i2cPort == other.i2cPort &&
               pollingRate == other.pollingRate &&
               additional == other.additional;
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
    String additionalConfig;
    std::vector<ConfigChangeCallback> changeCallbacks;
    
    // Flag to prevent recursive notification
    bool notifyingCallbacks;
    
    // Private helper methods
    bool loadConfigFromFile();
    bool createDefaultConfig();
    void notifyConfigChanged(const String& newConfig);
    
    // Helper methods for file operations
    bool writeConfigToFile(const JsonDocument& doc);
    bool readConfigFromFile(JsonDocument& doc);

public:
    // Constructor
    ConfigManager(ErrorHandler* err);
    
    // Initialization 
    bool begin();
    
    // Board identification methods
    String getBoardIdentifier();
    bool setBoardIdentifier(String identifier);
    
    // Sensor configuration methods
    std::vector<SensorConfig> getSensorConfigs();
    bool updateSensorConfigs(const std::vector<SensorConfig>& configs);
    
    // Complete configuration methods
    String getConfigJson();
    bool updateConfigFromJson(const String& jsonConfig);
    
    // Granular configuration methods
    bool updateSensorConfigFromJson(const String& jsonConfig);
    bool updateAdditionalConfigFromJson(const String& jsonConfig);
    
    // Change notification
    void registerChangeCallback(ConfigChangeCallback callback);
    
    // Method to safely disable notifications (for recursive operations)
    void disableNotifications(bool disable);
};
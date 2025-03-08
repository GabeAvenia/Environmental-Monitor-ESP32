#pragma once

#include <Arduino.h>
#include <vector>
#include <LittleFS.h>
#include "../error/ErrorHandler.h"

// Structure for sensor configurations
struct SensorConfig {
    String name;
    String type;
    int address;  // I2C address or SPI SS pin
    bool isSPI;   // True if this is an SPI sensor
};

class ConfigManager {
private:
    ErrorHandler* errorHandler;
    String boardId;
    std::vector<SensorConfig> sensorConfigs;
    
    // Private helper methods
    bool loadConfigFromFile();
    bool createDefaultConfig();

public:
    // Constructor
    ConfigManager(ErrorHandler* err);
    
    // Public methods
    bool begin();
    String getBoardIdentifier();
    bool setBoardIdentifier(String identifier);
    std::vector<SensorConfig> getSensorConfigs();
    String getConfigJson();
};
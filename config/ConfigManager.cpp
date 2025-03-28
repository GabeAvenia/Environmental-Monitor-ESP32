#include "ConfigManager.h"
#include <ArduinoJson.h>
#include "../Constants.h"

// Constructor
ConfigManager::ConfigManager(ErrorHandler* err) : errorHandler(err) {
}

// Initialize the configuration manager
bool ConfigManager::begin() {
    return loadConfigFromFile();
}

// Load configuration from the JSON file
bool ConfigManager::loadConfigFromFile() {
    // Check if config file exists
    if (!LittleFS.exists(Constants::CONFIG_FILE_PATH)) {
        errorHandler->logWarning("Config file not found, creating default");
        if (!createDefaultConfig()) {
            return false;
        }
    }
    
    // Open and read config file
    File configFile = LittleFS.open(Constants::CONFIG_FILE_PATH, "r");
    if (!configFile) {
        errorHandler->logError(ERROR, "Failed to open config file");
        return false;
    }
    
    // Parse JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();
    
    if (error) {
        errorHandler->logError(ERROR, "Failed to parse config: " + String(error.c_str()));
        return false;
    }
    
    // Extract configuration
    boardId = doc["Board ID"].as<String>();
    
    // Clear existing configurations
    sensorConfigs.clear();
    
    // Load I2C sensors
    JsonArray i2cSensors = doc["I2C Sensors"].as<JsonArray>();
    for (JsonObject sensor : i2cSensors) {
        SensorConfig config;
        config.name = sensor["Sensor Name"].as<String>();
        config.type = sensor["Sensor Type"].as<String>();
        config.address = sensor["Address (HEX)"].as<int>();
        config.isSPI = false;
        sensorConfigs.push_back(config);
        
        errorHandler->logInfo("Loaded I2C sensor: " + config.name + " (" + config.type + ") at address 0x" + String(config.address, HEX));
    }
    
    // Load SPI sensors
    JsonArray spiSensors = doc["SPI Sensors"].as<JsonArray>();
    for (JsonObject sensor : spiSensors) {
        SensorConfig config;
        config.name = sensor["Sensor Name"].as<String>();
        config.type = sensor["Sensor Type"].as<String>();
        config.address = sensor["SS Pin"].as<int>();
        config.isSPI = true;
        sensorConfigs.push_back(config);
        
        errorHandler->logInfo("Loaded SPI sensor: " + config.name + " (" + config.type + ") on SS pin " + String(config.address));
    }
    
    return true;
}

// Create a default configuration file
bool ConfigManager::createDefaultConfig() {
    JsonDocument doc;
    
    // Create default configuration
    doc["Board ID"] = "GPower EM-" + String(ESP.getEfuseMac(), HEX);
    
    // Add I2C sensors array and first sensor
    JsonArray i2cSensors = doc["I2C Sensors"].to<JsonArray>();
    JsonObject i2cSensor = i2cSensors.add<JsonObject>();
    i2cSensor["Sensor Name"] = "I2C01";
    i2cSensor["Sensor Type"] = "SHT41";
    i2cSensor["Address (HEX)"] = 0x44;  // SHT41 default address
    
    // Add empty SPI sensors array
    doc["SPI Sensors"] = JsonArray();
    
    // Save to file
    File configFile = LittleFS.open(Constants::CONFIG_FILE_PATH, "w");
    if (!configFile) {
        errorHandler->logError(ERROR, "Failed to create config file");
        return false;
    }
    
    if (serializeJson(doc, configFile) == 0) {
        errorHandler->logError(ERROR, "Failed to write config");
        configFile.close();
        return false;
    }
    
    configFile.close();
    
    // Load the configurations we just created
    return loadConfigFromFile();
}

// Get the board identifier
String ConfigManager::getBoardIdentifier() {
    return boardId;
}

// Set the board identifier
bool ConfigManager::setBoardIdentifier(String identifier) {
    // Update board ID in memory
    boardId = identifier;
    
    // Update config file
    File configFile = LittleFS.open(Constants::CONFIG_FILE_PATH, "r");
    if (!configFile) {
        errorHandler->logError(ERROR, "Failed to open config file for reading");
        return false;
    }
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();
    
    if (error) {
        errorHandler->logError(ERROR, "Failed to parse config when updating board ID");
        return false;
    }
    
    // Update the board ID
    doc["Board ID"] = boardId;
    
    // Save the updated configuration
    configFile = LittleFS.open(Constants::CONFIG_FILE_PATH, "w");
    if (!configFile) {
        errorHandler->logError(ERROR, "Failed to open config file for writing");
        return false;
    }
    
    if (serializeJson(doc, configFile) == 0) {
        errorHandler->logError(ERROR, "Failed to write updated config");
        configFile.close();
        return false;
    }
    
    configFile.close();
    errorHandler->logInfo("Updated Board ID to: " + boardId);
    
    // Notify about the configuration change
    String configJson;
    serializeJson(doc, configJson);
    notifyConfigChanged(configJson);
    
    return true;
}

// Get all sensor configurations
std::vector<SensorConfig> ConfigManager::getSensorConfigs() {
    return sensorConfigs;
}

// Update sensor configurations
bool ConfigManager::updateSensorConfigs(const std::vector<SensorConfig>& configs) {
    // Update memory copy
    sensorConfigs = configs;
    
    // Load existing config
    File configFile = LittleFS.open(Constants::CONFIG_FILE_PATH, "r");
    if (!configFile) {
        errorHandler->logError(ERROR, "Failed to open config file for reading");
        return false;
    }
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();
    
    if (error) {
        errorHandler->logError(ERROR, "Failed to parse config when updating sensors");
        return false;
    }
    
    // Clear existing sensors
    doc["I2C Sensors"] = JsonArray();
    doc["SPI Sensors"] = JsonArray();
    
    // Add updated sensors
    JsonArray i2cSensors = doc["I2C Sensors"].to<JsonArray>();
    JsonArray spiSensors = doc["SPI Sensors"].to<JsonArray>();
    
    for (const auto& config : configs) {
        if (config.isSPI) {
            JsonObject sensor = spiSensors.add<JsonObject>();
            sensor["Sensor Name"] = config.name;
            sensor["Sensor Type"] = config.type;
            sensor["SS Pin"] = config.address;
        } else {
            JsonObject sensor = i2cSensors.add<JsonObject>();
            sensor["Sensor Name"] = config.name;
            sensor["Sensor Type"] = config.type;
            sensor["Address (HEX)"] = config.address;
        }
    }
    
    // Save the updated configuration
    configFile = LittleFS.open(Constants::CONFIG_FILE_PATH, "w");
    if (!configFile) {
        errorHandler->logError(ERROR, "Failed to open config file for writing");
        return false;
    }
    
    if (serializeJson(doc, configFile) == 0) {
        errorHandler->logError(ERROR, "Failed to write updated config");
        configFile.close();
        return false;
    }
    
    configFile.close();
    errorHandler->logInfo("Updated sensor configurations");
    
    // Notify about the configuration change
    String configJson;
    serializeJson(doc, configJson);
    notifyConfigChanged(configJson);
    
    return true;
}

// Get the complete configuration as a JSON string
String ConfigManager::getConfigJson() {
    if (!LittleFS.exists(Constants::CONFIG_FILE_PATH)) {
        errorHandler->logWarning("Config file not found for retrieval");
        return "{}";
    }
    
    File configFile = LittleFS.open(Constants::CONFIG_FILE_PATH, "r");
    if (!configFile) {
        errorHandler->logError(ERROR, "Failed to open config file for retrieval");
        return "{}";
    }
    
    String configStr = configFile.readString();
    configFile.close();
    
    return configStr;
}

// Update configuration from JSON string
bool ConfigManager::updateConfigFromJson(const String& jsonConfig) {
    // Parse the JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonConfig);
    
    if (error) {
        errorHandler->logError(ERROR, "Failed to parse JSON config: " + String(error.c_str()));
        return false;
    }
    
    // Save to file
    File configFile = LittleFS.open(Constants::CONFIG_FILE_PATH, "w");
    if (!configFile) {
        errorHandler->logError(ERROR, "Failed to open config file for writing");
        return false;
    }
    
    if (serializeJson(doc, configFile) == 0) {
        errorHandler->logError(ERROR, "Failed to write config");
        configFile.close();
        return false;
    }
    
    configFile.close();
    errorHandler->logInfo("Updated configuration from JSON");
    
    // Reload the configuration
    bool success = loadConfigFromFile();
    
    // Notify about the configuration change
    if (success) {
        notifyConfigChanged(jsonConfig);
    }
    
    return success;
}

// Register a callback for configuration changes
void ConfigManager::registerChangeCallback(ConfigChangeCallback callback) {
    if (callback) {
        changeCallbacks.push_back(callback);
    }
}

// Notify all registered callbacks about a configuration change
void ConfigManager::notifyConfigChanged(const String& newConfig) {
    for (auto callback : changeCallbacks) {
        callback(newConfig);
    }
}

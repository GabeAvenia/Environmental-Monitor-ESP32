#include "ConfigManager.h"
#include <ArduinoJson.h>
#include "../Constants.h"

// Constructor
ConfigManager::ConfigManager(ErrorHandler* err) : errorHandler(err), notifyingCallbacks(false) {
}

// Initialize the configuration manager
bool ConfigManager::begin() {
    return loadConfigFromFile();
}

void ConfigManager::disableNotifications(bool disable) {
    notifyingCallbacks = disable;
}

// Register a callback for configuration changes
void ConfigManager::registerChangeCallback(ConfigChangeCallback callback) {
    if (callback) {
        changeCallbacks.push_back(callback);
        errorHandler->logInfo("Registered config change callback, total callbacks: " + String(changeCallbacks.size()));
    }
}

// Notify all registered callbacks about a configuration change
void ConfigManager::notifyConfigChanged(const String& newConfig) {
    // Prevent recursive notifications
    if (notifyingCallbacks) {
        errorHandler->logWarning("Preventing recursive notification of config changes");
        return;
    }
    
    notifyingCallbacks = true;
    
    errorHandler->logInfo("Notifying " + String(changeCallbacks.size()) + " callbacks about config change");
    for (auto callback : changeCallbacks) {
        errorHandler->logInfo("Calling a callback...");
        callback(newConfig);
    }
    
    notifyingCallbacks = false;
    errorHandler->logInfo("All callbacks notified");
}

// Load configuration from the JSON file
bool ConfigManager::loadConfigFromFile() {
    errorHandler->logInfo("Loading config file");
    
    // Check if config file exists
    if (!LittleFS.exists(Constants::CONFIG_FILE_PATH)) {
        errorHandler->logWarning("Config file not found, creating default");
        if (!createDefaultConfig()) {
            errorHandler->logError(ERROR, "Failed to create default config");
            return false;
        }
    }
    
    // Open and read config file
    errorHandler->logInfo("Opening config file for reading");
    File configFile = LittleFS.open(Constants::CONFIG_FILE_PATH, "r");
    if (!configFile) {
        errorHandler->logError(ERROR, "Failed to open config file");
        return false;
    }
    
    String fileContent = configFile.readString();
    errorHandler->logInfo("Read " + String(fileContent.length()) + " bytes from config file");
    errorHandler->logInfo("Content: " + fileContent.substring(0, 50) + "...");
    
    // Need to reopen the file since we've read it to the end
    configFile.close();
    configFile = LittleFS.open(Constants::CONFIG_FILE_PATH, "r");
    
    // Parse JSON
    JsonDocument doc;
    
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();
    
    if (error) {
        errorHandler->logError(ERROR, "Failed to parse config: " + String(error.c_str()));
        return false;
    }
    
    // Check if deserialization exceeded memory limits
    if (doc.overflowed()) {
        errorHandler->logError(ERROR, "Config too large for available memory");
        return false;
    }
    
    errorHandler->logInfo("JSON parsed successfully");
    
    // Extract configuration - look for both old and new key names for backward compatibility
    if (!doc["Environmental Monitor ID"].isNull()) {
        boardId = doc["Environmental Monitor ID"].as<String>();
        errorHandler->logInfo("Using Environmental Monitor ID: " + boardId);
    } else if (!doc["Board ID"].isNull()) {
        boardId = doc["Board ID"].as<String>();
        errorHandler->logInfo("Using Board ID: " + boardId);
    } else {
        boardId = "GPower EM-" + String(ESP.getEfuseMac(), HEX);
        errorHandler->logInfo("No ID found, using default: " + boardId);
    }
    
    // Clear existing configurations
    sensorConfigs.clear();
    errorHandler->logInfo("Cleared existing sensor configs");
    
    // Load I2C sensors
    JsonArray i2cSensors = doc["I2C Sensors"].as<JsonArray>();
    errorHandler->logInfo("I2C Sensors array exists: " + String(i2cSensors ? "YES" : "NO"));
    
    if (i2cSensors) {
        errorHandler->logInfo("Found " + String(i2cSensors.size()) + " I2C sensors");
        for (JsonObject sensor : i2cSensors) {
            SensorConfig config;
            config.name = sensor["Sensor Name"].as<String>();
            config.type = sensor["Sensor Type"].as<String>();
            config.address = sensor["Address (HEX)"].as<int>();
            config.isSPI = false;
            
            errorHandler->logInfo("Found I2C sensor: " + config.name + " of type " + config.type);
            
            // Handle I2C port (new field)
            if (!sensor["I2C Port"].isNull()) {
                String portStr = sensor["I2C Port"].as<String>();
                config.i2cPort = I2CManager::stringToPort(portStr);
                errorHandler->logInfo("Sensor using I2C port " + portStr);
            } else {
                // Default to I2C0 if not specified
                config.i2cPort = I2CPort::I2C0;
                errorHandler->logInfo("Sensor defaulting to I2C0 (no port specified)");
            }
            
            // Read polling rate with a default if not present
            config.pollingRate = 1000; // Default 1 second
            if (!sensor["Polling Rate[1000 ms]"].isNull()) {
                config.pollingRate = sensor["Polling Rate[1000 ms]"].as<uint32_t>();
                errorHandler->logInfo("Sensor polling rate: " + String(config.pollingRate) + "ms");
            } else {
                errorHandler->logInfo("Using default polling rate: 1000ms");
            }
            
            // Read additional settings
            if (!sensor["Additional"].isNull()) {
                config.additional = sensor["Additional"].as<String>();
                errorHandler->logInfo("Additional settings: " + config.additional);
            } else {
                config.additional = ""; // Empty string for no additional settings
            }
            
            sensorConfigs.push_back(config);
            
            errorHandler->logInfo("Added I2C sensor: " + config.name);
        }
    }
    
    // Load SPI sensors
    JsonArray spiSensors = doc["SPI Sensors"].as<JsonArray>();
    errorHandler->logInfo("SPI Sensors array exists: " + String(spiSensors ? "YES" : "NO"));
    
    if (spiSensors) {
        errorHandler->logInfo("Found " + String(spiSensors.size()) + " SPI sensors");
        for (JsonObject sensor : spiSensors) {
            SensorConfig config;
            config.name = sensor["Sensor Name"].as<String>();
            config.type = sensor["Sensor Type"].as<String>();
            config.address = sensor["SS Pin"].as<int>();
            config.isSPI = true;
            
            errorHandler->logInfo("Found SPI sensor: " + config.name + " of type " + config.type);
            
            // SPI sensors don't use I2C port
            config.i2cPort = I2CPort::I2C0; // Default value, not used
            
            // Read polling rate with a default if not present
            config.pollingRate = 1000; // Default 1 second
            if (!sensor["Polling Rate[1000 ms]"].isNull()) {
                config.pollingRate = sensor["Polling Rate[1000 ms]"].as<uint32_t>();
                errorHandler->logInfo("Sensor polling rate: " + String(config.pollingRate) + "ms");
            } else {
                errorHandler->logInfo("Using default polling rate: 1000ms");
            }
            
            // Read additional settings
            if (!sensor["Additional"].isNull()) {
                config.additional = sensor["Additional"].as<String>();
                errorHandler->logInfo("Additional settings: " + config.additional);
            } else {
                config.additional = ""; // Empty string for no additional settings
            }
            
            sensorConfigs.push_back(config);
            
            errorHandler->logInfo("Added SPI sensor: " + config.name);
        }
    }
    
    errorHandler->logInfo("Configuration loaded successfully with " + String(sensorConfigs.size()) + " sensors");
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
    i2cSensor["I2C Port"] = "I2C0";
    i2cSensor["Address (HEX)"] = 0x44;  // SHT41 default address
    i2cSensor["Polling Rate[1000 ms]"] = 1000;  // Default 1 second
    i2cSensor["Additional"] = "";  // No additional settings by default
    
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
    
    // Check whether to use Environmental Monitor ID or Board ID
    if (!doc["Environmental Monitor ID"].isNull()) {
        doc["Environmental Monitor ID"] = boardId;
    } else {
        // Update the board ID - use Board ID key
        doc["Board ID"] = boardId;
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
            sensor["Polling Rate[1000 ms]"] = config.pollingRate;
            // Add the additional field
            if (config.additional.length() > 0) {
                sensor["Additional"] = config.additional;
            }
        } else {
            JsonObject sensor = i2cSensors.add<JsonObject>();
            sensor["Sensor Name"] = config.name;
            sensor["Sensor Type"] = config.type;
            sensor["I2C Port"] = I2CManager::portToString(config.i2cPort);
            sensor["Address (HEX)"] = config.address;
            sensor["Polling Rate[1000 ms]"] = config.pollingRate;
            // Add the additional field
            if (config.additional.length() > 0) {
                sensor["Additional"] = config.additional;
            }
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
    
    // Notify about the configuration change, but only if we're not already in a notification
    if (!notifyingCallbacks) {
        String configJson;
        serializeJson(doc, configJson);
        notifyConfigChanged(configJson);
    }
    
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
    // Log a shorter version of the config to avoid huge logs
    errorHandler->logInfo("Received config update: " + 
                          jsonConfig.substring(0, std::min(50, (int)jsonConfig.length())) + 
                          (jsonConfig.length() > 50 ? "..." : ""));
    
    // Extract clean JSON data by finding the opening brace
    int jsonStart = jsonConfig.indexOf('{');
    if (jsonStart < 0) {
        errorHandler->logError(ERROR, "No JSON object found in config");
        return false;
    }
    String cleanJson = jsonConfig.substring(jsonStart);
    
    // Parse the JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, cleanJson);
    
    if (error || doc.overflowed()) {
        errorHandler->logError(ERROR, "Failed to parse JSON config: " + 
                              (error ? String(error.c_str()) : "Document too large"));
        return false;
    }

    // Standardize ID field (convert legacy format if needed)
    standardizeConfigFields(doc);
    
    // Save to file
    if (!writeConfigToFile(doc)) {
        return false;
    }
    
    // Reload and notify
    bool success = loadConfigFromFile();
    if (success) {
        errorHandler->logInfo("Config reloaded successfully, notifying listeners");
        notifyConfigChanged(jsonConfig);
    } else {
        errorHandler->logError(ERROR, "Failed to reload configuration");
    }
    
    return success;
}

// Helper to standardize config fields (handle older formats)
void ConfigManager::standardizeConfigFields(JsonDocument& doc) {
    // Check which ID field is used
    if (!doc["Environmental Monitor ID"].isNull()) {
        // Convert to Board ID
        doc["Board ID"] = doc["Environmental Monitor ID"];
        doc.remove("Environmental Monitor ID");
    } else if (doc["Board ID"].isNull()) {
        // Add default ID if none exists
        doc["Board ID"] = "GPower EM-" + String(ESP.getEfuseMac(), HEX);
    }
}

// Helper to write config to file
bool ConfigManager::writeConfigToFile(const JsonDocument& doc) {
    File configFile = LittleFS.open(Constants::CONFIG_FILE_PATH, "w");
    if (!configFile) {
        errorHandler->logError(ERROR, "Failed to open config file for writing");
        return false;
    }
    
    size_t written = serializeJson(doc, configFile);
    configFile.close();
    
    if (written == 0) {
        errorHandler->logError(ERROR, "Failed to write config - 0 bytes written");
        return false;
    }
    
    return true;
}
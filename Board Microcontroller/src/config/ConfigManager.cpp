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
    Serial.println("DEBUG: loadConfigFromFile called");
    
    // Check if config file exists
    if (!LittleFS.exists(Constants::CONFIG_FILE_PATH)) {
        Serial.println("DEBUG: Config file not found, creating default");
        errorHandler->logWarning("Config file not found, creating default");
        if (!createDefaultConfig()) {
            Serial.println("DEBUG: Failed to create default config");
            return false;
        }
    }
    
    // Open and read config file
    Serial.println("DEBUG: Opening config file for reading");
    File configFile = LittleFS.open(Constants::CONFIG_FILE_PATH, "r");
    if (!configFile) {
        Serial.println("DEBUG: Failed to open config file");
        errorHandler->logError(ERROR, "Failed to open config file");
        return false;
    }
    
    String fileContent = configFile.readString();
    Serial.println("DEBUG: Read " + String(fileContent.length()) + " bytes from config file");
    Serial.println("DEBUG: Content: " + fileContent.substring(0, 50) + "...");
    
    // Need to reopen the file since we've read it to the end
    configFile.close();
    configFile = LittleFS.open(Constants::CONFIG_FILE_PATH, "r");
    
    // Parse JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();
    
    if (error) {
        Serial.println("DEBUG: Failed to parse config: " + String(error.c_str()));
        errorHandler->logError(ERROR, "Failed to parse config: " + String(error.c_str()));
        return false;
    }
    
    Serial.println("DEBUG: JSON parsed successfully");
    
    // Extract configuration - look for both old and new key names for backward compatibility
    if (doc["Environmental Monitor ID"]) {
        boardId = doc["Environmental Monitor ID"].as<String>();
        Serial.println("DEBUG: Using Environmental Monitor ID: " + boardId);
    } else if (doc["Board ID"]) {
        boardId = doc["Board ID"].as<String>();
        Serial.println("DEBUG: Using Board ID: " + boardId);
    } else {
        boardId = "GPower EM-" + String(ESP.getEfuseMac(), HEX);
        Serial.println("DEBUG: No ID found, using default: " + boardId);
    }
    
    // Clear existing configurations
    sensorConfigs.clear();
    Serial.println("DEBUG: Cleared existing sensor configs");
    
    // Load I2C sensors
    JsonArray i2cSensors = doc["I2C Sensors"].as<JsonArray>();
    Serial.println("DEBUG: I2C Sensors array exists: " + String(i2cSensors ? "YES" : "NO"));
    
    if (i2cSensors) {
        Serial.println("DEBUG: Found " + String(i2cSensors.size()) + " I2C sensors");
        for (JsonObject sensor : i2cSensors) {
            SensorConfig config;
            config.name = sensor["Sensor Name"].as<String>();
            config.type = sensor["Sensor Type"].as<String>();
            config.address = sensor["Address (HEX)"].as<int>();
            config.isSPI = false;
            
            Serial.println("DEBUG: Found I2C sensor: " + config.name + " of type " + config.type);
            
            // Handle I2C port (new field)
            if (!sensor["I2C Port"].isNull()) {
                String portStr = sensor["I2C Port"].as<String>();
                config.i2cPort = I2CManager::stringToPort(portStr);
                Serial.println("DEBUG: Sensor using I2C port " + portStr);
            } else {
                // Default to I2C0 if not specified
                config.i2cPort = I2CPort::I2C0;
                Serial.println("DEBUG: Sensor defaulting to I2C0 (no port specified)");
            }
            
            // Read polling rate with a default if not present
            config.pollingRate = 1000; // Default 1 second
            if (!sensor["Polling Rate[1000 ms]"].isNull()) {
                config.pollingRate = sensor["Polling Rate[1000 ms]"].as<uint32_t>();
                Serial.println("DEBUG: Sensor polling rate: " + String(config.pollingRate) + "ms");
            } else {
                Serial.println("DEBUG: Using default polling rate: 1000ms");
            }
            
            sensorConfigs.push_back(config);
            
            Serial.println("DEBUG: Added I2C sensor: " + config.name);
        }
    }
    
    // Load SPI sensors
    JsonArray spiSensors = doc["SPI Sensors"].as<JsonArray>();
    Serial.println("DEBUG: SPI Sensors array exists: " + String(spiSensors ? "YES" : "NO"));
    
    if (spiSensors) {
        Serial.println("DEBUG: Found " + String(spiSensors.size()) + " SPI sensors");
        for (JsonObject sensor : spiSensors) {
            SensorConfig config;
            config.name = sensor["Sensor Name"].as<String>();
            config.type = sensor["Sensor Type"].as<String>();
            config.address = sensor["SS Pin"].as<int>();
            config.isSPI = true;
            
            Serial.println("DEBUG: Found SPI sensor: " + config.name + " of type " + config.type);
            
            // SPI sensors don't use I2C port
            config.i2cPort = I2CPort::I2C0; // Default value, not used
            
            // Read polling rate with a default if not present
            config.pollingRate = 1000; // Default 1 second
            if (!sensor["Polling Rate[1000 ms]"].isNull()) {
                config.pollingRate = sensor["Polling Rate[1000 ms]"].as<uint32_t>();
                Serial.println("DEBUG: Sensor polling rate: " + String(config.pollingRate) + "ms");
            } else {
                Serial.println("DEBUG: Using default polling rate: 1000ms");
            }
            
            sensorConfigs.push_back(config);
            
            Serial.println("DEBUG: Added SPI sensor: " + config.name);
        }
    }
    
    Serial.println("DEBUG: Configuration loaded successfully with " + String(sensorConfigs.size()) + " sensors");
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
    if (doc["Environmental Monitor ID"]) {
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
        } else {
            JsonObject sensor = i2cSensors.add<JsonObject>();
            sensor["Sensor Name"] = config.name;
            sensor["Sensor Type"] = config.type;
            sensor["I2C Port"] = I2CManager::portToString(config.i2cPort);
            sensor["Address (HEX)"] = config.address;
            sensor["Polling Rate[1000 ms]"] = config.pollingRate;
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
    errorHandler->logInfo("Received config update: " + jsonConfig);
    Serial.println("DEBUG: Updating config with: " + jsonConfig.substring(0, 50) + "...");
    
    // Parse the JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonConfig);
    
    if (error) {
        errorHandler->logError(ERROR, "Failed to parse JSON config: " + String(error.c_str()));
        Serial.println("DEBUG: JSON parse error: " + String(error.c_str()));
        return false;
    }
    
    Serial.println("DEBUG: JSON parsed successfully");
    
    // Check which ID field is used
    if (doc["Environmental Monitor ID"]) {
        Serial.println("DEBUG: Found 'Environmental Monitor ID'");
        // Convert to Board ID
        doc["Board ID"] = doc["Environmental Monitor ID"];
        doc.remove("Environmental Monitor ID");
    } else if (doc["Board ID"]) {
        Serial.println("DEBUG: Found 'Board ID'");
    } else {
        Serial.println("DEBUG: No ID field found");
    }
    
    // Test serializing to string to see what we're about to write
    String serializedDoc;
    serializeJson(doc, serializedDoc);
    Serial.println("DEBUG: About to write: " + serializedDoc.substring(0, 50) + "...");
    
    // Check if the file exists and can be opened for reading
    if (LittleFS.exists(Constants::CONFIG_FILE_PATH)) {
        Serial.println("DEBUG: Config file exists");
        File testFile = LittleFS.open(Constants::CONFIG_FILE_PATH, "r");
        if (testFile) {
            testFile.close();
            Serial.println("DEBUG: Confirmed config file can be opened for reading");
        } else {
            Serial.println("DEBUG: Config file exists but CANNOT be opened for reading");
        }
    } else {
        Serial.println("DEBUG: Config file does not exist");
    }
    
    // Save to file - force file creation
    Serial.println("DEBUG: Attempting to open config file for writing...");
    File configFile = LittleFS.open(Constants::CONFIG_FILE_PATH, "w");
    if (!configFile) {
        errorHandler->logError(ERROR, "Failed to open config file for writing");
        Serial.println("DEBUG: Failed to open config file for writing!");
        return false;
    }
    
    Serial.println("DEBUG: Config file opened for writing");
    
    size_t written = serializeJson(doc, configFile);
    if (written == 0) {
        errorHandler->logError(ERROR, "Failed to write config");
        Serial.println("DEBUG: Failed to write config - 0 bytes written");
        configFile.close();
        return false;
    }
    
    Serial.println("DEBUG: Wrote " + String(written) + " bytes to config file");
    configFile.close();
    
    // Verify the file was written correctly
    configFile = LittleFS.open(Constants::CONFIG_FILE_PATH, "r");
    if (!configFile) {
        errorHandler->logError(ERROR, "Failed to open config file for verification");
        Serial.println("DEBUG: Can't open config file for verification!");
        return false;
    }
    
    String fileContent = configFile.readString();
    configFile.close();
    
    Serial.println("DEBUG: Read back file content: " + fileContent.substring(0, 50) + "...");
    
    // Attempt to reload the configuration
    Serial.println("DEBUG: Reloading configuration...");
    bool success = loadConfigFromFile();
    
    if (success) {
        Serial.println("DEBUG: Config reloaded successfully");
        errorHandler->logInfo("Config reloaded successfully, notifying listeners");
        
        // Count how many callbacks are registered
        Serial.println("DEBUG: Notifying " + String(changeCallbacks.size()) + " callbacks");
        notifyConfigChanged(jsonConfig);
    } else {
        Serial.println("DEBUG: Failed to reload configuration!");
        errorHandler->logError(ERROR, "Failed to reload configuration");
    }
    
    return success;
}

// Register a callback for configuration changes
void ConfigManager::registerChangeCallback(ConfigChangeCallback callback) {
    if (callback) {
        changeCallbacks.push_back(callback);
        Serial.println("DEBUG: Registered config change callback, total callbacks: " + String(changeCallbacks.size()));
    }
}

// Notify all registered callbacks about a configuration change
void ConfigManager::notifyConfigChanged(const String& newConfig) {
    Serial.println("DEBUG: Notifying " + String(changeCallbacks.size()) + " callbacks about config change");
    for (auto callback : changeCallbacks) {
        Serial.println("DEBUG: Calling a callback...");
        callback(newConfig);
    }
    Serial.println("DEBUG: All callbacks notified");
}
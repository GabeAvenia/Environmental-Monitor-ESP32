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
        errorHandler->logError(INFO, "Registered config change callback, total callbacks: " + String(changeCallbacks.size()));
    }
}

// Notify all registered callbacks about a configuration change
void ConfigManager::notifyConfigChanged(const String& newConfig) {
    // Prevent recursive notifications
    if (notifyingCallbacks) {
        errorHandler->logError(WARNING, "Preventing recursive notification of config changes");
        return;
    }
    
    notifyingCallbacks = true;
    
    errorHandler->logError(INFO, "Notifying " + String(changeCallbacks.size()) + " callbacks about config change");
    for (auto callback : changeCallbacks) {
        errorHandler->logError(INFO, "Calling a callback...");
        callback(newConfig);
    }
    
    notifyingCallbacks = false;
    errorHandler->logError(INFO, "All callbacks notified");
}

// Load configuration from the JSON file
bool ConfigManager::loadConfigFromFile() {
    errorHandler->logError(INFO, "Loading config file");
    
    // Define polling rate constraints
    const uint32_t DEFAULT_POLLING_RATE = 1000;   // Default: 1 second
    const uint32_t MIN_POLLING_RATE = 50;         // Minimum: 50ms
    const uint32_t MAX_POLLING_RATE = 300000;     // Maximum: 5 minutes (300 seconds)
    
    // Check if config file exists
    if (!LittleFS.exists(Constants::CONFIG_FILE_PATH)) {
        errorHandler->logError(WARNING, "Config file not found, creating default");
        if (!createDefaultConfig()) {
            errorHandler->logError(ERROR, "Failed to create default config");
            return false;
        }
    }
    
    // Open and read config file
    errorHandler->logError(INFO, "Opening config file for reading");
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
    
    // Check if deserialization exceeded memory limits
    if (doc.overflowed()) {
        errorHandler->logError(ERROR, "Config too large for available memory");
        return false;
    }
    
    errorHandler->logError(INFO, "JSON parsed successfully");
    
    // Extract Environmental Monitor IDentifier - handle both old and new key names for backward compatibility
    if (doc["Environmental Monitor ID"].is<String>()) {
        boardId = doc["Environmental Monitor ID"].as<String>();
        errorHandler->logError(INFO, "Using Environmental Monitor ID: " + boardId);
    } else {
        boardId = "GPower EM-" + String(ESP.getEfuseMac(), HEX);
        errorHandler->logError(INFO, "No ID found, using default: " + boardId);
    }
    
    // Clear existing configurations
    sensorConfigs.clear();
    additionalConfig = "";
    errorHandler->logError(INFO, "Cleared existing configurations");
    
    // Load I2C sensors
    JsonArray i2cSensors = doc["I2C Sensors"].as<JsonArray>();
    errorHandler->logError(INFO, "I2C Sensors array exists: " + String(i2cSensors.size() > 0 ? "YES" : "NO"));
    
    if (i2cSensors.size() > 0) {
        errorHandler->logError(INFO, "Found " + String(i2cSensors.size()) + " I2C sensors");
        for (JsonObject sensor : i2cSensors) {
            if (!sensor["Sensor Name"].is<String>() || !sensor["Sensor Type"].is<String>() || 
                !sensor["Address (HEX)"].is<int>()) {
                errorHandler->logError(WARNING, "Skipping I2C sensor with missing required fields");
                continue;
            }
            
            SensorConfig config;
            config.name = sensor["Sensor Name"].as<String>();
            config.type = sensor["Sensor Type"].as<String>();
            config.address = sensor["Address (HEX)"].as<int>();
            config.isSPI = false;
            
            errorHandler->logError(INFO, "Found I2C sensor: " + config.name + " of type " + config.type);
            
            // Handle I2C port (new field)
            if (sensor["I2C Port"].is<String>()) {
                String portStr = sensor["I2C Port"].as<String>();
                config.i2cPort = I2CManager::stringToPort(portStr);
                errorHandler->logError(INFO, "Sensor using I2C port " + portStr);
            } else {
                // Default to I2C0 if not specified
                config.i2cPort = I2CPort::I2C0;
                errorHandler->logError(INFO, "Sensor defaulting to I2C0 (no port specified)");
            }
            
            // Read and validate polling rate
            if (sensor["Polling Rate[1000 ms]"].is<uint32_t>()) {
                uint32_t rate = sensor["Polling Rate[1000 ms]"].as<uint32_t>();
                // Apply validation rules
                if (rate < MIN_POLLING_RATE) {
                    errorHandler->logError(WARNING, "Polling rate too low for sensor " + config.name + 
                                             " (" + String(rate) + "ms), using " + 
                                             String(MIN_POLLING_RATE) + "ms minimum");
                    config.pollingRate = MIN_POLLING_RATE;
                } else if (rate > MAX_POLLING_RATE) {
                    errorHandler->logError(WARNING, "Polling rate too high for sensor " + config.name + 
                                             " (" + String(rate) + "ms), using " + 
                                             String(MAX_POLLING_RATE) + "ms maximum");
                    config.pollingRate = MAX_POLLING_RATE;
                } else {
                    config.pollingRate = rate;
                }
                errorHandler->logError(INFO, "Sensor polling rate: " + String(config.pollingRate) + "ms");
            } else {
                config.pollingRate = DEFAULT_POLLING_RATE;
                errorHandler->logError(INFO, "Using default polling rate: " + String(DEFAULT_POLLING_RATE) + "ms");
            }
            
            // Read additional settings
            if (sensor["Additional"].is<String>()) {
                config.additional = sensor["Additional"].as<String>();
                errorHandler->logError(INFO, "Additional settings: " + config.additional);
            } else {
                config.additional = ""; // Empty string for no additional settings
            }
            
            sensorConfigs.push_back(config);
            errorHandler->logError(INFO, "Added I2C sensor: " + config.name);
        }
    }
    
    // Load SPI sensors
    JsonArray spiSensors = doc["SPI Sensors"].as<JsonArray>();
    errorHandler->logError(INFO, "SPI Sensors array exists: " + String(spiSensors.size() > 0 ? "YES" : "NO"));
    
    if (spiSensors.size() > 0) {
        errorHandler->logError(INFO, "Found " + String(spiSensors.size()) + " SPI sensors");
        for (JsonObject sensor : spiSensors) {
            if (!sensor["Sensor Name"].is<String>() || !sensor["Sensor Type"].is<String>() || 
                !sensor["SS Pin"].is<int>()) {
                errorHandler->logError(WARNING, "Skipping SPI sensor with missing required fields");
                continue;
            }
            
            SensorConfig config;
            config.name = sensor["Sensor Name"].as<String>();
            config.type = sensor["Sensor Type"].as<String>();
            config.address = sensor["SS Pin"].as<int>();
            config.isSPI = true;
            
            errorHandler->logError(INFO, "Found SPI sensor: " + config.name + " of type " + config.type);
            
            // SPI sensors don't use I2C port
            config.i2cPort = I2CPort::I2C0; // Default value, not used
            
            // Read and validate polling rate
            if (sensor["Polling Rate[1000 ms]"].is<uint32_t>()) {
                uint32_t rate = sensor["Polling Rate[1000 ms]"].as<uint32_t>();
                // Apply validation rules
                if (rate < MIN_POLLING_RATE) {
                    errorHandler->logError(WARNING, "Polling rate too low for sensor " + config.name + 
                                             " (" + String(rate) + "ms), using " + 
                                             String(MIN_POLLING_RATE) + "ms minimum");
                    config.pollingRate = MIN_POLLING_RATE;
                } else if (rate > MAX_POLLING_RATE) {
                    errorHandler->logError(WARNING, "Polling rate too high for sensor " + config.name + 
                                             " (" + String(rate) + "ms), using " + 
                                             String(MAX_POLLING_RATE) + "ms maximum");
                    config.pollingRate = MAX_POLLING_RATE;
                } else {
                    config.pollingRate = rate;
                }
                errorHandler->logError(INFO, "Sensor polling rate: " + String(config.pollingRate) + "ms");
            } else {
                config.pollingRate = DEFAULT_POLLING_RATE;
                errorHandler->logError(INFO, "Using default polling rate: " + String(DEFAULT_POLLING_RATE) + "ms");
            }
            
            // Read additional settings
            if (sensor["Additional"].is<String>()) {
                config.additional = sensor["Additional"].as<String>();
                errorHandler->logError(INFO, "Additional settings: " + config.additional);
            } else {
                config.additional = ""; // Empty string for no additional settings
            }
            
            sensorConfigs.push_back(config);
            errorHandler->logError(INFO, "Added SPI sensor: " + config.name);
        }
    }
    
    // Load Additional configuration if present
    if (doc["Additional"].is<JsonObject>()) {
        // Serialize the Additional field to a string
        String additionalJson;
        serializeJson(doc["Additional"], additionalJson);
        additionalConfig = additionalJson;
        errorHandler->logError(INFO, "Loaded additional configuration section");
    }
    
    errorHandler->logError(INFO, "Configuration loaded successfully with " + String(sensorConfigs.size()) + " sensors");
    return true;
}

// Create a default configuration file
bool ConfigManager::createDefaultConfig() {
    const uint32_t DEFAULT_POLLING_RATE = 1000;  // Default: 1 second
    
    JsonDocument doc;
    
    // Create default configuration
    doc["Environmental Monitor ID"] = "GPower EM-" + String(ESP.getEfuseMac(), HEX);
    
    // Add I2C sensors array and first sensor
    JsonArray i2cSensors = doc["I2C Sensors"].to<JsonArray>();
    JsonObject i2cSensor = i2cSensors.add<JsonObject>();
    i2cSensor["Sensor Name"] = "I2C01";
    i2cSensor["Sensor Type"] = "SHT41";
    i2cSensor["I2C Port"] = "I2C0";
    i2cSensor["Address (HEX)"] = 0x44;  // SHT41 default address
    i2cSensor["Polling Rate[1000 ms]"] = DEFAULT_POLLING_RATE;
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
    // Update Environmental Monitor ID in memory
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
        errorHandler->logError(ERROR, "Failed to parse config when updating Environmental Monitor ID");
        return false;
    }
    
    // Update the Environmental Monitor ID in the JSON document
    doc["Environmental Monitor ID"] = boardId;
        
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
    errorHandler->logError(INFO, "Updated Environmental Monitor ID to: " + boardId);
    
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
    errorHandler->logError(INFO, "Updated sensor configurations");
    
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
        errorHandler->logError(WARNING, "Config file not found for retrieval");
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
    errorHandler->logError(INFO, "Received config update: " + 
                         jsonConfig.substring(0, std::min(50, (int)jsonConfig.length())) + 
                         (jsonConfig.length() > 50 ? "..." : ""));
    
    // Make a backup of the current configuration
    String backupConfig = getConfigJson();
    
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

    // Save to file
    if (!writeConfigToFile(doc)) {
        errorHandler->logError(ERROR, "Failed to write new configuration to file");
        return false;
    }
    
    // Reload and notify
    bool success = loadConfigFromFile();
    if (success) {
        errorHandler->logError(INFO, "Config reloaded successfully, notifying listeners");
        notifyConfigChanged(jsonConfig);
    } else {
        errorHandler->logError(ERROR, "Failed to reload configuration, rolling back to previous state");
        
        // Restore the previous configuration
        JsonDocument backupDoc;
        DeserializationError backupError = deserializeJson(backupDoc, backupConfig);
        
        if (!backupError && writeConfigToFile(backupDoc)) {
            errorHandler->logError(INFO, "Successfully rolled back to previous configuration");
            loadConfigFromFile(); // Reload the backup configuration
        } else {
            errorHandler->logError(ERROR, "Failed to roll back to previous configuration");
        }
    }
    
    return success;
}

// Helper to write JSON document to file
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

// Helper to read config from file
bool ConfigManager::readConfigFromFile(JsonDocument& doc) {
    if (!LittleFS.exists(Constants::CONFIG_FILE_PATH)) {
        errorHandler->logError(ERROR, "Config file not found");
        return false;
    }
    
    File configFile = LittleFS.open(Constants::CONFIG_FILE_PATH, "r");
    if (!configFile) {
        errorHandler->logError(ERROR, "Failed to open config file for reading");
        return false;
    }
    
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();
    
    if (error) {
        errorHandler->logError(ERROR, "Failed to parse config file: " + String(error.c_str()));
        return false;
    }
    
    return true;
}

// Update only the sensor configuration from JSON
bool ConfigManager::updateSensorConfigFromJson(const String& jsonConfig) {
    // Handle empty input - erase sensors with warning
    if (jsonConfig.length() == 0 || jsonConfig == "{}" || jsonConfig == "null") {
        errorHandler->logError(WARNING, "Empty sensor configuration received - clearing all sensors");
        sensorConfigs.clear();
        
        // Update the configuration file
        return updateSensorConfigs(sensorConfigs);
    }
    
    // Clean the input JSON and parse it
    String cleanJson = jsonConfig;
    int jsonStart = cleanJson.indexOf('{');
    if (jsonStart > 0) {
        cleanJson = cleanJson.substring(jsonStart);
    }
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, cleanJson);
    
    if (error || doc.overflowed()) {
        errorHandler->logError(ERROR, "Failed to parse sensor configuration JSON: " + 
                              (error ? String(error.c_str()) : "Document too large"));
        return false;
    }
    
    // Temporary storage for new configurations
    std::vector<SensorConfig> newSensorConfigs;
    
    // Extract I2C sensors if present
    if (doc["I2C Sensors"].is<JsonArray>()) {
        JsonArray i2cSensors = doc["I2C Sensors"].as<JsonArray>();
        
        for (JsonObject sensor : i2cSensors) {
            if (!sensor["Sensor Name"].is<String>() || !sensor["Sensor Type"].is<String>() || 
                !sensor["Address (HEX)"].is<int>()) {
                errorHandler->logError(ERROR, "Missing required fields in I2C sensor configuration");
                continue;
            }
            
            SensorConfig config;
            config.name = sensor["Sensor Name"].as<String>();
            config.type = sensor["Sensor Type"].as<String>();
            config.address = sensor["Address (HEX)"].as<int>();
            config.isSPI = false;
            
            // Optional fields with defaults
            config.i2cPort = I2CManager::stringToPort(
                sensor["I2C Port"].is<String>() ? sensor["I2C Port"].as<String>() : "I2C0");
            config.pollingRate = sensor["Polling Rate[1000 ms]"].is<uint32_t>() ? 
                sensor["Polling Rate[1000 ms]"].as<uint32_t>() : 1000;
            config.additional = sensor["Additional"].is<String>() ? 
                sensor["Additional"].as<String>() : "";
            
            // Apply polling rate limits
            config.pollingRate = constrain(config.pollingRate, 50, 300000);
            
            newSensorConfigs.push_back(config);
        }
    }
    
    // Extract SPI sensors if present
    if (doc["SPI Sensors"].is<JsonArray>()) {
        JsonArray spiSensors = doc["SPI Sensors"].as<JsonArray>();
        
        for (JsonObject sensor : spiSensors) {
            if (!sensor["Sensor Name"].is<String>() || !sensor["Sensor Type"].is<String>() || 
                !sensor["SS Pin"].is<int>()) {
                errorHandler->logError(ERROR, "Missing required fields in SPI sensor configuration");
                continue;
            }
            
            SensorConfig config;
            config.name = sensor["Sensor Name"].as<String>();
            config.type = sensor["Sensor Type"].as<String>();
            config.address = sensor["SS Pin"].as<int>();
            config.isSPI = true;
            
            // Optional fields with defaults
            config.pollingRate = sensor["Polling Rate[1000 ms]"].is<uint32_t>() ? 
                sensor["Polling Rate[1000 ms]"].as<uint32_t>() : 1000;
            config.additional = sensor["Additional"].is<String>() ? 
                sensor["Additional"].as<String>() : "";
            
            // Apply polling rate limits
            config.pollingRate = constrain(config.pollingRate, 50, 300000);
            
            newSensorConfigs.push_back(config);
        }
    }
    
    // Verify we have at least one valid sensor if we received config
    if (newSensorConfigs.empty() && 
        (doc["I2C Sensors"].is<JsonArray>() || doc["SPI Sensors"].is<JsonArray>())) {
        errorHandler->logError(WARNING, "No valid sensors found in configuration, keeping existing sensors");
        return false;
    }
    
    // Backup existing config before updating
    std::vector<SensorConfig> oldSensorConfigs = sensorConfigs;
    
    // Update sensor configs
    sensorConfigs = newSensorConfigs;
    
    // Read current configuration file to get non-sensor parts
    JsonDocument fullDoc;
    if (!readConfigFromFile(fullDoc)) {
        // Failed to read existing config, revert to old sensors
        sensorConfigs = oldSensorConfigs;
        return false;
    }
    
    // Clear existing sensor arrays
    fullDoc["I2C Sensors"] = JsonArray();
    fullDoc["SPI Sensors"] = JsonArray();
    
    // Add updated sensors
    JsonArray i2cSensors = fullDoc["I2C Sensors"].to<JsonArray>();
    JsonArray spiSensors = fullDoc["SPI Sensors"].to<JsonArray>();
    
    for (const auto& config : sensorConfigs) {
        if (config.isSPI) {
            JsonObject sensor = spiSensors.add<JsonObject>();
            sensor["Sensor Name"] = config.name;
            sensor["Sensor Type"] = config.type;
            sensor["SS Pin"] = config.address;
            sensor["Polling Rate[1000 ms]"] = config.pollingRate;
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
            if (config.additional.length() > 0) {
                sensor["Additional"] = config.additional;
            }
        }
    }
    
    // Write updated configuration to file
    if (!writeConfigToFile(fullDoc)) {
        // If writing fails, revert to old configuration
        sensorConfigs = oldSensorConfigs;
        errorHandler->logError(ERROR, "Failed to write updated sensor configuration");
        return false;
    }
    
    errorHandler->logError(INFO, "Sensor configuration updated successfully with " + 
                       String(sensorConfigs.size()) + " sensors");
    
    // Notify about the configuration change
    String configJson;
    serializeJson(fullDoc, configJson);
    notifyConfigChanged(configJson);
    
    return true;
}

// Update only the additional configuration
bool ConfigManager::updateAdditionalConfigFromJson(const String& jsonConfig) {
    // Handle empty input - erase additional config with warning
    if (jsonConfig.length() == 0 || jsonConfig == "{}" || jsonConfig == "null") {
        errorHandler->logError(WARNING, "Empty additional configuration received - clearing additional section");
        additionalConfig = "";
        
        // Update the configuration file
        JsonDocument doc;
        if (!readConfigFromFile(doc)) {
            errorHandler->logError(ERROR, "Failed to read existing configuration");
            return false;
        }
        
        // Remove Additional field if it exists
        if (doc["Additional"].is<JsonObject>()) {
            doc.remove("Additional");
        }
        
        // Write updated configuration to file
        return writeConfigToFile(doc);
    }
    
    // Clean the input JSON and parse it
    String cleanJson = jsonConfig;
    int jsonStart = cleanJson.indexOf('{');
    if (jsonStart > 0) {
        cleanJson = cleanJson.substring(jsonStart);
    }
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, cleanJson);
    
    if (error || doc.overflowed()) {
        errorHandler->logError(ERROR, "Failed to parse additional configuration JSON: " + 
                              (error ? String(error.c_str()) : "Document too large"));
        return false;
    }
    
    // Extract Additional configuration if present
    String newAdditionalConfig = "";
    if (doc["Additional"].is<JsonObject>()) {
        // Serialize the Additional field to a string if it's an object
        serializeJson(doc["Additional"], newAdditionalConfig);
    } else if (doc["Additional"].is<String>()) {
        // Use the string directly if it's a string
        newAdditionalConfig = doc["Additional"].as<String>();
    } else {
        // Convert to string if it's another type
        newAdditionalConfig = doc["Additional"].as<String>();
    }
    
    // Backup existing additional config
    String oldAdditionalConfig = additionalConfig;
    
    // Update additional config
    additionalConfig = newAdditionalConfig;
    
    // Read current configuration file to get other parts
    JsonDocument fullDoc;
    if (!readConfigFromFile(fullDoc)) {
        // Failed to read existing config, revert to old additional config
        additionalConfig = oldAdditionalConfig;
        return false;
    }
    
    // Update the Additional field
    if (additionalConfig.length() > 0) {
        // Parse the additionalConfig to a JSON object
        JsonDocument additionalDoc;
        DeserializationError additionalError = deserializeJson(additionalDoc, additionalConfig);
        
    if (!additionalError && !additionalConfig.startsWith("\"")) {
        // If it's a valid JSON object and not a JSON string, store as object
        fullDoc["Additional"] = additionalDoc;
    } else {
        // If not a valid JSON object or it's a JSON string, store as raw string
        fullDoc["Additional"] = additionalConfig;
    }
    }
    // Write updated configuration to file
    if (!writeConfigToFile(fullDoc)) {
        // If writing fails, revert to old configuration
        additionalConfig = oldAdditionalConfig;
        errorHandler->logError(ERROR, "Failed to write updated additional configuration");
        return false;
    }
    
    errorHandler->logError(INFO, "Additional configuration updated successfully");
    
    // Notify about the configuration change
    String configJson;
    serializeJson(fullDoc, configJson);
    notifyConfigChanged(configJson);
    
    return true;
}
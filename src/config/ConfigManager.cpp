#include "ConfigManager.h"
#include <ArduinoJson.h>
#include "../Constants.h"
#include "../sensors/SensorTypes.h"

ConfigManager::ConfigManager(ErrorHandler* err) : errorHandler(err), notifyingCallbacks(false) {
}

bool ConfigManager::begin() {
    return loadConfigFromFile();
}

void ConfigManager::disableNotifications(bool disable) {
    notifyingCallbacks = disable;
}

int ConfigManager::i2cPortStringToNumber(const String& portStr) {
    if (portStr.startsWith("I2C")) {
        String numStr = portStr.substring(3); // Extract number after "I2C"
        int portNum = numStr.toInt();

        // Validate port number (0, 1)
        if (portNum == 0 || portNum == 1) {
            return portNum;
        }
    }
    
    errorHandler->logError(ERROR, "Invalid I2C port string: " + portStr + 
                          " (valid: I2C0, I2C1)");
    return -1; // Invalid format
}

String ConfigManager::portNumberToI2CString(int portNum) {
    return "I2C" + String(portNum);
}

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
        errorHandler->logError(INFO, "Preventing recursive notification of config changes");
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
    
    const uint32_t DEFAULT_POLLING_RATE = Constants::System::DEFAULT_POLLING_RATE_MS;
    const uint32_t MIN_POLLING_RATE = Constants::System::MIN_POLLING_RATE_MS;
    const uint32_t MAX_POLLING_RATE = Constants::System::MAX_POLLING_RATE_MS;
    
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
    
    // Extract Environment Monitor IDentifier - handle both old and new key names for backward compatibility
    if (doc["Environment Monitor ID"].is<String>()) {
        boardId = doc["Environment Monitor ID"].as<String>();
        errorHandler->logError(INFO, "Using Environment Monitor ID: " + boardId);
    } else if (doc["Board ID"].is<String>()) {
        boardId = doc["Board ID"].as<String>();
        errorHandler->logError(INFO, "Using Board ID: " + boardId);
    } else {                                                            // No ID found, generate one from MAC address
        boardId = "GPower EM-" + String(ESP.getEfuseMac(), HEX);
        errorHandler->logError(INFO, "No ID found, using default: " + boardId);
    }
    
    // Clear existing configurations
    sensorConfigs.clear();
    additionalConfig = "";
    errorHandler->logError(INFO, "Cleared existing configurations");
    
    JsonArray i2cPeripherals = doc["I2C Peripherals"].as<JsonArray>();
    errorHandler->logError(INFO, "I2C Peripherals array exists: " + String(i2cPeripherals.size() > 0 ? "YES" : "NO"));
    
    if (i2cPeripherals.size() > 0) {
        errorHandler->logError(INFO, "Found " + String(i2cPeripherals.size()) + " I2C peripherals");
        for (JsonObject peripheral : i2cPeripherals) {
            if (!peripheral["Peripheral Name"].is<String>() || !peripheral["Peripheral Type"].is<String>() || 
                !peripheral["Address (HEX)"].is<int>()) {
                errorHandler->logError(WARNING, "Skipping I2C peripheral with missing required fields");
                continue;
            }
            
            SensorConfig config;
            config.name = peripheral["Peripheral Name"].as<String>();
            config.type = peripheral["Peripheral Type"].as<String>();
            config.communicationType = CommunicationType::I2C; // Set communication type to I2C
            config.address = peripheral["Address (HEX)"].as<int>();
            
            errorHandler->logError(INFO, "Found I2C peripheral: " + config.name + " of type " + config.type);
            
            if (peripheral["I2C Port"].is<String>()) {
                String portStr = peripheral["I2C Port"].as<String>();
                config.portNum = i2cPortStringToNumber(portStr);
                errorHandler->logError(INFO, "Peripheral using I2C port " + portStr + " (portNum: " + String(config.portNum) + ")");
            } else {
                // Default to port 0 if not specified
                config.portNum = 0;
                errorHandler->logError(INFO, "Peripheral defaulting to I2C0 (no port specified)");
            }
            
            // Read and validate polling rate
            if (peripheral["Polling Rate[1000 ms]"].is<uint32_t>()) {
                uint32_t rate = peripheral["Polling Rate[1000 ms]"].as<uint32_t>();
                // Apply validation rules
                if (rate < MIN_POLLING_RATE) {
                    errorHandler->logError(WARNING, "Polling rate too low for peripheral " + config.name + 
                                             " (" + String(rate) + "ms), using " + 
                                             String(MIN_POLLING_RATE) + "ms minimum");
                    config.pollingRate = MIN_POLLING_RATE;
                } else if (rate > MAX_POLLING_RATE) {
                    errorHandler->logError(WARNING, "Polling rate too high for peripheral " + config.name + 
                                             " (" + String(rate) + "ms), using " + 
                                             String(MAX_POLLING_RATE) + "ms maximum");
                    config.pollingRate = MAX_POLLING_RATE;
                } else {
                    config.pollingRate = rate;
                }
                errorHandler->logError(INFO, "Peripheral polling rate: " + String(config.pollingRate) + "ms");
            } else {
                config.pollingRate = DEFAULT_POLLING_RATE;
                errorHandler->logError(INFO, "Using default polling rate: " + String(DEFAULT_POLLING_RATE) + "ms");
            }
            
            if (peripheral["Additional"].is<String>()) {
                config.additional = peripheral["Additional"].as<String>();
                errorHandler->logError(INFO, "Additional settings: " + config.additional);
            } else {
                config.additional = ""; // Empty string for no additional settings
            }
            
            sensorConfigs.push_back(config);
            errorHandler->logError(INFO, "Added I2C peripheral: " + config.name);
        }
    }
    
    // Load SPI peripherals - using the "Peripheral" naming in JSON but "sensor" internally
    JsonArray spiPeripherals = doc["SPI Peripherals"].as<JsonArray>();
    errorHandler->logError(INFO, "SPI Peripherals array exists: " + String(spiPeripherals.size() > 0 ? "YES" : "NO"));
    
    if (spiPeripherals.size() > 0) {
        errorHandler->logError(INFO, "Found " + String(spiPeripherals.size()) + " SPI peripherals");
        for (JsonObject peripheral : spiPeripherals) {
            if (!peripheral["Peripheral Name"].is<String>() || !peripheral["Peripheral Type"].is<String>() || 
                !peripheral["SS Pin"].is<int>()) {
                errorHandler->logError(WARNING, "Skipping SPI peripheral with missing required fields");
                continue;
            }
            
            SensorConfig config;
            config.name = peripheral["Peripheral Name"].as<String>();
            config.type = peripheral["Peripheral Type"].as<String>();
            config.communicationType = CommunicationType::SPI; // Set communication type to SPI
            config.address = peripheral["SS Pin"].as<int>();
            config.portNum = 0; // Default to SPI port 0 for now
            
            errorHandler->logError(INFO, "Found SPI peripheral: " + config.name + " of type " + config.type);
            
            // Read and validate polling rate
            if (peripheral["Polling Rate[1000 ms]"].is<uint32_t>()) {
                uint32_t rate = peripheral["Polling Rate[1000 ms]"].as<uint32_t>();
                // Apply validation rules
                if (rate < MIN_POLLING_RATE) {
                    errorHandler->logError(WARNING, "Polling rate too low for peripheral " + config.name + 
                                             " (" + String(rate) + "ms), using " + 
                                             String(MIN_POLLING_RATE) + "ms minimum");
                    config.pollingRate = MIN_POLLING_RATE;
                } else if (rate > MAX_POLLING_RATE) {
                    errorHandler->logError(WARNING, "Polling rate too high for peripheral " + config.name + 
                                             " (" + String(rate) + "ms), using " + 
                                             String(MAX_POLLING_RATE) + "ms maximum");
                    config.pollingRate = MAX_POLLING_RATE;
                } else {
                    config.pollingRate = rate;
                }
                errorHandler->logError(INFO, "Peripheral polling rate: " + String(config.pollingRate) + "ms");
            } else {
                config.pollingRate = DEFAULT_POLLING_RATE;
                errorHandler->logError(INFO, "Using default polling rate: " + String(DEFAULT_POLLING_RATE) + "ms");
            }
            
            // Read additional settings
            if (peripheral["Additional"].is<String>()) {
                config.additional = peripheral["Additional"].as<String>();
                errorHandler->logError(INFO, "Additional settings: " + config.additional);
            } else {
                config.additional = "";
            }
            
            sensorConfigs.push_back(config);
            errorHandler->logError(INFO, "Added SPI peripheral: " + config.name);
        }
    }
    
    // Load Additional configuration if present
    if (doc["Additional"].is<String>()) {
        additionalConfig = doc["Additional"].as<String>();
        errorHandler->logError(INFO, "Loaded additional configuration: " + additionalConfig);
    } else {
        additionalConfig = "";
    }   
    
    errorHandler->logError(INFO, "Configuration loaded successfully with " + String(sensorConfigs.size()) + " peripherals");
    return true;
}

bool ConfigManager::updateConfigFromJson(const String& jsonConfig) {
    // Log a shorter version of the config to avoid huge logs
    errorHandler->logError(INFO, "Received config update: " + 
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
    
    // Backup current state in case we need to rollback
    String originalBoardId = boardId;
    std::vector<SensorConfig> originalSensorConfigs = sensorConfigs;
    String originalAdditionalConfig = additionalConfig;
    
    bool allUpdatesSuccessful = true;
    
    // Update board identifier if present (reuse existing function)
    if (doc["Environment Monitor ID"].is<String>() || doc["Board ID"].is<String>()) {
        String newBoardId;
        if (doc["Environment Monitor ID"].is<String>()) {
            newBoardId = doc["Environment Monitor ID"].as<String>();
        } else {
            newBoardId = doc["Board ID"].as<String>();
        }
        
        if (!setBoardIdentifier(newBoardId)) {
            errorHandler->logError(ERROR, "Failed to update board identifier");
            allUpdatesSuccessful = false;
        }
    }
    
    // Update sensor configuration if present (reuse existing function)
    if (allUpdatesSuccessful && (doc["I2C Peripherals"].is<JsonArray>() || doc["SPI Peripherals"].is<JsonArray>())) {
        // Create a JSON string with just the sensor config parts
        JsonDocument sensorDoc;
        if (doc["I2C Peripherals"].is<JsonArray>()) {
            sensorDoc["I2C Peripherals"] = doc["I2C Peripherals"];
        }
        if (doc["SPI Peripherals"].is<JsonArray>()) {
            sensorDoc["SPI Peripherals"] = doc["SPI Peripherals"];
        }
        
        String sensorConfigJson;
        serializeJson(sensorDoc, sensorConfigJson);
        
        if (!updateSensorConfigFromJson(sensorConfigJson)) {
            errorHandler->logError(ERROR, "Failed to update sensor configuration");
            allUpdatesSuccessful = false;
        }
    }
    
    // Update additional configuration if present (reuse existing function)
    if (allUpdatesSuccessful && doc["Additional"]) {
        JsonDocument additionalDoc;
        additionalDoc["Additional"] = doc["Additional"];
        
        String additionalConfigJson;
        serializeJson(additionalDoc, additionalConfigJson);
        
        if (!updateAdditionalConfigFromJson(additionalConfigJson)) {
            errorHandler->logError(ERROR, "Failed to update additional configuration");
            allUpdatesSuccessful = false;
        }
    }
    
    // If any update failed, rollback all changes
    if (!allUpdatesSuccessful) {
        errorHandler->logError(ERROR, "Configuration update failed, rolling back changes");
        
        // Disable notifications during rollback to prevent cascading updates
        disableNotifications(true);
        
        // Rollback board identifier
        boardId = originalBoardId;
        setBoardIdentifier(originalBoardId);
        
        // Rollback sensor configs
        updateSensorConfigs(originalSensorConfigs);
        
        // Rollback additional config
        additionalConfig = originalAdditionalConfig;
        
        // Re-enable notifications
        disableNotifications(false);
        
        return false;
    }
    
    errorHandler->logError(INFO, "Complete configuration update successful");
    
    // Notify about the configuration change (this will be the final combined config)
    String finalConfig = getConfigJson();
    notifyConfigChanged(finalConfig);
    
    return true;
}

bool ConfigManager::createDefaultConfig() {
    const uint32_t DEFAULT_POLLING_RATE = Constants::System::DEFAULT_POLLING_RATE_MS;  // Default: 1 second
    
    JsonDocument doc;
    
    // Create default configuration
    doc["Environment Monitor ID"] = "GPower EM-" + String(ESP.getEfuseMac(), HEX);
    
    // Add I2C peripherals array and first peripheral
    JsonArray i2cPeripherals = doc["I2C Peripherals"].to<JsonArray>();
    JsonObject i2cPeripheral = i2cPeripherals.add<JsonObject>();
    i2cPeripheral["Peripheral Name"] = "I2C01";
    i2cPeripheral["Peripheral Type"] = "SHT41";
    i2cPeripheral["I2C Port"] = "I2C0";
    i2cPeripheral["Address (HEX)"] = 0x44;  // SHT41 default address
    i2cPeripheral["Polling Rate[1000 ms]"] = DEFAULT_POLLING_RATE;
    i2cPeripheral["Additional"] = "";  // No additional settings by default
    
    doc["SPI Peripherals"] = JsonArray(); // Empty SPI peripherals by default
    doc["Additional"] = "";  // Empty "Additional" by default   
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
    // Update Environment Monitor ID in memory
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
        errorHandler->logError(ERROR, "Failed to parse config when updating Environment Monitor ID");
        return false;
    }
    
    doc["Environment Monitor ID"] = boardId;
        
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
    errorHandler->logError(INFO, "Updated Environment Monitor ID to: " + boardId);
    
    // Notify about the configuration change
    String configJson;
    serializeJson(doc, configJson);
    notifyConfigChanged(configJson);
    
    return true;
}

std::vector<SensorConfig> ConfigManager::getSensorConfigs() {
    return sensorConfigs;
}

// Sensors are called peripherals in the JSON file
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
        errorHandler->logError(ERROR, "Failed to parse config when updating peripherals");
        return false;
    }
    
    // Clear existing peripherals
    doc["I2C Peripherals"] = JsonArray();
    doc["SPI Peripherals"] = JsonArray();
    
    // Add updated peripherals
    JsonArray i2cPeripherals = doc["I2C Peripherals"].to<JsonArray>();
    JsonArray spiPeripherals = doc["SPI Peripherals"].to<JsonArray>();
    
    for (const auto& config : configs) {
        if (config.communicationType == CommunicationType::SPI) {
            JsonObject peripheral = spiPeripherals.add<JsonObject>();
            peripheral["Peripheral Name"] = config.name;
            peripheral["Peripheral Type"] = config.type;
            peripheral["SS Pin"] = config.address;
            peripheral["Polling Rate[1000 ms]"] = config.pollingRate;
            peripheral["Additional"] = config.additional;  // Always include
        } else { // I2C
            JsonObject peripheral = i2cPeripherals.add<JsonObject>();
            peripheral["Peripheral Name"] = config.name;
            peripheral["Peripheral Type"] = config.type;
            peripheral["I2C Port"] = portNumberToI2CString(config.portNum);
            peripheral["Address (HEX)"] = config.address;
            peripheral["Polling Rate[1000 ms]"] = config.pollingRate;
            peripheral["Additional"] = config.additional;  // Always include
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
    errorHandler->logError(INFO, "Updated peripheral configurations");
    
    // Notify about the configuration change, but only if not already in a notification
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

bool ConfigManager::updateSensorConfigFromJson(const String& jsonConfig) {
    // Handle empty input - erase peripherals with warning
    if (jsonConfig.length() == 0 || jsonConfig == "{}" || jsonConfig == "null") {
        errorHandler->logError(WARNING, "Empty peripheral configuration received - clearing all peripherals");
        sensorConfigs.clear();
        
        // Update the configuration file (reuse existing function)
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
        errorHandler->logError(ERROR, "Failed to parse peripheral configuration JSON: " + 
                              (error ? String(error.c_str()) : "Document too large"));
        return false;
    }
    
    // Temporary storage for new configurations
    std::vector<SensorConfig> newSensorConfigs;
    bool allValid = true;
    
    // Extract and validate I2C peripherals if present
    if (doc["I2C Peripherals"].is<JsonArray>()) {
        JsonArray i2cPeripherals = doc["I2C Peripherals"].as<JsonArray>();
        
        for (JsonObject peripheral : i2cPeripherals) {
            if (!peripheral["Peripheral Name"].is<String>() || !peripheral["Peripheral Type"].is<String>() || 
                !peripheral["Address (HEX)"].is<int>()) {
                errorHandler->logError(ERROR, "Missing required fields in I2C peripheral configuration");
                allValid = false;
                break;
            }
            
            SensorConfig config;
            config.name = peripheral["Peripheral Name"].as<String>();
            config.type = peripheral["Peripheral Type"].as<String>();
            config.address = peripheral["Address (HEX)"].as<int>();
            config.communicationType = CommunicationType::I2C;
            
            // Handle I2C port with validation
            if (peripheral["I2C Port"].is<String>()) {
                String portStr = peripheral["I2C Port"].as<String>();
                int portNum = i2cPortStringToNumber(portStr);
                if (portNum == -1) {
                    errorHandler->logError(ERROR, "Invalid I2C port for peripheral " + config.name + ": " + portStr);
                    allValid = false;
                    break;
                }
                config.portNum = portNum;
            } else {
                config.portNum = 0; // Default to port 0
            }
            
            // Handle polling rate (reuse existing validation logic)
            config.pollingRate = peripheral["Polling Rate[1000 ms]"].is<uint32_t>() ? 
                peripheral["Polling Rate[1000 ms]"].as<uint32_t>() : Constants::System::DEFAULT_POLLING_RATE_MS;
            config.pollingRate = constrain(config.pollingRate, 
                                         Constants::System::MIN_POLLING_RATE_MS, 
                                         Constants::System::MAX_POLLING_RATE_MS);
            
            config.additional = peripheral["Additional"].is<String>() ? 
                peripheral["Additional"].as<String>() : "";
            
            // Validate configuration
            String errorMessage;
            if (!validateSensorConfig(config, errorMessage)) {
                errorHandler->logError(ERROR, "Invalid I2C peripheral configuration for " + 
                                     config.name + ": " + errorMessage);
                allValid = false;
                break;
            }
            
            newSensorConfigs.push_back(config);
        }
    }
    
    // Extract and validate SPI peripherals if present (only if I2C validation passed)
    if (allValid && doc["SPI Peripherals"].is<JsonArray>()) {
        JsonArray spiPeripherals = doc["SPI Peripherals"].as<JsonArray>();
        
        for (JsonObject peripheral : spiPeripherals) {
            if (!peripheral["Peripheral Name"].is<String>() || !peripheral["Peripheral Type"].is<String>() || 
                !peripheral["SS Pin"].is<int>()) {
                errorHandler->logError(ERROR, "Missing required fields in SPI peripheral configuration");
                allValid = false;
                break;
            }
            
            SensorConfig config;
            config.name = peripheral["Peripheral Name"].as<String>();
            config.type = peripheral["Peripheral Type"].as<String>();
            config.address = peripheral["SS Pin"].as<int>();
            config.communicationType = CommunicationType::SPI;
            config.portNum = 0; // Default to port 0 for SPI
            
            // Handle polling rate (reuse existing validation logic)
            config.pollingRate = peripheral["Polling Rate[1000 ms]"].is<uint32_t>() ? 
                peripheral["Polling Rate[1000 ms]"].as<uint32_t>() : Constants::System::DEFAULT_POLLING_RATE_MS;
            config.pollingRate = constrain(config.pollingRate, 
                                         Constants::System::MIN_POLLING_RATE_MS, 
                                         Constants::System::MAX_POLLING_RATE_MS);
            
            config.additional = peripheral["Additional"].is<String>() ? 
                peripheral["Additional"].as<String>() : "";
            
            // Validate configuration
            String errorMessage;
            if (!validateSensorConfig(config, errorMessage)) {
                errorHandler->logError(ERROR, "Invalid SPI peripheral configuration for " + 
                                     config.name + ": " + errorMessage);
                allValid = false;
                break;
            }
            
            newSensorConfigs.push_back(config);
        }
    }
    
    // Only apply configuration if all peripherals are valid
    if (!allValid) {
        errorHandler->logError(ERROR, "Configuration rejected due to validation errors - no changes applied");
        return false;
    }
    
    // Check for duplicate sensor names
    for (size_t i = 0; i < newSensorConfigs.size(); i++) {
        for (size_t j = i + 1; j < newSensorConfigs.size(); j++) {
            if (newSensorConfigs[i].name == newSensorConfigs[j].name) {
                errorHandler->logError(ERROR, "Duplicate sensor name found: " + newSensorConfigs[i].name + 
                                     " - configuration rejected");
                return false;
            }
        }
    }
    
    // All validation passed - apply the configuration
    errorHandler->logError(INFO, "Peripheral configuration validation passed with " + 
                         String(newSensorConfigs.size()) + " peripherals");
    
    // Update using existing function (this handles file I/O and notifications)
    return updateSensorConfigs(newSensorConfigs);
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

bool ConfigManager::validateSensorConfig(const SensorConfig& config, String& errorMessage) {
    // Validate sensor name
    if (config.name.length() == 0) {
        errorMessage = "Sensor name cannot be empty";
        return false;
    }
    
    // Validate sensor type
    if (config.type.length() == 0) {
        errorMessage = "Sensor type cannot be empty";
        return false;
    }
    
    // Check if sensor type is supported (reuse existing function)
    SensorType sensorType = sensorTypeFromString(config.type);
    if (sensorType == SensorType::UNKNOWN) {
        errorMessage = "Unsupported sensor type: " + config.type;
        return false;
    }
    
    // Validate communication type specific settings
    if (config.communicationType == CommunicationType::I2C) {
        // Validate I2C port number (valid ports: 0, 1, or 100+)
        if (config.portNum < 0 || (config.portNum > 1 && config.portNum < 100)) {
            errorMessage = "Invalid I2C port number: " + String(config.portNum) + 
                         " (valid: 0, 1, or 100+)";
            return false;
        }
        
        // Validate I2C address range (standard 7-bit addresses: 0x08-0x77)
        if (config.address < 0x08 || config.address > 0x77) {
            errorMessage = "Invalid I2C address: 0x" + String(config.address, HEX) + 
                         " (valid range: 0x08-0x77)";
            return false;
        }
    } else if (config.communicationType == CommunicationType::SPI) {
        // Validate SPI SS pin (reuse existing Constants)
        size_t numSsPins = sizeof(Constants::Pins::SPI::SS_PINS) / sizeof(int);
        if (config.address < 0 || config.address >= (int)numSsPins) {
            errorMessage = "Invalid SPI SS pin: " + String(config.address) + 
                         " (valid range: 0-" + String(numSsPins - 1) + ")";
            return false;
        }
    }
    
    // Validate polling rate (reuse existing constants)
    if (config.pollingRate < Constants::System::MIN_POLLING_RATE_MS || 
        config.pollingRate > Constants::System::MAX_POLLING_RATE_MS) {
        errorMessage = "Invalid polling rate: " + String(config.pollingRate) + 
                     "ms (valid range: " + String(Constants::System::MIN_POLLING_RATE_MS) + 
                     "-" + String(Constants::System::MAX_POLLING_RATE_MS) + "ms)";
        return false;
    }
    
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
#include "SensorManager.h"

SensorManager::SensorManager(ConfigManager* configMgr, I2CManager* i2c, ErrorHandler* err, SPIManager* spi)
        : registry(err),
        factory(err, i2c, spi),
        configManager(configMgr),
        i2cManager(i2c),
        spiManager(spi),
        errorHandler(err),
        maxCacheAge(5000) {  // Default 5-second cache age
        readingCacheA.clear();
        readingCacheB.clear();
}

SensorManager::~SensorManager() {
    // Clean up all sensor instances
    auto sensors = registry.clear();
    for (auto sensor : sensors) {
        delete sensor;
    }
}

bool SensorManager::initializeSensors() {
    // Initialize I2C buses if not already initialized
    if (!i2cManager->isPortInitialized(I2CPort::I2C0)) {
        if (!i2cManager->beginPort(I2CPort::I2C0)) {
            errorHandler->logError(ERROR, "Failed to initialize I2C0");
        } else {
            errorHandler->logError(INFO, "Initialized I2C0 bus");
        }
    }
    
    if (!i2cManager->isPortInitialized(I2CPort::I2C1)) {
        if (!i2cManager->beginPort(I2CPort::I2C1)) {
            errorHandler->logError(ERROR, "Failed to initialize I2C1");
        } else {
            errorHandler->logError(INFO, "Initialized I2C1 bus");
        }
    }
    
    // Initialize SPI if available
    if (spiManager && !spiManager->isInitialized()) {
        if (!spiManager->begin()) {
            errorHandler->logError(ERROR, "Failed to initialize SPI");
        } else {
            errorHandler->logError(INFO, "Initialized SPI bus");
        }
    }
    
    // Scan I2C buses for devices
    std::vector<int> foundAddressesI2C0;
    std::vector<int> foundAddressesI2C1;
    
    errorHandler->logError(INFO, "Scanning I2C0 bus for devices...");
    bool i2c0HasDevices = i2cManager->scanBus(I2CPort::I2C0, foundAddressesI2C0);
    
    errorHandler->logError(INFO, "Scanning I2C1 bus for devices...");
    bool i2c1HasDevices = i2cManager->scanBus(I2CPort::I2C1, foundAddressesI2C1);

    if (!i2c0HasDevices && !i2c1HasDevices) {
        errorHandler->logError(WARNING, "No I2C devices found on any bus - check wiring if using I2C sensors!");
    }
    
    // Get sensor configurations
    auto sensorConfigs = configManager->getSensorConfigs();
    bool atLeastOneInitialized = false;
    
    // Clear any existing sensors and update info
    auto existingSensors = registry.clear();
    for (auto sensor : existingSensors) {
        delete sensor;
    }
    
    // Create and initialize sensors
    for (const auto& config : sensorConfigs) {
        // Test communication before initializing sensor
        if (config.isSPI) {
            if (!spiManager) {
                errorHandler->logError(ERROR, "SPI manager not available for sensor: " + config.name);
                continue;
            }
            testSPICommunication(config.address);
        } else {
            testI2CCommunication(config.i2cPort, config.address);
        }
        
        // Create the sensor
        ISensor* sensor = factory.createSensor(config);
        if (!sensor) {
            errorHandler->logError(ERROR, "Failed to create sensor: " + config.name);
            continue;
        }
        
        // Initialize the sensor
        if (!sensor->initialize()) {
            errorHandler->logError(ERROR, "Failed to initialize sensor: " + config.name);
            delete sensor;
            continue;
        }
        
        // Register the sensor
        registry.registerSensor(sensor);
        
        errorHandler->logError(INFO, "Sensor added to system: " + config.name + " with polling rate: " + 
                           String(config.pollingRate) + "ms");
        atLeastOneInitialized = true;
    }
    
    if (!atLeastOneInitialized) {
        errorHandler->logError(ERROR, "No sensors were initialized");
        return false;
    }
    
    return true;
}

bool SensorManager::reconfigureSensors(const String& configJson) {
    // Parse the new configuration
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, configJson);
    
    if (error || doc.overflowed()) {
        errorHandler->logError(ERROR, "Failed to parse sensor configuration JSON: " + 
                             String(error ? error.c_str() : "Memory overflow"));
        return false;
    }
    
    // Extract configurations from JSON
    std::vector<SensorConfig> oldConfigs = configManager->getSensorConfigs();
    std::vector<SensorConfig> newConfigs;
    
    // Extract I2C peripherals - UPDATED to use new naming scheme "Peripheral" in JSON
    if (doc["I2C Peripherals"].is<JsonArray>()) {
        JsonArray i2cPeripherals = doc["I2C Peripherals"].as<JsonArray>();
        for (JsonObject peripheral : i2cPeripherals) {
            SensorConfig config;
            
            // Required fields - UPDATED field names to match "Peripheral" naming in JSON
            config.name = peripheral["Peripheral Name"].as<String>();
            config.type = peripheral["Peripheral Type"].as<String>();
            config.address = peripheral["Address (HEX)"].as<int>();
            config.isSPI = false;
            
            // Optional fields with defaults
            config.i2cPort = I2CManager::stringToPort(
                peripheral["I2C Port"].isNull() ? "I2C0" : peripheral["I2C Port"].as<String>());
            config.pollingRate = peripheral["Polling Rate[1000 ms]"].isNull() ? 
                1000 : peripheral["Polling Rate[1000 ms]"].as<uint32_t>();
            config.additional = peripheral["Additional"].isNull() ? 
                "" : peripheral["Additional"].as<String>();
            
            // Apply polling rate limits
            config.pollingRate = constrain(config.pollingRate, 50, 300000);
            
            newConfigs.push_back(config);
        }
    }
    
    // Extract SPI peripherals - UPDATED to use new naming scheme "Peripheral" in JSON
    if (doc["SPI Peripherals"].is<JsonArray>()) {
        JsonArray spiPeripherals = doc["SPI Peripherals"].as<JsonArray>();
        for (JsonObject peripheral : spiPeripherals) {
            SensorConfig config;
            
            // Required fields - UPDATED field names to match "Peripheral" naming in JSON
            config.name = peripheral["Peripheral Name"].as<String>();
            config.type = peripheral["Peripheral Type"].as<String>();
            config.address = peripheral["SS Pin"].as<int>();
            config.isSPI = true;
            
            // Optional fields with defaults
            config.pollingRate = peripheral["Polling Rate[1000 ms]"].isNull() ? 
                1000 : peripheral["Polling Rate[1000 ms]"].as<uint32_t>();
            config.additional = peripheral["Additional"].isNull() ? 
                "" : peripheral["Additional"].as<String>();
            
            // Apply polling rate limits
            config.pollingRate = constrain(config.pollingRate, 50, 300000);
            
            newConfigs.push_back(config);
        }
    }
    
    // Determine which sensors to add and remove
    std::vector<SensorConfig> sensorsToAdd;
    std::vector<String> sensorsToRemove;
    compareConfigurations(oldConfigs, newConfigs, sensorsToAdd, sensorsToRemove);
    
    // Update the configuration in the ConfigManager
    configManager->disableNotifications(true);
    bool configUpdateSuccess = configManager->updateSensorConfigs(newConfigs);
    configManager->disableNotifications(false);
    
    if (!configUpdateSuccess) {
        errorHandler->logError(ERROR, "Failed to update sensor configuration");
        return false;
    }
    
    // Remove and add sensors as determined
    for (const auto& sensorName : sensorsToRemove) {
        ISensor* sensor = registry.unregisterSensor(sensorName);
        if (sensor) {
            errorHandler->logError(INFO, "Removing sensor: " + sensorName);
            delete sensor;
        }
    }
    
    bool allSuccess = true;
    for (const auto& config : sensorsToAdd) {
        errorHandler->logError(INFO, "Adding new sensor: " + config.name);
        
        // Create and initialize the sensor
        ISensor* sensor = factory.createSensor(config);
        if (!sensor || !sensor->initialize()) {
            if (sensor) delete sensor;
            errorHandler->logError(ERROR, "Failed to create/initialize sensor: " + config.name);
            allSuccess = false;
            continue;
        }
        
        // Register the successfully initialized sensor
        registry.registerSensor(sensor);
        errorHandler->logError(INFO, "Sensor added: " + config.name + 
                            " with polling rate: " + String(config.pollingRate) + "ms");
    }
    
    return allSuccess;
}

void SensorManager::compareConfigurations(
    const std::vector<SensorConfig>& oldConfigs,
    const std::vector<SensorConfig>& newConfigs,
    std::vector<SensorConfig>& toAdd,
    std::vector<String>& toRemove
) {
    // Clear output vectors
    toAdd.clear();
    toRemove.clear();
    
    // Find sensors to remove (in old but not in new)
    for (const auto& oldConfig : oldConfigs) {
        bool found = false;
        for (const auto& newConfig : newConfigs) {
            if (oldConfig.name == newConfig.name) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            toRemove.push_back(oldConfig.name);
        }
    }
    
    // Find sensors to add (in new but not in old)
    for (const auto& newConfig : newConfigs) {
        bool found = false;
        for (const auto& oldConfig : oldConfigs) {
            if (newConfig.name == oldConfig.name) {
                // Check if configuration changed
                if (newConfig.type != oldConfig.type ||
                    newConfig.address != oldConfig.address ||
                    newConfig.isSPI != oldConfig.isSPI ||
                    (!newConfig.isSPI && newConfig.i2cPort != oldConfig.i2cPort)) {
                    // Configuration changed, remove and re-add
                    toRemove.push_back(newConfig.name);
                    toAdd.push_back(newConfig);
                }
                found = true;
                break;
            }
        }
        
        if (!found) {
            toAdd.push_back(newConfig);
        }
    }
}

bool SensorManager::testI2CCommunication(I2CPort port, int address) {
    TwoWire* wire = i2cManager->getWire(port);
    if (!wire) {
        errorHandler->logError(ERROR, "Failed to get I2C bus for port " + I2CManager::portToString(port));
        return false;
    }
    
    wire->beginTransmission(address);
    byte error = wire->endTransmission();
    
    if (error == 0) {
        errorHandler->logError(INFO, "Direct I2C communication with address 0x" + String(address, HEX) + 
                          " on port " + I2CManager::portToString(port) + " successful");
        return true;
    } else {
        errorHandler->logError(ERROR, "Direct I2C communication with address 0x" + String(address, HEX) + 
                           " on port " + I2CManager::portToString(port) + 
                           " failed with error: " + String(error));
        return false;
    }
}

bool SensorManager::testSPICommunication(int ssPin) {
    if (!spiManager || !spiManager->isInitialized()) {
        errorHandler->logError(ERROR, "SPI not initialized for SS pin test: " + String(ssPin));
        return false;
    }
    
    bool success = spiManager->testDevice(ssPin);
    
    if (success) {
        errorHandler->logError(INFO, "SPI communication test successful on SS pin: " + String(ssPin));
    } else {
        errorHandler->logError(WARNING, "SPI communication test inconclusive on SS pin: " + String(ssPin) + 
                              " (may still work with specific device protocol)");
    }
    
    return true; // Return true even if the test is inconclusive, as it might still work with specific device
}

bool SensorManager::updateSensorCache(const String& sensorName) {
    ISensor* sensor = findSensor(sensorName);
    if (!sensor || !sensor->isConnected()) {
        return false;
    }
    
    // Get the active write buffer
    std::map<String, SensorCache>& activeCache = getActiveCache();
    SensorCache& cache = activeCache[sensorName];
    
    unsigned long currentTime = millis();
    bool updated = false;
    
    // Update temperature if supported
    if (sensor->supportsInterface(InterfaceType::TEMPERATURE)) {
        ITemperatureSensor* tempSensor = 
            static_cast<ITemperatureSensor*>(sensor->getInterface(InterfaceType::TEMPERATURE));
        
        float temp = tempSensor->readTemperature();
        cache.temperature = temp;
        cache.tempTimestamp = currentTime;
        cache.tempValid = !isnan(temp);
        
        updated = true;
    }
    
    // Update humidity if supported  
    if (sensor->supportsInterface(InterfaceType::HUMIDITY)) {
        IHumiditySensor* humSensor = 
            static_cast<IHumiditySensor*>(sensor->getInterface(InterfaceType::HUMIDITY));
        
        float hum = humSensor->readHumidity();
        cache.humidity = hum;
        cache.humTimestamp = currentTime;
        cache.humValid = !isnan(hum);
        
        updated = true;
    }
    
    return updated;
}

int SensorManager::updateReadings() {
    int successCount = 0;
    auto sensors = registry.getAllSensors();
    
    for (auto sensor : sensors) {
        if (sensor->isConnected()) {
            if (updateSensorCache(sensor->getName())) {
                successCount++;
            }
        }
    }
    
    // After updating all sensors, swap the buffers atomically
    currentBufferIndex.store(!currentBufferIndex.load());
    
    return successCount;
}

TemperatureReading SensorManager::getTemperatureSafe(const String& sensorName) {
    // Get the read buffer (not currently being written to)
    const std::map<String, SensorCache>& readCache = getReadCache();
    
    // Look for the sensor in the read cache
    auto it = readCache.find(sensorName);
    if (it != readCache.end() && it->second.tempValid) {
        return TemperatureReading(it->second.temperature, it->second.tempTimestamp);
    }
    
    // No valid reading available
    return TemperatureReading();
}

HumidityReading SensorManager::getHumiditySafe(const String& sensorName) {
    // Get the read buffer (not currently being written to)
    const std::map<String, SensorCache>& readCache = getReadCache();
    
    // Look for the sensor in the read cache
    auto it = readCache.find(sensorName);
    if (it != readCache.end() && it->second.humValid) {
        return HumidityReading(it->second.humidity, it->second.humTimestamp);
    }
    
    // No valid reading available
    return HumidityReading();
}

const SensorRegistry& SensorManager::getRegistry() const {
    return registry;
}

ISensor* SensorManager::findSensor(const String& name) {
    return registry.getSensorByName(name);
}

bool SensorManager::reconnectSensor(const String& sensorName) {
    ISensor* sensor = findSensor(sensorName);
    if (!sensor) {
        if (errorHandler) {
            errorHandler->logError(WARNING, "Cannot reconnect - sensor not found: " + sensorName);
        }
        return false;
    }
    
    if (sensor->isConnected()) {
        // Sensor is already connected
        return true;
    }
    
    if (errorHandler) {
        errorHandler->logError(INFO, "Attempting to reconnect sensor: " + sensorName);
    }
    
    // Special handling for Si7021 sensors based on type string rather than dynamic_cast
    if (sensor->getTypeString().indexOf("Si7021") >= 0) {
        // For Si7021 sensors, we need a special reconnection approach
        // Since we can't use dynamic_cast, we'll use a different approach
        if (errorHandler) {
            errorHandler->logError(INFO, "Using specialized reconnection for Si7021 sensor");
        }
        
        // First try regular initialize
        if (sensor->initialize()) {
            if (errorHandler) {
                errorHandler->logError(INFO, "Successfully reconnected Si7021 sensor via initialize: " + sensorName);
            }
            return true;
        }
        
        // If that didn't work, try a more aggressive approach with self-test
        if (sensor->performSelfTest()) {
            if (errorHandler) {
                errorHandler->logError(INFO, "Successfully reconnected Si7021 sensor via self-test: " + sensorName);
            }
            return true;
        }
        
        // Still failed - report and return
        if (errorHandler) {
            errorHandler->logError(ERROR, "Failed to reconnect Si7021 sensor: " + sensorName);
        }
        return false;
    }
    
    // Generic reconnection attempt for other sensors
    if (sensor->initialize()) {
        if (errorHandler) {
            errorHandler->logError(INFO, "Successfully reconnected sensor: " + sensorName);
        }
        return true;
    }
    
    // Try self-test if initialize didn't work
    if (sensor->performSelfTest()) {
        if (errorHandler) {
            errorHandler->logError(INFO, "Sensor reconnected via self-test: " + sensorName);
        }
        return true;
    }
    
    if (errorHandler) {
        errorHandler->logError(ERROR, "Failed to reconnect sensor: " + sensorName);
    }
    return false;
}

/**
 * @brief Attempt to reconnect all disconnected sensors
 * 
 * @return The number of successfully reconnected sensors
 */
int SensorManager::reconnectAllSensors() {
    int reconnectedCount = 0;
    
    auto registry = getRegistry();
    auto sensors = registry.getAllSensors();
    
    for (auto sensor : sensors) {
        if (!sensor->isConnected()) {
            if (reconnectSensor(sensor->getName())) {
                reconnectedCount++;
            }
        }
    }
    
    if (errorHandler && reconnectedCount > 0) {
        errorHandler->logError(INFO, "Reconnected " + String(reconnectedCount) + " sensors");
    }
    
    return reconnectedCount;
}
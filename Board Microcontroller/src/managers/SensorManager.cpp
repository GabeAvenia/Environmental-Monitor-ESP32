#include "SensorManager.h"

SensorManager::SensorManager(ConfigManager* configMgr, I2CManager* i2c, ErrorHandler* err, SPIManager* spi)
    : registry(err),
      factory(err, i2c, spi),
      configManager(configMgr),
      i2cManager(i2c),
      spiManager(spi),
      errorHandler(err) {
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
            errorHandler->logInfo("Initialized I2C0 bus");
        }
    }
    
    if (!i2cManager->isPortInitialized(I2CPort::I2C1)) {
        if (!i2cManager->beginPort(I2CPort::I2C1)) {
            errorHandler->logError(ERROR, "Failed to initialize I2C1");
        } else {
            errorHandler->logInfo("Initialized I2C1 bus");
        }
    }
    
    // Initialize SPI if available
    if (spiManager && !spiManager->isInitialized()) {
        if (!spiManager->begin()) {
            errorHandler->logError(ERROR, "Failed to initialize SPI");
        } else {
            errorHandler->logInfo("Initialized SPI bus");
        }
    }
    
    // Scan I2C buses for devices
    std::vector<int> foundAddressesI2C0;
    std::vector<int> foundAddressesI2C1;
    
    errorHandler->logInfo("Scanning I2C0 bus for devices...");
    bool i2c0HasDevices = i2cManager->scanBus(I2CPort::I2C0, foundAddressesI2C0);
    
    errorHandler->logInfo("Scanning I2C1 bus for devices...");
    bool i2c1HasDevices = i2cManager->scanBus(I2CPort::I2C1, foundAddressesI2C1);

    if (!i2c0HasDevices && !i2c1HasDevices) {
        errorHandler->logWarning("No I2C devices found on any bus - check wiring if using I2C sensors!");
    }
    
    // Get sensor configurations
    auto sensorConfigs = configManager->getSensorConfigs();
    bool atLeastOneInitialized = false;
    
    // Clear any existing sensors and update info
    auto existingSensors = registry.clear();
    for (auto sensor : existingSensors) {
        delete sensor;
    }
    sensorUpdateInfo.clear();
    
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
        
        // Set up polling info
        SensorUpdateInfo updateInfo(config.name, config.pollingRate);
        sensorUpdateInfo[config.name] = updateInfo;
        
        errorHandler->logInfo("Sensor added to system: " + config.name + " with polling rate: " + 
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
    
    if (error) {
        errorHandler->logError(ERROR, "Failed to parse sensor configuration JSON: " + String(error.c_str()));
        return false;
    }
    
    // Extract the old and new configurations
    std::vector<SensorConfig> oldConfigs = configManager->getSensorConfigs();
    std::vector<SensorConfig> newConfigs;
    
    // Parse I2C sensors
    JsonArray i2cSensors = doc["I2C Sensors"].as<JsonArray>();
    for (JsonObject sensor : i2cSensors) {
        SensorConfig config;
        config.name = sensor["Sensor Name"].as<String>();
        config.type = sensor["Sensor Type"].as<String>();
        config.address = sensor["Address (HEX)"].as<int>();
        config.isSPI = false;
        
        // Read I2C port
        if (!sensor["I2C Port"].isNull()) {
            String portStr = sensor["I2C Port"].as<String>();
            config.i2cPort = I2CManager::stringToPort(portStr);
        } else {
            config.i2cPort = I2CPort::I2C0; // Default
        }
        
        // Read polling rate with a default if not present
        config.pollingRate = 1000; // Default 1 second
        if (!sensor["Polling Rate[1000 ms]"].isNull()) {
            config.pollingRate = sensor["Polling Rate[1000 ms]"].as<uint32_t>();
        }
        
        newConfigs.push_back(config);
    }
    
    // Parse SPI sensors
    JsonArray spiSensors = doc["SPI Sensors"].as<JsonArray>();
    for (JsonObject sensor : spiSensors) {
        SensorConfig config;
        config.name = sensor["Sensor Name"].as<String>();
        config.type = sensor["Sensor Type"].as<String>();
        config.address = sensor["SS Pin"].as<int>();
        config.isSPI = true;
        
        // Read polling rate with a default if not present
        config.pollingRate = 1000; // Default 1 second
        if (!sensor["Polling Rate[1000 ms]"].isNull()) {
            config.pollingRate = sensor["Polling Rate[1000 ms]"].as<uint32_t>();
        }
        
        newConfigs.push_back(config);
    }
    
    // Determine which sensors to add and remove
    std::vector<SensorConfig> sensorsToAdd;
    std::vector<String> sensorsToRemove;
    compareConfigurations(oldConfigs, newConfigs, sensorsToAdd, sensorsToRemove);
    
    // Update the configuration in the ConfigManager
    configManager->updateSensorConfigs(newConfigs);
    
    // Remove sensors that are no longer in the configuration
    for (const auto& sensorName : sensorsToRemove) {
        ISensor* sensor = registry.unregisterSensor(sensorName);
        if (sensor) {
            errorHandler->logInfo("Removing sensor: " + sensorName);
            delete sensor;
            
            // Remove from update info
            sensorUpdateInfo.erase(sensorName);
        }
    }
    
    // Update polling rates for sensors that remain but may have changed
    for (const auto& config : newConfigs) {
        auto it = sensorUpdateInfo.find(config.name);
        if (it != sensorUpdateInfo.end() && it->second.pollingRateMs != config.pollingRate) {
            errorHandler->logInfo("Updating polling rate for sensor: " + config.name + 
                               " from " + String(it->second.pollingRateMs) + 
                               "ms to " + String(config.pollingRate) + "ms");
            it->second.pollingRateMs = config.pollingRate;
        }
    }
    
    // Add new sensors
    bool allSuccess = true;
    for (const auto& config : sensorsToAdd) {
        errorHandler->logInfo("Adding new sensor: " + config.name);
        
        // Test communication for the appropriate interface
        if (config.isSPI) {
            if (!spiManager) {
                errorHandler->logError(ERROR, "SPI manager not available for sensor: " + config.name);
                allSuccess = false;
                continue;
            }
            testSPICommunication(config.address);
        } else {
            testI2CCommunication(config.i2cPort, config.address);
        }
        
        // Create and initialize the sensor
        ISensor* sensor = factory.createSensor(config);
        if (!sensor) {
            errorHandler->logError(ERROR, "Failed to create sensor: " + config.name);
            allSuccess = false;
            continue;
        }
        
        if (!sensor->initialize()) {
            errorHandler->logError(ERROR, "Failed to initialize sensor: " + config.name);
            delete sensor;
            allSuccess = false;
            continue;
        }
        
        // Register the sensor
        registry.registerSensor(sensor);
        
        // Set up polling info
        SensorUpdateInfo updateInfo(config.name, config.pollingRate);
        sensorUpdateInfo[config.name] = updateInfo;
        
        errorHandler->logInfo("Sensor added to system: " + config.name + " with polling rate: " + 
                           String(config.pollingRate) + "ms");
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
        errorHandler->logInfo("Direct I2C communication with address 0x" + String(address, HEX) + 
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
        errorHandler->logInfo("SPI communication test successful on SS pin: " + String(ssPin));
    } else {
        errorHandler->logWarning("SPI communication test inconclusive on SS pin: " + String(ssPin) + 
                              " (may still work with specific device protocol)");
    }
    
    return true; // Return true even if the test is inconclusive, as it might still work with specific device
}

int SensorManager::updateReadings() {
    int successCount = 0;
    unsigned long currentTime = millis();
    
    // Use traditional loop instead of structured binding (C++17 feature)
    for (auto it = sensorUpdateInfo.begin(); it != sensorUpdateInfo.end(); ++it) {
        const String& sensorName = it->first;
        SensorUpdateInfo& updateInfo = it->second;
        
        // Check if it's time to update this sensor
        if (currentTime - updateInfo.lastUpdateTime >= updateInfo.pollingRateMs) {
            ISensor* sensor = findSensor(sensorName);
            if (sensor && sensor->isConnected()) {
                bool success = false;
                
                // Update readings based on sensor type
                if (sensor->supportsInterface(InterfaceType::TEMPERATURE)) {
                    ITemperatureSensor* tempSensor = 
                        static_cast<ITemperatureSensor*>(sensor->getInterface(InterfaceType::TEMPERATURE));
                    float temp = tempSensor->readTemperature();
                    success = !isnan(temp);
                }
                
                if (sensor->supportsInterface(InterfaceType::HUMIDITY)) {
                    IHumiditySensor* humSensor = 
                        static_cast<IHumiditySensor*>(sensor->getInterface(InterfaceType::HUMIDITY));
                    float hum = humSensor->readHumidity();
                    success = success || !isnan(hum);
                }
                
                if (success) {
                    successCount++;
                    updateInfo.lastUpdateTime = currentTime;
                }
            }
        }
    }
    
    return successCount;
}

TemperatureReading SensorManager::getTemperature(const String& sensorName) {
    ISensor* sensor = findSensor(sensorName);
    if (!sensor) {
        errorHandler->logWarning("Requested temperature from unknown sensor: " + sensorName);
        return TemperatureReading();
    }
    
    if (!sensor->supportsInterface(InterfaceType::TEMPERATURE)) {
        errorHandler->logWarning("Sensor does not support temperature: " + sensorName);
        return TemperatureReading();
    }
    
    if (!sensor->isConnected()) {
        errorHandler->logWarning("Attempted to read temperature from disconnected sensor: " + sensorName);
        return TemperatureReading();
    }
    
    ITemperatureSensor* tempSensor = 
        static_cast<ITemperatureSensor*>(sensor->getInterface(InterfaceType::TEMPERATURE));
    
    float value = tempSensor->readTemperature();
    return TemperatureReading(value, tempSensor->getTemperatureTimestamp());
}

HumidityReading SensorManager::getHumidity(const String& sensorName) {
    ISensor* sensor = findSensor(sensorName);
    if (!sensor) {
        errorHandler->logWarning("Requested humidity from unknown sensor: " + sensorName);
        return HumidityReading();
    }
    
    if (!sensor->supportsInterface(InterfaceType::HUMIDITY)) {
        errorHandler->logWarning("Sensor does not support humidity: " + sensorName);
        return HumidityReading();
    }
    
    if (!sensor->isConnected()) {
        errorHandler->logWarning("Attempted to read humidity from disconnected sensor: " + sensorName);
        return HumidityReading();
    }
    
    IHumiditySensor* humSensor = 
        static_cast<IHumiditySensor*>(sensor->getInterface(InterfaceType::HUMIDITY));
    
    float value = humSensor->readHumidity();
    return HumidityReading(value, humSensor->getHumidityTimestamp());
}

const SensorRegistry& SensorManager::getRegistry() const {
    return registry;
}

ISensor* SensorManager::findSensor(const String& name) {
    return registry.getSensorByName(name);
}
#include "SensorManager.h"

SensorManager::SensorManager(ConfigManager* configMgr, I2CManager* i2c, ErrorHandler* err)
    : registry(err),
      factory(err, i2c),
      configManager(configMgr),
      i2cManager(i2c),
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
    // Initialize I2C if not already initialized
    if (!i2cManager->isInitialized()) {
        if (!i2cManager->begin()) {
            errorHandler->logError(ERROR, "Failed to initialize I2C");
            return false;
        }
    }
    
    // Scan I2C bus for devices
    std::vector<int> foundAddresses;
    errorHandler->logInfo("Scanning I2C bus for devices...");
    i2cManager->scanBus(foundAddresses);

    if (foundAddresses.empty()) {
        errorHandler->logError(ERROR, "No I2C devices found on bus - check wiring!");
    } else {
        errorHandler->logInfo("Found " + String(foundAddresses.size()) + " I2C devices:");
        for (auto addr : foundAddresses) {
            errorHandler->logInfo("Device at address: 0x" + String(addr, HEX));
        }
    }
    
    // Get sensor configurations
    auto sensorConfigs = configManager->getSensorConfigs();
    bool atLeastOneInitialized = false;
    
    // Clear any existing sensors
    auto existingSensors = registry.clear();
    for (auto sensor : existingSensors) {
        delete sensor;
    }
    
    // Create and initialize sensors
    for (const auto& config : sensorConfigs) {
        // Skip SPI sensors if not implemented
        if (config.isSPI) {
            errorHandler->logWarning("SPI sensors not yet implemented, skipping: " + config.name);
            continue;
        }
        
        // Test direct I2C communication before initializing sensor
        if (!config.isSPI) {
            testI2CCommunication(config.address);
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
        errorHandler->logInfo("Sensor added to system: " + config.name);
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
        }
    }
    
    // Add new sensors
    bool allSuccess = true;
    for (const auto& config : sensorsToAdd) {
        errorHandler->logInfo("Adding new sensor: " + config.name);
        
        // Test I2C communication if this is an I2C sensor
        if (!config.isSPI) {
            testI2CCommunication(config.address);
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
                    newConfig.isSPI != oldConfig.isSPI) {
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

bool SensorManager::testI2CCommunication(int address) {
    TwoWire* wire = i2cManager->getWire();
    wire->beginTransmission(address);
    byte error = wire->endTransmission();
    
    if (error == 0) {
        errorHandler->logInfo("Direct I2C communication with address 0x" + String(address, HEX) + " successful");
        return true;
    } else {
        errorHandler->logError(ERROR, "Direct I2C communication with address 0x" + String(address, HEX) + 
                           " failed with error: " + String(error));
        return false;
    }
}

int SensorManager::updateReadings() {
    int successCount = 0;
    auto sensors = registry.getAllSensors();
    
    for (auto sensor : sensors) {
        if (sensor->isConnected()) {
            // Update readings based on sensor type
            bool success = false;
            
            if (auto tempSensor = dynamic_cast<ITemperatureSensor*>(sensor)) {
                float temp = tempSensor->readTemperature();
                success = !isnan(temp);
            }
            
            if (auto humSensor = dynamic_cast<IHumiditySensor*>(sensor)) {
                float hum = humSensor->readHumidity();
                success = success || !isnan(hum);
            }
            
            if (success) {
                successCount++;
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
    
    ITemperatureSensor* tempSensor = dynamic_cast<ITemperatureSensor*>(sensor);
    if (!tempSensor) {
        errorHandler->logWarning("Sensor does not support temperature: " + sensorName);
        return TemperatureReading();
    }
    
    if (!sensor->isConnected()) {
        errorHandler->logWarning("Attempted to read temperature from disconnected sensor: " + sensorName);
        return TemperatureReading();
    }
    
    float value = tempSensor->readTemperature();
    return TemperatureReading(value, tempSensor->getTemperatureTimestamp());
}

HumidityReading SensorManager::getHumidity(const String& sensorName) {
    ISensor* sensor = findSensor(sensorName);
    if (!sensor) {
        errorHandler->logWarning("Requested humidity from unknown sensor: " + sensorName);
        return HumidityReading();
    }
    
    IHumiditySensor* humSensor = dynamic_cast<IHumiditySensor*>(sensor);
    if (!humSensor) {
        errorHandler->logWarning("Sensor does not support humidity: " + sensorName);
        return HumidityReading();
    }
    
    if (!sensor->isConnected()) {
        errorHandler->logWarning("Attempted to read humidity from disconnected sensor: " + sensorName);
        return HumidityReading();
    }
    
    float value = humSensor->readHumidity();
    return HumidityReading(value, humSensor->getHumidityTimestamp());
}

const SensorRegistry& SensorManager::getRegistry() const {
    return registry;
}

ISensor* SensorManager::findSensor(const String& name) {
    return registry.getSensorByName(name);
}

#include "SensorManager.h"
#include <vector>
#include "../sensors/Sensor.h"
#include "../sensors/TemperatureHumiditySensor.h"
#include "../config/ConfigManager.h"
#include "../error/ErrorHandler.h"

SensorManager::SensorManager(ConfigManager* configMgr, ErrorHandler* err) 
    : i2cManager(err), configManager(configMgr), errorHandler(err) {
}

SensorManager::~SensorManager() {
    for (auto sensor : sensors) {
        delete sensor;
    }
    sensors.clear();
}

bool SensorManager::initializeSensors() {
    // Initialize I2C
    if (!i2cManager.begin()) {
        errorHandler->logError(ERROR, "Failed to initialize I2C");
        return false;
    }
    
    // Scan I2C bus for devices
    std::vector<int> foundAddresses;
    errorHandler->logInfo("Scanning I2C bus for devices...");
    i2cManager.scanBus(foundAddresses);

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
    bool allInitialized = true;
    
    for (const auto& config : sensorConfigs) {
        if (config.isSPI) {
            // SPI sensors not implemented in this iteration
            errorHandler->logWarning("SPI sensors not yet implemented, skipping: " + config.name);
            continue;
        }
        
        // Test direct I2C communication before initializing sensor
        testI2CCommunication(config.address);
        
        Sensor* sensor = createSensorFromConfig(config);
        if (sensor == nullptr) {
            continue;
        }
        
        if (!sensor->initialize()) {
            errorHandler->logError(ERROR, "Failed to initialize sensor: " + config.name);
            delete sensor;
            allInitialized = false;
            continue;
        }
        
        addSensor(sensor);
        errorHandler->logInfo("Sensor added to system: " + config.name);
    }
    
    if (sensors.empty()) {
        errorHandler->logError(ERROR, "No sensors were initialized");
        return false;
    }
    
    return allInitialized;
}

Sensor* SensorManager::createSensorFromConfig(const SensorConfig& config) {
    // For now, we only support SHT41
    if (config.type == "SHT41") {
        Sensor* sensor = new TemperatureHumiditySensor(config.name, config.address, errorHandler);
        return sensor;
    }
    
    errorHandler->logWarning("Unsupported sensor type: " + config.type);
    return nullptr;
}

bool SensorManager::testI2CCommunication(int address) {
    TwoWire* wire = i2cManager.getWire();
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

SensorReading SensorManager::getLatestReading(const String& sensorName) {
    for (auto sensor : sensors) {
        if (sensor->getName() == sensorName) {
            return sensor->getLatestReading();
        }
    }
    
    // Return empty reading if sensor not found
    errorHandler->logWarning("Requested reading for unknown sensor: " + sensorName);
    return SensorReading();
}

std::vector<SensorReading> SensorManager::getAllReadings() {
    std::vector<SensorReading> readings;
    for (auto sensor : sensors) {
        readings.push_back(sensor->getLatestReading());
    }
    return readings;
}

bool SensorManager::addSensor(Sensor* newSensor) {
    sensors.push_back(newSensor);
    return true;
}

bool SensorManager::removeSensor(const String& sensorName) {
    for (auto it = sensors.begin(); it != sensors.end(); ++it) {
        if ((*it)->getName() == sensorName) {
            delete *it;
            sensors.erase(it);
            return true;
        }
    }
    return false;
}

void SensorManager::updateReadings() {
    for (auto sensor : sensors) {
        sensor->readSensor();
    }
}

std::vector<Sensor*> SensorManager::getSensors() {
    return sensors;
}
#pragma once

#include <vector>
#include "../sensors/Sensor.h"
#include "../sensors/TemperatureHumiditySensor.h"
#include "I2CManager.h"
#include "../config/ConfigManager.h"
#include "../error/ErrorHandler.h"

class SensorManager {
private:
    std::vector<Sensor*> sensors;
    I2CManager i2cManager;
    ConfigManager* configManager;
    ErrorHandler* errorHandler;
    
    Sensor* createSensorFromConfig(const SensorConfig& config) {
        // For now, we only support SHT41
        if (config.type == "SHT41") {
            Sensor* sensor = new TemperatureHumiditySensor(config.name, config.address, errorHandler);
            return sensor;
        }
        
        errorHandler->logWarning("Unsupported sensor type: " + config.type);
        return nullptr;
    }
    
public:
    SensorManager(ConfigManager* configMgr, ErrorHandler* err) 
        : i2cManager(err), configManager(configMgr), errorHandler(err) {
    }
    
    ~SensorManager() {
        for (auto sensor : sensors) {
            delete sensor;
        }
        sensors.clear();
    }
    
    bool initializeSensors() {
        // Initialize I2C
        if (!i2cManager.begin()) {
            errorHandler->logError(ERROR, "Failed to initialize I2C");
            return false;
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
    
    SensorReading getLatestReading(const String& sensorName) {
        for (auto sensor : sensors) {
            if (sensor->getName() == sensorName) {
                return sensor->getLatestReading();
            }
        }
        
        // Return empty reading if sensor not found
        errorHandler->logWarning("Requested reading for unknown sensor: " + sensorName);
        return SensorReading();
    }
    
    std::vector<SensorReading> getAllReadings() {
        std::vector<SensorReading> readings;
        for (auto sensor : sensors) {
            readings.push_back(sensor->getLatestReading());
        }
        return readings;
    }
    
    bool addSensor(Sensor* newSensor) {
        sensors.push_back(newSensor);
        return true;
    }
    
    bool removeSensor(const String& sensorName) {
        for (auto it = sensors.begin(); it != sensors.end(); ++it) {
            if ((*it)->getName() == sensorName) {
                delete *it;
                sensors.erase(it);
                return true;
            }
        }
        return false;
    }
    
    void updateReadings() {
        for (auto sensor : sensors) {
            sensor->readSensor();
        }
    }
    
    std::vector<Sensor*> getSensors() {
        return sensors;
    }
};
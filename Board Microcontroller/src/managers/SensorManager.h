#pragma once

#include <vector>
#include "../sensors/Sensor.h"
#include "I2CManager.h"
#include "../config/ConfigManager.h"
#include "../error/ErrorHandler.h"

class SensorManager {
private:
    std::vector<Sensor*> sensors;
    I2CManager i2cManager;
    ConfigManager* configManager;
    ErrorHandler* errorHandler;
    
    Sensor* createSensorFromConfig(const SensorConfig& config);
    bool testI2CCommunication(int address);
    
public:
    SensorManager(ConfigManager* configMgr, ErrorHandler* err);
    ~SensorManager();
    
    bool initializeSensors();
    SensorReading getLatestReading(const String& sensorName);
    std::vector<SensorReading> getAllReadings();
    bool addSensor(Sensor* newSensor);
    bool removeSensor(const String& sensorName);
    void updateReadings();
    std::vector<Sensor*> getSensors();
};
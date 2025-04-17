#pragma once

#include <vector>
#include <map>
#include <atomic>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>

#include "../sensors/interfaces/ISensor.h"
#include "../sensors/readings/TemperatureReading.h"
#include "../sensors/readings/HumidityReading.h"
#include "../sensors/interfaces/ITemperatureSensor.h"
#include "../sensors/interfaces/IHumiditySensor.h"
#include "../sensors/SensorFactory.h"
#include "SensorRegistry.h"
#include "I2CManager.h"
#include "SPIManager.h"
#include "../config/ConfigManager.h"
#include "../error/ErrorHandler.h"

// Struct for caching sensor readings
struct SensorCache {
    float temperature = NAN;
    float humidity = NAN;
    unsigned long tempTimestamp = 0;
    unsigned long humTimestamp = 0;
    bool tempValid = false;
    bool humValid = false;
};

class SensorManager {
private:
    SensorRegistry registry;
    SensorFactory factory;
    ConfigManager* configManager;
    I2CManager* i2cManager;
    SPIManager* spiManager;
    ErrorHandler* errorHandler;
    
    // Double buffering for thread-safe readings
    std::atomic<bool> currentBufferIndex{false}; // false = buffer A, true = buffer B
    std::map<String, SensorCache> readingCacheA;
    std::map<String, SensorCache> readingCacheB;
    
    // Maximum age of cached readings in milliseconds
    unsigned long maxCacheAge = 5000;
    
    // Double buffer helpers
    std::map<String, SensorCache>& getActiveCache() {
        return currentBufferIndex.load() ? readingCacheB : readingCacheA;
    }
    
    const std::map<String, SensorCache>& getReadCache() const {
        return currentBufferIndex.load() ? readingCacheA : readingCacheB;
    }
    
    // Configuration comparison helper
    void compareConfigurations(
        const std::vector<SensorConfig>& oldConfigs,
        const std::vector<SensorConfig>& newConfigs,
        std::vector<SensorConfig>& toAdd,
        std::vector<String>& toRemove);
    
    // Communication testing helpers
    bool testI2CCommunication(I2CPort port, int address);
    bool testSPICommunication(int ssPin);
    
    // Update readings for a single sensor
    bool updateSensorCache(const String& sensorName);

public:
    // Constructor and destructor
    SensorManager(ConfigManager* configMgr, I2CManager* i2c, ErrorHandler* err, SPIManager* spi = nullptr);
    ~SensorManager();
    
    // Core functionality
    bool initializeSensors();
    bool reconfigureSensors(const String& configJson);
    int updateReadings();
    
    // Thread-safe reading methods
    TemperatureReading getTemperatureSafe(const String& sensorName);
    HumidityReading getHumiditySafe(const String& sensorName);
    
    // Cache configuration
    void setMaxCacheAge(unsigned long maxAgeMs) { maxCacheAge = maxAgeMs; }
    unsigned long getMaxCacheAge() const { return maxCacheAge; }
    
    // Accessors
    const SensorRegistry& getRegistry() const;
    ISensor* findSensor(const String& name);
    
    // Sensor recovery
    bool reconnectSensor(const String& sensorName);
    int reconnectAllSensors();
};
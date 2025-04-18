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

/**
 * @brief Structure for caching sensor readings
 */
struct SensorCache {
    float temperature = NAN;
    float humidity = NAN;
    unsigned long tempTimestamp = 0;
    unsigned long humTimestamp = 0;
    bool tempValid = false;
    bool humValid = false;
};

/**
 * @brief Manages sensor configuration, initialization, and readings
 * 
 * Provides thread-safe access to sensor readings using double buffering
 * and handles sensor lifecycle including reconnection
 */
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
    
    /**
     * @brief Get the active buffer being written to
     * 
     * @return Reference to the active cache buffer
     */
    std::map<String, SensorCache>& getActiveCache() {
        return currentBufferIndex.load() ? readingCacheB : readingCacheA;
    }
    
    /**
     * @brief Get the buffer for safe reading by other threads
     * 
     * @return Const reference to the read cache buffer
     */
    const std::map<String, SensorCache>& getReadCache() const {
        return currentBufferIndex.load() ? readingCacheA : readingCacheB;
    }
    
    /**
     * @brief Compare old and new sensor configurations
     * 
     * Determines which sensors need to be added or removed
     * 
     * @param oldConfigs Previous sensor configurations
     * @param newConfigs New sensor configurations
     * @param toAdd Output vector of sensors to add
     * @param toRemove Output vector of sensors to remove
     */
    void compareConfigurations(
        const std::vector<SensorConfig>& oldConfigs,
        const std::vector<SensorConfig>& newConfigs,
        std::vector<SensorConfig>& toAdd,
        std::vector<String>& toRemove);
    
    /**
     * @brief Test communication with an I2C device
     * 
     * @param port I2C port to use
     * @param address I2C address to test
     * @return true if communication successful
     */
    bool testI2CCommunication(I2CPort port, int address);
    
    /**
     * @brief Test communication with an SPI device
     * 
     * @param ssPin SPI slave select pin
     * @return true if communication successful
     */
    bool testSPICommunication(int ssPin);
    
    /**
     * @brief Update readings for a single sensor
     * 
     * @param sensorName Name of the sensor to update
     * @return true if update successful
     */
    bool updateSensorCache(const String& sensorName);

public:
    /**
     * @brief Constructor for SensorManager
     * 
     * @param configMgr Configuration manager
     * @param i2c I2C manager
     * @param err Error handler
     * @param spi Optional SPI manager
     */
    SensorManager(ConfigManager* configMgr, I2CManager* i2c, ErrorHandler* err, SPIManager* spi = nullptr);
    
    /**
     * @brief Destructor - cleans up all sensors
     */
    ~SensorManager();
    
    /**
     * @brief Initialize all sensors from configuration
     * 
     * @return true if at least one sensor initialized successfully
     */
    bool initializeSensors();
    
    /**
     * @brief Reconfigure sensors based on new configuration
     * 
     * @param configJson New configuration in JSON format
     * @return true if reconfiguration successful
     */
    bool reconfigureSensors(const String& configJson);
    
    /**
     * @brief Update readings from all sensors
     * 
     * @return Number of sensors successfully updated
     */
    int updateReadings();
    
    /**
     * @brief Get temperature reading in a thread-safe manner
     * 
     * @param sensorName Name of the sensor
     * @return Temperature reading with validity information
     */
    TemperatureReading getTemperatureSafe(const String& sensorName);
    
    /**
     * @brief Get humidity reading in a thread-safe manner
     * 
     * @param sensorName Name of the sensor
     * @return Humidity reading with validity information
     */
    HumidityReading getHumiditySafe(const String& sensorName);
    
    /**
     * @brief Set maximum age for cached readings
     * 
     * @param maxAgeMs Maximum age in milliseconds
     */
    void setMaxCacheAge(unsigned long maxAgeMs) { maxCacheAge = maxAgeMs; }
    
    /**
     * @brief Get current maximum cache age
     * 
     * @return Maximum age in milliseconds
     */
    unsigned long getMaxCacheAge() const { return maxCacheAge; }
    
    /**
     * @brief Get the sensor registry
     * 
     * @return Reference to the sensor registry
     */
    const SensorRegistry& getRegistry() const;
    
    /**
     * @brief Find a sensor by name
     * 
     * @param name Sensor name
     * @return Pointer to the sensor or nullptr if not found
     */
    ISensor* findSensor(const String& name);
    
    /**
     * @brief Attempt to reconnect a disconnected sensor
     * 
     * @param sensorName Name of the sensor
     * @return true if reconnection successful
     */
    bool reconnectSensor(const String& sensorName);
    
    /**
     * @brief Attempt to reconnect all disconnected sensors
     * 
     * @return Number of sensors successfully reconnected
     */
    int reconnectAllSensors();
};
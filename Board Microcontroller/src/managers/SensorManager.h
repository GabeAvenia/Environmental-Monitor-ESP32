#pragma once

#include <vector>
#include <map>
#include <memory>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

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

// Struct to cache sensor readings for better performance
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
    
    // Cache for sensor readings
    std::map<String, SensorCache> readingCache;
    
    // Maximum age of cached readings (in milliseconds)
    // Default to 5 seconds, can be adjusted based on requirements
    unsigned long maxCacheAge = 5000;
    
    // Mutex for thread safety (reference to the one created in main.cpp)
    SemaphoreHandle_t* sensorMutex = nullptr;
    
    /**
     * @brief Compare old and new configurations to identify sensors to add, remove, or reconfigure.
     * 
     * @param oldConfigs Vector of previous sensor configurations.
     * @param newConfigs Vector of new sensor configurations.
     * @param toAdd Output vector that will be filled with configurations to add.
     * @param toRemove Output vector that will be filled with sensor names to remove.
     */
    void compareConfigurations(
        const std::vector<SensorConfig>& oldConfigs,
        const std::vector<SensorConfig>& newConfigs,
        std::vector<SensorConfig>& toAdd,
        std::vector<String>& toRemove
    );
    
    /**
     * @brief Test I2C communication with a sensor at the specified address and port.
     * 
     * @param port The I2C port to use.
     * @param address The I2C address to test.
     * @return true if communication successful, false otherwise.
     */
    bool testI2CCommunication(I2CPort port, int address);
    
    /**
     * @brief Test SPI communication with a device on the specified SS pin.
     * 
     * @param ssPin The SPI slave select pin.
     * @return true if communication successful, false otherwise.
     */
    bool testSPICommunication(int ssPin);
    
    /**
     * @brief Update cached reading for a specific sensor if needed
     * 
     * @param sensorName The name of the sensor
     * @param forceUpdate Force an update even if the cache is still valid
     * @return true if cache was updated successfully
     */
    bool updateSensorCache(const String& sensorName, bool forceUpdate = false);

public:
    /**
     * @brief Constructor for SensorManager.
     * 
     * @param configMgr Pointer to the configuration manager.
     * @param i2c Pointer to the I2C manager.
     * @param err Pointer to the error handler.
     * @param spi Optional pointer to the SPI manager.
     */
    SensorManager(ConfigManager* configMgr, I2CManager* i2c, ErrorHandler* err, SPIManager* spi = nullptr);
    
    /**
     * @brief Destructor.
     */
    ~SensorManager();
    
    /**
     * @brief Initialize all sensors based on current configuration.
     * 
     * @return true if at least some sensors were initialized, false if all failed.
     */
    bool initializeSensors();
    
    /**
     * @brief Reconfigure sensors based on a new JSON configuration.
     * 
     * @param configJson The new sensor configuration as a JSON string.
     * @return true if reconfiguration was successful, false otherwise.
     */
    bool reconfigureSensors(const String& configJson);
    
    /**
     * @brief Update readings from all sensors.
     * 
     * @param forceUpdate Force updating all readings even if cache is still valid
     * @return Number of sensors successfully updated.
     */
    int updateReadings(bool forceUpdate = false);
    
    /**
     * @brief Get the latest temperature reading from a specific sensor.
     * Will use cached value if available and not too old.
     * 
     * @param sensorName The name of the sensor to read from.
     * @return The temperature reading.
     */
    TemperatureReading getTemperature(const String& sensorName);
    
    /**
     * @brief Get the latest humidity reading from a specific sensor.
     * Will use cached value if available and not too old.
     * 
     * @param sensorName The name of the sensor to read from.
     * @return The humidity reading.
     */
    HumidityReading getHumidity(const String& sensorName);
    
    /**
     * @brief Set the maximum age for cached readings
     * 
     * @param maxAgeMs Maximum age in milliseconds
     */
    void setMaxCacheAge(unsigned long maxAgeMs) {
        maxCacheAge = maxAgeMs;
    }
    
    /**
     * @brief Get the current maximum cache age
     * 
     * @return Maximum cache age in milliseconds
     */
    unsigned long getMaxCacheAge() const {
        return maxCacheAge;
    }
    
    /**
     * @brief Get the sensor registry.
     * 
     * @return Reference to the sensor registry.
     */
    const SensorRegistry& getRegistry() const;
    
    /**
     * @brief Find a sensor by name.
     * 
     * @param name The name of the sensor to find.
     * @return Pointer to the sensor, or nullptr if not found.
     */
    ISensor* findSensor(const String& name);

    /**
     * @brief Set the mutex for thread safety
     * 
     * @param mutex Pointer to the mutex
     */
    void setSensorMutex(SemaphoreHandle_t* mutex) {
        sensorMutex = mutex;
    }
    
    /**
     * @brief Thread-safe wrapper for getTemperature
     * 
     * @param sensorName The name of the sensor to read from
     * @return The temperature reading
     */
    TemperatureReading getTemperatureSafe(const String& sensorName) {
        TemperatureReading reading;
        
        // Try to take the mutex
        if (sensorMutex && *sensorMutex && xSemaphoreTake(*sensorMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            // Get the reading while holding the mutex
            reading = getTemperature(sensorName);
            
            // Release the mutex
            xSemaphoreGive(*sensorMutex);
        } else {
            // Mutex acquisition failed or not initialized
            if (sensorMutex && *sensorMutex) {
                errorHandler->logWarning("Failed to acquire mutex for temperature reading: " + sensorName);
            }
            // If mutex is null, just get the reading without mutex protection
            reading = getTemperature(sensorName);
        }
        
        return reading;
    }
    
    /**
     * @brief Thread-safe wrapper for getHumidity
     * 
     * @param sensorName The name of the sensor to read from
     * @return The humidity reading
     */
    HumidityReading getHumiditySafe(const String& sensorName) {
        HumidityReading reading;
        
        // Try to take the mutex
        if (sensorMutex && *sensorMutex && xSemaphoreTake(*sensorMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            // Get the reading while holding the mutex
            reading = getHumidity(sensorName);
            
            // Release the mutex
            xSemaphoreGive(*sensorMutex);
        } else {
            // Mutex acquisition failed or not initialized
            if (sensorMutex && *sensorMutex) {
                errorHandler->logWarning("Failed to acquire mutex for humidity reading: " + sensorName);
            }
            // If mutex is null, just get the reading without mutex protection
            reading = getHumidity(sensorName);
        }
        
        return reading;
    }
};
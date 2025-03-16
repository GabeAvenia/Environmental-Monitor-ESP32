#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "interfaces/ISensor.h"
#include "../config/ConfigManager.h"
#include "../error/ErrorHandler.h"
#include "../managers/I2CManager.h"
#include "SHT41Sensor.h"
#include "SensorTypes.h"

/**
 * @brief Factory class for creating sensors based on configuration.
 * 
 * This class is responsible for creating instances of different sensor types
 * based on configuration data.
 */
class SensorFactory {
private:
    ErrorHandler* errorHandler;
    I2CManager* i2cManager;
    
    /**
     * @brief Create an SHT41 sensor instance.
     * 
     * @param config The sensor configuration.
     * @return Pointer to the created sensor, or nullptr if creation failed.
     */
    ISensor* createSHT41Sensor(const SensorConfig& config) {
        if (config.isSPI) {
            errorHandler->logError(ERROR, "SHT41 does not support SPI interface");
            return nullptr;
        }
        
        // Create an SHT41 sensor with the specified configuration
        SHT41Sensor* sensor = new SHT41Sensor(
            config.name,
            config.address,
            i2cManager->getWire(),
            errorHandler
        );
        
        return sensor;
    }
    
    /**
     * @brief Create a BME280 sensor instance.
     * 
     * @param config The sensor configuration.
     * @return Pointer to the created sensor, or nullptr if creation failed.
     */
    ISensor* createBME280Sensor(const SensorConfig& config) {
        // For now, not implemented
        errorHandler->logError(ERROR, "BME280 sensor support not implemented yet");
        return nullptr;
    }
    
    /**
     * @brief Create a TMP117 sensor instance.
     * 
     * @param config The sensor configuration.
     * @return Pointer to the created sensor, or nullptr if creation failed.
     */
    ISensor* createTMP117Sensor(const SensorConfig& config) {
        // For now, not implemented
        errorHandler->logError(ERROR, "TMP117 sensor support not implemented yet");
        return nullptr;
    }

public:
    /**
     * @brief Constructor for SensorFactory.
     * 
     * @param err Pointer to the error handler for logging.
     * @param i2c Pointer to the I2C manager for I2C communication.
     */
    SensorFactory(ErrorHandler* err, I2CManager* i2c) 
        : errorHandler(err), i2cManager(i2c) {
    }
    
    /**
     * @brief Create a sensor instance based on the provided configuration.
     * 
     * @param config The sensor configuration.
     * @return Pointer to the created sensor, or nullptr if creation failed.
     */
    ISensor* createSensor(const SensorConfig& config) {
        SensorType type = sensorTypeFromString(config.type);
        
        // Log sensor creation attempt
        errorHandler->logInfo("Creating sensor: " + config.name + " of type " + sensorTypeToString(type));
        
        // Create appropriate sensor based on type
        switch (type) {
            case SensorType::SHT41:
                return createSHT41Sensor(config);
                
            case SensorType::BME280:
                return createBME280Sensor(config);
                
            case SensorType::TMP117:
                return createTMP117Sensor(config);
                
            default:
                errorHandler->logError(ERROR, "Unknown sensor type: " + config.type);
                return nullptr;
        }
    }
};
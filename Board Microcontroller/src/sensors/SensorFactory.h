#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "interfaces/ISensor.h"
#include "../config/ConfigManager.h"
#include "../error/ErrorHandler.h"
#include "../managers/I2CManager.h"
#include "../managers/SPIManager.h"
#include "SHT41Sensor.h"
#include "Si7021Sensor.h"
#include "PT100Sensor.h"
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
    SPIManager* spiManager;
    
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
        
        // Initialize the appropriate I2C bus if not already done
        if (!i2cManager->isPortInitialized(config.i2cPort)) {
            if (!i2cManager->beginPort(config.i2cPort)) {
                errorHandler->logError(ERROR, "Failed to initialize I2C port " + 
                                    I2CManager::portToString(config.i2cPort) + 
                                    " for sensor " + config.name);
                return nullptr;
            }
        }
        
        // Get the appropriate TwoWire instance
        TwoWire* wire = i2cManager->getWire(config.i2cPort);
        if (!wire) {
            errorHandler->logError(ERROR, "Failed to get I2C bus for port " + 
                                I2CManager::portToString(config.i2cPort));
            return nullptr;
        }
        
        // Create an SHT41 sensor with the specified configuration
        SHT41Sensor* sensor = new SHT41Sensor(
            config.name,
            config.address,
            wire,
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

    /**
     * @brief Create a Si7021 sensor instance.
     * 
     * @param config The sensor configuration.
     * @return Pointer to the created sensor, or nullptr if creation failed.
     */
    ISensor* createSi7021Sensor(const SensorConfig& config) {
        if (config.isSPI) {
            errorHandler->logError(ERROR, "Si7021 does not support SPI interface");
            return nullptr;
        }
        
        // Initialize the appropriate I2C bus if not already done
        if (!i2cManager->isPortInitialized(config.i2cPort)) {
            if (!i2cManager->beginPort(config.i2cPort)) {
                errorHandler->logError(ERROR, "Failed to initialize I2C port " + 
                                    I2CManager::portToString(config.i2cPort) + 
                                    " for sensor " + config.name);
                return nullptr;
            }
        }
        
        // Get the appropriate TwoWire instance
        TwoWire* wire = i2cManager->getWire(config.i2cPort);
        if (!wire) {
            errorHandler->logError(ERROR, "Failed to get I2C bus for port " + 
                                I2CManager::portToString(config.i2cPort));
            return nullptr;
        }
        
        // Create a Si7021 sensor with the specified configuration
        Si7021Sensor* sensor = new Si7021Sensor(
            config.name,
            config.address,
            wire,
            errorHandler
        );
        
        return sensor;
    }

    /**
     * @brief Create a PT100 RTD sensor instance.
     * 
     * @param config The sensor configuration.
     * @return Pointer to the created sensor, or nullptr if creation failed.
     */
    ISensor* createPT100Sensor(const SensorConfig& config) {
        if (!config.isSPI) {
            errorHandler->logError(ERROR, "PT100 RTD requires SPI interface");
            return nullptr;
        }
        
        // Make sure SPI is initialized
        if (!spiManager || !spiManager->isInitialized()) {
            errorHandler->logError(ERROR, "SPI not initialized for PT100 sensor: " + config.name);
            return nullptr;
        }
        
        // Create a PT100 sensor with the specified configuration
        // Default reference resistor is 430.0 ohms for Adafruit board
        // Default to 3-wire PT100
        PT100Sensor* sensor = new PT100Sensor(
            config.name,
            config.address, // SS Pin
            spiManager,
            errorHandler,
            430.0, // Reference resistor value
            3      // 3-wire PT100
        );
        
        return sensor;
    }

public:
    /**
     * @brief Constructor for SensorFactory.
     * 
     * @param err Pointer to the error handler for logging.
     * @param i2c Pointer to the I2C manager for I2C communication.
     * @param spi Pointer to the SPI manager for SPI communication.
     */
    SensorFactory(ErrorHandler* err, I2CManager* i2c, SPIManager* spi = nullptr) 
        : errorHandler(err), i2cManager(i2c), spiManager(spi) {
    }
    
    /**
     * @brief Set the SPI manager.
     * 
     * @param spi Pointer to the SPI manager.
     */
    void setSPIManager(SPIManager* spi) {
        spiManager = spi;
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
        errorHandler->logInfo("Creating sensor: " + config.name + " of type " + config.type);
        if (!config.isSPI) {
            errorHandler->logInfo("I2C configuration: Port " + I2CManager::portToString(config.i2cPort) + 
                              ", Address 0x" + String(config.address, HEX));
        } else {
            errorHandler->logInfo("SPI configuration: SS Pin " + String(config.address));
            
            // Check if SPI manager is available
            if (!spiManager) {
                errorHandler->logError(ERROR, "SPI manager not provided for SPI sensor: " + config.name);
                return nullptr;
            }
        }
        
        // Create appropriate sensor based on type
        switch (type) {
            case SensorType::SHT41:
                return createSHT41Sensor(config);
                
            case SensorType::BME280:
                return createBME280Sensor(config);
                
            case SensorType::TMP117:
                return createTMP117Sensor(config);
                
            case SensorType::SI7021:
                return createSi7021Sensor(config);
                
            case SensorType::PT100_RTD:
                return createPT100Sensor(config);
                
            default:
                errorHandler->logError(ERROR, "Unknown sensor type: " + config.type);
                return nullptr;
        }
    }
};
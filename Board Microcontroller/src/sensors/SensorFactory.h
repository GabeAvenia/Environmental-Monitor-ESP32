#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "interfaces/ISensor.h"
#include "../config/ConfigManager.h"
#include "../error/ErrorHandler.h"
#include "../managers/I2CManager.h"
#include "../managers/SPIManager.h"
#include "SensorTypes.h"

// Forward declarations
class SHT41Sensor;
class Si7021Sensor;
class PT100Sensor;

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
     * @brief Create a sensor using the template method pattern.
     * 
     * @tparam SensorType The type of sensor to create
     * @param config The sensor configuration
     * @return Pointer to the created sensor, or nullptr if creation failed
     */
    template<typename SensorType>
    ISensor* createI2CSensor(const SensorConfig& config);
    
    /**
     * @brief Create a PT100 RTD sensor instance.
     * 
     * @param config The sensor configuration.
     * @return Pointer to the created sensor, or nullptr if creation failed.
     */
    ISensor* createPT100Sensor(const SensorConfig& config);
    
    /**
     * @brief Parse additional settings for PT100 sensors.
     * 
     * @param additional The additional settings string
     * @param refResistor Output parameter for reference resistor value
     * @param wireMode Output parameter for wire mode
     */
    void parseAdditionalPT100Settings(const String& additional, 
                                      float& refResistor, 
                                      int& wireMode);

public:
    /**
     * @brief Constructor for SensorFactory.
     * 
     * @param err Pointer to the error handler for logging.
     * @param i2c Pointer to the I2C manager for I2C communication.
     * @param spi Pointer to the SPI manager for SPI communication.
     */
    SensorFactory(ErrorHandler* err, I2CManager* i2c, SPIManager* spi = nullptr);
    
    /**
     * @brief Set the SPI manager.
     * 
     * @param spi Pointer to the SPI manager.
     */
    void setSPIManager(SPIManager* spi);
    
    /**
     * @brief Create a sensor instance based on the provided configuration.
     * 
     * @param config The sensor configuration.
     * @return Pointer to the created sensor, or nullptr if creation failed.
     */
    ISensor* createSensor(const SensorConfig& config);
};
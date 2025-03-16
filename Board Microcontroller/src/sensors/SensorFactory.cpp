
#include <Arduino.h>
#include <Wire.h>
#include "interfaces/ISensor.h"
#include "../config/ConfigManager.h"
#include "../error/ErrorHandler.h"
#include "../managers/I2CManager.h"

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
    ISensor* createSHT41Sensor(const SensorConfig& config);
    
    /**
     * @brief Create a BME280 sensor instance.
     * 
     * @param config The sensor configuration.
     * @return Pointer to the created sensor, or nullptr if creation failed.
     */
    ISensor* createBME280Sensor(const SensorConfig& config);
    
    /**
     * @brief Create a TMP117 sensor instance.
     * 
     * @param config The sensor configuration.
     * @return Pointer to the created sensor, or nullptr if creation failed.
     */
    ISensor* createTMP117Sensor(const SensorConfig& config);

public:
    /**
     * @brief Constructor for SensorFactory.
     * 
     * @param err Pointer to the error handler for logging.
     * @param i2c Pointer to the I2C manager for I2C communication.
     */
    SensorFactory(ErrorHandler* err, I2CManager* i2c);
    
    /**
     * @brief Create a sensor instance based on the provided configuration.
     * 
     * @param config The sensor configuration.
     * @return Pointer to the created sensor, or nullptr if creation failed.
     */
    ISensor* createSensor(const SensorConfig& config);
};
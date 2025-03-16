#pragma once

#include <Adafruit_SHT4x.h>
#include <Wire.h>
#include "BaseSensor.h"
#include "interfaces/ITemperatureSensor.h"
#include "interfaces/IHumiditySensor.h"
#include "readings/TemperatureReading.h"
#include "readings/HumidityReading.h"

/**
 * @brief Implementation of the SHT41 temperature and humidity sensor.
 * 
 * This class provides access to the SHT41 sensor, which can measure
 * both temperature and humidity.
 */
class SHT41Sensor : public BaseSensor, 
                   public ITemperatureSensor,
                   public IHumiditySensor {
private:
    mutable Adafruit_SHT4x sht4;               ///< Underlying Adafruit driver (mutable for const methods)
    TwoWire* wire;                     ///< I2C bus for communication
    int i2cAddress;                    ///< I2C address of the sensor
    mutable float lastTemperature;     ///< Last temperature reading
    mutable float lastHumidity;        ///< Last humidity reading
    mutable unsigned long tempTimestamp;      ///< Timestamp of last temperature reading
    mutable unsigned long humidityTimestamp;  ///< Timestamp of last humidity reading
    
    /**
     * @brief Update both temperature and humidity readings from the sensor.
     * 
     * @return True if the readings were successfully updated, false otherwise.
     */
    bool updateReadings() const;

public:
    /**
     * @brief Constructor for SHT41Sensor.
     * 
     * @param sensorName Name/identifier for this sensor instance.
     * @param address I2C address of the sensor.
     * @param i2cBus The I2C bus to use (default is Wire1).
     * @param err Pointer to the error handler for logging.
     */
    SHT41Sensor(const String& sensorName, int address, TwoWire* i2cBus, ErrorHandler* err);
    
    /**
     * @brief Destructor.
     */
    ~SHT41Sensor();
    
    /**
     * @brief Initialize the sensor hardware.
     * 
     * @return true if initialization succeeded, false otherwise.
     */
    bool initialize() override;
    
    /**
     * @brief Read the current temperature value from the sensor.
     * 
     * @return The temperature in degrees Celsius, or NAN if reading failed.
     */
    float readTemperature() override;
    
    /**
     * @brief Read the current humidity value from the sensor.
     * 
     * @return The relative humidity as a percentage (0-100), or NAN if reading failed.
     */
    float readHumidity() override;
    
    /**
     * @brief Get the timestamp of the last temperature reading.
     * 
     * @return Timestamp in milliseconds (from millis()).
     */
    unsigned long getTemperatureTimestamp() const override;
    
    /**
     * @brief Get the timestamp of the last humidity reading.
     * 
     * @return Timestamp in milliseconds (from millis()).
     */
    unsigned long getHumidityTimestamp() const override;
    
    /**
     * @brief Perform a self-test to verify the sensor is functioning properly.
     * 
     * @return true if the self-test passed, false otherwise.
     */
    bool performSelfTest() override;
    
    /**
     * @brief Get detailed information about the sensor.
     * 
     * @return A string containing sensor information.
     */
    String getSensorInfo() const override;
    
    /**
     * @brief Check if this sensor supports a specific interface.
     * 
     * @param type The interface type to check for.
     * @return true if the sensor supports the interface, false otherwise.
     */
    bool supportsInterface(InterfaceType type) const override;
    
    /**
     * @brief Get the interface implementation for a specific type.
     * 
     * @param type The interface type to get.
     * @return Pointer to the interface, or nullptr if not supported.
     */
    void* getInterface(InterfaceType type) const override;
};
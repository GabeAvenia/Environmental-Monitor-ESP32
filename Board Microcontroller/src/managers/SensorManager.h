#pragma once

#include <vector>
#include <map>
#include <memory>
#include <ArduinoJson.h>

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

// Structure to track sensor update times
struct SensorUpdateInfo {
    String sensorName;          // Name of the sensor
    uint32_t pollingRateMs;     // How often to update in milliseconds
    uint64_t lastUpdateTime;    // Last time this sensor was updated
    
    SensorUpdateInfo() : 
        sensorName(""), pollingRateMs(1000), lastUpdateTime(0) {}
    
    SensorUpdateInfo(const String& name, uint32_t rate) : 
        sensorName(name), pollingRateMs(rate), lastUpdateTime(0) {}
};

class SensorManager {
private:
    SensorRegistry registry;
    SensorFactory factory;
    ConfigManager* configManager;
    I2CManager* i2cManager;
    SPIManager* spiManager;  // Make sure this field exists!
    ErrorHandler* errorHandler;
    std::map<String, SensorUpdateInfo> sensorUpdateInfo;
    
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
     * @brief Update readings from all sensors that are due for an update.
     * 
     * @return Number of sensors successfully updated.
     */
    int updateReadings();
    
    /**
     * @brief Get the latest temperature reading from a specific sensor.
     * 
     * @param sensorName The name of the sensor to read from.
     * @return The temperature reading.
     */
    TemperatureReading getTemperature(const String& sensorName);
    
    /**
     * @brief Get the latest humidity reading from a specific sensor.
     * 
     * @param sensorName The name of the sensor to read from.
     * @return The humidity reading.
     */
    HumidityReading getHumidity(const String& sensorName);
    
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
};
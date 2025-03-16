#pragma once

#include <Arduino.h>
#include <vector>
#include <algorithm>
#include "../sensors/interfaces/ISensor.h"
#include "../sensors/interfaces/ITemperatureSensor.h"
#include "../sensors/interfaces/IHumiditySensor.h"
#include "../sensors/interfaces/IPressureSensor.h"
#include "../sensors/interfaces/ICO2Sensor.h"
#include "../error/ErrorHandler.h"

/**
 * @brief Registry for managing sensor instances.
 * 
 * This class maintains collections of sensors organized by their capabilities,
 * allowing for efficient lookup by type or name.
 */
class SensorRegistry {
private:
    std::vector<ISensor*> allSensors;
    std::vector<ITemperatureSensor*> temperatureSensors;
    std::vector<IHumiditySensor*> humiditySensors;
    std::vector<IPressureSensor*> pressureSensors;
    std::vector<ICO2Sensor*> co2Sensors;
    ErrorHandler* errorHandler;
    
    /**
     * @brief Remove a sensor from a specific vector.
     * 
     * @tparam T The sensor interface type.
     * @param vec The vector to remove from.
     * @param sensor The sensor to remove.
     */
    template<typename T>
    void removeFromVector(std::vector<T*>& vec, ISensor* sensor) {
        vec.erase(
            std::remove_if(
                vec.begin(), 
                vec.end(), 
                [sensor](T* s) { return static_cast<ISensor*>(s) == sensor; }
            ),
            vec.end()
        );
    }

public:
    /**
     * @brief Constructor for SensorRegistry.
     * 
     * @param err Pointer to the error handler for logging.
     */
    SensorRegistry(ErrorHandler* err);
    
    /**
     * @brief Destructor.
     * 
     * Note: This does not delete the sensor instances, as they are owned by SensorManager.
     */
    ~SensorRegistry();
    
    /**
     * @brief Register a sensor in the registry.
     * 
     * @param sensor Pointer to the sensor to register.
     */
    void registerSensor(ISensor* sensor);
    
    /**
     * @brief Unregister a sensor from the registry by name.
     * 
     * @param sensorName The name of the sensor to unregister.
     * @return The unregistered sensor pointer, or nullptr if not found.
     */
    ISensor* unregisterSensor(const String& sensorName);
    
    /**
     * @brief Clear all sensors from the registry.
     * 
     * @return Vector of all sensors that were in the registry.
     */
    std::vector<ISensor*> clear();
    
    /**
     * @brief Get all sensors in the registry.
     * 
     * @return Vector of all sensor pointers.
     */
    std::vector<ISensor*> getAllSensors() const;
    
    /**
     * @brief Get all temperature sensors.
     * 
     * @return Vector of temperature sensor pointers.
     */
    std::vector<ITemperatureSensor*> getTemperatureSensors() const;
    
    /**
     * @brief Get all humidity sensors.
     * 
     * @return Vector of humidity sensor pointers.
     */
    std::vector<IHumiditySensor*> getHumiditySensors() const;
    
    /**
     * @brief Get all pressure sensors.
     * 
     * @return Vector of pressure sensor pointers.
     */
    std::vector<IPressureSensor*> getPressureSensors() const;
    
    /**
     * @brief Get all CO2 sensors.
     * 
     * @return Vector of CO2 sensor pointers.
     */
    std::vector<ICO2Sensor*> getCO2Sensors() const;
    
    /**
     * @brief Get a sensor by name.
     * 
     * @param name The name of the sensor to find.
     * @return Pointer to the found sensor, or nullptr if not found.
     */
    ISensor* getSensorByName(const String& name) const;
    
    /**
     * @brief Check if a sensor with the given name exists in the registry.
     * 
     * @param name The sensor name to check for.
     * @return true if the sensor exists, false otherwise.
     */
    bool hasSensor(const String& name) const;
    
    /**
     * @brief Get the number of sensors in the registry.
     * 
     * @return The number of sensors.
     */
    size_t count() const;
};

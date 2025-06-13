/**
 * @file SensorRegistry.h
 * @brief Registry for managing sensor instances
 * @author Gabriel Avenia
 * @date May 2025
 * @defgroup sensor_registry Sensor Registry
 * @ingroup sensor_management
 * @brief Components for tracking and managing sensor instances
 * @{
 */

 #pragma once

 #include <Arduino.h>
 #include <vector>
 #include <algorithm>
 #include "../sensors/interfaces/ISensor.h"
 #include "../sensors/interfaces/ITemperatureSensor.h"
 #include "../sensors/interfaces/IHumiditySensor.h"
 #include "../sensors/interfaces/InterfaceTypes.h"
 #include "../error/ErrorHandler.h"
 
 /**
  * @brief Registry for managing sensor instances
  * This class maintains collections of sensors organized by their capabilities,
  * allowing for efficient lookup by type or name. It serves as a central 
  * repository for all active sensors in the system.
  */
 class SensorRegistry {
 private:
     /**
      * @brief List of all sensor instances
      */
     std::vector<ISensor*> allSensors;
     
     /**
      * @brief Error handler for logging
      */
     ErrorHandler* errorHandler;
     
     /**
      * @brief Helper to get a specific interface from a sensor if supported
      * @tparam T The interface type to get
      * @param sensor The sensor to get the interface from
      * @param type The InterfaceType enum value
      * @return Pointer to the interface, or nullptr if not supported
      */
     template<typename T>
     T* getInterfaceFrom(ISensor* sensor, InterfaceType type) const {
         if (sensor && sensor->supportsInterface(type)) {
             return static_cast<T*>(sensor->getInterface(type));
         }
         return nullptr;
     }
 
 public:
     /**
      * @brief Constructor for SensorRegistry
      * @param err Pointer to the error handler for logging
      */
     SensorRegistry(ErrorHandler* err);
     
     /**
      * @brief Destructor
      * Note: This does not delete the sensor instances, as they are owned by SensorManager
      */
     ~SensorRegistry();
     
     /**
      * @brief Register a sensor in the registry
      * Adds a sensor to the registry, making it available for lookup by name or type.
      * @param sensor Pointer to the sensor to register
      */
     void registerSensor(ISensor* sensor);
     
     /**
      * @brief Unregister a sensor from the registry by name
      * Removes a sensor from the registry, but does not delete it.
      * @param sensorName The name of the sensor to unregister
      * @return The unregistered sensor pointer, or nullptr if not found
      */
     ISensor* unregisterSensor(const String& sensorName);
     
     /**
      * @brief Clear all sensors from the registry
      * Removes all sensors from the registry, but does not delete them.
      * @return Vector of all sensors that were in the registry
      */
     std::vector<ISensor*> clear();
     
     /**
      * @brief Get all sensors in the registry
      * @return Vector of all sensor pointers
      */
     std::vector<ISensor*> getAllSensors() const;
     
     /**
      * @brief Get all temperature sensors
      * Returns all sensors that implement the ITemperatureSensor interface.
      * @return Vector of temperature sensor pointers
      */
     std::vector<ITemperatureSensor*> getTemperatureSensors() const;
     
     /**
      * @brief Get all humidity sensors
      * Returns all sensors that implement the IHumiditySensor interface.
      * @return Vector of humidity sensor pointers
      */
     std::vector<IHumiditySensor*> getHumiditySensors() const;
     
     /**
      * @brief Get a sensor by name
      * @param name The name of the sensor to find
      * @return Pointer to the found sensor, or nullptr if not found
      */
     ISensor* getSensorByName(const String& name) const;
     
     /**
      * @brief Check if a sensor with the given name exists in the registry
      * @param name The sensor name to check for
      * @return true if the sensor exists, false otherwise
      */
     bool hasSensor(const String& name) const;
     
     /**
      * @brief Get the number of sensors in the registry
      * @return The number of sensors
      */
     size_t count() const;
 };
 
 /** @} */ // End of sensor_registry group

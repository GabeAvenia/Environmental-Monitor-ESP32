/**
 * @file BaseSensor.h
 * @brief Base class for all sensor implementations
 * @author Gabriel Avenia
 * @date May 2025
 *
 * @defgroup sensors Sensors
 * @brief Sensor implementations and common base classes
 * @{
 */

 #pragma once

 #include <Arduino.h>
 #include "interfaces/ISensor.h"
 #include "interfaces/InterfaceTypes.h"
 #include "../error/ErrorHandler.h"
 #include "SensorTypes.h"
 
 /**
  * @brief Base class for all sensor implementations
  * 
  * This abstract class implements common functionality for all sensors
  * and serves as a base for concrete sensor implementations. It provides
  * default implementations where possible and enforces the ISensor interface.
  */
 class BaseSensor : public ISensor {
 protected:
     String name;                   ///< Unique identifier for this sensor
     SensorType type;               ///< Type of sensor
     bool connected;                ///< Connection status
     ErrorHandler* errorHandler;    ///< Error reporting mechanism
 
 public:
     /**
      * @brief Constructor for BaseSensor
      * 
      * @param sensorName The name/identifier for this sensor instance
      * @param sensorType The type of sensor
      * @param err Pointer to the error handler for logging
      */
     BaseSensor(const String& sensorName, SensorType sensorType, ErrorHandler* err)
         : name(sensorName), type(sensorType), connected(false), errorHandler(err) {}
     
     /**
      * @brief Virtual destructor
      */
     virtual ~BaseSensor() {}
     
     /**
      * @brief Get the name of the sensor
      * 
      * @return The sensor's name/identifier
      */
     String getName() const override {
         return name;
     }
     
     /**
      * @brief Check if the sensor is currently connected and operational
      * 
      * @return true if connected, false otherwise
      */
     bool isConnected() const override {
         return connected;
     }
     
     /**
      * @brief Get the type of this sensor as an enum value
      * 
      * @return The sensor type
      */
     SensorType getType() const {
         return type;
     }
     
     /**
      * @brief Get the type of this sensor as a string
      * 
      * @return The sensor type as a string
      */
     String getTypeString() const override {
         return sensorTypeToString(type);
     }
     
     /**
      * @brief Default implementation of supportsInterface that returns false for all types
      * 
      * Derived classes should override this to indicate which interfaces they support.
      * 
      * @param type The interface type to check for
      * @return true if the sensor supports the interface, false otherwise
      */
     bool supportsInterface(InterfaceType type) const override {
         return false;
     }
     
     /**
      * @brief Default implementation of getInterface that returns nullptr for all types
      * 
      * Derived classes should override this to return the appropriate interface pointer.
      * 
      * @param type The interface type to get
      * @return Pointer to the interface, or nullptr if not supported
      */
     void* getInterface(InterfaceType type) const override {
         return nullptr;
     }
 };
 
 /** @} */ // End of sensors group
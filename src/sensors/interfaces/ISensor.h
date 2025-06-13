 /**
  * @file ISensor.h
  * @brief Base interface for all sensors
  * @author Gabriel Avenia
  * @date May 2025
  * @ingroup sensor_interfaces
  */
 
  #pragma once
 
  #include <Arduino.h>
  #include "InterfaceTypes.h"
  
  /**
   * @brief Base interface for all sensors
   * Defines core functionality that all sensors must implement,
   * regardless of the specific measurements they provide.
   */
  class ISensor {
  public:
      /**
       * @brief Virtual destructor
       */
      virtual ~ISensor() = default;
      
      /**
       * @brief Get the sensor's name
       * @return String identifier for the sensor
       */
      virtual String getName() const = 0;
      
      /**
       * @brief Check if sensor is connected and working
       * @return true if operational, false otherwise
       */
      virtual bool isConnected() const = 0;
      
      /**
       * @brief Initialize sensor hardware
       * @return true if initialization successful
       */
      virtual bool initialize() = 0;
      
      /**
       * @brief Perform sensor self-test
       * @return true if self-test passed
       */
      virtual bool performSelfTest() = 0;
      
      /**
       * @brief Get detailed sensor information
       * @return Multi-line string with sensor details
       */
      virtual String getSensorInfo() const = 0;
      
      /**
       * @brief Get sensor type as string
       * @return String representation of sensor type
       */
      virtual String getTypeString() const = 0;
      
      /**
       * @brief Check if sensor supports a specific interface
       * @param type Interface type to check
       * @return true if interface is supported
       */
      virtual bool supportsInterface(InterfaceType type) const = 0;
      
      /**
       * @brief Get interface implementation
       * @param type Interface type to get
       * @return void pointer to interface or nullptr if not supported
       */
      virtual void* getInterface(InterfaceType type) const = 0;
  };

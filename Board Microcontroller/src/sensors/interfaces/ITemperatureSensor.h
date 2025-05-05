/**
 * @file ITemperatureSensor.h
 * @brief Interface for temperature measurement
 * @author Gabriel Avenia
 * @date May 2025
 *
 * @ingroup sensor_interfaces
 */

 #pragma once

 #include <Arduino.h>
 
 /**
  * @brief Interface for sensors that can measure temperature
  * 
  * Defines the methods that must be implemented by any
  * sensor that provides temperature measurement capability.
  */
 class ITemperatureSensor {
 public:
     /**
      * @brief Virtual destructor
      */
     virtual ~ITemperatureSensor() = default;
     
     /**
      * @brief Read current temperature from sensor
      * 
      * @return Temperature in degrees Celsius or NAN if reading failed
      */
     virtual float readTemperature() = 0;
     
     /**
      * @brief Get timestamp of last temperature reading
      * 
      * @return Timestamp in milliseconds from millis()
      */
     virtual unsigned long getTemperatureTimestamp() const = 0;
 };

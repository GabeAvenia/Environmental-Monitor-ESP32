/**
 * @file IHumiditySensor.h
 * @brief Interface for humidity measurement
 * @author Gabriel Avenia
 * @date May 2025
 *
 * @ingroup sensor_interfaces
 */

 #pragma once

 #include <Arduino.h>
 
 /**
  * @brief Interface for sensors that can measure humidity
  * 
  * Defines the methods that must be implemented by any
  * sensor that provides humidity measurement capability.
  */
 class IHumiditySensor {
 public:
     /**
      * @brief Virtual destructor
      */
     virtual ~IHumiditySensor() = default;
     
     /**
      * @brief Read current humidity from sensor
      * 
      * @return Relative humidity percentage (0-100) or NAN if reading failed
      */
     virtual float readHumidity() = 0;
     
     /**
      * @brief Get timestamp of last humidity reading
      * 
      * @return Timestamp in milliseconds from millis()
      */
     virtual unsigned long getHumidityTimestamp() const = 0;
 };
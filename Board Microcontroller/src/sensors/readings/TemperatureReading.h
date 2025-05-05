/**
 * @file TemperatureReading.h
 * @brief Structure for temperature reading data
 * @author Gabriel Avenia
 * @date May 2025
 * @defgroup sensor_readings Sensor Readings
 * @ingroup sensors
 * @brief Data structures for sensor measurements
 * @{
 */

 #pragma once

 #include <Arduino.h>
 
 /**
  * @brief Structure to hold temperature reading data
  * Contains the temperature value, timestamp, and validity information.
  * Used to safely pass temperature readings between components.
  */
 struct TemperatureReading {
     float value;              ///< Temperature value in degrees Celsius
     unsigned long timestamp;  ///< Timestamp when reading was taken (millis)
     bool valid;               ///< Whether the reading is valid
     
     /**
      * @brief Default constructor - creates an invalid reading
      */
     TemperatureReading() 
         : value(NAN), timestamp(0), valid(false) {}
     
     /**
      * @brief Create a new temperature reading
      * @param temp Temperature value in degrees Celsius
      * @param time Timestamp when reading was taken (default: current time)
      */
     TemperatureReading(float temp, unsigned long time = millis()) 
         : value(temp), timestamp(time), valid(!isnan(temp)) {}
         
     /**
      * @brief Convert the reading to a string representation
      * @return A string representation of the temperature reading
      */
     String toString() const {
         if (!valid) return "Invalid";
         return String(value) + " °C";
     }
 };
 
 /** @} */ // End of sensor_readings group
 

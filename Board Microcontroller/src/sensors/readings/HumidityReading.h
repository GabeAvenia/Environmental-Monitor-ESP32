/**
 * @file HumidityReading.h
 * @brief Structure for humidity reading data
 * @author Gabriel Avenia
 * @date May 2025
 * @ingroup sensor_readings
 */

 #pragma once

 #include <Arduino.h>
 
 /**
  * @brief Structure to hold humidity reading data
  * Contains the humidity value, timestamp, and validity information.
  * Used to safely pass humidity readings between components.
  */
 struct HumidityReading {
     float value;              ///< Humidity value as percentage (0-100)
     unsigned long timestamp;  ///< Timestamp when reading was taken (millis)
     bool valid;               ///< Whether the reading is valid
     
     /**
      * @brief Default constructor - creates an invalid reading
      */
     HumidityReading() 
         : value(NAN), timestamp(0), valid(false) {}
     
     /**
      * @brief Create a new humidity reading
      * @param humidity Humidity value as percentage (0-100)
      * @param time Timestamp when reading was taken (default: current time)
      */
     HumidityReading(float humidity, unsigned long time = millis()) 
         : value(humidity), timestamp(time), valid(!isnan(humidity)) {}
         
     /**
      * @brief Convert the reading to a string representation
      * @return A string representation of the humidity reading
      */
     String toString() const {
         if (!valid) return "Invalid";
         return String(value) + " %";
     }
 };

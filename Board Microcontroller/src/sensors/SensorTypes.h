/**
 * @file SensorTypes.h
 * @brief Enumeration of supported sensor types
 * @author Gabriel Avenia
 * @date May 2025
 * @defgroup sensor_types Sensor Types
 * @ingroup sensors
 * @brief Supported sensor type definitions
 * @{
 */

 #pragma once

 /**
  * @brief Enumeration of supported sensor types
  * This enum defines the sensor models that the system can work with.
  * When adding support for a new sensor model, add it here.
  */
 enum class SensorType {
     UNKNOWN,    ///< Unknown or unsupported sensor type
     SHT41,      ///< Sensirion SHT41 temperature/humidity sensor
     SI7021,     ///< Silicon Labs Si7021 temperature/humidity sensor
     PT100_RTD   ///< PT100 RTD temperature sensor with MAX31865
 };
 
 /**
  * @brief Convert a string sensor type to the enum representation
  * @param typeStr The string representation of the sensor type
  * @return The corresponding SensorType enum value, or UNKNOWN if not recognized
  */
 inline SensorType sensorTypeFromString(const String& typeStr) {
     if (typeStr.equalsIgnoreCase("SHT41")) return SensorType::SHT41;
     
     // New sensor types
     if (typeStr.equalsIgnoreCase("SI7021") || 
         typeStr.equalsIgnoreCase("Adafruit SI7021")) return SensorType::SI7021;
     
     if (typeStr.equalsIgnoreCase("PT100_RTD") || 
         typeStr.equalsIgnoreCase("Adafruit PT100 RTD")) return SensorType::PT100_RTD;
     
     return SensorType::UNKNOWN;
 }
 
 /**
  * @brief Convert a SensorType enum to its string representation
  * @param type The SensorType enum value
  * @return The string representation of the sensor type
  */
 inline String sensorTypeToString(SensorType type) {
     switch (type) {
         case SensorType::SHT41: return "SHT41";
         case SensorType::SI7021: return "Adafruit SI7021";
         case SensorType::PT100_RTD: return "Adafruit PT100 RTD";
         default: return "UNKNOWN";
     }
 }
 
 /** @} */ // End of sensor_types group

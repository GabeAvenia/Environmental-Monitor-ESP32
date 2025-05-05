/**
 * @file InterfaceTypes.h
 * @brief Enumeration of sensor interface types
 * @author Gabriel Avenia
 * @date May 2025
 * @defgroup sensor_interfaces Sensor Interfaces
 * @ingroup sensors
 * @brief Interface definitions for sensor capabilities
 * @{
 */

 #pragma once

 /**
  * @brief Enumeration of interface types that sensors can implement
  * Defines the different measurement capabilities that sensors
  * may support. Used for runtime interface discovery.
  */
 enum class InterfaceType {
     TEMPERATURE,  ///< Temperature measurement capability
     HUMIDITY,     ///< Humidity measurement capability
     CO2           ///< CO2 measurement capability
 };
 
 /** @} */ // End of sensor_interfaces group
 

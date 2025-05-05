/**
 * @file ConfigManager.h
 * @brief Configuration management system
 * @author Gabriel Avenia
 * @date May 2025
 *
 * @defgroup configuration Configuration Management
 * @brief Components for system configuration and persistence
 * @{
 */

 #pragma once

 #include <Arduino.h>
 #include <vector>
 #include <LittleFS.h>
 #include <ArduinoJson.h>
 #include "../error/ErrorHandler.h"
 #include "../managers/I2CManager.h"
 
 /**
  * @brief Structure for sensor configurations
  * 
  * Holds all configuration parameters for a specific sensor,
  * including communication settings and operational parameters.
  */
 struct SensorConfig {
     String name;          ///< Unique name identifier for the sensor
     String type;          ///< Type/model of the sensor
     int address;          ///< I2C address or SPI SS pin
     bool isSPI;           ///< True if this is an SPI sensor
     I2CPort i2cPort;      ///< Which I2C bus to use (only for I2C sensors)
     uint32_t pollingRate; ///< Polling rate in milliseconds
     String additional;    ///< Additional sensor-specific settings
     
     /**
      * @brief Equality operator for comparing configurations
      * 
      * @param other The configuration to compare with
      * @return true if configurations are identical
      */
     bool operator==(const SensorConfig& other) const {
         return name == other.name && 
                type == other.type && 
                address == other.address && 
                isSPI == other.isSPI &&
                i2cPort == other.i2cPort &&
                pollingRate == other.pollingRate &&
                additional == other.additional;
     }
     
     /**
      * @brief Inequality operator
      * 
      * @param other The configuration to compare with
      * @return true if configurations differ in any way
      */
     bool operator!=(const SensorConfig& other) const {
         return !(*this == other);
     }
 };
 
 /**
  * @brief Callback function type for configuration changes
  * 
  * Used to notify interested parties when configuration changes.
  */
 typedef void (*ConfigChangeCallback)(const String& newConfig);
 
 /**
  * @brief Manages system configuration and persistence
  * 
  * This class handles loading, saving, and modifying the system configuration,
  * including sensors, identification, and general settings. It provides a 
  * JSON-based interface for configuration updates.
  */
 class ConfigManager {
 private:
     /**
      * @brief Error handler for logging
      */
     ErrorHandler* errorHandler;
     
     /**
      * @brief Board identifier
      */
     String boardId;
     
     /**
      * @brief Vector of sensor configurations
      */
     std::vector<SensorConfig> sensorConfigs;
     
     /**
      * @brief Additional configuration JSON
      */
     String additionalConfig;
     
     /**
      * @brief Vector of configuration change callbacks
      */
     std::vector<ConfigChangeCallback> changeCallbacks;
     
     /**
      * @brief Flag to prevent recursive notification
      */
     bool notifyingCallbacks;
     
     /**
      * @brief Load configuration from file
      * 
      * @return true if loading succeeded
      */
     bool loadConfigFromFile();
     
     /**
      * @brief Create default configuration
      * 
      * @return true if creation succeeded
      */
     bool createDefaultConfig();
     
     /**
      * @brief Notify all registered callbacks about configuration changes
      * 
      * @param newConfig The new configuration JSON
      */
     void notifyConfigChanged(const String& newConfig);
     
     /**
      * @brief Helper methods for file operations
      * @{
      */
     bool writeConfigToFile(const JsonDocument& doc);
     bool readConfigFromFile(JsonDocument& doc);
     /** @} */
 
 public:
     /**
      * @brief Constructor
      * 
      * @param err Pointer to error handler
      */
     ConfigManager(ErrorHandler* err);
     
     /**
      * @brief Initialize the configuration manager
      * 
      * Loads the configuration from file or creates a default one
      * if none exists.
      * 
      * @return true if initialization succeeded
      */
     bool begin();
     
     /**
      * @brief Enable or disable notifications
      * 
      * Used to prevent recursive notifications during complex
      * configuration updates.
      * 
      * @param disable Whether to disable notifications
      */
     void disableNotifications(bool disable);
     
     /**
      * @brief Board identification methods
      * @{
      */
     
     /**
      * @brief Get the board identifier
      * 
      * @return The board's unique identifier
      */
     String getBoardIdentifier();
     
     /**
      * @brief Set the board identifier
      * 
      * @param identifier The new identifier
      * @return true if update succeeded
      */
     bool setBoardIdentifier(String identifier);
     /** @} */
     
     /**
      * @brief Sensor configuration methods
      * @{
      */
     
     /**
      * @brief Get all sensor configurations
      * 
      * @return Vector of sensor configurations
      */
     std::vector<SensorConfig> getSensorConfigs();
     
     /**
      * @brief Update sensor configurations
      * 
      * @param configs Vector of new sensor configurations
      * @return true if update succeeded
      */
     bool updateSensorConfigs(const std::vector<SensorConfig>& configs);
     /** @} */
     
     /**
      * @brief Complete configuration methods
      * @{
      */
     
     /**
      * @brief Get complete configuration as JSON
      * 
      * @return Configuration JSON string
      */
     String getConfigJson();
     
     /**
      * @brief Update configuration from JSON
      * 
      * @param jsonConfig Complete configuration JSON
      * @return true if update succeeded
      */
     bool updateConfigFromJson(const String& jsonConfig);
     /** @} */
     
     /**
      * @brief Granular configuration methods
      * @{
      */
     
     /**
      * @brief Update only sensor configuration from JSON
      * 
      * @param jsonConfig Sensor configuration JSON
      * @return true if update succeeded
      */
     bool updateSensorConfigFromJson(const String& jsonConfig);
     
     /**
      * @brief Update only additional configuration
      * 
      * @param jsonConfig Additional configuration JSON
      * @return true if update succeeded
      */
     bool updateAdditionalConfigFromJson(const String& jsonConfig);
     /** @} */
     
     /**
      * @brief Change notification
      * @{
      */
     
     /**
      * @brief Register callback for configuration changes
      * 
      * @param callback Function to call when configuration changes
      */
     void registerChangeCallback(ConfigChangeCallback callback);
     /** @} */
 };
 
 /** @} */ // End of configuration group
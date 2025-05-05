/**
 * @file CommunicationManager.h
 * @brief Manager for external communications and command processing
 * @author Gabriel Avenia
 * @date May 2025
 *
 * @defgroup communication Communication
 * @brief Components for managing external communications
 * @{
 */

 #pragma once

 #include <Arduino.h>
 #include <map>
 #include <functional>
 #include <vector>
 #include "Constants.h"
 
 // Forward declarations for SCPI parser classes
 class SCPI_Parser;
 class SCPI_Commands;
 class SCPI_Parameters;
 class Stream;
 
 #include "../managers/SensorManager.h"
 #include "../config/ConfigManager.h"
 #include "../error/ErrorHandler.h"
 #include "../managers/LedManager.h"
 
 /**
  * @brief Command Handler function signature
  * 
  * @param params Vector of string parameters from the command
  * @return true if command was successfully processed
  */
 typedef std::function<bool(const std::vector<String>&)> CommandHandler;
 
 /**
  * @brief Manages communication with external systems using SCPI commands
  * 
  * Processes incoming commands through both a custom command handler and SCPI parser,
  * providing a unified interface for controlling the device via serial. Supports
  * custom command processing and SCPI standards compliance.
  */
 class CommunicationManager {
 private:
     /**
      * @brief SCPI parser instance
      */
     SCPI_Parser* scpiParser;
     
     /**
      * @brief References to other system managers
      * @{
      */
     SensorManager* sensorManager;
     ConfigManager* configManager;
     ErrorHandler* errorHandler;
     LedManager* ledManager = nullptr;
     /** @} */
     
     /**
      * @brief Map of command strings to handler functions
      */
     std::map<String, CommandHandler> commandHandlers;
 
     /**
      * @brief Static reference to UART debug serial
      */
     static Print* uartDebugSerial;
     
     /**
      * @brief Singleton instance for callback access
      */
     static CommunicationManager* instance;
     
     /**
      * @brief Destructor - cleans up allocated resources
      */
     ~CommunicationManager();
 
     /**
      * @brief Register all commands with their handlers
      */
     void registerCommandHandlers();
 
     /**
      * @brief Collect readings from a sensor into a values vector
      * 
      * @param sensorName The name of the sensor to read from
      * @param measurements The measurements to read (comma-separated or empty for all)
      * @param values Vector to collect the values into
      */
     void collectSensorReadings(const String& sensorName, const String& measurements, std::vector<String>& values);
 
 public:
     /**
      * @brief Constructor for CommunicationManager
      * 
      * @param sensorMgr Pointer to sensor manager
      * @param configMgr Pointer to configuration manager
      * @param err Pointer to error handler
      * @param led Optional pointer to LED manager
      */
     CommunicationManager(SensorManager* sensorMgr, ConfigManager* configMgr, 
     ErrorHandler* err, LedManager* led = nullptr);
     
     /**
      * @brief Initialize the communication system
      * 
      * @param baudRate Serial communication baud rate
      */
     void begin(long baudRate);
     
     /**
      * @brief Register all SCPI commands with the parser
      */
     void setupCommands();
     
     /**
      * @brief Process incoming serial data and handle complete commands
      */
     void processIncomingData();
     
     /**
      * @brief Set the LED manager
      * 
      * @param led Pointer to LED manager
      */
     void setLedManager(LedManager* led);
     
     /**
      * @brief Process a single incoming command line
      */
     void processCommandLine();
     
     /**
      * @brief Parse a raw command string into command and parameters
      * 
      * @param rawCommand The full command string
      * @param command Output parameter for the extracted command
      * @param params Output parameter for the extracted parameters
      */
     void parseCommand(const String& rawCommand, String& command, std::vector<String>& params);
 
     /**
      * @brief Process a command using the registered handler
      * 
      * @param command The command to process
      * @param params The command parameters
      * @return true if command was found and processed successfully
      */
     bool processCommand(const String& command, const std::vector<String>& params);
 
     /**
      * @name Command handlers
      * @{
      */
     
     /**
      * @brief Handle device identification command (*IDN?)
      * 
      * @param params Command parameters (not used)
      * @return true if command processed successfully
      */
     bool handleIdentify(const std::vector<String>& params);
     
     /**
      * @brief Handle measurement query command (MEAS?)
      * 
      * @param params Sensor and measurement parameters
      * @return true if command processed successfully
      */
     bool handleMeasure(const std::vector<String>& params);
     
     /**
      * @brief Handle sensor listing command (SYST:SENS:LIST?)
      * 
      * @param params Command parameters (not used)
      * @return true if command processed successfully
      */
     bool handleListSensors(const std::vector<String>& params);
     
     /**
      * @brief Handle configuration query command (SYST:CONF?)
      * 
      * @param params Command parameters (not used)
      * @return true if command processed successfully
      */
     bool handleGetConfig(const std::vector<String>& params);
     
     /**
      * @brief Handle board ID setting command (SYST:CONF:BOARD:ID)
      * 
      * @param params Board ID parameter
      * @return true if command processed successfully
      */
     bool handleSetBoardId(const std::vector<String>& params);
     
     /**
      * @brief Handle configuration update command (SYST:CONF:UPDATE)
      * 
      * @param params Configuration JSON
      * @return true if command processed successfully
      */
     bool handleUpdateConfig(const std::vector<String>& params);
     
     /**
      * @brief Handle sensor configuration update command (SYST:CONF:SENS:UPDATE)
      * 
      * @param params Sensor configuration JSON
      * @return true if command processed successfully
      */
     bool handleUpdateSensorConfig(const std::vector<String>& params);
     
     /**
      * @brief Handle additional configuration update command (SYST:CONF:ADD:UPDATE)
      * 
      * @param params Additional configuration JSON
      * @return true if command processed successfully
      */
     bool handleUpdateAdditionalConfig(const std::vector<String>& params);
     
     /**
      * @brief Handle reset command (RESET)
      * 
      * @param params Command parameters (not used)
      * @return true if command processed successfully
      */
     bool handleReset(const std::vector<String>& params);
     
     /**
      * @brief Handle filesystem test command (TEST:FS)
      * 
      * @param params Command parameters (not used)
      * @return true if command processed successfully
      */
     bool handleTestFilesystem(const std::vector<String>& params);
     
     /**
      * @brief Handle configuration update test command (TEST:UPDATE)
      * 
      * @param params Command parameters (not used)
      * @return true if command processed successfully
      */
     bool handleTestUpdateConfig(const std::vector<String>& params);
     
     /**
      * @brief Handle echo command (ECHO)
      * 
      * @param params Text to echo
      * @return true if command processed successfully
      */
     bool handleEcho(const std::vector<String>& params);
     
     /**
      * @brief Handle log routing command (SYST:LOG)
      * 
      * @param params Destination and severity parameters
      * @return true if command processed successfully
      */
     bool handleLogRouting(const std::vector<String>& params);
     
     /**
      * @brief Handle log status query command (SYST:LOG?)
      * 
      * @param params Command parameters (not used)
      * @return true if command processed successfully
      */
     bool handleLogStatus(const std::vector<String>& params);
     
     /**
      * @brief Handle LED identification command (SYST:LED:IDENT)
      * 
      * @param params Command parameters (not used)
      * @return true if command processed successfully
      */
     bool handleLedIdentify(const std::vector<String>& params);
     
     /**
      * @brief Handle test error level commands
      * 
      * @param params Command parameters
      * @param severity Error severity level
      * @return true if command processed successfully
      */
     bool handleTestErrorLevel(const std::vector<String>& params, ErrorSeverity severity);
     
     /**
      * @brief Handle test info level command (TEST:INFO)
      * 
      * @param params Command parameters
      * @return true if command processed successfully
      */
     bool handleTestInfoLevel(const std::vector<String>& params) { return handleTestErrorLevel(params, INFO); }
     
     /**
      * @brief Handle test warning level command (TEST:WARNING)
      * 
      * @param params Command parameters
      * @return true if command processed successfully
      */
     bool handleTestWarningLevel(const std::vector<String>& params) { return handleTestErrorLevel(params, WARNING); }
     
     /**
      * @brief Handle test error level command (TEST:ERROR)
      * 
      * @param params Command parameters
      * @return true if command processed successfully
      */
     bool handleTestErrorLevel(const std::vector<String>& params) { return handleTestErrorLevel(params, ERROR); }
     
     /**
      * @brief Handle test fatal level command (TEST:FATAL)
      * 
      * @param params Command parameters
      * @return true if command processed successfully
      */
     bool handleTestFatalLevel(const std::vector<String>& params) { return handleTestErrorLevel(params, FATAL); }
     /** @} */
     
     /**
      * @name Static and accessor methods
      * @{
      */
     
     /**
      * @brief Get singleton instance
      * 
      * @return Pointer to the singleton instance
      */
     static CommunicationManager* getInstance();
     
     /**
      * @brief Get the sensor manager
      * 
      * @return Pointer to the sensor manager
      */
     SensorManager* getSensorManager();
     
     /**
      * @brief Get the configuration manager
      * 
      * @return Pointer to the configuration manager
      */
     ConfigManager* getConfigManager();
     
     /**
      * @brief Get the error handler
      * 
      * @return Pointer to the error handler
      */
     ErrorHandler* getErrorHandler();
     
     /**
      * @brief Get the SCPI parser
      * 
      * @return Pointer to the SCPI parser
      */
     SCPI_Parser* getScpiParser() { return scpiParser; }
     
     /**
      * @brief Set the UART debug serial pointer for message routing
      * 
      * @param debugSerial Pointer to the debug serial
      */
     static void setUartDebugSerialPtr(Print* debugSerial);
     /** @} */
 };
 
 /** @} */ // End of communication group
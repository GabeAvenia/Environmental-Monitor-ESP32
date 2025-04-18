#pragma once

#include <Arduino.h>
#include <map>
#include <functional>
#include <vector>

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
 * @brief Manages communication with external systems, handling SCPI commands
 * 
 * Processes incoming commands through both a custom command handler and SCPI parser,
 * providing a unified interface for controlling the device via serial.
 */
class CommunicationManager {
private:
    SCPI_Parser* scpiParser;
    SensorManager* sensorManager;
    ConfigManager* configManager;
    ErrorHandler* errorHandler;
    LedManager* ledManager = nullptr;
    
    // Command handling
    std::map<String, CommandHandler> commandHandlers;

    // Static reference to UART debug serial
    static Print* uartDebugSerial;
    
    // Singleton instance for callback access
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
    
    /**
     * @brief Handle routing messages to specified output streams based on severity
     * 
     * @param params Parameters containing the destination
     * @param severity The message severity level to configure
     * @return true if routing was successfully configured
     */
    bool handleSetMessageRoute(const std::vector<String>& params, const String& severity);

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

    // Command handlers
    bool handleIdentify(const std::vector<String>& params);
    bool handleMeasure(const std::vector<String>& params);
    bool handleListSensors(const std::vector<String>& params);
    bool handleGetConfig(const std::vector<String>& params);
    bool handleSetBoardId(const std::vector<String>& params);
    bool handleUpdateConfig(const std::vector<String>& params);
    bool handleUpdateSensorConfig(const std::vector<String>& params);
    bool handleUpdateAdditionalConfig(const std::vector<String>& params);
    bool handleReset(const std::vector<String>& params);
    bool handleTestFilesystem(const std::vector<String>& params);
    bool handleTestUpdateConfig(const std::vector<String>& params);
    bool handleEcho(const std::vector<String>& params);
    
    // Message routing command handlers
    bool handleMessageRoutingStatus(const std::vector<String>& params);
    bool handleMessageRoutingSet(const std::vector<String>& params);
    bool handleInfoRoute(const std::vector<String>& params);
    bool handleWarningRoute(const std::vector<String>& params);
    bool handleErrorRoute(const std::vector<String>& params);
    bool handleLedIdentify(const std::vector<String>& params);
    
    // Static and accessor methods
    static CommunicationManager* getInstance();
    SensorManager* getSensorManager();
    ConfigManager* getConfigManager();
    ErrorHandler* getErrorHandler();
    SCPI_Parser* getScpiParser() { return scpiParser; }
    
    /**
     * @brief Set the UART debug serial pointer for message routing
     * 
     * @param debugSerial Pointer to the debug serial
     */
    static void setUartDebugSerialPtr(Print* debugSerial);
};
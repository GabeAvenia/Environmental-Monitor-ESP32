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

/**
 * Command Handler - Function signature for command processing functions
 * @param params - Command parameters
 * @return true if command was successfully processed
 */
typedef std::function<bool(const std::vector<String>&)> CommandHandler;

/**
 * @brief Configuration for streaming a specific sensor's measurements
 */
struct StreamingConfig {
    String sensorName;
    bool streamTemperature;
    bool streamHumidity;
    bool streamPressure;
    bool streamCO2;
    
    /**
     * @brief Constructor with default to stream all measurements
     * 
     * @param name Sensor name
     */
    StreamingConfig(const String& name) : 
        sensorName(name), 
        streamTemperature(true),
        streamHumidity(true), 
        streamPressure(true),
        streamCO2(true) {}
    
    /**
     * @brief Constructor with specific measurements to stream
     * 
     * @param name Sensor name
     * @param measurements String containing measurement codes (TEMP, HUM, PRES, CO2)
     */
    StreamingConfig(const String& name, const String& measurements) : 
        sensorName(name), 
        streamTemperature(false),
        streamHumidity(false), 
        streamPressure(false),
        streamCO2(false) {
        
        // Convert to uppercase for case-insensitive comparison
        String upperMeasurements = measurements;
        upperMeasurements.toUpperCase();
        
        // Parse measurements string (e.g., "TEMP,HUM" for temperature and humidity)
        if (upperMeasurements.indexOf("TEMP") >= 0) streamTemperature = true;
        if (upperMeasurements.indexOf("HUM") >= 0) streamHumidity = true;
        if (upperMeasurements.indexOf("PRES") >= 0) streamPressure = true;
        if (upperMeasurements.indexOf("CO2") >= 0) streamCO2 = true;
    }
};

class CommunicationManager {
private:
    SCPI_Parser* scpiParser;
    SensorManager* sensorManager;
    ConfigManager* configManager;
    ErrorHandler* errorHandler;
    
    // Command handling
    std::map<String, CommandHandler> commandHandlers;

    // Streaming functionality
    bool isStreaming;
    unsigned long lastStreamTime;
    unsigned long streamInterval;
    std::vector<StreamingConfig> streamingConfigs;
    
    // Static reference to UART debug serial
    static Print* uartDebugSerial;
    
    // Singleton instance for callback access
    static CommunicationManager* instance;
    
    /**
     * Parse a raw command string into command and parameters
     * @param rawCommand - The full command string
     * @param command - Output parameter that will contain the command
     * @param params - Output parameter that will contain the parameters
     */
    void parseCommand(const String& rawCommand, String& command, std::vector<String>& params);
    
    /**
     * Register all commands with their handlers
     */
    void registerCommandHandlers();
    
    /**
     * Process a command using the registered handler
     * @param command - The command to process
     * @param params - The command parameters
     * @return true if command was found and processed successfully
     */
    bool processCommand(const String& command, const std::vector<String>& params);
    
    /**
     * Collect readings from a sensor into a values vector
     * @param sensorName - The name of the sensor to read from
     * @param measurements - The measurements to read (comma-separated or empty for all)
     * @param values - Vector to collect the values into
     */
    void collectSensorReadings(const String& sensorName, const String& measurements, std::vector<String>& values);
    
    // Command handler methods
    bool handleIdentify(const std::vector<String>& params);
    bool handleMeasureSingle(const std::vector<String>& params);
    bool handleListSensors(const std::vector<String>& params);
    bool handleGetConfig(const std::vector<String>& params);
    bool handleSetBoardId(const std::vector<String>& params);
    bool handleUpdateConfig(const std::vector<String>& params);
    bool handleStreamStart(const std::vector<String>& params);
    bool handleStreamStop(const std::vector<String>& params);
    bool handleStreamStatus(const std::vector<String>& params);
    bool handleTestFilesystem(const std::vector<String>& params);
    bool handleTestUpdateConfig(const std::vector<String>& params);
    bool handleEcho(const std::vector<String>& params);
    
    // Message routing command handlers
    bool handleMessageRoutingStatus(const std::vector<String>& params);
    bool handleMessageRoutingSet(const std::vector<String>& params);
    bool handleSetMessageRoute(const std::vector<String>& params, const String& severity);
    bool handleInfoRoute(const std::vector<String>& params);
    bool handleWarningRoute(const std::vector<String>& params);
    bool handleErrorRoute(const std::vector<String>& params);
    bool handleCriticalRoute(const std::vector<String>& params);
    
public:
    CommunicationManager(SensorManager* sensorMgr, ConfigManager* configMgr, ErrorHandler* err);
    ~CommunicationManager();
    
    void begin(long baudRate);
    void setupCommands();
    void processIncomingData();
    
    // Streaming methods
    bool startStreaming(const std::vector<StreamingConfig>& configs);
    void stopStreaming();
    bool isCurrentlyStreaming() const;
    void handleStreaming();
    
    // Getter methods for use in callbacks
    static CommunicationManager* getInstance();
    SensorManager* getSensorManager();
    ConfigManager* getConfigManager();
    ErrorHandler* getErrorHandler();
    
    /**
     * @brief Set the UART debug serial pointer for message routing
     * @param debugSerial Pointer to the debug serial
     */
    static void setUartDebugSerialPtr(Print* debugSerial);
};
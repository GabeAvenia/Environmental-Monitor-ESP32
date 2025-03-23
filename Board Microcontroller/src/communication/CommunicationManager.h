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
    std::vector<String> streamingSensors;
    
    // Verbose logging control
    bool verboseLogging;
    
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
    
    // No helper method needed for SCPI registration due to API limitations
    
    /**
     * Process a command using the registered handler
     * @param command - The command to process
     * @param params - The command parameters
     * @return true if command was found and processed successfully
     */
    bool processCommand(const String& command, const std::vector<String>& params);
    
    // Command handler methods
    bool handleIdentify(const std::vector<String>& params);
    bool handleMeasureTemperature(const std::vector<String>& params);
    bool handleMeasureHumidity(const std::vector<String>& params);
    bool handleListSensors(const std::vector<String>& params);
    bool handleGetConfig(const std::vector<String>& params);
    bool handleSetBoardId(const std::vector<String>& params);
    bool handleUpdateConfig(const std::vector<String>& params);
    bool handleStreamStart(const std::vector<String>& params);
    bool handleStreamStop(const std::vector<String>& params);
    bool handleStreamStatus(const std::vector<String>& params);
    bool handleVerboseLog(const std::vector<String>& params);
    bool handleTestFilesystem(const std::vector<String>& params);
    bool handleTestUpdateConfig(const std::vector<String>& params);
    bool handleEcho(const std::vector<String>& params);
    
public:
    CommunicationManager(SensorManager* sensorMgr, ConfigManager* configMgr, ErrorHandler* err);
    ~CommunicationManager();
    
    void begin(long baudRate);
    void setupCommands();
    void processIncomingData();
    
    // Streaming methods
    bool startStreaming(const std::vector<String>& sensorNames);
    void stopStreaming();
    bool isCurrentlyStreaming() const;
    void handleStreaming();
    
    // Logging control
    void setVerboseLogging(bool enable);
    bool isVerboseLogging() const;
    
    // Getter methods for use in callbacks
    static CommunicationManager* getInstance();
    SensorManager* getSensorManager();
    ConfigManager* getConfigManager();
    ErrorHandler* getErrorHandler();
};
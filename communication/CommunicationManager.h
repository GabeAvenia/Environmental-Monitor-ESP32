#pragma once

#include <Arduino.h>

// Forward declare SCPI classes instead of including them directly
class SCPI_Parser;
class SCPI_Commands;
class SCPI_Parameters;
class Stream;

#include "../managers/SensorManager.h"
#include "../config/ConfigManager.h"
#include "../error/ErrorHandler.h"

// Forward declarations of callback functions
void idnHandler(SCPI_Commands commands, SCPI_Parameters parameters, Stream& interface);
void measureTempHandler(SCPI_Commands commands, SCPI_Parameters parameters, Stream& interface);
void measureHumHandler(SCPI_Commands commands, SCPI_Parameters parameters, Stream& interface);
void listSensorsHandler(SCPI_Commands commands, SCPI_Parameters parameters, Stream& interface);
void getConfigHandler(SCPI_Commands commands, SCPI_Parameters parameters, Stream& interface);
void setBoardIdHandler(SCPI_Commands commands, SCPI_Parameters parameters, Stream& interface);
void updateConfigHandler(SCPI_Commands commands, SCPI_Parameters parameters, Stream& interface);

class CommunicationManager {
private:
    SCPI_Parser* scpiParser;
    SensorManager* sensorManager;
    ConfigManager* configManager;
    ErrorHandler* errorHandler;
    
    // Singleton instance for callback access
    static CommunicationManager* instance;
    
public:
    CommunicationManager(SensorManager* sensorMgr, ConfigManager* configMgr, ErrorHandler* err);
    ~CommunicationManager();
    
    void begin(long baudRate);
    void setupCommands();
    void processIncomingData();
    
    // Getter methods for use in callbacks
    static CommunicationManager* getInstance();
    SensorManager* getSensorManager();
    ConfigManager* getConfigManager();
    ErrorHandler* getErrorHandler();
};

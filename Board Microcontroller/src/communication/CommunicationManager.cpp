#include "CommunicationManager.h"
#include "VrekerSCPIWrapper.h" // Include the wrapper instead of directly including SCPI library
#include "../Constants.h"

// Initialize the singleton instance
CommunicationManager* CommunicationManager::instance = nullptr;

CommunicationManager::CommunicationManager(SensorManager* sensorMgr, ConfigManager* configMgr, ErrorHandler* err) :
    sensorManager(sensorMgr),
    configManager(configMgr),
    errorHandler(err) {
    instance = this;
    scpiParser = new SCPI_Parser();
}

CommunicationManager::~CommunicationManager() {
    delete scpiParser;
}

void CommunicationManager::begin(long baudRate) {
    errorHandler->logInfo("Communication manager initialized");
}

void CommunicationManager::setupCommands() {
    // Register SCPI commands with proper function pointers
    scpiParser->RegisterCommand(F("*IDN?"), idnHandler);
    scpiParser->RegisterCommand(F("MEAS:TEMP?"), measureTempHandler);
    scpiParser->RegisterCommand(F("MEAS:HUM?"), measureHumHandler);
    scpiParser->RegisterCommand(F("SYST:SENS:LIST?"), listSensorsHandler);
    scpiParser->RegisterCommand(F("SYST:CONF?"), getConfigHandler);
    scpiParser->RegisterCommand(F("SYST:CONF:BOARD:ID"), setBoardIdHandler);
    
    // Add a simple echo command for testing
    scpiParser->RegisterCommand(F("ECHO"), [](SCPI_Commands commands, SCPI_Parameters parameters, Stream& interface) {
        String message = parameters.Size() > 0 ? parameters[0] : "ECHO";
        interface.println("ECHO: " + message);
    });
    
    errorHandler->logInfo("SCPI commands registered");
}

void CommunicationManager::processIncomingData() {
    if (Serial.available()) {
        // Direct command handling for testing
        String rawCommand = Serial.readStringUntil('\n');
        rawCommand.trim();
        
        errorHandler->logInfo("Received raw command: '" + rawCommand + "'");
        
        if (rawCommand == "TEST") {
            Serial.println("Serial communication test successful");
            return;
        }
        
        // Manual command processing for key commands (bypass SCPI parser issues)
        if (rawCommand == "*IDN?") {
            String response = String(Constants::PRODUCT_NAME) + "," + 
                              configManager->getBoardIdentifier() + "," +
                              String(Constants::FIRMWARE_VERSION);
            errorHandler->logInfo("Sending IDN response: " + response);
            Serial.println(response);
            return;
        }
        
        if (rawCommand == "MEAS:TEMP?") {
            SensorReading reading = sensorManager->getLatestReading("I2C01");
            errorHandler->logInfo("Sending temperature: " + String(reading.temperature));
            Serial.println(reading.temperature);
            return;
        }
        
        if (rawCommand == "MEAS:HUM?") {
            SensorReading reading = sensorManager->getLatestReading("I2C01");
            errorHandler->logInfo("Sending humidity: " + String(reading.humidity));
            Serial.println(reading.humidity);
            return;
        }
    }
}

CommunicationManager* CommunicationManager::getInstance() {
    return instance;
}

SensorManager* CommunicationManager::getSensorManager() {
    return sensorManager;
}

ConfigManager* CommunicationManager::getConfigManager() {
    return configManager;
}

ErrorHandler* CommunicationManager::getErrorHandler() {
    return errorHandler;
}

// Global callback implementations
void idnHandler(SCPI_Commands commands, SCPI_Parameters parameters, Stream& interface) {
    auto mgr = CommunicationManager::getInstance();
    interface.println(String(Constants::PRODUCT_NAME) + "," + 
                     mgr->getConfigManager()->getBoardIdentifier() + "," +
                     String(Constants::FIRMWARE_VERSION));
}

void measureTempHandler(SCPI_Commands commands, SCPI_Parameters parameters, Stream& interface) {
    auto mgr = CommunicationManager::getInstance();
    String sensorName = parameters.Size() > 0 ? parameters[0] : "I2C01";
    SensorReading reading = mgr->getSensorManager()->getLatestReading(sensorName);
    interface.println(reading.temperature);
}

void measureHumHandler(SCPI_Commands commands, SCPI_Parameters parameters, Stream& interface) {
    auto mgr = CommunicationManager::getInstance();
    String sensorName = parameters.Size() > 0 ? parameters[0] : "I2C01";
    SensorReading reading = mgr->getSensorManager()->getLatestReading(sensorName);
    interface.println(reading.humidity);
}

void listSensorsHandler(SCPI_Commands commands, SCPI_Parameters parameters, Stream& interface) {
    auto mgr = CommunicationManager::getInstance();
    auto sensors = mgr->getSensorManager()->getSensors();
    for (auto sensor : sensors) {
        interface.println(sensor->getName() + "," + sensor->getType());
    }
}

void getConfigHandler(SCPI_Commands commands, SCPI_Parameters parameters, Stream& interface) {
    auto mgr = CommunicationManager::getInstance();
    interface.println(mgr->getConfigManager()->getConfigJson());
}

void setBoardIdHandler(SCPI_Commands commands, SCPI_Parameters parameters, Stream& interface) {
    auto mgr = CommunicationManager::getInstance();
    if (parameters.Size() > 0) {
        String newId = parameters[0];
        bool success = mgr->getConfigManager()->setBoardIdentifier(newId);
        interface.println(success ? "OK" : "ERROR");
    } else {
        interface.println("ERROR: No board ID specified");
    }
}
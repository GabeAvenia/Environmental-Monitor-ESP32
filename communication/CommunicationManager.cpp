#include "CommunicationManager.h"
#include "VrekerSCPIWrapper.h" // Include the wrapper instead of directly including SCPI library
#include "../Constants.h"
#include "../sensors/readings/TemperatureReading.h"
#include "../sensors/readings/HumidityReading.h"

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
    scpiParser->RegisterCommand(F("SYST:CONF:UPDATE"), updateConfigHandler);
    
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
        
        if (rawCommand.startsWith("MEAS:TEMP?")) {
            // Extract sensor name if provided
            String sensorName = "I2C01"; // Default sensor
            int spacePos = rawCommand.indexOf(' ');
            if (spacePos > 0) {
                sensorName = rawCommand.substring(spacePos + 1);
            }
            
            TemperatureReading reading = sensorManager->getTemperature(sensorName);
            errorHandler->logInfo("Sending temperature: " + String(reading.value) + " for sensor " + sensorName);
            Serial.println(reading.valid ? String(reading.value) : "ERROR");
            return;
        }
        
        if (rawCommand.startsWith("MEAS:HUM?")) {
            // Extract sensor name if provided
            String sensorName = "I2C01"; // Default sensor
            int spacePos = rawCommand.indexOf(' ');
            if (spacePos > 0) {
                sensorName = rawCommand.substring(spacePos + 1);
            }
            
            HumidityReading reading = sensorManager->getHumidity(sensorName);
            errorHandler->logInfo("Sending humidity: " + String(reading.value) + " for sensor " + sensorName);
            Serial.println(reading.valid ? String(reading.value) : "ERROR");
            return;
        }
        
        if (rawCommand == "SYST:SENS:LIST?") {
            auto registry = sensorManager->getRegistry();
            auto sensors = registry.getAllSensors();
            
            for (auto sensor : sensors) {
                String capabilities = "";
                if (dynamic_cast<ITemperatureSensor*>(sensor)) capabilities += "T";
                if (dynamic_cast<IHumiditySensor*>(sensor)) capabilities += "H";
                if (dynamic_cast<IPressureSensor*>(sensor)) capabilities += "P";
                if (dynamic_cast<ICO2Sensor*>(sensor)) capabilities += "C";
                
                Serial.println(sensor->getName() + "," + 
                              dynamic_cast<BaseSensor*>(sensor)->getTypeString() + "," +
                              capabilities + "," +
                              (sensor->isConnected() ? "CONNECTED" : "DISCONNECTED"));
            }
            return;
        }
        
        if (rawCommand == "SYST:CONF?") {
            String config = configManager->getConfigJson();
            Serial.println(config);
            return;
        }
        
        // Try to handle with SCPI parser
        scpiParser->ProcessInput(rawCommand, Serial);
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
    
    TemperatureReading reading = mgr->getSensorManager()->getTemperature(sensorName);
    interface.println(reading.valid ? String(reading.value) : "ERROR");
}

void measureHumHandler(SCPI_Commands commands, SCPI_Parameters parameters, Stream& interface) {
    auto mgr = CommunicationManager::getInstance();
    String sensorName = parameters.Size() > 0 ? parameters[0] : "I2C01";
    
    HumidityReading reading = mgr->getSensorManager()->getHumidity(sensorName);
    interface.println(reading.valid ? String(reading.value) : "ERROR");
}

void listSensorsHandler(SCPI_Commands commands, SCPI_Parameters parameters, Stream& interface) {
    auto mgr = CommunicationManager::getInstance();
    auto registry = mgr->getSensorManager()->getRegistry();
    auto sensors = registry.getAllSensors();
    
    for (auto sensor : sensors) {
        String capabilities = "";
        if (dynamic_cast<ITemperatureSensor*>(sensor)) capabilities += "T";
        if (dynamic_cast<IHumiditySensor*>(sensor)) capabilities += "H";
        if (dynamic_cast<IPressureSensor*>(sensor)) capabilities += "P";
        if (dynamic_cast<ICO2Sensor*>(sensor)) capabilities += "C";
        
        interface.println(sensor->getName() + "," + 
                         dynamic_cast<BaseSensor*>(sensor)->getTypeString() + "," +
                         capabilities + "," +
                         (sensor->isConnected() ? "CONNECTED" : "DISCONNECTED"));
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

void updateConfigHandler(SCPI_Commands commands, SCPI_Parameters parameters, Stream& interface) {
    auto mgr = CommunicationManager::getInstance();
    if (parameters.Size() > 0) {
        String newConfig = parameters[0];
        bool success = mgr->getConfigManager()->updateConfigFromJson(newConfig);
        interface.println(success ? "OK" : "ERROR");
    } else {
        interface.println("ERROR: No configuration provided");
    }
}

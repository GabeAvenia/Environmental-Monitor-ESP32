#include "CommunicationManager.h"
#include "VrekerSCPIWrapper.h" // Include the wrapper instead of directly including SCPI library
#include "../Constants.h"
#include "../sensors/readings/TemperatureReading.h"
#include "../sensors/readings/HumidityReading.h"
#include "../sensors/interfaces/InterfaceTypes.h"

// Initialize the singleton instance
CommunicationManager* CommunicationManager::instance = nullptr;

CommunicationManager::CommunicationManager(SensorManager* sensorMgr, ConfigManager* configMgr, ErrorHandler* err) :
    sensorManager(sensorMgr),
    configManager(configMgr),
    errorHandler(err),
    isStreaming(false),
    lastStreamTime(0),
    streamInterval(1000),
    verboseLogging(false) {
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
    
    // Register streaming commands
    scpiParser->RegisterCommand(F("STREAM:START"), streamStartHandler);
    scpiParser->RegisterCommand(F("STREAM:STOP"), streamStopHandler);
    scpiParser->RegisterCommand(F("STREAM:STATUS?"), streamStatusHandler);
    
    // Register logging control command
    scpiParser->RegisterCommand(F("SYST:LOG:VERB"), verboseLogHandler);
    
    // Add a simple echo command for testing
    scpiParser->RegisterCommand(F("ECHO"), [](SCPI_Commands commands, SCPI_Parameters parameters, Stream& interface) {
        String message = parameters.Size() > 0 ? String(parameters[0]) : "ECHO";
        interface.println("ECHO: " + message);
    });
    
    errorHandler->logInfo("SCPI commands registered");
}

void CommunicationManager::processIncomingData() {
    // Check if Serial is connected
    if (!Serial) {
        if (isStreaming) {
            // If Serial is disconnected, stop streaming
            stopStreaming();
        }
        return;
    }
    
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
                if (sensor->supportsInterface(InterfaceType::TEMPERATURE)) capabilities += "T";
                if (sensor->supportsInterface(InterfaceType::HUMIDITY)) capabilities += "H";
                if (sensor->supportsInterface(InterfaceType::PRESSURE)) capabilities += "P";
                if (sensor->supportsInterface(InterfaceType::CO2)) capabilities += "C";
                
                Serial.println(sensor->getName() + "," + 
                              sensor->getTypeString() + "," +
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
        
        // Handle streaming commands manually
        if (rawCommand.startsWith("STREAM:START")) {
            std::vector<String> sensorNames;
            
            // Parse parameters - now only sensor names, no interval
            String paramsStr = rawCommand.substring(12);
            paramsStr.trim(); // trim() modifies the string in-place
            
            if (paramsStr.length() > 0) {
                // Split space-separated sensor names
                while (paramsStr.length() > 0) {
                    int spacePos = paramsStr.indexOf(' ');
                    if (spacePos > 0) {
                        sensorNames.push_back(paramsStr.substring(0, spacePos));
                        paramsStr = paramsStr.substring(spacePos + 1);
                    } else {
                        sensorNames.push_back(paramsStr);
                        paramsStr = "";
                    }
                }
            }
            
            // If no sensors specified, use all available
            if (sensorNames.empty()) {
                auto registry = sensorManager->getRegistry();
                auto allSensors = registry.getAllSensors();
                
                for (auto sensor : allSensors) {
                    sensorNames.push_back(sensor->getName());
                }
            }
            
            bool success = startStreaming(sensorNames);
            Serial.println(success ? "OK" : "ERROR");
            return;
        }
        
        if (rawCommand == "STREAM:STOP") {
            stopStreaming();
            Serial.println("OK");
            return;
        }
        
        if (rawCommand == "STREAM:STATUS?") {
            Serial.println(isStreaming ? "RUNNING" : "STOPPED");
            return;
        }
        
        // Handle verbose logging toggle
        if (rawCommand.startsWith("SYST:LOG:VERB")) {
            bool enableVerbose = false;
            
            // Parse parameter
            if (rawCommand.length() > 13) { // Length of "SYST:LOG:VERB" is 13
                String param = rawCommand.substring(14);
                param.trim(); // trim() modifies the string in-place and returns void
                enableVerbose = (param == "ON" || param == "1");
            }
            
            setVerboseLogging(enableVerbose);
            Serial.println("OK");
            return;
        }
        
        // For SCPI_Parser, we need to use the correct function
        char buff[rawCommand.length() + 1];
        rawCommand.toCharArray(buff, rawCommand.length() + 1);
        scpiParser->ProcessInput(Serial, buff);
    }
    
    // Handle streaming if active
    handleStreaming();
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

void CommunicationManager::setVerboseLogging(bool enable) {
    verboseLogging = enable;
    errorHandler->logInfo(enable ? "Verbose logging enabled" : "Verbose logging disabled");
}

bool CommunicationManager::isVerboseLogging() const {
    return verboseLogging;
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
    String sensorName = parameters.Size() > 0 ? String(parameters[0]) : "I2C01";
    
    TemperatureReading reading = mgr->getSensorManager()->getTemperature(sensorName);
    interface.println(reading.valid ? String(reading.value) : "ERROR");
}

void measureHumHandler(SCPI_Commands commands, SCPI_Parameters parameters, Stream& interface) {
    auto mgr = CommunicationManager::getInstance();
    String sensorName = parameters.Size() > 0 ? String(parameters[0]) : "I2C01";
    
    HumidityReading reading = mgr->getSensorManager()->getHumidity(sensorName);
    interface.println(reading.valid ? String(reading.value) : "ERROR");
}

void listSensorsHandler(SCPI_Commands commands, SCPI_Parameters parameters, Stream& interface) {
    auto mgr = CommunicationManager::getInstance();
    auto registry = mgr->getSensorManager()->getRegistry();
    auto sensors = registry.getAllSensors();
    
    for (auto sensor : sensors) {
        String capabilities = "";
        if (sensor->supportsInterface(InterfaceType::TEMPERATURE)) capabilities += "T";
        if (sensor->supportsInterface(InterfaceType::HUMIDITY)) capabilities += "H";
        if (sensor->supportsInterface(InterfaceType::PRESSURE)) capabilities += "P";
        if (sensor->supportsInterface(InterfaceType::CO2)) capabilities += "C";
        
        interface.println(sensor->getName() + "," + 
                         sensor->getTypeString() + "," +
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
        String newId = String(parameters[0]);
        bool success = mgr->getConfigManager()->setBoardIdentifier(newId);
        interface.println(success ? "OK" : "ERROR");
    } else {
        interface.println("ERROR: No board ID specified");
    }
}

void updateConfigHandler(SCPI_Commands commands, SCPI_Parameters parameters, Stream& interface) {
    auto mgr = CommunicationManager::getInstance();
    if (parameters.Size() > 0) {
        String newConfig = String(parameters[0]);
        bool success = mgr->getConfigManager()->updateConfigFromJson(newConfig);
        interface.println(success ? "OK" : "ERROR");
    } else {
        interface.println("ERROR: No configuration provided");
    }
}

// Streaming command handlers
void streamStartHandler(SCPI_Commands commands, SCPI_Parameters parameters, Stream& interface) {
    auto mgr = CommunicationManager::getInstance();
    
    // Default to all sensors if none specified
    std::vector<String> sensors;
    
    // Get sensor names if specified
    for (size_t i = 0; i < parameters.Size(); i++) {
        sensors.push_back(String(parameters[i]));
    }
    
    if (sensors.empty()) {
        // If no sensors specified, use all available sensors
        auto registry = mgr->getSensorManager()->getRegistry();
        auto allSensors = registry.getAllSensors();
        
        for (auto sensor : allSensors) {
            sensors.push_back(sensor->getName());
        }
    }
    
    // Start streaming - no interval parameter needed anymore
    bool success = mgr->startStreaming(sensors);
    interface.println(success ? "OK" : "ERROR");
}

void streamStopHandler(SCPI_Commands commands, SCPI_Parameters parameters, Stream& interface) {
    auto mgr = CommunicationManager::getInstance();
    mgr->stopStreaming();
    interface.println("OK");
}

void streamStatusHandler(SCPI_Commands commands, SCPI_Parameters parameters, Stream& interface) {
    auto mgr = CommunicationManager::getInstance();
    interface.println(mgr->isCurrentlyStreaming() ? "RUNNING" : "STOPPED");
}

void verboseLogHandler(SCPI_Commands commands, SCPI_Parameters parameters, Stream& interface) {
    auto mgr = CommunicationManager::getInstance();
    
    bool enableVerbose = false;
    if (parameters.Size() > 0) {
        String param = String(parameters[0]);
        enableVerbose = (param.equalsIgnoreCase("ON") || param == "1");
    }
    
    mgr->setVerboseLogging(enableVerbose);
    interface.println("OK");
}

// Streaming methods implementation
bool CommunicationManager::startStreaming(const std::vector<String>& sensorNames) {
    if (sensorNames.empty()) {
        errorHandler->logInfo("Cannot start streaming: no sensors specified");
        return false;
    }
    
    // Validate all sensors exist and find the fastest polling rate
    bool allValid = true;
    uint32_t fastestRate = UINT32_MAX;
    
    auto configs = configManager->getSensorConfigs();
    
    for (const auto& name : sensorNames) {
        if (!sensorManager->findSensor(name)) {
            errorHandler->logInfo("Cannot start streaming: sensor not found: " + name);
            allValid = false;
            continue;
        }
        
        // Find this sensor's config to get its polling rate
        for (const auto& config : configs) {
            if (config.name == name) {
                // Update fastest rate if this one is faster
                if (config.pollingRate < fastestRate) {
                    fastestRate = config.pollingRate;
                }
                break;
            }
        }
    }
    
    if (!allValid) {
        return false;
    }
    
    // Sanity check for polling rate
    if (fastestRate == UINT32_MAX || fastestRate < 100) {
        fastestRate = 1000; // Default to 1 second if couldn't determine or too fast
    }
    
    // Set streaming parameters
    streamingSensors = sensorNames;
    streamInterval = fastestRate;
    lastStreamTime = 0; // Force immediate update
    isStreaming = true;
    
    if (verboseLogging) {
        Serial.println("[INFO] Started streaming " + String(streamingSensors.size()) + 
                    " sensors at " + String(streamInterval) + "ms interval");
    }
    errorHandler->logInfo("Started streaming " + String(streamingSensors.size()) + 
                       " sensors at " + String(streamInterval) + "ms interval");
    return true;
}

void CommunicationManager::stopStreaming() {
    if (isStreaming) {
        isStreaming = false;
        streamingSensors.clear();
        if (verboseLogging) {
            Serial.println("[INFO] Stopped streaming");
        }
        errorHandler->logInfo("Stopped streaming");
    }
}

bool CommunicationManager::isCurrentlyStreaming() const {
    return isStreaming;
}

void CommunicationManager::handleStreaming() {
    if (!isStreaming || !Serial) {
        // If we're not streaming or serial is disconnected, stop streaming
        if (isStreaming) {
            stopStreaming();
        }
        return;
    }
    
    unsigned long currentTime = millis();
    if (currentTime - lastStreamTime >= streamInterval) {
        lastStreamTime = currentTime;
        
        // Stream data for each sensor
        for (const auto& sensorName : streamingSensors) {
            ISensor* sensor = sensorManager->findSensor(sensorName);
            if (!sensor || !sensor->isConnected()) {
                continue;
            }
            
            String output = sensorName + ",";
            
            // Add temperature if available
            if (sensor->supportsInterface(InterfaceType::TEMPERATURE)) {
                TemperatureReading tempReading = sensorManager->getTemperature(sensorName);
                output += tempReading.valid ? String(tempReading.value) : "NA";
            } else {
                output += "NA";
            }
            
            output += ",";
            
            // Add humidity if available
            if (sensor->supportsInterface(InterfaceType::HUMIDITY)) {
                HumidityReading humReading = sensorManager->getHumidity(sensorName);
                output += humReading.valid ? String(humReading.value) : "NA";
            } else {
                output += "NA";
            }
            
            // Send data
            Serial.println(output);
        }
    }
}
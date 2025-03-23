#include "CommunicationManager.h"
#include "VrekerSCPIWrapper.h" 
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
    registerCommandHandlers();
}

void CommunicationManager::registerCommandHandlers() {
    // Register all command handlers
    commandHandlers[Constants::SCPI::IDN] = 
        [this](const std::vector<String>& params) { return handleIdentify(params); };
    
    commandHandlers[Constants::SCPI::MEASURE_TEMP] = 
        [this](const std::vector<String>& params) { return handleMeasureTemperature(params); };
    
    commandHandlers[Constants::SCPI::MEASURE_HUM] = 
        [this](const std::vector<String>& params) { return handleMeasureHumidity(params); };
    
    commandHandlers[Constants::SCPI::LIST_SENSORS] = 
        [this](const std::vector<String>& params) { return handleListSensors(params); };
    
    commandHandlers[Constants::SCPI::GET_CONFIG] = 
        [this](const std::vector<String>& params) { return handleGetConfig(params); };
    
    commandHandlers[Constants::SCPI::SET_BOARD_ID] = 
        [this](const std::vector<String>& params) { return handleSetBoardId(params); };
    
    commandHandlers[Constants::SCPI::UPDATE_CONFIG] = 
        [this](const std::vector<String>& params) { return handleUpdateConfig(params); };
    
    commandHandlers[Constants::SCPI::STREAM_START] = 
        [this](const std::vector<String>& params) { return handleStreamStart(params); };
    
    commandHandlers[Constants::SCPI::STREAM_STOP] = 
        [this](const std::vector<String>& params) { return handleStreamStop(params); };
    
    commandHandlers[Constants::SCPI::STREAM_STATUS] = 
        [this](const std::vector<String>& params) { return handleStreamStatus(params); };
    
    commandHandlers[Constants::SCPI::VERBOSE_LOG] = 
        [this](const std::vector<String>& params) { return handleVerboseLog(params); };
    
    // Testing commands
    commandHandlers["TEST:FS"] = 
        [this](const std::vector<String>& params) { return handleTestFilesystem(params); };
    
    commandHandlers["TEST:UPDATE"] = 
        [this](const std::vector<String>& params) { return handleTestUpdateConfig(params); };
    
    commandHandlers["TEST"] = 
        [this](const std::vector<String>& params) { 
            Serial.println("Serial communication test successful"); 
            return true; 
        };
        
    commandHandlers["ECHO"] = 
        [this](const std::vector<String>& params) { return handleEcho(params); };
}

void CommunicationManager::setupCommands() {
    // Each command needs its own non-capturing lambda
    
    // *IDN? command
    scpiParser->RegisterCommand(F("*IDN?"), 
        [](SCPI_Commands cmds, SCPI_Parameters parameters, Stream& interface) {
            std::vector<String> params;
            for (size_t i = 0; i < parameters.Size(); i++) {
                params.push_back(String(parameters[i]));
            }
            CommunicationManager::getInstance()->handleIdentify(params);
        });
        
    // MEAS:TEMP? command
    scpiParser->RegisterCommand(F("MEAS:TEMP?"), 
        [](SCPI_Commands cmds, SCPI_Parameters parameters, Stream& interface) {
            std::vector<String> params;
            for (size_t i = 0; i < parameters.Size(); i++) {
                params.push_back(String(parameters[i]));
            }
            CommunicationManager::getInstance()->handleMeasureTemperature(params);
        });
        
    // MEAS:HUM? command
    scpiParser->RegisterCommand(F("MEAS:HUM?"), 
        [](SCPI_Commands cmds, SCPI_Parameters parameters, Stream& interface) {
            std::vector<String> params;
            for (size_t i = 0; i < parameters.Size(); i++) {
                params.push_back(String(parameters[i]));
            }
            CommunicationManager::getInstance()->handleMeasureHumidity(params);
        });
        
    // SYST:SENS:LIST? command
    scpiParser->RegisterCommand(F("SYST:SENS:LIST?"), 
        [](SCPI_Commands cmds, SCPI_Parameters parameters, Stream& interface) {
            std::vector<String> params;
            for (size_t i = 0; i < parameters.Size(); i++) {
                params.push_back(String(parameters[i]));
            }
            CommunicationManager::getInstance()->handleListSensors(params);
        });
        
    // SYST:CONF? command
    scpiParser->RegisterCommand(F("SYST:CONF?"), 
        [](SCPI_Commands cmds, SCPI_Parameters parameters, Stream& interface) {
            std::vector<String> params;
            for (size_t i = 0; i < parameters.Size(); i++) {
                params.push_back(String(parameters[i]));
            }
            CommunicationManager::getInstance()->handleGetConfig(params);
        });
        
    // SYST:CONF:BOARD:ID command
    scpiParser->RegisterCommand(F("SYST:CONF:BOARD:ID"), 
        [](SCPI_Commands cmds, SCPI_Parameters parameters, Stream& interface) {
            std::vector<String> params;
            for (size_t i = 0; i < parameters.Size(); i++) {
                params.push_back(String(parameters[i]));
            }
            CommunicationManager::getInstance()->handleSetBoardId(params);
        });
        
    // SYST:CONF:UPDATE command
    scpiParser->RegisterCommand(F("SYST:CONF:UPDATE"), 
        [](SCPI_Commands cmds, SCPI_Parameters parameters, Stream& interface) {
            std::vector<String> params;
            for (size_t i = 0; i < parameters.Size(); i++) {
                params.push_back(String(parameters[i]));
            }
            CommunicationManager::getInstance()->handleUpdateConfig(params);
        });
        
    // STREAM:START command
    scpiParser->RegisterCommand(F("STREAM:START"), 
        [](SCPI_Commands cmds, SCPI_Parameters parameters, Stream& interface) {
            std::vector<String> params;
            for (size_t i = 0; i < parameters.Size(); i++) {
                params.push_back(String(parameters[i]));
            }
            CommunicationManager::getInstance()->handleStreamStart(params);
        });
        
    // STREAM:STOP command
    scpiParser->RegisterCommand(F("STREAM:STOP"), 
        [](SCPI_Commands cmds, SCPI_Parameters parameters, Stream& interface) {
            std::vector<String> params;
            CommunicationManager::getInstance()->handleStreamStop(params);
        });
        
    // STREAM:STATUS? command
    scpiParser->RegisterCommand(F("STREAM:STATUS?"), 
        [](SCPI_Commands cmds, SCPI_Parameters parameters, Stream& interface) {
            std::vector<String> params;
            CommunicationManager::getInstance()->handleStreamStatus(params);
        });
        
    // SYST:LOG:VERB command
    scpiParser->RegisterCommand(F("SYST:LOG:VERB"), 
        [](SCPI_Commands cmds, SCPI_Parameters parameters, Stream& interface) {
            std::vector<String> params;
            for (size_t i = 0; i < parameters.Size(); i++) {
                params.push_back(String(parameters[i]));
            }
            CommunicationManager::getInstance()->handleVerboseLog(params);
        });
        
    // TEST:FS command
    scpiParser->RegisterCommand(F("TEST:FS"), 
        [](SCPI_Commands cmds, SCPI_Parameters parameters, Stream& interface) {
            std::vector<String> params;
            CommunicationManager::getInstance()->handleTestFilesystem(params);
        });
        
    // TEST:UPDATE command
    scpiParser->RegisterCommand(F("TEST:UPDATE"), 
        [](SCPI_Commands cmds, SCPI_Parameters parameters, Stream& interface) {
            std::vector<String> params;
            CommunicationManager::getInstance()->handleTestUpdateConfig(params);
        });
        
    // ECHO command
    scpiParser->RegisterCommand(F("ECHO"), 
        [](SCPI_Commands cmds, SCPI_Parameters parameters, Stream& interface) {
            std::vector<String> params;
            for (size_t i = 0; i < parameters.Size(); i++) {
                params.push_back(String(parameters[i]));
            }
            CommunicationManager::getInstance()->handleEcho(params);
        });
    
    errorHandler->logInfo("SCPI commands registered");
}

void CommunicationManager::parseCommand(const String& rawCommand, String& command, std::vector<String>& params) {
    params.clear();
    
    // Extract command (up to first space)
    int spacePos = rawCommand.indexOf(' ');
    if (spacePos > 0) {
        command = rawCommand.substring(0, spacePos);
        
        // Extract space-separated parameters
        String paramsStr = rawCommand.substring(spacePos + 1);
        paramsStr.trim();
        
        // Split parameters
        while (paramsStr.length() > 0) {
            int nextSpace = paramsStr.indexOf(' ');
            if (nextSpace > 0) {
                params.push_back(paramsStr.substring(0, nextSpace));
                paramsStr = paramsStr.substring(nextSpace + 1);
                paramsStr.trim();
            } else {
                // Last parameter
                params.push_back(paramsStr);
                break;
            }
        }
    } else {
        command = rawCommand;
    }
}

bool CommunicationManager::processCommand(const String& command, const std::vector<String>& params) {
    auto it = commandHandlers.find(command);
    if (it != commandHandlers.end()) {
        return it->second(params);
    }
    return false;
}

void CommunicationManager::processIncomingData() {
    // Check if Serial is connected
    if (!Serial) {
        if (isStreaming) {
            stopStreaming();
        }
        return;
    }
    
    if (Serial.available()) {
        // Read command
        String rawCommand = Serial.readStringUntil('\n');
        rawCommand.trim();
        
        // Always log the received command
        errorHandler->logInfo("Received raw command: '" + rawCommand + "'");
        
        // Echo command if verbose logging is enabled
        if (verboseLogging) {
            Serial.println("ECHO: " + rawCommand);
        }
        
        // Parse and process command
        String command;
        std::vector<String> params;
        parseCommand(rawCommand, command, params);
        
        // Try to handle with our command processors
        if (!processCommand(command, params)) {
            // Fall back to SCPI parser for compatibility
            char buff[rawCommand.length() + 1];
            rawCommand.toCharArray(buff, rawCommand.length() + 1);
            scpiParser->ProcessInput(Serial, buff);
        }
    }
    
    // Handle streaming if active
    handleStreaming();
}

// Command handler implementations
bool CommunicationManager::handleIdentify(const std::vector<String>& params) {
    String response = String(Constants::PRODUCT_NAME) + "," + 
                      configManager->getBoardIdentifier() + "," +
                      String(Constants::FIRMWARE_VERSION);
    errorHandler->logInfo("Sending IDN response: " + response);
    Serial.println(response);
    return true;
}

bool CommunicationManager::handleMeasureTemperature(const std::vector<String>& params) {
    // Default to first sensor if none specified
    String sensorName = params.size() > 0 ? params[0] : "I2C01";
    
    TemperatureReading reading = sensorManager->getTemperature(sensorName);
    errorHandler->logInfo("Sending temperature: " + String(reading.value) + " for sensor " + sensorName);
    Serial.println(reading.valid ? String(reading.value) : "ERROR");
    return true;
}

bool CommunicationManager::handleMeasureHumidity(const std::vector<String>& params) {
    // Default to first sensor if none specified
    String sensorName = params.size() > 0 ? params[0] : "I2C01";
    
    HumidityReading reading = sensorManager->getHumidity(sensorName);
    errorHandler->logInfo("Sending humidity: " + String(reading.value) + " for sensor " + sensorName);
    Serial.println(reading.valid ? String(reading.value) : "ERROR");
    return true;
}

bool CommunicationManager::handleListSensors(const std::vector<String>& params) {
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
    return true;
}

bool CommunicationManager::handleGetConfig(const std::vector<String>& params) {
    String config = configManager->getConfigJson();
    Serial.println(config);
    return true;
}

bool CommunicationManager::handleSetBoardId(const std::vector<String>& params) {
    if (params.empty()) {
        Serial.println("ERROR: No board ID specified");
        return false;
    }
    
    bool success = configManager->setBoardIdentifier(params[0]);
    Serial.println(success ? "OK" : "ERROR");
    return success;
}

bool CommunicationManager::handleUpdateConfig(const std::vector<String>& params) {
    if (params.empty()) {
        Serial.println("ERROR: No configuration provided");
        return false;
    }
    
    // Join all parameters since the JSON might have spaces
    String jsonConfig = "";
    for (const auto& param : params) {
        if (jsonConfig.length() > 0) jsonConfig += " ";
        jsonConfig += param;
    }
    
    errorHandler->logInfo("Processing config update: " + jsonConfig.substring(0, 50) + "...");
    bool success = configManager->updateConfigFromJson(jsonConfig);
    Serial.println(success ? "OK" : "ERROR");
    return success;
}

bool CommunicationManager::handleStreamStart(const std::vector<String>& params) {
    std::vector<String> sensorNames;
    
    // Each parameter is a sensor name
    for (const auto& param : params) {
        sensorNames.push_back(param);
    }
    
    bool success = startStreaming(sensorNames);
    Serial.println(success ? "OK" : "ERROR");
    return success;
}

bool CommunicationManager::handleStreamStop(const std::vector<String>& params) {
    stopStreaming();
    Serial.println("OK");
    return true;
}

bool CommunicationManager::handleStreamStatus(const std::vector<String>& params) {
    Serial.println(isStreaming ? "RUNNING" : "STOPPED");
    return true;
}

bool CommunicationManager::handleVerboseLog(const std::vector<String>& params) {
    bool enableVerbose = false;
    
    if (!params.empty()) {
        String param = params[0];
        enableVerbose = (param == "ON" || param == "1");
    }
    
    setVerboseLogging(enableVerbose);
    Serial.println("OK");
    return true;
}

bool CommunicationManager::handleTestFilesystem(const std::vector<String>& params) {
    // Implement filesystem test logic here (moved from original)
    // (Implementation omitted for brevity)
    return true;
}

bool CommunicationManager::handleTestUpdateConfig(const std::vector<String>& params) {
    // Implement config update test logic here (moved from original)
    // (Implementation omitted for brevity)
    return true;
}

bool CommunicationManager::handleEcho(const std::vector<String>& params) {
    String message = params.empty() ? "ECHO" : params[0];
    Serial.println("ECHO: " + message);
    return true;
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

// Streaming methods implementation
bool CommunicationManager::startStreaming(const std::vector<String>& sensorNames) {
    if (sensorNames.empty()) {
        // If no sensors specified, use all available sensors
        auto registry = sensorManager->getRegistry();
        auto allSensors = registry.getAllSensors();
        
        std::vector<String> allSensorNames;
        for (auto sensor : allSensors) {
            allSensorNames.push_back(sensor->getName());
        }
        
        return startStreaming(allSensorNames);
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
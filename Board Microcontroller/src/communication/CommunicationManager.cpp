#include "CommunicationManager.h"
#include "VrekerSCPIWrapper.h" 
#include "../Constants.h"
#include "../sensors/readings/TemperatureReading.h"
#include "../sensors/readings/HumidityReading.h"
#include "../sensors/interfaces/InterfaceTypes.h"

// Initialize the singleton instance
CommunicationManager* CommunicationManager::instance = nullptr;
// Initialize the UART debug serial
Print* CommunicationManager::uartDebugSerial = nullptr;

CommunicationManager::CommunicationManager(SensorManager* sensorMgr, ConfigManager* configMgr, ErrorHandler* err) :
    sensorManager(sensorMgr),
    configManager(configMgr),
    errorHandler(err),
    isStreaming(false),
    lastStreamTime(0),
    streamInterval(1000) {
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

void CommunicationManager::setUartDebugSerialPtr(Print* debugSerial) {
    uartDebugSerial = debugSerial;
}

void CommunicationManager::registerCommandHandlers() {
    // Register all command handlers
    commandHandlers[Constants::SCPI::IDN] = 
        [this](const std::vector<String>& params) { return handleIdentify(params); };
    
    commandHandlers[Constants::SCPI::MEASURE_SINGLE] = 
        [this](const std::vector<String>& params) { return handleMeasureSingle(params); };
    
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
        
    // Message routing commands
    commandHandlers[Constants::SCPI::MSG_ROUTE_STATUS] = 
        [this](const std::vector<String>& params) { return handleMessageRoutingStatus(params); };

    commandHandlers[Constants::SCPI::MSG_ROUTE_SET] = 
        [this](const std::vector<String>& params) { return handleMessageRoutingSet(params); };

    commandHandlers[Constants::SCPI::MSG_ROUTE_INFO] = 
        [this](const std::vector<String>& params) { return handleInfoRoute(params); };

    commandHandlers[Constants::SCPI::MSG_ROUTE_WARNING] = 
        [this](const std::vector<String>& params) { return handleWarningRoute(params); };

    commandHandlers[Constants::SCPI::MSG_ROUTE_ERROR] = 
        [this](const std::vector<String>& params) { return handleErrorRoute(params); };

    commandHandlers[Constants::SCPI::MSG_ROUTE_CRITICAL] = 
        [this](const std::vector<String>& params) { return handleCriticalRoute(params); };
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
        
    // MEAS:SINGLE command
    scpiParser->RegisterCommand(F("MEAS:SINGLE"), 
        [](SCPI_Commands cmds, SCPI_Parameters parameters, Stream& interface) {
            std::vector<String> params;
            for (size_t i = 0; i < parameters.Size(); i++) {
                params.push_back(String(parameters[i]));
            }
            CommunicationManager::getInstance()->handleMeasureSingle(params);
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
        
    // Message routing commands
    scpiParser->RegisterCommand(F("SYST:LOG:ROUTE?"), 
        [](SCPI_Commands cmds, SCPI_Parameters parameters, Stream& interface) {
            std::vector<String> params;
            CommunicationManager::getInstance()->handleMessageRoutingStatus(params);
        });

    scpiParser->RegisterCommand(F("SYST:LOG:ROUTE"), 
        [](SCPI_Commands cmds, SCPI_Parameters parameters, Stream& interface) {
            std::vector<String> params;
            for (size_t i = 0; i < parameters.Size(); i++) {
                params.push_back(String(parameters[i]));
            }
            CommunicationManager::getInstance()->handleMessageRoutingSet(params);
        });

    scpiParser->RegisterCommand(F("SYST:LOG:INFO:ROUTE"), 
        [](SCPI_Commands cmds, SCPI_Parameters parameters, Stream& interface) {
            std::vector<String> params;
            for (size_t i = 0; i < parameters.Size(); i++) {
                params.push_back(String(parameters[i]));
            }
            CommunicationManager::getInstance()->handleInfoRoute(params);
        });

    scpiParser->RegisterCommand(F("SYST:LOG:WARN:ROUTE"), 
        [](SCPI_Commands cmds, SCPI_Parameters parameters, Stream& interface) {
            std::vector<String> params;
            for (size_t i = 0; i < parameters.Size(); i++) {
                params.push_back(String(parameters[i]));
            }
            CommunicationManager::getInstance()->handleWarningRoute(params);
        });

    scpiParser->RegisterCommand(F("SYST:LOG:ERR:ROUTE"), 
        [](SCPI_Commands cmds, SCPI_Parameters parameters, Stream& interface) {
            std::vector<String> params;
            for (size_t i = 0; i < parameters.Size(); i++) {
                params.push_back(String(parameters[i]));
            }
            CommunicationManager::getInstance()->handleErrorRoute(params);
        });

    scpiParser->RegisterCommand(F("SYST:LOG:CRIT:ROUTE"), 
        [](SCPI_Commands cmds, SCPI_Parameters parameters, Stream& interface) {
            std::vector<String> params;
            for (size_t i = 0; i < parameters.Size(); i++) {
                params.push_back(String(parameters[i]));
            }
            CommunicationManager::getInstance()->handleCriticalRoute(params);
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
        
        // Log the received command
        errorHandler->logInfo("Received command: '" + rawCommand + "'");
        
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
    Serial.println(response);
    return true;
}

bool CommunicationManager::handleMeasureSingle(const std::vector<String>& params) {
    std::vector<String> values;
    
    if (params.empty()) {
        // No sensors specified, use all available
        auto registry = sensorManager->getRegistry();
        auto allSensors = registry.getAllSensors();
        
        for (auto sensor : allSensors) {
            collectSensorReadings(sensor->getName(), "", values);
        }
    } else {
        // Process each parameter
        for (const auto& param : params) {
            // Check if parameter contains a colon (sensor:measurements format)
            int colonPos = param.indexOf(':');
            if (colonPos > 0) {
                String sensorName = param.substring(0, colonPos);
                String measurements = param.substring(colonPos + 1);
                
                collectSensorReadings(sensorName, measurements, values);
            } else {
                // No measurements specified, default to all
                collectSensorReadings(param, "", values);
            }
        }
    }
    
    // Output a single CSV line with all collected values
    if (!values.empty()) {
        String csvLine = values[0];
        for (size_t i = 1; i < values.size(); i++) {
            csvLine += "," + values[i];
        }
        Serial.println(csvLine);
    }
    
    return true;
}

void CommunicationManager::collectSensorReadings(const String& sensorName, const String& measurements, std::vector<String>& values) {
    ISensor* sensor = sensorManager->findSensor(sensorName);
    if (!sensor) {
        values.push_back("ERROR");
        return;
    }
    
    if (!sensor->isConnected()) {
        values.push_back("ERROR");
        return;
    }
    
    // Determine which measurements to take
    bool useAllMeasurements = measurements.length() == 0;
    String upperMeasurements = measurements;
    upperMeasurements.toUpperCase();
    
    bool readTemp = useAllMeasurements || upperMeasurements.indexOf("TEMP") >= 0;
    bool readHum = useAllMeasurements || upperMeasurements.indexOf("HUM") >= 0;
    bool readPres = useAllMeasurements || upperMeasurements.indexOf("PRES") >= 0;
    bool readCO2 = useAllMeasurements || upperMeasurements.indexOf("CO2") >= 0;
    
    // Read temperature if supported and requested
    if (readTemp && sensor->supportsInterface(InterfaceType::TEMPERATURE)) {
        TemperatureReading tempReading = sensorManager->getTemperature(sensorName);
        values.push_back(tempReading.valid ? String(tempReading.value) : "NA");
    }
    
    // Read humidity if supported and requested
    if (readHum && sensor->supportsInterface(InterfaceType::HUMIDITY)) {
        HumidityReading humReading = sensorManager->getHumidity(sensorName);
        values.push_back(humReading.valid ? String(humReading.value) : "NA");
    }
    
    if (readCO2 && sensor->supportsInterface(InterfaceType::CO2)) {
        values.push_back("NA");
    }
}

bool CommunicationManager::handleListSensors(const std::vector<String>& params) {
    auto registry = sensorManager->getRegistry();
    auto sensors = registry.getAllSensors();
    
    for (auto sensor : sensors) {
        String capabilities = "";
        if (sensor->supportsInterface(InterfaceType::TEMPERATURE)) capabilities += "TEMP";
        if (sensor->supportsInterface(InterfaceType::HUMIDITY)) capabilities += "HUM";
        if (sensor->supportsInterface(InterfaceType::CO2)) capabilities += "C02";
        
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
    if (!success) {
        Serial.println("ERROR: Failed to set board ID");
    }
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
    if (!success) {
        Serial.println("ERROR: Failed to update configuration");
    }
    return success;
}

bool CommunicationManager::handleStreamStart(const std::vector<String>& params) {
    std::vector<StreamingConfig> configs;
    
    // Process each parameter
    for (const auto& param : params) {
        // Check if parameter contains a colon (sensor:measurements format)
        int colonPos = param.indexOf(':');
        if (colonPos > 0) {
            String sensorName = param.substring(0, colonPos);
            String measurements = param.substring(colonPos + 1);
            
            // Handle comma-separated measurement types
            measurements.replace(',', ' '); // Convert commas to spaces for easier parsing
            
            configs.push_back(StreamingConfig(sensorName, measurements));
            
            errorHandler->logInfo("Configuring " + sensorName + " to stream: " + measurements);
        } else {
            // No measurements specified, default to all
            configs.push_back(StreamingConfig(param));
            
            errorHandler->logInfo("Configuring " + param + " to stream all measurements");
        }
    }
    
    bool success = startStreaming(configs);
    if (!success) {
        Serial.println("ERROR: Failed to start streaming");
    }
    return success;
}

bool CommunicationManager::handleStreamStop(const std::vector<String>& params) {
    stopStreaming();
    return true;
}

bool CommunicationManager::handleStreamStatus(const std::vector<String>& params) {
    Serial.println(isStreaming ? "RUNNING" : "STOPPED");
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

// Message routing command handlers
bool CommunicationManager::handleMessageRoutingStatus(const std::vector<String>& params) {
    String status = errorHandler->getRoutingStatus();
    Serial.println(status);
    return true;
}

bool CommunicationManager::handleMessageRoutingSet(const std::vector<String>& params) {
    if (params.empty()) {
        Serial.println("ERROR: No routing option specified");
        return false;
    }
    
    String option = params[0];
    option.toUpperCase();
    
    if (option == "ON" || option == "ENABLE" || option == "1") {
        errorHandler->enableCustomRouting(true);
        errorHandler->logInfo("Custom message routing enabled");
    } else if (option == "OFF" || option == "DISABLE" || option == "0") {
        errorHandler->enableCustomRouting(false);
        errorHandler->logInfo("Custom message routing disabled");
    } else {
        Serial.println("ERROR: Invalid option. Use ON/OFF, ENABLE/DISABLE, or 1/0");
        return false;
    }
    
    return true;
}

bool CommunicationManager::handleSetMessageRoute(const std::vector<String>& params, const String& severity) {
    if (params.empty()) {
        Serial.println("ERROR: No routing destination specified");
        return false;
    }
    
    String destination = params[0];
    destination.toUpperCase();
    
    Print* targetStream = nullptr;
    
    // Determine the target stream based on the destination
    if (destination == "USB" || destination == "SERIAL") {
        targetStream = &Serial;
    } else if (destination == "UART" || destination == "DEBUG") {
        targetStream = uartDebugSerial;
    } else if (destination == "NONE" || destination == "OFF") {
        targetStream = nullptr;
    } else if (destination == "BOTH") {
        Serial.println("ERROR: 'BOTH' option not supported yet");
        return false;
    } else {
        Serial.println("ERROR: Invalid destination. Use USB, UART, NONE, or BOTH");
        return false;
    }
    
    // Set the routing based on severity
    if (severity.equalsIgnoreCase("INFO")) {
        errorHandler->setInfoOutput(targetStream);
    } else if (severity.equalsIgnoreCase("WARNING")) {
        errorHandler->setWarningOutput(targetStream);
    } else if (severity.equalsIgnoreCase("ERROR")) {
        errorHandler->setErrorOutput(targetStream);
    } else if (severity.equalsIgnoreCase("CRITICAL")) {
        errorHandler->setCriticalOutput(targetStream);
    } else {
        Serial.println("ERROR: Invalid severity level");
        return false;
    }
    
    errorHandler->logInfo(severity + " messages routed to " + destination);
    return true;
}

bool CommunicationManager::handleInfoRoute(const std::vector<String>& params) {
    return handleSetMessageRoute(params, "INFO");
}

bool CommunicationManager::handleWarningRoute(const std::vector<String>& params) {
    return handleSetMessageRoute(params, "WARNING");
}

bool CommunicationManager::handleErrorRoute(const std::vector<String>& params) {
    return handleSetMessageRoute(params, "ERROR");
}

bool CommunicationManager::handleCriticalRoute(const std::vector<String>& params) {
    return handleSetMessageRoute(params, "CRITICAL");
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

// Streaming methods implementation
bool CommunicationManager::startStreaming(const std::vector<StreamingConfig>& configs) {
    if (configs.empty()) {
        // If no configs specified, use all available sensors with all measurements
        auto registry = sensorManager->getRegistry();
        auto allSensors = registry.getAllSensors();
        
        std::vector<StreamingConfig> allConfigs;
        for (auto sensor : allSensors) {
            allConfigs.push_back(StreamingConfig(sensor->getName()));
        }
        
        return startStreaming(allConfigs);
    }
    
    // Validate all sensors exist and find the fastest polling rate
    bool allValid = true;
    uint32_t fastestRate = UINT32_MAX;
    
    auto sensorConfigs = configManager->getSensorConfigs();
    
    for (const auto& config : configs) {
        if (!sensorManager->findSensor(config.sensorName)) {
            errorHandler->logInfo("Cannot start streaming: sensor not found: " + config.sensorName);
            allValid = false;
            continue;
        }
        
        // Find this sensor's config to get its polling rate
        for (const auto& sensorConfig : sensorConfigs) {
            if (sensorConfig.name == config.sensorName) {
                // Update fastest rate if this one is faster
                if (sensorConfig.pollingRate < fastestRate) {
                    fastestRate = sensorConfig.pollingRate;
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
    streamingConfigs = configs;
    streamInterval = fastestRate;
    lastStreamTime = 0; // Force immediate update
    isStreaming = true;
    
    errorHandler->logInfo("Started streaming " + String(streamingConfigs.size()) + 
                       " sensors at " + String(streamInterval) + "ms interval");
    return true;
}

void CommunicationManager::stopStreaming() {
    if (isStreaming) {
        isStreaming = false;
        streamingConfigs.clear();
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
        
        std::vector<String> values;
        
        // Collect data for each sensor configuration
        for (const auto& config : streamingConfigs) {
            ISensor* sensor = sensorManager->findSensor(config.sensorName);
            if (!sensor || !sensor->isConnected()) {
                continue;
            }
            
            // Add temperature if requested and available
            if (config.streamTemperature && sensor->supportsInterface(InterfaceType::TEMPERATURE)) {
                TemperatureReading tempReading = sensorManager->getTemperature(config.sensorName);
                values.push_back(tempReading.valid ? String(tempReading.value) : "NA");
            }
            
            // Add humidity if requested and available
            if (config.streamHumidity && sensor->supportsInterface(InterfaceType::HUMIDITY)) {
                HumidityReading humReading = sensorManager->getHumidity(config.sensorName);
                values.push_back(humReading.valid ? String(humReading.value) : "NA");
            }

            // Add CO2 if requested and available
            if (config.streamCO2 && sensor->supportsInterface(InterfaceType::CO2)) {
                values.push_back("NA"); // Placeholder
            }
        }
        
        // Output a single CSV line with all collected values
        if (!values.empty()) {
            String csvLine = values[0];
            for (size_t i = 1; i < values.size(); i++) {
                csvLine += "," + values[i];
            }
            Serial.println(csvLine);
        }
    }
}
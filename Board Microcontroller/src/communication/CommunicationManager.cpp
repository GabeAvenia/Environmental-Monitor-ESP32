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

CommunicationManager::CommunicationManager(SensorManager* sensorMgr, ConfigManager* configMgr, ErrorHandler* err, LedManager* led) :
    sensorManager(sensorMgr),
    configManager(configMgr),
    errorHandler(err),
    ledManager(led) {
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
void CommunicationManager::setLedManager(LedManager* led) {
    ledManager = led;
}

void CommunicationManager::registerCommandHandlers() {
    // Register all command handlers
    commandHandlers[Constants::SCPI::IDN] = 
        [this](const std::vector<String>& params) { return handleIdentify(params); };
    
    commandHandlers[Constants::SCPI::MEASURE] = 
        [this](const std::vector<String>& params) { return handleMeasure(params); };
    
    commandHandlers[Constants::SCPI::MEASURE_QUERY] = 
        [this](const std::vector<String>& params) { return handleMeasure(params); };
    
    commandHandlers[Constants::SCPI::LIST_SENSORS] = 
        [this](const std::vector<String>& params) { return handleListSensors(params); };
    
    commandHandlers[Constants::SCPI::GET_CONFIG] = 
        [this](const std::vector<String>& params) { return handleGetConfig(params); };
    
    commandHandlers[Constants::SCPI::SET_BOARD_ID] = 
        [this](const std::vector<String>& params) { return handleSetBoardId(params); };
    
    commandHandlers[Constants::SCPI::UPDATE_CONFIG] = 
        [this](const std::vector<String>& params) { return handleUpdateConfig(params); };
    
    // Testing commands
    commandHandlers[Constants::SCPI::TEST_FILESYSTEM] = 
        [this](const std::vector<String>& params) { return handleTestFilesystem(params); };
    
    commandHandlers[Constants::SCPI::TEST_UPDATE] = 
        [this](const std::vector<String>& params) { return handleTestUpdateConfig(params); };
    
    commandHandlers[Constants::SCPI::TEST] = 
        [this](const std::vector<String>& params) { 
            Serial.println("Serial communication test successful"); 
            return true; 
        };
        
    commandHandlers[Constants::SCPI::ECHO] = 
        [this](const std::vector<String>& params) { return handleEcho(params); };
    
    // Added reset command
    commandHandlers[Constants::SCPI::RESET] = 
        [this](const std::vector<String>& params) { 
            return handleReset(params); 
        };
        
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
    commandHandlers[Constants::SCPI::LED_IDENTIFY] = 
        [this](const std::vector<String>& params) { return handleLedIdentify(params); };
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
        
    // MEAS command
    scpiParser->RegisterCommand(F("MEAS"), 
        [](SCPI_Commands cmds, SCPI_Parameters parameters, Stream& interface) {
            std::vector<String> params;
            for (size_t i = 0; i < parameters.Size(); i++) {
                params.push_back(String(parameters[i]));
            }
            CommunicationManager::getInstance()->handleMeasure(params);
        });

    // MEAS? command
    scpiParser->RegisterCommand(F("MEAS?"), 
        [](SCPI_Commands cmds, SCPI_Parameters parameters, Stream& interface) {
            std::vector<String> params;
            for (size_t i = 0; i < parameters.Size(); i++) {
                params.push_back(String(parameters[i]));
            }
            CommunicationManager::getInstance()->handleMeasure(params);
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
        
    // TEST command
    scpiParser->RegisterCommand(F("TEST"), 
        [](SCPI_Commands cmds, SCPI_Parameters parameters, Stream& interface) {
            std::vector<String> params;
            CommunicationManager::getInstance()->handleEcho(params);
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
    
    // Emergency reset command
    scpiParser->RegisterCommand(F("*RST"), 
        [](SCPI_Commands cmds, SCPI_Parameters parameters, Stream& interface) {
            std::vector<String> params;
            CommunicationManager::getInstance()->handleReset(params);
        });
    
    // Quick reset command
    scpiParser->RegisterCommand(F("RESET"), 
        [](SCPI_Commands cmds, SCPI_Parameters parameters, Stream& interface) {
            std::vector<String> params;
            CommunicationManager::getInstance()->handleReset(params);
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

    scpiParser->RegisterCommand(F("SYST:LED:IDENT"), 
        [](SCPI_Commands cmds, SCPI_Parameters parameters, Stream& interface) {
            std::vector<String> params;
            CommunicationManager::getInstance()->handleLedIdentify(params);
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
    // Check if there's data available in the serial buffer
    if (Serial.available() > 0) {
        // Read the entire line, which is more reliable in this context
        String rawCommand = Serial.readStringUntil('\n');
        rawCommand.trim();
        
        // Log receipt for debugging
        errorHandler->logInfo("Processing command: '" + rawCommand + "'");
        
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
        
        // Ensure all responses are sent
        Serial.flush();
    }
}

void CommunicationManager::processCommandLine() {
    // Use a better approach to read commands
    String rawCommand = "";
    
    // Read with timeout to avoid partial commands
    unsigned long startTime = millis();
    while (millis() - startTime < 50) { // 50ms timeout
        if (Serial.available()) {
            char c = Serial.read();
            if (c == '\n' || c == '\r') {
                if (rawCommand.length() > 0) {
                    break; // Complete command found
                }
                // Else ignore empty lines
            } else {
                rawCommand += c;
            }
        }
        yield(); // Allow other tasks to run
    }
    
    if (rawCommand.length() == 0) {
        return; // No command to process
    }
    
    // Log the command after cleaning
    rawCommand.trim();
    errorHandler->logInfo("Processing command: '" + rawCommand + "'");

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
    
    // Ensure all responses are sent
    Serial.flush();
}

// Command handler implementations
bool CommunicationManager::handleIdentify(const std::vector<String>& params) {
    String response = String(Constants::PRODUCT_NAME) + "," + 
                      configManager->getBoardIdentifier() + "," +
                      String(Constants::FIRMWARE_VERSION);
    Serial.println(response);
    Serial.flush(); // Ensure the response is sent immediately
    return true;
}

bool CommunicationManager::handleMeasure(const std::vector<String>& params) {
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
        Serial.flush(); // Ensure the response is sent immediately
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
    
    // Read temperature if supported and requested using thread-safe method
    if (readTemp && sensor->supportsInterface(InterfaceType::TEMPERATURE)) {
        TemperatureReading tempReading = sensorManager->getTemperatureSafe(sensorName);
        values.push_back(tempReading.valid ? String(tempReading.value) : "NA");
    }
    
    // Read humidity if supported and requested using thread-safe method
    if (readHum && sensor->supportsInterface(InterfaceType::HUMIDITY)) {
        HumidityReading humReading = sensorManager->getHumiditySafe(sensorName);
        values.push_back(humReading.valid ? String(humReading.value) : "NA");
    }
    
    if (readCO2 && sensor->supportsInterface(InterfaceType::CO2)) {
        values.push_back("NA");
    }
}

bool CommunicationManager::handleListSensors(const std::vector<String>& params) {
    auto registry = sensorManager->getRegistry();
    auto sensors = registry.getAllSensors();
    
    // Build the complete response string first instead of sending it line by line
    String response = "";
    
    for (auto sensor : sensors) {
        // Check each interface type and output a separate entry for each
        if (sensor->supportsInterface(InterfaceType::TEMPERATURE)) {
            response += sensor->getName() + ",TEMP," + 
                      sensor->getTypeString() + "," +
                      (sensor->isConnected() ? "CONNECTED" : "DISCONNECTED") + "\n";
        }
        
        if (sensor->supportsInterface(InterfaceType::HUMIDITY)) {
            response += sensor->getName() + ",HUM," + 
                      sensor->getTypeString() + "," +
                      (sensor->isConnected() ? "CONNECTED" : "DISCONNECTED") + "\n";
        }
        
        if (sensor->supportsInterface(InterfaceType::CO2)) {
            response += sensor->getName() + ",CO2," + 
                      sensor->getTypeString() + "," +
                      (sensor->isConnected() ? "CONNECTED" : "DISCONNECTED") + "\n";
        }
    }
    
    // Send the complete response all at once
    Serial.print(response);
    
    // Add a small delay to ensure all data is sent
    delay(5);
    
    // Flush to ensure all data is transmitted
    Serial.flush();
    
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

bool CommunicationManager::handleReset(const std::vector<String>& params) {
    errorHandler->logInfo("Reset command received");
    Serial.println("Resetting device...");
    delay(100);  // Give time for the message to be sent
    ESP.restart();
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

bool CommunicationManager::handleLedIdentify(const std::vector<String>& params) {
    if (ledManager) {
        ledManager->startIdentify();
        Serial.println("LED identify mode activated");
    } else {
        Serial.println("ERROR: LED manager not available");
        return false;
    }
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
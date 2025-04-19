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
    errorHandler->logError(INFO, "Communication manager initialized");
    registerCommandHandlers();
}

void CommunicationManager::setUartDebugSerialPtr(Print* debugSerial) {
    uartDebugSerial = debugSerial;
}
void CommunicationManager::setLedManager(LedManager* led) {
    ledManager = led;
}

void CommunicationManager::registerCommandHandlers() {
    // Register command handlers
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
        commandHandlers[Constants::SCPI::UPDATE_SENSOR_CONFIG] = 
    [this](const std::vector<String>& params) { return handleUpdateSensorConfig(params); };

    commandHandlers[Constants::SCPI::UPDATE_ADDITIONAL_CONFIG] = 
    [this](const std::vector<String>& params) { return handleUpdateAdditionalConfig(params); };


    commandHandlers[Constants::SCPI::TEST_FILESYSTEM] = 
        [this](const std::vector<String>& params) { return handleTestFilesystem(params); };
    
    commandHandlers[Constants::SCPI::TEST_UPDATE] = 
        [this](const std::vector<String>& params) { return handleTestUpdateConfig(params); };
    
    commandHandlers[Constants::SCPI::TEST] = 
        [this](const std::vector<String>& params) { 
            Serial.println("Serial communication test successful"); 
            return true; 
        };
    commandHandlers[Constants::SCPI::TEST_INFO] = 
        [this](const std::vector<String>& params) { return handleTestErrorLevel(params, INFO); };
    commandHandlers[Constants::SCPI::TEST_WARNING] = 
        [this](const std::vector<String>& params) { return handleTestErrorLevel(params, WARNING); };
    commandHandlers[Constants::SCPI::TEST_ERROR] = 
        [this](const std::vector<String>& params) { return handleTestErrorLevel(params, ERROR); };
    commandHandlers[Constants::SCPI::TEST_FATAL] = 
        [this](const std::vector<String>& params) { return handleTestErrorLevel(params, FATAL); };
    commandHandlers[Constants::SCPI::ECHO] = 
        [this](const std::vector<String>& params) { return handleEcho(params); };

    commandHandlers[Constants::SCPI::RESET] = 
        [this](const std::vector<String>& params) { 
            return handleReset(params); };
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

    commandHandlers[Constants::SCPI::LED_IDENTIFY] = 
        [this](const std::vector<String>& params) { return handleLedIdentify(params); };
}

#define REGISTER_COMMAND(cmd, handler) \
    scpiParser->RegisterCommand(F(cmd), \
        [](SCPI_Commands cmds, SCPI_Parameters parameters, Stream& interface) { \
            CommunicationManager* instance = CommunicationManager::getInstance(); \
            if (instance) { \
                std::vector<String> params; \
                for (size_t i = 0; i < parameters.Size(); i++) { \
                    params.push_back(String(parameters[i])); \
                } \
                instance->handler(params); \
            } \
        });

void CommunicationManager::setupCommands() {
    REGISTER_COMMAND("*IDN?", handleIdentify)
    REGISTER_COMMAND("MEAS", handleMeasure)
    REGISTER_COMMAND("MEAS?", handleMeasure)
    REGISTER_COMMAND("SYST:SENS:LIST?", handleListSensors)
    REGISTER_COMMAND("SYST:CONF?", handleGetConfig)
    REGISTER_COMMAND("SYST:CONF:BOARD:ID", handleSetBoardId)
    REGISTER_COMMAND("SYST:CONF:UPDATE", handleUpdateConfig)
    REGISTER_COMMAND("SYST:CONF:SENS:UPDATE", handleUpdateSensorConfig)
    REGISTER_COMMAND("SYST:CONF:ADD:UPDATE", handleUpdateAdditionalConfig)
    REGISTER_COMMAND("TEST", handleEcho)
    REGISTER_COMMAND("TEST:FS", handleTestFilesystem)
    REGISTER_COMMAND("TEST:UPDATE", handleTestUpdateConfig)
    REGISTER_COMMAND("ECHO", handleEcho)
    REGISTER_COMMAND("SYST:LOG:ROUTE?", handleMessageRoutingStatus)
    REGISTER_COMMAND("SYST:LOG:ROUTE", handleMessageRoutingSet)
    REGISTER_COMMAND("SYST:LOG:INFO:ROUTE", handleInfoRoute)
    REGISTER_COMMAND("SYST:LOG:WARN:ROUTE", handleWarningRoute)
    REGISTER_COMMAND("SYST:LOG:ERR:ROUTE", handleErrorRoute)
    REGISTER_COMMAND("SYST:LED:IDENT", handleLedIdentify)
    REGISTER_COMMAND("TEST:INFO", handleTestInfoLevel)
    REGISTER_COMMAND("TEST:WARNING", handleTestWarningLevel)
    REGISTER_COMMAND("TEST:ERROR", handleTestErrorLevel)
    REGISTER_COMMAND("TEST:FATAL", handleTestFatalLevel)

    
    errorHandler->logError(INFO, "SCPI commands registered");
}

void CommunicationManager::processIncomingData() {
    // Check if there's data available in the serial buffer
    if (Serial.available() > 0) {
        // Read the entire line
        String rawCommand = Serial.readStringUntil('\n');
        rawCommand.trim();
        
        // Log receipt for debugging
        errorHandler->logError(INFO, "Processing command: '" + rawCommand + "'");
        
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

void CommunicationManager::processCommandLine() {
    // Increased timeout and buffer size
    constexpr unsigned long COMMAND_TIMEOUT_MS = 200;
    constexpr size_t MAX_BUFFER_SIZE = 4096;          // 4KB 
    
    // Read a complete command with timeout
    String rawCommand = "";
    unsigned long startTime = millis();
    
    // Read until newline, timeout, or buffer full
    while ((millis() - startTime < COMMAND_TIMEOUT_MS) && (rawCommand.length() < MAX_BUFFER_SIZE)) {
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
    
    // Check if we hit the buffer limit
    if (rawCommand.length() >= MAX_BUFFER_SIZE) {
        errorHandler->logError(WARNING, "Command exceeds buffer size limit of " + String(MAX_BUFFER_SIZE) + " characters");
    }
    
    if (rawCommand.length() == 0) {
        return; // No command to process
    }
    
    // Trim whitespace and process command
    rawCommand.trim();
    errorHandler->logError(INFO, "Processing command: '" + 
                         (rawCommand.length() > 50 ? 
                            rawCommand.substring(0, 50) + "..." : 
                            rawCommand) + 
                         "' (" + String(rawCommand.length()) + " bytes)");

    // Parse and process command
    String command;
    std::vector<String> params;
    parseCommand(rawCommand, command, params);
    
    // Handle command through our handlers or fall back to SCPI parser
    if (!processCommand(command, params)) {
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
    errorHandler->logError(INFO, "MEAS values sent");
    return true;
}

void CommunicationManager::collectSensorReadings(const String& sensorName, const String& measurements, std::vector<String>& values) {
    ISensor* sensor = sensorManager->findSensor(sensorName);
    if (!sensor || !sensor->isConnected()) {
        values.push_back("ERROR");
        return;
    }
    
    // Determine which measurements to take
    bool useAllMeasurements = measurements.length() == 0;
    String upperMeasurements = measurements;
    upperMeasurements.toUpperCase();
    
    bool readTemp = useAllMeasurements || upperMeasurements.indexOf("TEMP") >= 0;
    bool readHum = useAllMeasurements || upperMeasurements.indexOf("HUM") >= 0;
    
    // Read temperature if supported and requested - use thread-safe method directly
    if (readTemp && sensor->supportsInterface(InterfaceType::TEMPERATURE)) {
        TemperatureReading tempReading = sensorManager->getTemperatureSafe(sensorName);
        values.push_back(tempReading.valid ? String(tempReading.value) : "ERROR");
    }
    
    // Read humidity if supported and requested - use thread-safe method directly
    if (readHum && sensor->supportsInterface(InterfaceType::HUMIDITY)) {
        HumidityReading humReading = sensorManager->getHumiditySafe(sensorName);
        values.push_back(humReading.valid ? String(humReading.value) : "ERROR");
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
    }
    
    // Send the complete response all at once
    Serial.print(response);
    // Small delay to ensure all data is sent
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
        errorHandler->logError(ERROR, "No board ID specified");
        return false;
    }
    
    bool success = configManager->setBoardIdentifier(params[0]);
    if (!success) {
        errorHandler->logError(ERROR, "Failed to update Board ID");
    }
    return success;
}

bool CommunicationManager::handleUpdateConfig(const std::vector<String>& params) {
    if (params.empty()) {
        errorHandler->logError(ERROR, "No configuration JSON provided");    
    }
    
    // Join all parameters since the JSON might have spaces
    String jsonConfig = "";
    for (const auto& param : params) {
        if (jsonConfig.length() > 0) jsonConfig += " ";
        jsonConfig += param;
    }
    
    errorHandler->logError(INFO, "Processing config update: " + jsonConfig.substring(0, 50) + "...");
    bool success = configManager->updateConfigFromJson(jsonConfig);
    if (!success) {
        errorHandler->logError(ERROR, "Failed to update configuration");
    }
    return success;
}

bool CommunicationManager::handleUpdateSensorConfig(const std::vector<String>& params) {
    if (params.empty()) {
        errorHandler->logError(WARNING, "No sensor configuration provided");
    }
    
    // Join all parameters since the JSON might have spaces
    String jsonConfig = "";
    for (const auto& param : params) {
        if (jsonConfig.length() > 0) jsonConfig += " ";
        jsonConfig += param;
    }
    
    errorHandler->logError(INFO, "Processing sensor config update: " + 
                         jsonConfig.substring(0, std::min(50, (int)jsonConfig.length())) + 
                         (jsonConfig.length() > 50 ? "..." : ""));
    
    bool success = configManager->updateSensorConfigFromJson(jsonConfig);
    if (!success) {
        errorHandler->logError(ERROR, "Failed to update sensor configuration");
        return false;
    }
    
    // Add this block to explicitly reinitialize the sensors
    errorHandler->logError(INFO, "Reinitializing sensors with new configuration");
    if (sensorManager->initializeSensors()) {
        errorHandler->logError(INFO, "Successfully reinitialized sensors with new configuration");
    } else {
        errorHandler->logError(ERROR, "Failed to reinitialize some sensors after configuration update");
    }
    
    return true;
}

bool CommunicationManager::handleUpdateAdditionalConfig(const std::vector<String>& params) {
    if (params.empty()) {
        errorHandler->logError(WARNING, "No additional configuration provided");
    }
    
    // Join all parameters since the JSON might have spaces
    String jsonConfig = "";
    for (const auto& param : params) {
        if (jsonConfig.length() > 0) jsonConfig += " ";
        jsonConfig += param;
    }
    
    errorHandler->logError(INFO, "Processing additional config update: " + 
                         jsonConfig.substring(0, std::min(50, (int)jsonConfig.length())) + 
                         (jsonConfig.length() > 50 ? "..." : ""));
    
    bool success = configManager->updateAdditionalConfigFromJson(jsonConfig);
    if (!success) {
        errorHandler->logError(ERROR, "Failed to update additional configuration");
    }
    return success;
}

bool CommunicationManager::handleReset(const std::vector<String>& params) {
    errorHandler->logError(INFO, "Reset command received");
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
        errorHandler->logError(INFO, "Custom message routing enabled");
    } else if (option == "OFF" || option == "DISABLE" || option == "0") {
        errorHandler->enableCustomRouting(false);
        errorHandler->logError(INFO, "Custom message routing disabled");
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
    } else {
        Serial.println("ERROR: Invalid severity level");
        return false;
    }
    
    errorHandler->logError(INFO, severity + " messages routed to " + destination);
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

bool CommunicationManager::handleTestErrorLevel(const std::vector<String>& params, ErrorSeverity severity) {
    String severityStr;
    switch (severity) {
        case INFO: severityStr = "INFO"; break;
        case WARNING: severityStr = "WARNING"; break;
        case ERROR: severityStr = "ERROR"; break;
        case FATAL: severityStr = "FATAL"; break;
        default: severityStr = "UNKNOWN"; break;
    }
    
    String message = "Test " + severityStr + " message";
    if (params.size() > 0) {
        message = params[0];
    }
    
    // Inform the user what's happening
    Serial.println("Logging " + severityStr + " message: \"" + message + "\"");
    Serial.flush();
    
    // Debug output for LED connections
    Serial.println("ErrorHandler has LED manager: " + String(errorHandler->getLedManager() != nullptr ? "YES" : "NO"));
    
    // Log the message
    bool isFatal = errorHandler->logError(severity, message);
    
    // Extra direct LED control for debugging
    if (ledManager && severity >= WARNING) {
        if (severity == WARNING) ledManager->indicateWarning();
        if (severity == ERROR) ledManager->indicateError();
        if (severity == FATAL) ledManager->indicateFatalError();
    }
    
    // For FATAL errors, handle specially
    if (isFatal) {
        // For FATAL, we might want to reset after some delay
        int resetDelay = -1;
        if (params.size() > 1) {
            resetDelay = params[1].toInt();
        }
        
        if (resetDelay > 0) {
            Serial.println("Device will reset after " + String(resetDelay) + "ms");
            Serial.flush();
            delay(resetDelay);
            ESP.restart();
        } else {
            Serial.println("Fatal error - device halted");
            Serial.flush();
            // Enter infinite loop, but keep LED updated
            while (true) {
                if (ledManager) {
                    ledManager->update();
                }
                delay(100);
                yield();
            }
        }
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
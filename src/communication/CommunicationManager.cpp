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

void CommunicationManager::registerCommands() {
    // Clear existing command registrations
    commandHandlers.clear();
    
    // Define a macro that registers a command both in our map and with the SCPI parser
    #define REGISTER_COMMAND(cmdConst, handlerFunc) \
        commandHandlers[cmdConst] = [this](const std::vector<String>& params) { \
            return this->handlerFunc(params); \
        }; \
        scpiParser->RegisterCommand(F(cmdConst), \
            [](SCPI_Commands cmds, SCPI_Parameters parameters, Stream& interface) { \
                CommunicationManager* instance = CommunicationManager::getInstance(); \
                if (instance) { \
                    std::vector<String> params; \
                    for (size_t i = 0; i < parameters.Size(); i++) { \
                        params.push_back(String(parameters[i])); \
                    } \
                    instance->handlerFunc(params); \
                } \
            });

    // Register all commands using Constants::SCPI namespace
    REGISTER_COMMAND(Constants::SCPI::IDN, handleIdentify)
    REGISTER_COMMAND(Constants::SCPI::MEASURE_QUERY, handleMeasure)
    REGISTER_COMMAND(Constants::SCPI::LIST_SENSORS, handleListSensors)
    REGISTER_COMMAND(Constants::SCPI::GET_CONFIG, handleGetConfig)
    REGISTER_COMMAND(Constants::SCPI::SET_BOARD_ID, handleSetBoardId)
    REGISTER_COMMAND(Constants::SCPI::UPDATE_CONFIG, handleUpdateConfig)
    REGISTER_COMMAND(Constants::SCPI::UPDATE_SENSOR_CONFIG, handleUpdateSensorConfig)
    REGISTER_COMMAND(Constants::SCPI::UPDATE_ADDITIONAL_CONFIG, handleUpdateAdditionalConfig)
    REGISTER_COMMAND(Constants::SCPI::TEST, handleEcho)
    REGISTER_COMMAND(Constants::SCPI::ECHO, handleEcho)
    REGISTER_COMMAND(Constants::SCPI::RESET, handleReset)
    REGISTER_COMMAND(Constants::SCPI::LOG_STATUS, handleLogStatus)
    REGISTER_COMMAND(Constants::SCPI::LOG_ROUTE, handleLogRouting)
    REGISTER_COMMAND(Constants::SCPI::LED_IDENTIFY, handleLedIdentify)
    REGISTER_COMMAND(Constants::SCPI::TEST_INFO, handleTestInfoLevel)
    REGISTER_COMMAND(Constants::SCPI::TEST_WARNING, handleTestWarningLevel)
    REGISTER_COMMAND(Constants::SCPI::TEST_ERROR, handleTestErrorLevel)
    REGISTER_COMMAND(Constants::SCPI::TEST_FATAL, handleTestFatalLevel)
    
    #undef REGISTER_COMMAND
    
    errorHandler->logError(INFO, "SCPI commands registered");
}

void CommunicationManager::begin(long baudRate) {
    errorHandler->logError(INFO, "Communication manager initialized");
    registerCommands();  // Replace both previous command registration methods
}

void CommunicationManager::processCommandLine() {
    // Increased timeout and buffer size - keep within function as requested
    static constexpr unsigned long COMMAND_TIMEOUT_MS = Constants::Communication::COMMAND_TIMEOUT_MS;
    static constexpr size_t MAX_BUFFER_SIZE = Constants::Communication::MAX_BUFFER_SIZE;
    
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
    
    // Check for errors or empty command
    if (rawCommand.length() >= MAX_BUFFER_SIZE) {
        errorHandler->logError(ERROR, "Command exceeds buffer size limit of " + String(MAX_BUFFER_SIZE) + " characters");
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
    
    // Track if the command was recognized
    bool commandRecognized = false;
    
    // Handle HELP or ? commands specially
    if (command.equalsIgnoreCase("HELP") || command.equalsIgnoreCase("?")) {
        // Provide basic help information
        Serial.println("Available commands:");
        Serial.println("*IDN? - Get device identification");
        Serial.println("MEAS? - Get measurements from all peripherals");
        Serial.println("MEAS? <sensor>[:measurement] - Get specific measurements");
        Serial.println("SYST:SENS:LIST? - List all available peripherals");
        Serial.println("SYST:CONF? - Get device configuration");
        Serial.println("RESET - Reset the device");
        Serial.println();
        commandRecognized = true;
    }
    // Try our command handlers
    else if (processCommand(command, params)) {
        commandRecognized = true;
    }
    // Fall back to SCPI parser for compatibility with shortened commands
    else {
        char buff[rawCommand.length() + 1];
        rawCommand.toCharArray(buff, rawCommand.length() + 1);
        
        // Capture output status to detect if SCPI parser handled the command
        size_t beforeSerialPos = Serial.availableForWrite();
        scpiParser->ProcessInput(Serial, buff);
        size_t afterSerialPos = Serial.availableForWrite();
        
        commandRecognized = (afterSerialPos != beforeSerialPos);
    }
    
    // Log unrecognized commands
    if (!commandRecognized) {
        errorHandler->logError(ERROR, "Unrecognized command: '" + 
                            (command.length() > 50 ? 
                               command.substring(0, 50) + "..." : 
                               command) + "'");
    }
    
    // Ensure all responses are sent
    Serial.flush();
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

// Command handler implementations
bool CommunicationManager::handleIdentify(const std::vector<String>& params) {
    String response = String(Constants::PRODUCT_NAME) + "," + 
                      configManager->getBoardIdentifier() + "," +
                      String(Constants::FIRMWARE_VERSION);
    Serial.println(response);
    Serial.flush();
    return true;
}

bool CommunicationManager::handleMeasure(const std::vector<String>& params) {
    std::vector<String> values;
    
    try {
        if (params.empty()) {
            // No sensors specified, use all available
            auto registry = sensorManager->getRegistry();
            auto allSensors = registry.getAllSensors();
            
            errorHandler->logError(INFO, "MEAS: Collecting data from all " + String(allSensors.size()) + " available peripherals");
            
            for (auto sensor : allSensors) {
                collectSensorReadings(sensor->getName(), "", values);
            }
        } else {
            std::map<String, String> sensorRequests;
            
            // Parse parameters and group by sensor name
            for (const auto& param : params) {
                // Check if parameter contains a colon (sensor:measurements format)
                int colonPos = param.indexOf(':');
                String sensorName, measurements;
                
                if (colonPos > 0) {
                    sensorName = param.substring(0, colonPos);
                    measurements = param.substring(colonPos + 1);
                    errorHandler->logError(INFO, "MEAS: Reading " + sensorName + " with measurements: " + measurements);
                } else {
                    sensorName = param;
                    measurements = "";
                    errorHandler->logError(INFO, "MEAS: Reading " + sensorName + " with all available measurements");
                }
                
                // FIXED: Combine multiple measurement requests for the same sensor
                if (sensorRequests.find(sensorName) == sensorRequests.end()) {
                    // First measurement request for this sensor
                    sensorRequests[sensorName] = measurements;
                } else {
                    // Additional measurement request for existing sensor
                    if (measurements.length() > 0) {
                        if (sensorRequests[sensorName].length() > 0) {
                            // Combine with existing measurements (comma-separated)
                            sensorRequests[sensorName] += "," + measurements;
                        } else {
                            // Replace empty string (all measurements) with specific measurement
                            sensorRequests[sensorName] = measurements;
                        }
                    }
                    // If measurements is empty (meaning all measurements), keep existing value
                }
            }
            
            // Collect data for each unique sensor
            for (const auto& [sensorName, measurements] : sensorRequests) {
                collectSensorReadings(sensorName, measurements, values);
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
            errorHandler->logError(INFO, "MEAS: CSV response sent with " + String(values.size()) + " values");
        } else {
            errorHandler->logError(WARNING, "MEAS: No measurement values were collected!");
            Serial.println("ERROR");
            Serial.flush();
        }
        
    } catch (...) {
        // Catch any other unforeseen errors
        errorHandler->logError(WARNING, "MEAS: Unexpected exception during measurement");
        Serial.println("ERROR");
        Serial.flush();
        return false;
    }
    
    return true;
}

void CommunicationManager::collectSensorReadings(const String& sensorName, const String& measurements, std::vector<String>& values) {
    // Constants for retry logic
    const int MAX_RETRIES = Constants::Communication::MAX_READING_RETRIES;
    const int RETRY_DELAY_MS = Constants::Communication::READING_RETRY_DELAY_MS;
    
    // Find the sensor
    ISensor* sensor = sensorManager->findSensor(sensorName);
    bool sensorOk = (sensor != nullptr && sensor->isConnected());
    
    if (!sensorOk) {
        errorHandler->logError(WARNING, "Peripheral " + sensorName + " not found or not connected");
        return;
    }
    
    // Determine which measurements to take
    bool useAllMeasurements = measurements.length() == 0;
    String upperMeasurements = measurements;
    upperMeasurements.toUpperCase();
    
    bool readTemp = useAllMeasurements || upperMeasurements.indexOf("TEMP") >= 0;
    bool readHum = useAllMeasurements || upperMeasurements.indexOf("HUM") >= 0;
    
    if (readTemp && sensorOk && sensor->supportsInterface(InterfaceType::TEMPERATURE)) {
        TemperatureReading tempReading = sensorManager->getTemperatureSafe(sensorName);
        
        if (tempReading.valid) {
            values.push_back(String(tempReading.value));
        } else {
            // Only if first read failed, apply retry logic with delays
            bool success = false;
            String tempValue = "ERROR";
            
            for (int attempt = 1; attempt < MAX_RETRIES && !success; attempt++) {
                // Log retry information at INFO level
                errorHandler->logError(INFO, "Retry #" + String(attempt) + " for temperature reading from " + sensorName);
                delay(RETRY_DELAY_MS); // Only delay during retries when needed
                
                try {
                    tempReading = sensorManager->getTemperatureSafe(sensorName);
                    
                    if (tempReading.valid) {
                        tempValue = String(tempReading.value);
                        success = true;
                        errorHandler->logError(INFO, "Successfully read temperature from " + sensorName + " after " + String(attempt+1) + " attempts");
                        break;
                    }
                } catch (...) {
                    // Continue to next retry attempt
                }
            }
            
            values.push_back(success ? tempValue : "ERROR");
        }
    }
    
    // Read humidity if requested and supported
    if (readHum && sensorOk && sensor->supportsInterface(InterfaceType::HUMIDITY)) {
        // Try first reading without any delay
        HumidityReading humReading = sensorManager->getHumiditySafe(sensorName);
        
        if (humReading.valid) {
            // Fast path - reading succeeded on first try (most common case)
            values.push_back(String(humReading.value));
        } else {
            // Only if first read failed, apply retry logic with delays
            bool success = false;
            String humValue = "ERROR";
            
            for (int attempt = 1; attempt < MAX_RETRIES && !success; attempt++) {
                // Log retry information at INFO level
                errorHandler->logError(INFO, "Retry #" + String(attempt) + " for humidity reading from " + sensorName);
                delay(RETRY_DELAY_MS); // Only delay during retries when needed
                
                try {
                    humReading = sensorManager->getHumiditySafe(sensorName);
                    
                    if (humReading.valid) {
                        humValue = String(humReading.value);
                        success = true;
                        errorHandler->logError(INFO, "Successfully read humidity from " + sensorName + " after " + String(attempt+1) + " attempts");
                        break;
                    }
                } catch (...) {
                    // Continue to next retry attempt
                }
            }
            
            values.push_back(success ? humValue : "ERROR");
        }
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
    
    // Join all parameters since the board ID might have spaces
    String boardId = "";
    for (const auto& param : params) {
        if (boardId.length() > 0) boardId += " ";
        boardId += param;
    }
    
    // Trim any leading/trailing whitespace
    boardId.trim();
    
    // Validate that the board ID is not empty after trimming
    if (boardId.length() == 0) {
        errorHandler->logError(ERROR, "Board ID cannot be empty");
        return false;
    }
    
    // Check for commas which would break the *IDN? response format
    if (boardId.indexOf(',') >= 0) {
        errorHandler->logError(ERROR, "Board ID cannot contain commas");
        return false;
    }
    
    errorHandler->logError(INFO, "Setting board ID to: '" + boardId + "'");
    
    bool success = configManager->setBoardIdentifier(boardId);
    if (!success) {
        errorHandler->logError(ERROR, "Failed to update Board ID");
    } else {
        errorHandler->logError(INFO, "Successfully updated board ID to: '" + boardId + "'");
    }
    return success;
}

bool CommunicationManager::handleUpdateConfig(const std::vector<String>& params) {
    if (params.empty()) {
        errorHandler->logError(ERROR, "No configuration JSON provided");
        return false;
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
    errorHandler->logError(INFO, "Reinitializing peripherals with new configuration");
    if (sensorManager->initializeSensors()) {
        errorHandler->logError(INFO, "Successfully reinitialized peripherals with new configuration");
    } else {
        errorHandler->logError(ERROR, "Failed to reinitialize some peripherals after configuration update");
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

bool CommunicationManager::handleEcho(const std::vector<String>& params) {
    String message = params.empty() ? "ECHO" : params[0];
    Serial.println("ECHO: " + message);
    return true;
}

bool CommunicationManager::handleLogStatus(const std::vector<String>& params) {
    String status = errorHandler->getRoutingStatus();
    Serial.println(status);
    return true;
}

bool CommunicationManager::handleLogRouting(const std::vector<String>& params) {
    if (params.empty()) {
        errorHandler->logError(ERROR, "Format is SYST:LOG <destination>,<severity>");
        return false;
    }
    
    // Check if the parameters are in a single comma-separated string or in two separate parameters
    String destination, severityStr;
    
    if (params.size() == 1 && params[0].indexOf(',') > 0) {
        // Format: "USB,INFO" (comma-separated in a single parameter)
        int commaPos = params[0].indexOf(',');
        destination = params[0].substring(0, commaPos);
        severityStr = params[0].substring(commaPos + 1);
    } else if (params.size() >= 2) {
        // Format: "USB" "INFO" (two separate parameters)
        destination = params[0];
        severityStr = params[1];
    } else {
        errorHandler->logError(ERROR, "Format is SYST:LOG <destination>,<severity>");
        return false;
    }
    
    destination.toUpperCase();
    severityStr.toUpperCase();
    
    // Determine the target stream based on destination
    Print* targetStream = nullptr;
    if (destination == "USB" || destination == "SERIAL") {
        targetStream = &Serial;
    } else if (destination == "UART" || destination == "DEBUG") {
        targetStream = uartDebugSerial;
    } else if (destination == "NONE" || destination == "OFF") {
        targetStream = nullptr;
    } else {
        errorHandler->logError(ERROR, "Invalid destination. Use USB, UART, or NONE");
        return false;
    }
    
    // Determine severity
    ErrorSeverity minSeverity = ErrorHandler::stringToSeverity(severityStr);
    
    // Enable custom routing if it's not already enabled
    errorHandler->enableCustomRouting(true);
    
    // Set the routing configuration
    errorHandler->setOutputSeverity(targetStream, minSeverity);
    
    errorHandler->logError(INFO, "Log routing updated: " + destination + " will show " + 
                 ErrorHandler::severityToString(minSeverity) + " and higher");
    
    return true;
}

bool CommunicationManager::handleLedIdentify(const std::vector<String>& params) {
    if (ledManager) {
        ledManager->startIdentify();
        errorHandler->logError(INFO, "identify mode activated");
    } else {
        errorHandler->logError(ERROR, "LED manager not available");
        return false;
    }
    return true;
}

bool CommunicationManager::handleTestErrorLevel(const std::vector<String>& params, ErrorSeverity severity) {
    String severityStr = ErrorHandler::severityToString(severity);
    
    String message = "Test " + severityStr + " message";
    if (params.size() > 0) {
        message = params[0];
    }
    bool isFatal = errorHandler->logError(severity, message);
    
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
            errorHandler->logError(FATAL, "Device will reset after " + String(resetDelay) + "ms");
            Serial.flush();
            delay(resetDelay);
            ESP.restart();
        } else {
            errorHandler->logError(FATAL, "Fatal error - device halted");
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

void CommunicationManager::setUartDebugSerialPtr(Print* debugSerial) {
    uartDebugSerial = debugSerial;
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
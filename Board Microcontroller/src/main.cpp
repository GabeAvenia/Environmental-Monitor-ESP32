#include <Arduino.h>
#include <LittleFS.h>
#include <esp_heap_caps.h>
#include "Constants.h"
#include "error/ErrorHandler.h"
#include "config/ConfigManager.h"
#include "managers/I2CManager.h"
#include "managers/SPIManager.h"
#include "managers/SensorManager.h"
#include "managers/LedManager.h"
#include "managers/TaskManager.h"
#include "communication/CommunicationManager.h"

// Define UART pins for debug output
#define UART_TX_PIN 5   // GPIO 5
#define UART_RX_PIN 16  // GPIO 16

// Global components
ErrorHandler* errorHandler = nullptr;
ConfigManager* configManager = nullptr;
I2CManager* i2cManager = nullptr;
SPIManager* spiManager = nullptr;
SensorManager* sensorManager = nullptr;
CommunicationManager* commManager = nullptr;
HardwareSerial* debugSerial = nullptr;
LedManager* ledManager = nullptr;
TaskManager* taskManager = nullptr;

// Global references to serial ports
Print* usbSerial = nullptr;
Print* uartDebugSerial = nullptr;

// Configuration change callback
void onConfigChanged(const String& newConfig) {
    // When configuration changes, reconfigure sensors
    if (sensorManager && taskManager) {
        // Get the mutex directly
        SemaphoreHandle_t mutex = taskManager->getSensorMutex();
        bool mutexTaken = false;
        
        // Try to take the mutex if it's valid
        if (mutex != nullptr) {
            mutexTaken = (xSemaphoreTake(mutex, pdMS_TO_TICKS(250)) == pdTRUE);
        }
        
        // Reconfigure sensors
        sensorManager->reconfigureSensors(newConfig);
        
        // Release the mutex if we took it
        if (mutexTaken) {
            xSemaphoreGive(mutex);
        }
    }
}

// Helper function to pass debug serial to communication manager
void setUartDebugSerial(Print* debugSerial) {
    if (commManager) {
        CommunicationManager::setUartDebugSerialPtr(debugSerial);
    }
}

// Handle basic commands when taskManager isn't running
void handleCommand(const String& cmd) {
    if (cmd == "*IDN?") {
        Serial.println(String(Constants::PRODUCT_NAME) + "," + 
                    configManager->getBoardIdentifier() + "," +
                    String(Constants::FIRMWARE_VERSION));
    } 
    else if (cmd == "SYST:LED:IDENT") {
        if (ledManager) {
            ledManager->startIdentify();
            Serial.println("LED identify mode activated");
        }
    }
    else if (cmd == "RESET") {
        Serial.println("Resetting device...");
        delay(100);  // Give time for the message to be sent
        ESP.restart();
    }
    else if (cmd == "TEST") {
        Serial.println("Serial communication test successful");
    }
    else if (cmd.startsWith("ECHO")) {
        Serial.println("ECHO: " + cmd);
    }
    else if (cmd == "TASK:STATUS") {
        // Command to get task status
        if (taskManager) {
            Serial.println(taskManager->getTaskStatusString());
            Serial.println(taskManager->getTaskMemoryInfo());
        } else {
            Serial.println("Task manager not initialized or unavailable");
        }
    }
    else if (cmd == "MEMORY") {
        // Command to get memory info using ESP32-specific functions
        Serial.println("=== Memory Information ===");
        Serial.println("ESP32 Total Heap: " + String(ESP.getHeapSize()) + " bytes");
        Serial.println("ESP32 Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
        
        // Get largest block that can be allocated
        Serial.println("ESP32 Max Alloc Heap: " + String(ESP.getMaxAllocHeap()) + " bytes");
        
        // Get minimum free heap ever
        Serial.println("ESP32 Min Free Heap: " + String(ESP.getMinFreeHeap()) + " bytes");
        
        // Calculate fragmentation
        int fragPercent = 100 - (ESP.getMaxAllocHeap() * 100) / ESP.getFreeHeap();
        if (fragPercent < 0) fragPercent = 0;  // Protect against negative values
        Serial.println("Heap Fragmentation: " + String(fragPercent) + "%");
        
        // Check if PSRAM is available
        if (ESP.getPsramSize() > 0) {
            Serial.println("=== PSRAM Information ===");
            Serial.println("PSRAM Size: " + String(ESP.getPsramSize()) + " bytes");
            Serial.println("PSRAM Free: " + String(ESP.getFreePsram()) + " bytes");
            Serial.println("PSRAM Min Free: " + String(ESP.getMinFreePsram()) + " bytes");
            Serial.println("PSRAM Max Alloc: " + String(ESP.getMaxAllocPsram()) + " bytes");
        }
    }
    else if (cmd == "HELP") {
        Serial.println("Available commands:");
        Serial.println("  *IDN?           - Get device identification");
        Serial.println("  MEAS?           - Get all sensor measurements");
        Serial.println("  MEAS? [sensor]  - Get measurements from specific sensor");
        Serial.println("  SYST:SENS:LIST? - List all available sensors");
        Serial.println("  SYST:CONF?      - Get current configuration");
        Serial.println("  SYST:LED:IDENT  - Flash LED for device identification");
        Serial.println("  TASK:STATUS     - Show task status");
        Serial.println("  MEMORY          - Show memory usage");
        Serial.println("  RESET           - Restart the device");
    }
    else if (commManager) {
        // Process through the communication manager
        String command;
        std::vector<String> params;
        commManager->parseCommand(cmd, command, params);
        commManager->processCommand(command, params);
    }
}

void setup() {
    // Initialize USB Serial for command interface
    Serial.begin(115200);
    
    // Wait a moment for serial to initialize
    delay(500);
    
    // Print an immediate message to verify serial works
    Serial.println("ESP32 starting up - Initializing components...");
    usbSerial = &Serial;
    
    // Initialize UART for debug messages
    debugSerial = &Serial2;
    debugSerial->begin(115200, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
    delay(100); // Give serial time to initialize
    uartDebugSerial = debugSerial;
    
    // Initialize the error handler with debug output
    errorHandler = new ErrorHandler(usbSerial, uartDebugSerial);
    
    // Give the system time to stabilize
    delay(200);
    
    // Initialize LED manager
    ledManager = new LedManager(errorHandler);
    ledManager->begin();
    ledManager->setSetupMode();
    
    // Set info messages to go to UART only, not to serial terminal
    errorHandler->setInfoOutput(uartDebugSerial);
    errorHandler->enableCustomRouting(true);

    // Welcome message through logger
    errorHandler->logInfo("Starting " + String(Constants::PRODUCT_NAME) + " v" + String(Constants::FIRMWARE_VERSION));
    errorHandler->logInfo("Error handler initialized with custom routing");

    // Initialize LittleFS with the specific configuration
    if (!LittleFS.begin(true, "/litlefs", 10, "ffat")) {
        // Log to both terminal and UART for critical errors
        errorHandler->logCritical("LittleFS mount failed! System halted.");
        while (1) { 
            delay(1000);
        }
    }
    
    // Initialize configuration manager
    configManager = new ConfigManager(errorHandler);
    
    if (!configManager->begin()) {
        errorHandler->logCritical("Failed to initialize configuration. System halted.");
        while (1) {
            delay(1000);
        }
    }
    
    // Register for configuration changes
    configManager->registerChangeCallback(onConfigChanged);
    
    // Initialize I2C manager
    i2cManager = new I2CManager(errorHandler);
    if (!i2cManager->begin()) {
        errorHandler->logWarning("Failed to initialize I2C manager");
    }
    
    // Initialize SPI manager
    spiManager = new SPIManager(errorHandler);
    if (!spiManager->begin()) {
        errorHandler->logWarning("Failed to initialize SPI manager");
    } else {
        // Register SS pins
        for (int i = 0; i < MAX_SS_PINS; i++) {
            spiManager->registerSSPin(i);
        }
        errorHandler->logInfo("Registered logical SS pins 0-3");
    }
    
    // Give the system time to stabilize
    delay(200);
    
    // Create sensor manager with both I2C and SPI support
    sensorManager = new SensorManager(configManager, i2cManager, errorHandler, spiManager);
    
    // Initialize sensors
    if (!sensorManager->initializeSensors()) {
        errorHandler->logWarning("Some sensors failed to initialize");
    }
    
    // Configure sensor cache settings based on fastest polling rate from config
    uint32_t fastestRate = 1000; // Default 1 second
    auto sensorConfigs = configManager->getSensorConfigs();
    for (const auto& config : sensorConfigs) {
        if (config.pollingRate < fastestRate && config.pollingRate >= 50) {
            fastestRate = config.pollingRate;
        }
    }
    
    sensorManager->setMaxCacheAge(fastestRate);
    errorHandler->logInfo("Sensor cache configured with " + String(fastestRate) + "ms max age");
    
    // Give the system time to stabilize
    delay(200);
    
    // Setup communication
    commManager = new CommunicationManager(sensorManager, configManager, errorHandler, ledManager);
    commManager->begin(115200);
    
    // Pass the UART debug serial to the CommManager
    setUartDebugSerial(uartDebugSerial);
    commManager->setupCommands();
    
    // Give the system time to stabilize
    delay(300);
    
    // Initialize TaskManager - core task management
    taskManager = new TaskManager(sensorManager, commManager, ledManager, errorHandler);
    
    if (!taskManager->begin()) {
        errorHandler->logError(ERROR, "Failed to initialize task manager");
    } else {
        errorHandler->logInfo("Task manager initialized successfully");
        
        // Start tasks one by one with delays in between
        if (taskManager->startLedTask()) {
            errorHandler->logInfo("LED task started successfully");
            delay(300); // Give time for task to stabilize
            
            if (taskManager->startCommTask()) {
                errorHandler->logInfo("Communication task started successfully");
                delay(300); // Give time for task to stabilize
                
                if (taskManager->startSensorTask()) {
                    errorHandler->logInfo("Sensor task started successfully");
                    delay(300); // Give time for task to stabilize
                    
                    // Pass the sensor mutex to the sensor manager
                    sensorManager->setSensorMutex(taskManager->getSensorMutex());
                } else {
                    errorHandler->logWarning("Failed to start sensor task");
                }
            } else {
                errorHandler->logWarning("Failed to start communication task");
            }
        } else {
            errorHandler->logWarning("Failed to start LED task");
        }
    }
    
    errorHandler->logInfo("System initialization complete");
    errorHandler->logInfo("System ready. Environmental Monitor ID: " + configManager->getBoardIdentifier());
    ledManager->setNormalMode();

}

void loop() {
    // Even if tasks are running, we want to have a fallback mechanism
    // Check for serial commands in the main loop too (as a safety net)
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        
        // Use the main command handler
        handleCommand(command);
    }
    
    // Update LED in the main loop if LED task isn't running
    if (ledManager && (!taskManager || !taskManager->areAllTasksRunning())) {
        ledManager->update();
    }
    
    // Poll sensors in the main loop if sensor task isn't running
    static unsigned long lastPollingTime = 0;
    unsigned long currentTime = millis();
    
    if (sensorManager && (!taskManager || !taskManager->areAllTasksRunning())) {
        uint32_t pollingInterval = sensorManager->getMaxCacheAge();
        
        if (currentTime - lastPollingTime >= pollingInterval) {
            sensorManager->updateReadings(true);
            lastPollingTime = currentTime;
            
            // Signal LED
            if (ledManager && !ledManager->isIdentifying()) {
                ledManager->indicateReading();
            }
        }
    }
    
    // Always yield to the scheduler
    yield();
    
    // Short delay to prevent watchdog timeouts
    delay(10);
}
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

// Helper function to pass debug serial to communication manager
void setUartDebugSerial(Print* debugSerial) {
    if (commManager) {
        CommunicationManager::setUartDebugSerialPtr(debugSerial);
    }
}

void setup() {
    // Initialize USB Serial for command interface
    Serial.begin(115200);
    
    while (!Serial) delay(10);

    // Initialize USB Serial for debug messages
    usbSerial = &Serial;
    
    // Initialize UART for debug messages
    debugSerial = &Serial2;
    debugSerial->begin(115200, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
    delay(50); // Give serial time to initialize
    uartDebugSerial = debugSerial;
    
    // Initialize the error handler with debug output
    errorHandler = new ErrorHandler(usbSerial, uartDebugSerial);
    
    // Give the system time to stabilize
    delay(50);
    
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
        // Log to both terminal and UART for errors
        errorHandler->logError(ERROR, "Failed to mount LittleFS file system");
        while (1) { 
            delay(1000);
        }
    }
    
    // Initialize configuration manager
    configManager = new ConfigManager(errorHandler);
    
    if (!configManager->begin()) {
        errorHandler->logError(ERROR, "Failed to initialize configuration manager");
        while (1) {
            delay(1000);
        }
    }
    
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
    delay(20);
    
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
    delay(20);
    
    // Setup communication
    commManager = new CommunicationManager(sensorManager, configManager, errorHandler, ledManager);
    commManager->begin(115200);
    
    // Pass the UART debug serial to the CommManager
    setUartDebugSerial(uartDebugSerial);
    commManager->setupCommands();
    
    // Give the system time to stabilize
    delay(20);
    
    // Initialize TaskManager - core task management
    taskManager = new TaskManager(sensorManager, commManager, ledManager, errorHandler);
    
    if (!taskManager->begin()) {
        errorHandler->logError(ERROR, "Failed to initialize task manager");
    } else {
        errorHandler->logInfo("Task manager initialized successfully");
        
        // Start tasks one by one with delays in between
        if (taskManager->startLedTask()) {
            errorHandler->logInfo("LED task started successfully");
            delay(200); // Give time for task to stabilize
            
            if (taskManager->startCommTask()) {
                errorHandler->logInfo("Communication task started successfully");
                delay(20); // Give time for task to stabilize
                
                if (taskManager->startSensorTask()) {
                    errorHandler->logInfo("Sensor task started successfully");
                    delay(30); // Give time for task to stabilize

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
 
    // Always yield to the scheduler
    yield();

}
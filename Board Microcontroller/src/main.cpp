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
const int UART_TX_PIN = Constants::Pins::UART::TX;
const int UART_RX_PIN = Constants::Pins::UART::RX;

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
    Serial.setRxBufferSize(4096); // Set RX buffer size to 4096 bytes
    // Wait for USB Serial to be ready up to 500 ms
    unsigned long startTime = millis();
    while (!Serial && (millis() - startTime < 500)) {
        delay(10); // Wait for USB Serial to be ready
    }

    // Initialize USB Serial for debug messages
    usbSerial = &Serial;
    
    // Initialize UART for debug messages
    debugSerial = &Serial2;
    debugSerial->begin(115200, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
    while (!debugSerial && (millis() - startTime < 1000)) {
        delay(10); // Wait for USB Serial to be ready
    }
    uartDebugSerial = debugSerial;
    
    // Initialize the error handler with debug output
    errorHandler = new ErrorHandler(usbSerial, uartDebugSerial);
    
    // Give the system time to stabilize
    delay(50);

    // Initialize LED manager
    ledManager = new LedManager(errorHandler);
    ledManager->begin();
    ledManager->setSetupMode();
    
    // Configure error routing:
    // - INFO and higher go to UART
    // - WARNING and higher go to USB
    errorHandler->setOutputSeverity(uartDebugSerial, INFO);     // UART gets INFO and higher
    errorHandler->setOutputSeverity(usbSerial, WARNING);        // USB gets WARNING and higher
    errorHandler->enableCustomRouting(true);
    errorHandler->setLedManager(ledManager);
    
    // Welcome message through logger
    errorHandler->logError(INFO, "Starting " + String(Constants::PRODUCT_NAME) + " v" + String(Constants::FIRMWARE_VERSION));
    errorHandler->logError(INFO, "Error handler initialized with custom routing");

    // Initialize LittleFS with the specific configuration
    if (!LittleFS.begin(true, "/litlefs", 10, "ffat")) {
        // Log to both terminal and UART for errors
        errorHandler->logError(FATAL, "Failed to mount LittleFS file system");
        while (1) { 
            delay(1000);
        }
    }
    
    // Initialize configuration manager
    configManager = new ConfigManager(errorHandler);
    
    if (!configManager->begin()) {
        errorHandler->logError(FATAL, "Failed to initialize configuration manager");
        while (1) {
            delay(1000);
        }
    }
    
    // Initialize I2C manager
    i2cManager = new I2CManager(errorHandler);
    if (!i2cManager->begin()) {
        errorHandler->logError(WARNING, "Failed to initialize I2C manager");
    }
    
    // Initialize SPI manager
    spiManager = new SPIManager(errorHandler);
    if (!spiManager->begin()) {
        errorHandler->logError(WARNING, "Failed to initialize SPI manager");
    } else {
        // Register SS pins - using array size to determine how many pins to register
        size_t numSsPins = sizeof(Constants::Pins::SPI::SS_PINS) / sizeof(int);
        for (size_t i = 0; i < numSsPins; i++) {
            spiManager->registerSSPin(i);
        }
        errorHandler->logError(INFO, "Registered " + String(numSsPins) + " logical SS pins");
    }
    // Give the system time to stabilize
    delay(20);
    
    // Create sensor manager with both I2C and SPI support
    sensorManager = new SensorManager(configManager, i2cManager, errorHandler, spiManager);
    
    // Initialize sensors
    if (!sensorManager->initializeSensors()) {
        errorHandler->logError(WARNING, "Some sensors failed to initialize");
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
    errorHandler->logError(INFO, "Sensor cache configured with " + String(fastestRate) + "ms max age");
    
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
        errorHandler->logError(FATAL, "Failed to initialize task manager");
    } else {
        errorHandler->logError(INFO, "Task manager initialized successfully");
        
        // Start tasks one by one with delays in between
        if (taskManager->startLedTask()) {
            errorHandler->logError(INFO, "LED task started successfully");
            delay(200); // Give time for task to stabilize
            
            if (taskManager->startCommTask()) {
                errorHandler->logError(INFO, "Communication task started successfully");
                delay(20); // Give time for task to stabilize
                
                if (taskManager->startSensorTask()) {
                    errorHandler->logError(INFO, "Sensor task started successfully");
                    delay(30); // Give time for task to stabilize

                } else {
                    errorHandler->logError(WARNING, "Failed to start sensor task");
                }
            } else {
                errorHandler->logError(WARNING, "Failed to start communication task");
            }
        } else {
            errorHandler->logError(WARNING, "Failed to start LED task");
        }
    }
    
    errorHandler->logError(INFO, "System initialization complete");
    errorHandler->logError(INFO, "System ready. Environmental Monitor ID: " + configManager->getBoardIdentifier());
    ledManager->setNormalMode();
}

void loop() {
    // Always yield to the scheduler
    yield();
}
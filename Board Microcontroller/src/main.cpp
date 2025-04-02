#include <Arduino.h>
#include <LittleFS.h>
#include "Constants.h"
#include "error/ErrorHandler.h"
#include "config/ConfigManager.h"
#include "managers/I2CManager.h"
#include "managers/SPIManager.h"
#include "managers/SensorManager.h"
#include "communication/CommunicationManager.h"

// Define UART pins for debug output
#define UART_TX_PIN 14  // GPIO14
#define UART_RX_PIN 15  // GPIO15

// Forward declaration for setting the debug serial in the CommunicationManager
void setUartDebugSerial(Print* debugSerial);

// Global components
ErrorHandler* errorHandler = nullptr;
ConfigManager* configManager = nullptr;
I2CManager* i2cManager = nullptr;
SPIManager* spiManager = nullptr;
SensorManager* sensorManager = nullptr;
CommunicationManager* commManager = nullptr;
HardwareSerial* debugSerial = nullptr;

// Global references to serial ports
Print* usbSerial = nullptr;
Print* uartDebugSerial = nullptr;

// Configuration change callback
void onConfigChanged(const String& newConfig) {
    // When configuration changes, reconfigure sensors
    if (sensorManager) {
        sensorManager->reconfigureSensors(newConfig);
    }
}

// Helper function to pass debug serial to communication manager
void setUartDebugSerial(Print* debugSerial) {
    if (commManager) {
        CommunicationManager::setUartDebugSerialPtr(debugSerial);
    }
}

void setup() {
    // Initialize USB Serial for command interface
    Serial.begin(115200);
    delay(50); // Give serial time to initialize
    Serial.setRxBufferSize(2048); // Increase buffer to handle larger commands
    usbSerial = &Serial;
    
    // Initialize UART for debug messages
    debugSerial = &Serial2;
    debugSerial->begin(115200, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
    delay(50); // Give serial time to initialize
    uartDebugSerial = debugSerial;

    // Initialize LittleFS with the specific configuration
    if (!LittleFS.begin(true, "/litlefs", 10, "ffat")) {
        Serial.println("LittleFS mount failed! System halted.");
        if (debugSerial) {
            debugSerial->println("LittleFS mount failed! System halted.");
        }
        while (1) delay(1000);
    }
    
    // Welcome message on both interfaces
    if (usbSerial) {
        usbSerial->println("Starting " + String(Constants::PRODUCT_NAME) + " v" + String(Constants::FIRMWARE_VERSION));
    }
    if (uartDebugSerial) {
        uartDebugSerial->println("Debug UART initialized");
        uartDebugSerial->println("Starting " + String(Constants::PRODUCT_NAME) + " v" + String(Constants::FIRMWARE_VERSION));
    }
    
    // Initialize error handler with both output streams
    errorHandler = new ErrorHandler(usbSerial, uartDebugSerial);
    errorHandler->logInfo("Error handler initialized with custom routing");
    
    // Initialize components
    configManager = new ConfigManager(errorHandler);
    i2cManager = new I2CManager(errorHandler);
    spiManager = new SPIManager(errorHandler);
    
    if (!configManager->begin()) {
        errorHandler->logCritical("Failed to initialize configuration. System halted.");
        while (1) delay(1000);
    }
    
    // Register for configuration changes
    configManager->registerChangeCallback(onConfigChanged);
    
    // Initialize SPI manager with the default pins from the header file
    if (spiManager->isInitialized()) {
        // Register all A0-A3 pins as SS pins (using logical indices 0-3)
        for (int i = 0; i < MAX_SS_PINS; i++) {
            spiManager->registerSSPin(i);
        }
        errorHandler->logInfo("Registered logical SS pins 0-3");
    }
    
    // Create sensor manager with both I2C and SPI support
    sensorManager = new SensorManager(configManager, i2cManager, errorHandler, spiManager);
    
    // Initialize sensors
    if (!sensorManager->initializeSensors()) {
        errorHandler->logWarning("Some sensors failed to initialize");
    }
    
    // Setup communication
    commManager = new CommunicationManager(sensorManager, configManager, errorHandler);
    commManager->begin(115200);
    
    // Pass the UART debug serial to the CommManager
    setUartDebugSerial(uartDebugSerial);
    
    commManager->setupCommands();
    
    errorHandler->logInfo("System initialization complete");
    
    if (usbSerial) {
        usbSerial->println("System ready. Environmental Monitor ID: " + configManager->getBoardIdentifier());
    }
    if (uartDebugSerial) {
        uartDebugSerial->println("Debug serial ready. Environmental Monitor ID: " + configManager->getBoardIdentifier());
    }
}

void loop() {
    // Process incoming commands
    commManager->processIncomingData();
    
    // Update sensor readings based on individual polling rates
    sensorManager->updateReadings();
    
    // Small delay to prevent hogging the CPU
    delay(10);
}
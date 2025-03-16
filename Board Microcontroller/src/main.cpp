#include <Arduino.h>
#include <LittleFS.h>
#include "Constants.h"
#include "error/ErrorHandler.h"
#include "config/ConfigManager.h"
#include "managers/I2CManager.h"
#include "managers/SensorManager.h"
#include "communication/CommunicationManager.h"

// Global components
ErrorHandler* errorHandler = nullptr;
ConfigManager* configManager = nullptr;
I2CManager* i2cManager = nullptr;
SensorManager* sensorManager = nullptr;
CommunicationManager* commManager = nullptr;

// Configuration change callback
void onConfigChanged(const String& newConfig) {
    // When configuration changes, reconfigure sensors
    if (sensorManager) {
        sensorManager->reconfigureSensors(newConfig);
    }
}

void setup() {
    Serial.begin(115200);
    delay(500); // Give serial time to initialize
    
    // Initialize LittleFS with the specific configuration
    if (!LittleFS.begin(true, "/litlefs", 10, "ffat")) {
        Serial.println("LittleFS mount failed! System halted.");
        while (1) delay(1000);
    }
    
    Serial.println("Starting " + String(Constants::PRODUCT_NAME) + " v" + String(Constants::FIRMWARE_VERSION));
    
    // Initialize components
    errorHandler = new ErrorHandler(&Serial);
    configManager = new ConfigManager(errorHandler);
    i2cManager = new I2CManager(errorHandler);
    
    if (!configManager->begin()) {
        errorHandler->logCritical("Failed to initialize configuration. System halted.");
        while (1) delay(1000);
    }
    
    // Register for configuration changes
    configManager->registerChangeCallback(onConfigChanged);
    
    // Initialize I2C
    if (!i2cManager->begin()) {
        errorHandler->logError(ERROR, "Failed to initialize I2C. Continuing with limited functionality.");
    }
    
    // Create sensor manager
    sensorManager = new SensorManager(configManager, i2cManager, errorHandler);
    
    // Initialize communication manager
    commManager = new CommunicationManager(sensorManager, configManager, errorHandler);
    
    // Initialize sensors
    if (!sensorManager->initializeSensors()) {
        errorHandler->logWarning("Some sensors failed to initialize");
    }
    
    // Setup communication
    commManager->begin(115200);
    commManager->setupCommands();
    
    errorHandler->logInfo("System initialization complete");
    Serial.println("System ready. Environmental Monitor ID: " + configManager->getBoardIdentifier());
}

void loop() {
    // Process incoming commands
    commManager->processIncomingData();
    
    // Update sensor readings based on individual polling rates
    sensorManager->updateReadings();
    
    // Small delay to prevent hogging the CPU
    delay(10);
}
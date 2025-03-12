#include <Arduino.h>
#include <LittleFS.h>
#include "Constants.h"
#include "error/ErrorHandler.h"
#include "config/ConfigManager.h"
#include "managers/SensorManager.h"
#include "communication/CommunicationManager.h"

// Do NOT include SCPI_Parser here to avoid multiple definition errors

ErrorHandler* errorHandler = nullptr;
ConfigManager* configManager = nullptr;
SensorManager* sensorManager = nullptr;
CommunicationManager* commManager = nullptr;

unsigned long lastReadingTime = 0;
const unsigned long readingInterval = 1000; // 1 second

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
  
  if (!configManager->begin()) {
    errorHandler->logCritical("Failed to initialize configuration. System halted.");
    while (1) delay(1000);
  }
  
  sensorManager = new SensorManager(configManager, errorHandler);
  commManager = new CommunicationManager(sensorManager, configManager, errorHandler);
  
  // Initialize sensors
  if (!sensorManager->initializeSensors()) {
    errorHandler->logWarning("Some sensors failed to initialize");
  }
  
  // Setup communication
  commManager->begin(115200);
  commManager->setupCommands();
  
  errorHandler->logInfo("System initialization complete");
  Serial.println("System ready. Board ID: " + configManager->getBoardIdentifier());
}

void loop() {
  // Add direct serial command handling for testing
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    if (input.equals("TEST")) {
      Serial.println("Serial communication test successful");
    } else {
      // Process regular commands through the communication manager
      commManager->processIncomingData();
    }
  }
  
  // Update sensor readings periodically
  unsigned long currentTime = millis();
  if (currentTime - lastReadingTime >= readingInterval) {
    sensorManager->updateReadings();
    lastReadingTime = currentTime;
  }
}
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
#include "communication/CommunicationManager.h"
#include "SimpleLedTask.h"
#include "SimpleSensorTask.h" 
#include "SimpleCommTask.h"

// Define UART pins for debug output
#define UART_TX_PIN 5   // GPIO 5
#define UART_RX_PIN 16  // GPIO 16

// Poll sensors at this interval (milliseconds)
#define DEFAULT_POLLING_INTERVAL 1000
unsigned long lastPollingTime = 0;
unsigned long lastSerialCheck = 0;

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
LedManager* ledManager = nullptr;

// Task components
SimpleLedTask* ledTask = nullptr;
SimpleSensorTask* sensorTask = nullptr;
SimpleCommTask* commTask = nullptr;

// Global references to serial ports
Print* usbSerial = nullptr;
Print* uartDebugSerial = nullptr;

// Mutex for thread safety
SemaphoreHandle_t sensorMutex = nullptr;

// Configuration change callback
void onConfigChanged(const String& newConfig) {
    // When configuration changes, reconfigure sensors
    if (sensorManager) {
        // Take the mutex if sensor task is running
        bool mutexTaken = false;
        if (sensorMutex != nullptr && sensorTask && sensorTask->isRunning()) {
            mutexTaken = (xSemaphoreTake(sensorMutex, pdMS_TO_TICKS(100)) == pdTRUE);
        }
        
        sensorManager->reconfigureSensors(newConfig);
        
        // Release the mutex if we took it
        if (mutexTaken) {
            xSemaphoreGive(sensorMutex);
        }
    }
}

// Helper function to pass debug serial to communication manager
void setUartDebugSerial(Print* debugSerial) {
    if (commManager) {
        CommunicationManager::setUartDebugSerialPtr(debugSerial);
    }
}

// Find fastest polling rate from sensor configurations
uint32_t getFastestPollingRate() {
    uint32_t fastestRate = UINT32_MAX;
    const uint32_t MIN_POLL_RATE = 50;      // 50ms minimum
    const uint32_t MAX_POLL_RATE = 300000;  // 5 minutes maximum
    
    if (configManager) {
        auto sensorConfigs = configManager->getSensorConfigs();
        
        for (const auto& config : sensorConfigs) {
            if (config.pollingRate < fastestRate) {
                fastestRate = config.pollingRate;
            }
        }
    }
    
    // Apply range validation and default
    if (fastestRate == UINT32_MAX) {
        fastestRate = DEFAULT_POLLING_INTERVAL; // Use default (1000ms)
    } else if (fastestRate < MIN_POLL_RATE) {
        fastestRate = MIN_POLL_RATE;            // Enforce minimum
    } else if (fastestRate > MAX_POLL_RATE) {
        fastestRate = MAX_POLL_RATE;            // Enforce maximum
    }
    
    return fastestRate;
}

void setup() {
    // Initialize USB Serial for command interface
    Serial.begin(115200);
    
    // Print an immediate message to verify serial works
    Serial.println("ESP32 starting up - Initializing multi-core tasks...");
    delay(100); // Give serial time to initialize
    usbSerial = &Serial;
    
    // Initialize UART for debug messages
    debugSerial = &Serial2;
    debugSerial->begin(115200, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
    delay(10); // Give serial time to initialize
    uartDebugSerial = debugSerial;
    
    // Initialize the error handler with debug output to UART only
    errorHandler = new ErrorHandler(usbSerial, uartDebugSerial);
    
    // Initialize LED manager
    ledManager = new LedManager(errorHandler);
    ledManager->begin();
    ledManager->setSetupMode();
    
    // CRITICAL - Set info messages to go to UART only, not to serial terminal
    errorHandler->setInfoOutput(uartDebugSerial);
    errorHandler->enableCustomRouting(true);

    // Welcome message through logger, will go to UART only
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
    
    // Initialize remaining components
    configManager = new ConfigManager(errorHandler);
    
    // Initialize configuration manager
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
    
    // Configure sensor cache settings - adjust cache max age based on polling rates
    uint32_t fastestRate = getFastestPollingRate();
    sensorManager->setMaxCacheAge(fastestRate); // Set cache timeout equal to polling rate
    errorHandler->logInfo("Sensor cache configured with " + String(fastestRate) + "ms max age");
    
    // Setup communication
    commManager = new CommunicationManager(sensorManager, configManager, errorHandler, ledManager);
    commManager->begin(115200);
    
    // Pass the UART debug serial to the CommManager
    setUartDebugSerial(uartDebugSerial);
    
    commManager->setupCommands();
    
    // Create mutex for sensor access
    sensorMutex = xSemaphoreCreateMutex();
    if (sensorMutex == nullptr) {
        errorHandler->logWarning("Failed to create sensor mutex");
    }
    
    // Initialize tasks - but don't start them automatically
    ledTask = new SimpleLedTask(ledManager, errorHandler);
    sensorTask = new SimpleSensorTask(sensorManager, errorHandler, &sensorMutex);
    
    // Use the ultra minimal comm task
    commTask = new SimpleCommTask(errorHandler);
    
    errorHandler->logInfo("System initialization complete");
    errorHandler->logInfo("System ready. Environmental Monitor ID: " + configManager->getBoardIdentifier());
    ledManager->setNormalMode();
    
    // Print a simple ready message to the main terminal
    Serial.println("GPower Environmental Monitor ready (Multi-Core Mode)");
    Serial.println("Task commands: TASK:START:LED, TASK:STOP:LED, TASK:START:SENSOR, TASK:STOP:SENSOR");
    Serial.println("              TASK:START:COMM, TASK:STOP:COMM, TASK:START:ALL, TASK:STOP:ALL");
    Serial.println("              TASK:STATUS, MEMORY");
}

void loop() {
    unsigned long currentTime = millis();
    
    // Check for serial commands (if comm task is not running)
    if (!commTask || !commTask->isRunning()) {
        if (currentTime - lastSerialCheck >= 10) {
            lastSerialCheck = currentTime;
            
            if (Serial.available() > 0) {
                String command = Serial.readStringUntil('\n');
                command.trim();
                
                // Log the received command
                Serial.print("Processing: ");
                Serial.println(command);
                
                // Process basic commands directly for reliability
                if (command == "*IDN?") {
                    Serial.println(String(Constants::PRODUCT_NAME) + "," + 
                                configManager->getBoardIdentifier() + "," +
                                String(Constants::FIRMWARE_VERSION));
                } 
                else if (command == "SYST:LED:IDENT") {
                    if (ledManager) {
                        ledManager->startIdentify();
                        Serial.println("LED identify mode activated");
                    }
                }
                else if (command == "RESET") {
                    Serial.println("Resetting device...");
                    delay(100);  // Give time for the message to be sent
                    ESP.restart();
                }
                else if (command == "TEST") {
                    Serial.println("Serial communication test successful");
                }
                else if (command.startsWith("ECHO")) {
                    Serial.println("ECHO: " + command);
                }
                else if (command == "MEMORY") {
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
                    
                    // Add task info if running
                    Serial.println("=== Task Information ===");
                    if (ledTask) {
                        Serial.println("LED Task: " + String(ledTask->isRunning() ? "RUNNING" : "STOPPED"));
                        if (ledTask->isRunning()) {
                            Serial.println("LED Task stack remaining: " + String(ledTask->getStackHighWaterMark()) + " words");
                        }
                    }
                    
                    if (sensorTask) {
                        Serial.println("Sensor Task: " + String(sensorTask->isRunning() ? "RUNNING" : "STOPPED"));
                        if (sensorTask->isRunning()) {
                            Serial.println("Sensor Task stack remaining: " + String(sensorTask->getStackHighWaterMark()) + " words");
                        }
                    }
                    
                    if (commTask) {
                        Serial.println("Comm Task: " + String(commTask->isRunning() ? "RUNNING" : "STOPPED"));
                        if (commTask->isRunning()) {
                            Serial.println("Comm Task stack remaining: " + String(commTask->getStackHighWaterMark()) + " words");
                        }
                    }
                }
                else if (command == "TASK:STATUS") {
                    // Command to get task status
                    Serial.println("=== Task Status ===");
                    if (ledTask) {
                        Serial.println("LED Task: " + String(ledTask->isRunning() ? "RUNNING" : "STOPPED"));
                        if (ledTask->isRunning()) {
                            Serial.println("LED Task stack remaining: " + String(ledTask->getStackHighWaterMark()) + " words");
                        }
                    }
                    
                    if (sensorTask) {
                        Serial.println("Sensor Task: " + String(sensorTask->isRunning() ? "RUNNING" : "STOPPED"));
                        if (sensorTask->isRunning()) {
                            Serial.println("Sensor Task stack remaining: " + String(sensorTask->getStackHighWaterMark()) + " words");
                        }
                    }
                    
                    if (commTask) {
                        Serial.println("Comm Task: " + String(commTask->isRunning() ? "RUNNING" : "STOPPED"));
                        if (commTask->isRunning()) {
                            Serial.println("Comm Task stack remaining: " + String(commTask->getStackHighWaterMark()) + " words");
                        }
                    }
                }
                else if (command == "TASK:START:LED") {
                    // Command to start the LED task
                    if (ledTask && ledTask->start()) {
                        Serial.println("LED task started successfully");
                    } else {
                        Serial.println("Failed to start LED task");
                    }
                }
                else if (command == "TASK:STOP:LED") {
                    // Command to stop the LED task
                    if (ledTask) {
                        ledTask->stop();
                        Serial.println("LED task stopped");
                    }
                }
                else if (command == "TASK:START:SENSOR") {
                    // Command to start the sensor task
                    if (sensorTask && sensorTask->start()) {
                        Serial.println("Sensor task started successfully");
                    } else {
                        Serial.println("Failed to start sensor task");
                    }
                }
                else if (command == "TASK:STOP:SENSOR") {
                    // Command to stop the sensor task
                    if (sensorTask) {
                        sensorTask->stop();
                        Serial.println("Sensor task stopped");
                    }
                }
                else if (command == "TASK:START:COMM") {
                    // Command to start the communication task
                    if (commTask && commTask->start()) {
                        Serial.println("Communication task started successfully");
                    } else {
                        Serial.println("Failed to start communication task");
                    }
                }
                else if (command == "TASK:STOP:COMM") {
                    // Command to stop the communication task
                    if (commTask) {
                        commTask->stop();
                        Serial.println("Communication task stopped");
                    }
                }
                else if (command == "TASK:START:ALL") {
                    // Command to start all tasks
                    bool allSuccess = true;
                    
                    if (ledTask) {
                        allSuccess &= ledTask->start();
                    }
                    
                    if (sensorTask) {
                        allSuccess &= sensorTask->start();
                    }
                    
                    if (commTask) {
                        allSuccess &= commTask->start();
                    }
                    
                    if (allSuccess) {
                        Serial.println("All tasks started successfully");
                    } else {
                        Serial.println("Some tasks failed to start");
                    }
                }
                else if (command == "TASK:STOP:ALL") {
                    // Command to stop all tasks
                    if (commTask) {
                        commTask->stop();
                    }
                    
                    if (sensorTask) {
                        sensorTask->stop();
                    }
                    
                    if (ledTask) {
                        ledTask->stop();
                    }
                    
                    Serial.println("All tasks stopped");
                }
                else {
                    // For other commands, properly parse and forward
                    if (commManager) {
                        String cmd;
                        std::vector<String> params;
                        commManager->parseCommand(command, cmd, params);
                        
                        // Process the command directly
                        bool success = commManager->processCommand(cmd, params);
                        
                        if (!success) {
                            Serial.println("Command not recognized: " + command);
                        }
                    }
                }
                
                // Flush to ensure all data is sent
                Serial.flush();
            }
        }
    }
    
    // Update sensor readings if task is not running
    if ((!sensorTask || !sensorTask->isRunning()) && 
        (currentTime - lastPollingTime >= getFastestPollingRate())) {
        if (sensorManager) {
            sensorManager->updateReadings(true);
        }
        lastPollingTime = currentTime;
        
        // Signal LED if not in task mode
        if (ledManager && !ledManager->isIdentifying() && (!ledTask || !ledTask->isRunning())) {
            ledManager->indicateReading();
        }
    }
    
    // Update the LED states if the task is not running
    if (ledManager && (!ledTask || !ledTask->isRunning())) {
        ledManager->update();
    }
    
    // Brief delay to prevent CPU hogging
    delay(1);
}
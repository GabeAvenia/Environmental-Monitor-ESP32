#include <Arduino.h>
#include <LittleFS.h>
#include <esp_task_wdt.h>
#include <esp_system.h>
#include "Constants.h"
#include "error/ErrorHandler.h"
#include "config/ConfigManager.h"
#include "managers/I2CManager.h"
#include "managers/SPIManager.h"
#include "managers/SensorManager.h"
#include "managers/LedManager.h"
#include "communication/CommunicationManager.h"

// Define UART pins for debug output
#define UART_TX_PIN 5   // GPIO 5
#define UART_RX_PIN 16  // GPIO 16

// Watchdog configuration - disabled by default
// We'll use a much longer timeout as the hardware watchdog was causing issues
#define WATCHDOG_TIMEOUT_SECONDS 30  // Increased to 30 seconds
#define WATCHDOG_ENABLED false       // Disable the watchdog by default

unsigned long lastWatchdogFeed = 0;
bool watchdogEnabled = false;

// Poll sensors at this interval (milliseconds)
#define DEFAULT_POLLING_INTERVAL 1000
unsigned long lastPollingTime = 0;

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

// Global references to serial ports
Print* usbSerial = nullptr;
Print* uartDebugSerial = nullptr;

// Watchdog functions
void enableWatchdog() {
    if (WATCHDOG_ENABLED) {
        esp_task_wdt_init(WATCHDOG_TIMEOUT_SECONDS, true); // true = panic on timeout
        esp_task_wdt_add(NULL); // Add current task to WDT
        watchdogEnabled = true;
    } else {
        watchdogEnabled = false;
    }
}

void feedWatchdog() {
    if (watchdogEnabled) {
        esp_task_wdt_reset();
        lastWatchdogFeed = millis();
    }
}

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
    ledManager = new LedManager(errorHandler);
    ledManager->begin();
    ledManager->setSetupMode();
    delay(10); // Give serial time to initialize
    usbSerial = &Serial;
    
    // Initialize UART for debug messages
    debugSerial = &Serial2;
    debugSerial->begin(115200, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
    delay(10); // Give serial time to initialize
    uartDebugSerial = debugSerial;
    
    // Initialize the error handler with debug output to UART only
    errorHandler = new ErrorHandler(usbSerial, uartDebugSerial);
    
    // CRITICAL - Set info messages to go to UART only, not to serial terminal
    errorHandler->setInfoOutput(uartDebugSerial);
    errorHandler->enableCustomRouting(true);
    
    // Initialize hardware watchdog
    enableWatchdog();
    feedWatchdog();

    // Welcome message through logger, will go to UART only
    errorHandler->logInfo("Starting " + String(Constants::PRODUCT_NAME) + " v" + String(Constants::FIRMWARE_VERSION));
    errorHandler->logInfo("Error handler initialized with custom routing");

    // Initialize LittleFS with the specific configuration
    if (!LittleFS.begin(true, "/litlefs", 10, "ffat")) {
        // Log to both terminal and UART for critical errors
        errorHandler->logCritical("LittleFS mount failed! System halted.");
        while (1) { 
            delay(1000);
            feedWatchdog(); // Keep feeding watchdog even in error state
        }
    }
    
    feedWatchdog();
    
    // Initialize remaining components
    configManager = new ConfigManager(errorHandler);
    
    // Initialize configuration manager
    if (!configManager->begin()) {
        errorHandler->logCritical("Failed to initialize configuration. System halted.");
        while (1) {
            delay(1000);
            feedWatchdog(); // Keep feeding watchdog even in error state
        }
    }
    
    feedWatchdog();
    
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
    
    // Feed watchdog after SPI initialization
    feedWatchdog();
    
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
    
    // Feed watchdog after sensor initialization
    feedWatchdog();
    
    // Setup communication
    commManager = new CommunicationManager(sensorManager, configManager, errorHandler, ledManager);
    commManager->begin(115200);
    
    // Pass the UART debug serial to the CommManager
    setUartDebugSerial(uartDebugSerial);
    
    commManager->setupCommands();
    
    // Final watchdog feed after full initialization
    feedWatchdog();
    
    errorHandler->logInfo("System initialization complete");
    errorHandler->logInfo("System ready. Environmental Monitor ID: " + configManager->getBoardIdentifier());
    ledManager->setNormalMode();    
    // Print a simple ready message to the main terminal
    Serial.println(String(Constants::PRODUCT_NAME) + " ready.");

}

void loop() {
    // Feed watchdog if enabled
    feedWatchdog();
    if (ledManager) {
        ledManager->update();
    }
    // Process commands if available
    commManager->processIncomingData();
    
    // Update sensor readings based on configured polling rates
    // This will only update sensors that need updating based on the cache age
    unsigned long currentTime = millis();
    uint32_t pollingInterval = getFastestPollingRate();
    
 
    // to save power - cache system will still update when needed
    uint32_t backgroundPollingInterval = pollingInterval;
    
    // At least 1 second (using ternary operator instead of max)
    backgroundPollingInterval = (backgroundPollingInterval > 1000) ? backgroundPollingInterval : 1000;
    
    if (currentTime - lastPollingTime >= pollingInterval) {
        // Force update readings every polling cycle to ensure fresh values
        sensorManager->updateReadings(true);
        lastPollingTime = currentTime;
        if (ledManager && !ledManager->isIdentifying()) {
            ledManager->indicateReading();
        }  // Pulse green when reading sensors
        // Small delay to allow other tasks to run
        vTaskDelay(5);
    } else {
        // Minimal delay when not polling
        vTaskDelay(1);
    }
}
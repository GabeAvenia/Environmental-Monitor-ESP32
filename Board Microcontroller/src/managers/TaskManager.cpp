#include "TaskManager.h"
#include "managers/SensorManager.h"
#include "managers/LedManager.h"
#include "communication/CommunicationManager.h"
#include "error/ErrorHandler.h"
#include "Constants.h"

// Static task functions that call the appropriate object method
void TaskManager::sensorTaskFunction(void* pvParameters) {
    TaskManager* taskManager = static_cast<TaskManager*>(pvParameters);
    taskManager->sensorTask();
}

void TaskManager::commTaskFunction(void* pvParameters) {
    TaskManager* taskManager = static_cast<TaskManager*>(pvParameters);
    taskManager->commTask();
}

void TaskManager::ledTaskFunction(void* pvParameters) {
    TaskManager* taskManager = static_cast<TaskManager*>(pvParameters);
    taskManager->ledTask();
}

TaskManager::TaskManager(SensorManager* sensorMgr, CommunicationManager* commMgr, 
                        LedManager* ledMgr, ErrorHandler* errHandler)
    : sensorManager(sensorMgr),
      commManager(commMgr),
      ledManager(ledMgr),
      errorHandler(errHandler) {
}

TaskManager::~TaskManager() {
    cleanupTasks();
}

bool TaskManager::begin() {
    // Create the mutex for sensor access
    sensorMutex = xSemaphoreCreateMutex();
    
    if (sensorMutex == nullptr) {
        if (errorHandler) {
            errorHandler->logError(ERROR, "Failed to create sensor mutex");
        }
        return false;
    }
    
    // Pass the mutex to the sensor manager
    if (sensorManager) {
        sensorManager->setSensorMutex(&sensorMutex);
        if (errorHandler) {
            errorHandler->logInfo("Sensor mutex initialized and passed to sensor manager");
        }
    }
    
    return true;
}

bool TaskManager::startAllTasks() {
    bool success = true;
    
    // Start the LED task
    success &= startLedTask();
    
    // Start the sensor task
    success &= startSensorTask();
    
    // Start the communication task
    success &= startCommTask();
    
    tasksInitialized = success;
    return success;
}

bool TaskManager::startLedTask() {
    if (ledTaskHandle != nullptr) {
        // Task already running
        return true;
    }
    
    if (!ledManager) {
        if (errorHandler) {
            errorHandler->logError(ERROR, "LED manager not initialized for task creation");
        }
        return false;
    }
    
    // Create the LED task on Core 0
    BaseType_t result = xTaskCreatePinnedToCore(
        ledTaskFunction,          // Task function
        TASK_NAME_LED,            // Task name
        STACK_SIZE_LED,           // Stack size
        this,                     // Task parameter (this pointer)
        PRIORITY_LED,             // Priority
        &ledTaskHandle,           // Task handle
        CORE_LED                  // Core ID
    );
    
    if (result != pdPASS) {
        if (errorHandler) {
            errorHandler->logError(ERROR, "Failed to create LED task");
        }
        return false;
    }
    
    if (errorHandler) {
        errorHandler->logInfo("LED task created successfully on Core " + String(CORE_LED));
    }
    
    return true;
}

bool TaskManager::startSensorTask() {
    if (sensorTaskHandle != nullptr) {
        // Task already running
        return true;
    }
    
    if (!sensorManager) {
        if (errorHandler) {
            errorHandler->logError(ERROR, "Sensor manager not initialized for task creation");
        }
        return false;
    }
    
    // Create the sensor task on Core 1
    BaseType_t result = xTaskCreatePinnedToCore(
        sensorTaskFunction,       // Task function
        TASK_NAME_SENSOR,         // Task name
        STACK_SIZE_SENSOR,        // Stack size
        this,                     // Task parameter (this pointer)
        PRIORITY_SENSOR,          // Priority
        &sensorTaskHandle,        // Task handle
        CORE_SENSOR               // Core ID
    );
    
    if (result != pdPASS) {
        if (errorHandler) {
            errorHandler->logError(ERROR, "Failed to create sensor task");
        }
        return false;
    }
    
    if (errorHandler) {
        errorHandler->logInfo("Sensor task created successfully on Core " + String(CORE_SENSOR));
    }
    
    return true;
}

bool TaskManager::startCommTask() {
    if (commTaskHandle != nullptr) {
        // Task already running
        return true;
    }
    
    if (!commManager) {
        if (errorHandler) {
            errorHandler->logError(ERROR, "Communication manager not initialized for task creation");
        }
        return false;
    }
    
    // Create the communication task on Core 0
    BaseType_t result = xTaskCreatePinnedToCore(
        commTaskFunction,         // Task function
        TASK_NAME_COMM,           // Task name
        STACK_SIZE_COMM,          // Stack size
        this,                     // Task parameter (this pointer)
        PRIORITY_COMM,            // Priority
        &commTaskHandle,          // Task handle
        CORE_COMM                 // Core ID
    );
    
    if (result != pdPASS) {
        if (errorHandler) {
            errorHandler->logError(ERROR, "Failed to create communication task");
        }
        return false;
    }
    
    if (errorHandler) {
        errorHandler->logInfo("Communication task created successfully on Core " + String(CORE_COMM));
    }
    
    return true;
}

bool TaskManager::areAllTasksRunning() const {
    return (ledTaskHandle != nullptr && 
            sensorTaskHandle != nullptr &&
            commTaskHandle != nullptr);
}

void TaskManager::cleanupTasks() {
    // Delete all tasks if they exist
    if (ledTaskHandle != nullptr) {
        vTaskDelete(ledTaskHandle);
        ledTaskHandle = nullptr;
    }
    
    if (sensorTaskHandle != nullptr) {
        vTaskDelete(sensorTaskHandle);
        sensorTaskHandle = nullptr;
    }
    
    if (commTaskHandle != nullptr) {
        vTaskDelete(commTaskHandle);
        commTaskHandle = nullptr;
    }
    
    // Delete the mutex if it exists
    if (sensorMutex != nullptr) {
        vSemaphoreDelete(sensorMutex);
        sensorMutex = nullptr;
    }
    
    tasksInitialized = false;
}

SemaphoreHandle_t* TaskManager::getSensorMutex() {
    return &sensorMutex;
}

String TaskManager::getTaskStateString(TaskHandle_t handle) {
    if (handle == nullptr) {
        return "NOT_CREATED";
    }
    
    eTaskState state = eTaskGetState(handle);
    
    switch (state) {
        case eRunning:
            return "RUNNING";
        case eReady:
            return "READY";
        case eBlocked:
            return "BLOCKED";
        case eSuspended:
            return "SUSPENDED";
        case eDeleted:
            return "DELETED";
        default:
            return "UNKNOWN";
    }
}

String TaskManager::getTaskStatusString() const {
    String status = "Task Status:\n";
    
    status += "LED Task: " + getTaskStateString(ledTaskHandle);
    if (ledTaskHandle) {
        status += " (Core " + String(CORE_LED) + ")\n";
    } else {
        status += "\n";
    }
    
    status += "Sensor Task: " + getTaskStateString(sensorTaskHandle);
    if (sensorTaskHandle) {
        status += " (Core " + String(CORE_SENSOR) + ")\n";
    } else {
        status += "\n";
    }
    
    status += "Communication Task: " + getTaskStateString(commTaskHandle);
    if (commTaskHandle) {
        status += " (Core " + String(CORE_COMM) + ")\n";
    } else {
        status += "\n";
    }
    
    return status;
}

String TaskManager::getTaskMemoryInfo() const {
    String info = "Task Memory Info:\n";
    
    if (ledTaskHandle) {
        info += "LED Task: " + String(uxTaskGetStackHighWaterMark(ledTaskHandle)) + 
                " words remaining\n";
    }
    
    if (sensorTaskHandle) {
        info += "Sensor Task: " + String(uxTaskGetStackHighWaterMark(sensorTaskHandle)) + 
                " words remaining\n";
    }
    
    if (commTaskHandle) {
        info += "Communication Task: " + String(uxTaskGetStackHighWaterMark(commTaskHandle)) + 
                " words remaining\n";
    }
    
    // Add overall free heap
    info += "Free heap: " + String(ESP.getFreeHeap()) + " bytes\n";
    
    return info;
}

// Task implementation methods

void TaskManager::sensorTask() {
    // Wait for SensorManager to be fully initialized
    while (sensorManager == nullptr) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    if (errorHandler) {
        errorHandler->logInfo("Sensor polling task started on Core " + String(xPortGetCoreID()));
    }
    
    uint32_t lastPollingTime = 0;
    
    // Task loop
    while (true) {
        unsigned long currentTime = millis();
        uint32_t pollingInterval = 1000; // Default interval
        
        // Get fastest polling rate from sensor manager
        if (sensorManager) {
            pollingInterval = sensorManager->getMaxCacheAge();
        }
        
        // Check if it's time to poll
        if (currentTime - lastPollingTime >= pollingInterval) {
            // Take the mutex before accessing the sensor manager
            if (sensorMutex != NULL && xSemaphoreTake(sensorMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                // Update all sensor readings
                if (sensorManager) {
                    sensorManager->updateReadings(true);
                }
                
                // Release the mutex
                xSemaphoreGive(sensorMutex);
                
                // Update timestamp
                lastPollingTime = currentTime;
                
                // Signal the LED
                if (ledManager && !ledManager->isIdentifying()) {
                    ledManager->indicateReading();
                }
            }
        }
        
        // Short delay to prevent task starvation
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void TaskManager::commTask() {
    // Wait for CommunicationManager to be fully initialized
    while (commManager == nullptr) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    if (errorHandler) {
        errorHandler->logInfo("Communication task started on Core " + String(xPortGetCoreID()));
    }
    
    // Task loop
    while (true) {
        // Check for serial data with a quick timeout
        if (Serial.available() > 0) {
            // Read the entire command line
            String command = Serial.readStringUntil('\n');
            command.trim();
            
            // Log the received command
            Serial.print("Processing: ");
            Serial.println(command);
            
            // Process basic commands directly for reliability
            if (command == "*IDN?") {
                Serial.println(String(Constants::PRODUCT_NAME) + "," + 
                           "ID-" + String(ESP.getEfuseMac(), HEX) + "," +
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
            else if (command == "TASK:STATUS") {
                // Custom command to get task status
                Serial.println(getTaskStatusString());
            }
            else if (command == "TASK:MEMORY") {
                // Custom command to get memory info
                Serial.println(getTaskMemoryInfo());
            }
            else {
                // For other commands, parse and handle
                if (commManager) {
                    // Use the traditional command processing
                    commManager->processIncomingData();
                }
            }
            
            // Ensure all responses are sent
            Serial.flush();
        }
        
        // Short delay to prevent task starvation
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void TaskManager::ledTask() {
    // Wait for LedManager to be fully initialized
    while (ledManager == nullptr) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    if (errorHandler) {
        errorHandler->logInfo("LED update task started on Core " + String(xPortGetCoreID()));
    }
    
    // Task loop
    while (true) {
        // Update LED states
        if (ledManager) {
            ledManager->update();
        }
        
        // Short delay
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
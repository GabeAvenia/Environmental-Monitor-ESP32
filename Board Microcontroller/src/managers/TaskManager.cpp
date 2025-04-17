#include "managers/TaskManager.h"
#include "managers/SensorManager.h"
#include "managers/LedManager.h"
#include "communication/CommunicationManager.h"
#include "error/ErrorHandler.h"
#include <algorithm> // For std::max

// Static task functions that call the appropriate object method
void TaskManager::sensorTaskFunction(void* pvParameters) {
    // Add a delay before accessing parameters to ensure system is stable
    vTaskDelay(pdMS_TO_TICKS(50));
    
    TaskManager* taskManager = static_cast<TaskManager*>(pvParameters);
    if (taskManager) {
        taskManager->sensorTask();
    } else {
        // Safety check - this should never happen
        vTaskDelete(NULL);
    }
}

void TaskManager::commTaskFunction(void* pvParameters) {
    // Add a delay before accessing parameters to ensure system is stable
    vTaskDelay(pdMS_TO_TICKS(50));
    
    TaskManager* taskManager = static_cast<TaskManager*>(pvParameters);
    if (taskManager) {
        taskManager->commTask();
    } else {
        // Safety check - this should never happen
        vTaskDelete(NULL);
    }
}

void TaskManager::ledTaskFunction(void* pvParameters) {
    // Add a delay before accessing parameters to ensure system is stable
    vTaskDelay(pdMS_TO_TICKS(50));
    
    TaskManager* taskManager = static_cast<TaskManager*>(pvParameters);
    if (taskManager) {
        taskManager->ledTask();
    } else {
        // Safety check - this should never happen
        vTaskDelete(NULL);
    }
}

TaskManager::TaskManager(SensorManager* sensorMgr, CommunicationManager* commMgr, 
                        LedManager* ledMgr, ErrorHandler* errHandler)
    : sensorManager(sensorMgr),
      commManager(commMgr),
      ledManager(ledMgr),
      errorHandler(errHandler) {
    // Initialize all task handles to nullptr
    sensorTaskHandle = nullptr;
    commTaskHandle = nullptr;
    ledTaskHandle = nullptr;
}

TaskManager::~TaskManager() {
    cleanupTasks();
}

bool TaskManager::begin() {
    // Since we're using double buffering for thread safety, we don't need a mutex
    // We're keeping this method for future initialization if needed
    
    if (errorHandler) {
        errorHandler->logInfo("Task manager initialization started");
    }
    
    // Initialize any resources needed for tasks (but not the mutex)
    
    if (errorHandler) {
        errorHandler->logInfo("Task manager initialized successfully");
    }
    
    return true;
}

bool TaskManager::startAllTasks() {
    bool success = true;
    
    // Start the LED task first - it's the simplest
    success &= startLedTask();
    
    // Give time for the LED task to initialize
    delay(100);
    
    // Start the communication task next
    success &= startCommTask();
    
    // Give time for the communication task to initialize
    delay(100);
    
    // Start the sensor task last
    success &= startSensorTask();
    
    // Give time for all tasks to stabilize
    delay(100);
    
    // No need to pass a mutex to SensorManager anymore
    // Just log success or failure
    if (success) {
        if (errorHandler) {
            errorHandler->logInfo("All tasks started successfully");
        }
    } else {
        if (errorHandler) {
            errorHandler->logError(ERROR, "Failed to start all tasks");
        }
    }
    
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
    
    // Create the LED task on Core 0 with a larger stack
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
    
    // Create the sensor task on Core 0 with a larger stack
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
    
    // Create the communication task on Core 1 with a larger stack
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

    
    tasksInitialized = false;
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

void TaskManager::sensorTask() {
    // Safety check for null pointers
    if (!sensorManager) {
        if (errorHandler) {
            errorHandler->logError(ERROR, "Sensor manager is null in sensor task!");
        }
        vTaskDelete(NULL);
        return;
    }
    
    if (errorHandler) {
        errorHandler->logInfo("Sensor polling task started on Core " + String(xPortGetCoreID()));
    }
    
    uint32_t lastPollingTime = 0;
    uint32_t lastI2CRecoveryTime = 0;
    const uint32_t I2C_RECOVERY_INTERVAL = 5000;
    
    // Task loop
    while (true) {
        if (!sensorManager) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        
        unsigned long currentTime = millis();
        uint32_t pollingInterval = std::max((uint32_t)sensorManager->getMaxCacheAge(), (uint32_t)50);
        
        // Check if it's time to poll
        if (currentTime - lastPollingTime >= pollingInterval) {
            // No mutex needed for our double-buffered approach!
            
            // Check for disconnected sensors and reconnect if needed
            if (currentTime - lastI2CRecoveryTime > I2C_RECOVERY_INTERVAL) {
                bool needsRecovery = false;
                auto registry = sensorManager->getRegistry();
                auto sensors = registry.getAllSensors();
                
                for (auto sensor : sensors) {
                    if (!sensor->isConnected()) {
                        needsRecovery = true;
                        break;
                    }
                }
                
                if (needsRecovery) {
                    if (errorHandler) {
                        errorHandler->logInfo("Attempting sensor recovery");
                    }
                    
                    // Try to reconnect sensors
                    sensorManager->reconnectAllSensors();
                    lastI2CRecoveryTime = currentTime;
                }
            }
            
            // Update all readings - this will write to the active buffer
            // and then swap buffers atomically when done
            sensorManager->updateReadings();
            
            // Update timestamp
            lastPollingTime = currentTime;
            
            // Signal the LED
            if (ledManager && !ledManager->isIdentifying()) {
                ledManager->indicateReading();
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void TaskManager::commTask() {
    // Safety check
    if (!commManager) {
        if (errorHandler) {
            errorHandler->logError(ERROR, "Communication manager is null in comm task!");
        }
        // Delete the task - it can't function
        vTaskDelete(NULL);
        return;
    }
    
    if (errorHandler) {
        errorHandler->logInfo("Communication task started on Core " + String(xPortGetCoreID()));
    }
    
    // Task loop
    while (true) {
        // Check for serial data
        if (Serial.available() > 0 && commManager) {
            // Process the command with error handling
            try {
                commManager->processCommandLine();
            } catch (...) {
                // Handle exceptions (though Arduino doesn't fully support them)
                if (errorHandler) {
                    errorHandler->logError(ERROR, "Exception in command processing");
                }
                
                // Consume any remaining input to prevent command buildup
                while (Serial.available()) {
                    Serial.read();
                }
            }
        }
        
        // Delay to reduce CPU usage
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void TaskManager::ledTask() {
    // Safety check
    if (!ledManager) {
        if (errorHandler) {
            errorHandler->logError(ERROR, "LED manager is null in LED task!");
        }
        // Delete the task - it can't function
        vTaskDelete(NULL);
        return;
    }
    
    if (errorHandler) {
        errorHandler->logInfo("LED update task started on Core " + String(xPortGetCoreID()));
    }
    
    // Task loop
    while (true) {
        // Update LED states - check ledManager is still valid
        if (ledManager) {
            ledManager->update();
        }
        
        // Use a slightly longer delay to reduce CPU usage
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
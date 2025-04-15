#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include "managers/SensorManager.h"
#include "error/ErrorHandler.h"

/**
 * @brief A minimal class that handles sensor polling on Core 1
 */
class SimpleSensorTask {
private:
    TaskHandle_t sensorTaskHandle = nullptr;
    SensorManager* sensorManager = nullptr;
    ErrorHandler* errorHandler = nullptr;
    SemaphoreHandle_t* sensorMutex = nullptr;
    
    // Constants for the task
    static constexpr uint32_t STACK_SIZE = 4096;
    static constexpr UBaseType_t PRIORITY = 1;
    static constexpr BaseType_t CORE_ID = 1;  // Run on Core 1
    
    // Static function for the task
    static void sensorTaskFunction(void* parameter) {
        SimpleSensorTask* task = static_cast<SimpleSensorTask*>(parameter);
        
        if (task && task->errorHandler) {
            task->errorHandler->logInfo("Sensor task started on Core " + String(xPortGetCoreID()));
        }
        
        uint32_t lastPollingTime = 0;
        
        while (true) {
            unsigned long currentTime = millis();
            uint32_t pollingInterval = 1000; // Default interval
            
            // Get fastest polling rate from sensor manager
            if (task && task->sensorManager) {
                pollingInterval = task->sensorManager->getMaxCacheAge();
            }
            
            // Check if it's time to poll
            if (currentTime - lastPollingTime >= pollingInterval) {
                // Take the mutex if available
                bool mutexTaken = false;
                if (task && task->sensorMutex && *(task->sensorMutex)) {
                    mutexTaken = (xSemaphoreTake(*(task->sensorMutex), pdMS_TO_TICKS(100)) == pdTRUE);
                }
                
                // Update all sensor readings
                if (task && task->sensorManager) {
                    task->sensorManager->updateReadings(true);
                }
                
                // Release the mutex if we took it
                if (mutexTaken) {
                    xSemaphoreGive(*(task->sensorMutex));
                }
                
                // Update timestamp
                lastPollingTime = currentTime;
            }
            
            // Short delay to prevent task starvation
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

public:
    SimpleSensorTask(SensorManager* sensorMgr, ErrorHandler* errHandler, SemaphoreHandle_t* mutex = nullptr) 
        : sensorManager(sensorMgr), errorHandler(errHandler), sensorMutex(mutex) {
    }
    
    /**
     * @brief Start the sensor task
     * @return true if started successfully, false otherwise
     */
    bool start() {
        if (sensorTaskHandle != nullptr) {
            // Task already running
            return true;
        }
        
        if (!sensorManager) {
            if (errorHandler) {
                errorHandler->logError(ERROR, "Sensor manager not initialized for task");
            }
            return false;
        }
        
        // Create the task
        BaseType_t result = xTaskCreatePinnedToCore(
            sensorTaskFunction,     // Function
            "SensorTask",           // Name
            STACK_SIZE,             // Stack size
            this,                   // Parameter
            PRIORITY,               // Priority
            &sensorTaskHandle,      // Handle
            CORE_ID                 // Core ID
        );
        
        if (result != pdPASS) {
            if (errorHandler) {
                errorHandler->logError(ERROR, "Failed to create sensor task");
            }
            return false;
        }
        
        if (errorHandler) {
            errorHandler->logInfo("Sensor task created successfully on Core " + String(CORE_ID));
        }
        
        return true;
    }
    
    /**
     * @brief Stop the sensor task
     */
    void stop() {
        if (sensorTaskHandle != nullptr) {
            vTaskDelete(sensorTaskHandle);
            sensorTaskHandle = nullptr;
            
            if (errorHandler) {
                errorHandler->logInfo("Sensor task stopped");
            }
        }
    }
    
    /**
     * @brief Check if the sensor task is running
     * @return true if running, false otherwise
     */
    bool isRunning() const {
        return sensorTaskHandle != nullptr;
    }
    
    /**
     * @brief Get stack high water mark
     * @return Remaining words in stack, or 0 if task not running
     */
    UBaseType_t getStackHighWaterMark() const {
        if (sensorTaskHandle) {
            return uxTaskGetStackHighWaterMark(sensorTaskHandle);
        }
        return 0;
    }
    
    /**
     * @brief Set the sensor mutex
     * @param mutex Pointer to the mutex
     */
    void setSensorMutex(SemaphoreHandle_t* mutex) {
        sensorMutex = mutex;
    }
};

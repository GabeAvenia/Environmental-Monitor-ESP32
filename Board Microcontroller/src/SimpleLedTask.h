#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "managers/LedManager.h"
#include "error/ErrorHandler.h"

/**
 * @brief A very minimal class that handles only the LED task
 */
class SimpleLedTask {
private:
    TaskHandle_t ledTaskHandle = nullptr;
    LedManager* ledManager = nullptr;
    ErrorHandler* errorHandler = nullptr;
    
    // Constants for the task
    static constexpr uint32_t STACK_SIZE = 2048;
    static constexpr UBaseType_t PRIORITY = 1;
    static constexpr BaseType_t CORE_ID = 0;  // Run on Core 0
    
    // Static function for the task
    static void ledTaskFunction(void* parameter) {
        SimpleLedTask* task = static_cast<SimpleLedTask*>(parameter);
        
        if (task && task->errorHandler) {
            task->errorHandler->logInfo("LED task started on Core " + String(xPortGetCoreID()));
        }
        
        while (true) {
            // Update LED state
            if (task && task->ledManager) {
                task->ledManager->update();
            }
            
            // Short delay to prevent task starvation
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }

public:
    SimpleLedTask(LedManager* ledMgr, ErrorHandler* errHandler) 
        : ledManager(ledMgr), errorHandler(errHandler) {
    }
    
    /**
     * @brief Start the LED task
     * @return true if started successfully, false otherwise
     */
    bool start() {
        if (ledTaskHandle != nullptr) {
            // Task already running
            return true;
        }
        
        if (!ledManager) {
            if (errorHandler) {
                errorHandler->logError(ERROR, "LED manager not initialized for task");
            }
            return false;
        }
        
        // Very simple task creation
        BaseType_t result = xTaskCreatePinnedToCore(
            ledTaskFunction,      // Function
            "LedTask",            // Name
            STACK_SIZE,           // Stack size
            this,                 // Parameter
            PRIORITY,             // Priority
            &ledTaskHandle,       // Handle
            CORE_ID               // Core ID
        );
        
        if (result != pdPASS) {
            if (errorHandler) {
                errorHandler->logError(ERROR, "Failed to create LED task");
            }
            return false;
        }
        
        if (errorHandler) {
            errorHandler->logInfo("LED task created successfully on Core " + String(CORE_ID));
        }
        
        return true;
    }
    
    /**
     * @brief Stop the LED task
     */
    void stop() {
        if (ledTaskHandle != nullptr) {
            vTaskDelete(ledTaskHandle);
            ledTaskHandle = nullptr;
            
            if (errorHandler) {
                errorHandler->logInfo("LED task stopped");
            }
        }
    }
    
    /**
     * @brief Check if the LED task is running
     * @return true if running, false otherwise
     */
    bool isRunning() const {
        return ledTaskHandle != nullptr;
    }
    
    /**
     * @brief Get stack high water mark
     * @return Remaining words in stack, or 0 if task not running
     */
    UBaseType_t getStackHighWaterMark() const {
        if (ledTaskHandle) {
            return uxTaskGetStackHighWaterMark(ledTaskHandle);
        }
        return 0;
    }
};
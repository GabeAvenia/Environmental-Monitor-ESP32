#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "Constants.h"
#include "error/ErrorHandler.h"

/**
 * @brief Ultra minimal communication task that only handles basic commands
 */
class SimpleCommTask {
private:
    TaskHandle_t commTaskHandle = nullptr;
    ErrorHandler* errorHandler = nullptr;
    
    // Constants for the task
    static constexpr uint32_t STACK_SIZE = 8192;  // Much larger stack
    static constexpr UBaseType_t PRIORITY = 1;    // Same priority as other tasks
    static constexpr BaseType_t CORE_ID = 0;      // Run on Core 0
    
    // Static function for the task
    static void commTaskFunction(void* parameter) {
        SimpleCommTask* task = static_cast<SimpleCommTask*>(parameter);
        
        if (task && task->errorHandler) {
            task->errorHandler->logInfo("Ultra simple communication task started on Core " + String(xPortGetCoreID()));
        }
        
        // Give system time to stabilize after task creation
        vTaskDelay(pdMS_TO_TICKS(500));
        
        while (true) {
            // Only the most basic functionality: check for *IDN? command
            if (Serial.available() > 0) {
                String command = Serial.readStringUntil('\n');
                command.trim();
                
                // Only handle the simplest commands
                if (command == "*IDN?") {
                    Serial.println("GPower Environmental Monitor (Minimal Mode)");
                }
                else if (command == "TEST") {
                    Serial.println("Serial communication test successful (Minimal Mode)");
                }
                else if (command.startsWith("ECHO")) {
                    Serial.println("ECHO: " + command + " (Minimal Mode)");
                }
                
                // Flush to ensure all data is sent
                Serial.flush();
            }
            
            // Longer delay to prevent issues
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

public:
    SimpleCommTask(ErrorHandler* errHandler) 
        : errorHandler(errHandler) {
    }
    
    /**
     * @brief Start the communication task
     * @return true if started successfully, false otherwise
     */
    bool start() {
        if (commTaskHandle != nullptr) {
            // Task already running
            return true;
        }
        
        // Create the task with very simple parameters
        BaseType_t result = xTaskCreatePinnedToCore(
            commTaskFunction,       // Function
            "CommTask",             // Name
            STACK_SIZE,             // Much larger stack size
            this,                   // Parameter
            PRIORITY,               // Lower priority
            &commTaskHandle,        // Handle
            CORE_ID                 // Core ID
        );
        
        if (result != pdPASS) {
            if (errorHandler) {
                errorHandler->logError(ERROR, "Failed to create minimal communication task");
            }
            return false;
        }
        
        if (errorHandler) {
            errorHandler->logInfo("Minimal communication task created successfully on Core " + String(CORE_ID));
        }
        
        return true;
    }
    
    /**
     * @brief Stop the communication task
     */
    void stop() {
        if (commTaskHandle != nullptr) {
            vTaskDelete(commTaskHandle);
            commTaskHandle = nullptr;
            
            if (errorHandler) {
                errorHandler->logInfo("Communication task stopped");
            }
        }
    }
    
    /**
     * @brief Check if the communication task is running
     * @return true if running, false otherwise
     */
    bool isRunning() const {
        return commTaskHandle != nullptr;
    }
    
    /**
     * @brief Get stack high water mark
     * @return Remaining words in stack, or 0 if task not running
     */
    UBaseType_t getStackHighWaterMark() const {
        if (commTaskHandle) {
            return uxTaskGetStackHighWaterMark(commTaskHandle);
        }
        return 0;
    }
};
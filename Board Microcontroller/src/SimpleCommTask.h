#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include "error/ErrorHandler.h"

/**
 * @brief A class that handles communication on Core 1 safely
 */
class SimpleCommTask {
private:
    TaskHandle_t commTaskHandle = nullptr;
    ErrorHandler* errorHandler = nullptr;
    SemaphoreHandle_t* commMutex = nullptr;
    
    // Constants for the task
    static constexpr uint32_t STACK_SIZE = 4096;
    static constexpr UBaseType_t PRIORITY = 1;
    static constexpr BaseType_t CORE_ID = 1;  // Run on Core 1
    
    // Communication buffer
    static constexpr size_t MAX_BUFFER_SIZE = 256;
    char rxBuffer[MAX_BUFFER_SIZE];
    
    // Static function for the task - needed for FreeRTOS
    static void commTaskFunction(void* parameter) {
        SimpleCommTask* task = static_cast<SimpleCommTask*>(parameter);
        
        // Validate task pointer
        if (!task) {
            return;  // Exit if task pointer is invalid
        }
        
        // Log start of task if error handler exists
        if (task->errorHandler) {
            task->errorHandler->logInfo("Communication task started on Core " + String(xPortGetCoreID()));
        }
        
        // Main task loop
        while (true) {
            // Take the mutex if available
            bool mutexTaken = false;
            if (task->commMutex && *(task->commMutex)) {
                mutexTaken = (xSemaphoreTake(*(task->commMutex), pdMS_TO_TICKS(100)) == pdTRUE);
            }
            
            // Process communication
            if (task && task->processComm()) {
                // Communication was processed
                if (task->errorHandler) {
                    task->errorHandler->logInfo("Communication processed");
                }
            }
            
            // Release the mutex if we took it
            if (mutexTaken) {
                xSemaphoreGive(*(task->commMutex));
            }
            
            // Delay to prevent task starvation
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
    
    /**
     * @brief Process communication data
     * @return true if communication was processed, false otherwise
     */
    bool processComm() {
        // Safe implementation that doesn't allocate memory dynamically
        if (Serial.available() > 0) {
            size_t bytesRead = 0;
            
            // Clear buffer first
            memset(rxBuffer, 0, MAX_BUFFER_SIZE);
            
            // Read data safely (prevent buffer overflow)
            while (Serial.available() > 0 && bytesRead < MAX_BUFFER_SIZE - 1) {
                rxBuffer[bytesRead] = Serial.read();
                bytesRead++;
                // Small delay to allow more bytes to arrive
                delayMicroseconds(100);
            }
            
            // Ensure null termination
            rxBuffer[bytesRead] = '\0';
            
            // Process the received data (just echo it back for this example)
            if (bytesRead > 0) {
                Serial.print("Received: ");
                Serial.println(rxBuffer);
                return true;
            }
        }
        return false;
    }

public:
    /**
     * @brief Constructor that initializes the task
     * @param errHandler Error handler for logging
     * @param mutex Optional mutex for thread safety
     */
    SimpleCommTask(ErrorHandler* errHandler, SemaphoreHandle_t* mutex = nullptr) 
        : errorHandler(errHandler), commMutex(mutex) {
        // Zero-initialize the buffer
        memset(rxBuffer, 0, MAX_BUFFER_SIZE);
    }
    
    /**
     * @brief Destructor that ensures the task is stopped
     */
    ~SimpleCommTask() {
        stop();
    }
    
    /**
     * @brief Start the communication task
     * @return true if started successfully, false otherwise
     */
    bool start() {
        // Check if task is already running
        if (commTaskHandle != nullptr) {
            // Task already running
            return true;
        }
        
        // Log error if error handler is unavailable
        if (!errorHandler) {
            // Cannot proceed without error handler
            return false;
        }
        
        // Create the task with error checking
        BaseType_t result = xTaskCreatePinnedToCore(
            commTaskFunction,      // Function
            "CommTask",            // Name
            STACK_SIZE,            // Stack size
            this,                  // Parameter (this pointer)
            PRIORITY,              // Priority
            &commTaskHandle,       // Handle
            CORE_ID                // Core ID
        );
        
        // Check if task creation was successful
        if (result != pdPASS) {
            if (errorHandler) {
                errorHandler->logError(ERROR, "Failed to create communication task");
            }
            commTaskHandle = nullptr;  // Ensure handle is null on failure
            return false;
        }
        
        // Log success
        if (errorHandler) {
            errorHandler->logInfo("Communication task created successfully on Core " + String(CORE_ID));
        }
        
        return true;
    }
    
    /**
     * @brief Stop the communication task
     */
    void stop() {
        if (commTaskHandle != nullptr) {
            // Delete the task and set handle to null
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
    
    /**
     * @brief Set the communication mutex
     * @param mutex Pointer to the mutex
     */
    void setCommMutex(SemaphoreHandle_t* mutex) {
        commMutex = mutex;
    }
};
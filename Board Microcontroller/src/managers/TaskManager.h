#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include "Constants.h"

// Forward declarations of manager classes
class SensorManager;
class CommunicationManager;
class LedManager;
class ErrorHandler;

/**
 * @brief Class for managing FreeRTOS tasks in a multi-core environment
 * 
 * This class provides a clean interface for creating and managing tasks
 * on specific cores of the ESP32, ensuring proper initialization, priority
 * handling, and synchronization between tasks.
 */
class TaskManager {
public:
    // Task names for identification
    static constexpr const char* TASK_NAME_SENSOR = "SensorTask";
    static constexpr const char* TASK_NAME_COMM = "CommTask";
    static constexpr const char* TASK_NAME_LED = "LedTask";
    
    // Task stack sizes (in words) - increased for stability
    static constexpr uint32_t STACK_SIZE_SENSOR = Constants::Tasks::STACK_SIZE_SENSOR;
    static constexpr uint32_t STACK_SIZE_COMM = Constants::Tasks::STACK_SIZE_COMM;
    static constexpr uint32_t STACK_SIZE_LED = Constants::Tasks::STACK_SIZE_LED;
    
    // Task priorities - keep lower than critical system tasks
    static constexpr UBaseType_t PRIORITY_SENSOR = Constants::Tasks::PRIORITY_SENSOR;    // Changed from 1
    static constexpr UBaseType_t PRIORITY_COMM = Constants::Tasks::PRIORITY_COMM;      // Changed from 2
    static constexpr UBaseType_t PRIORITY_LED = Constants::Tasks::PRIORITY_LED;       // Same
    
    // Core assignments - change for proper distribution
    static constexpr BaseType_t CORE_SENSOR = Constants::Tasks::CORE_SENSOR;  // Changed from Core 1 to Core 0
    static constexpr BaseType_t CORE_COMM = Constants::Tasks::CORE_COMM;    // Changed from Core 0 to Core 1
    static constexpr BaseType_t CORE_LED = Constants::Tasks::CORE_LED;     // Same (Core 0)

    /**
     * @brief Constructor for TaskManager
     * 
     * @param sensorMgr Pointer to the SensorManager
     * @param commMgr Pointer to the CommunicationManager
     * @param ledMgr Pointer to the LedManager
     * @param errHandler Pointer to the ErrorHandler
     */
    TaskManager(SensorManager* sensorMgr, CommunicationManager* commMgr, 
                LedManager* ledMgr, ErrorHandler* errHandler);
    
    /**
     * @brief Destructor - ensures all tasks are cleaned up
     */
    ~TaskManager();
    
    /**
     * @brief Initialize the task manager and create synchronization primitives
     * 
     * @return true on success, false on failure
     */
    bool begin();
    
    /**
     * @brief Start all tasks on their respective cores
     * 
     * @return true on success, false on failure
     */
    bool startAllTasks();
    
    /**
     * @brief Start only the LED task (simplest, lowest risk)
     * 
     * @return true on success, false on failure
     */
    bool startLedTask();
    
    /**
     * @brief Start only the sensor task
     * 
     * @return true on success, false on failure
     */
    bool startSensorTask();
    
    /**
     * @brief Start only the communication task
     * 
     * @return true on success, false on failure
     */
    bool startCommTask();
    
    /**
     * @brief Check if all tasks are running
     * 
     * @return true if all tasks are running, false otherwise
     */
    bool areAllTasksRunning() const;
    
    /**
     * @brief Get the status of all tasks
     * 
     * @return String containing status information for all tasks
     */
    String getTaskStatusString() const;
    
    /**
     * @brief Get the memory usage of all tasks
     * 
     * @return String containing memory usage information for all tasks
     */
    String getTaskMemoryInfo() const;
    
    /**
     * @brief Get the FreeRTOS task state as a string
     * 
     * @param handle The task handle
     * @return String representing the task state
     */
    static String getTaskStateString(TaskHandle_t handle);

private:
    // Task handles
    TaskHandle_t sensorTaskHandle = nullptr;
    TaskHandle_t commTaskHandle = nullptr;
    TaskHandle_t ledTaskHandle = nullptr;
    
    // Manager references
    SensorManager* sensorManager = nullptr;
    CommunicationManager* commManager = nullptr;
    LedManager* ledManager = nullptr;
    ErrorHandler* errorHandler = nullptr;
    
    // Task status tracking
    bool tasksInitialized = false;
    
    // Static task function that calls the appropriate object method
    static void sensorTaskFunction(void* pvParameters);
    static void commTaskFunction(void* pvParameters);
    static void ledTaskFunction(void* pvParameters);
    
    // Instance task methods
    void sensorTask();
    void commTask();
    void ledTask();
    
    // Helper methods
    void cleanupTasks();
};
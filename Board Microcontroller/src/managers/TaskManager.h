/**
 * @file TaskManager.h
 * @brief FreeRTOS task management for multi-core operation
 * @author Gabriel Avenia
 * @date May 2025
 * @defgroup task_management Task Management
 * @brief Components for managing FreeRTOS tasks and synchronization
 * @{
 */

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
  * @brief Manages FreeRTOS tasks in a multi-core environment
  * This class provides a clean interface for creating and managing tasks
  * on specific cores of the ESP32, ensuring proper initialization, priority
  * handling, and synchronization between tasks.
  */
 class TaskManager {
 public:
     /** 
      * @name Task names for identification
      * @{
      */
     static constexpr const char* TASK_NAME_SENSOR = "SensorTask";
     static constexpr const char* TASK_NAME_COMM = "CommTask";
     static constexpr const char* TASK_NAME_LED = "LedTask";
     /** @} */
     
     /** 
      * @name Task stack sizes (in words)
      * @{
      */
     static constexpr uint32_t STACK_SIZE_SENSOR = Constants::Tasks::STACK_SIZE_SENSOR;
     static constexpr uint32_t STACK_SIZE_COMM = Constants::Tasks::STACK_SIZE_COMM;
     static constexpr uint32_t STACK_SIZE_LED = Constants::Tasks::STACK_SIZE_LED;
     /** @} */
     
     /** 
      * @name Task priorities (higher number = higher priority)
      * @{
      */
     static constexpr UBaseType_t PRIORITY_SENSOR = Constants::Tasks::PRIORITY_SENSOR;
     static constexpr UBaseType_t PRIORITY_COMM = Constants::Tasks::PRIORITY_COMM;
     static constexpr UBaseType_t PRIORITY_LED = Constants::Tasks::PRIORITY_LED;
     /** @} */
     
     /** 
      * @name Core assignments for ESP32 dual-core operation
      * @{
      */
     static constexpr BaseType_t CORE_SENSOR = Constants::Tasks::CORE_SENSOR;
     static constexpr BaseType_t CORE_COMM = Constants::Tasks::CORE_COMM;
     static constexpr BaseType_t CORE_LED = Constants::Tasks::CORE_LED;
     /** @} */
 
     /**
      * @brief Constructor for TaskManager
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
      * @return true on success, false on failure
      */
     bool begin();
     
     /**
      * @brief Start all tasks on their respective cores
      * @return true on success, false on failure
      */
     bool startAllTasks();
     
     /**
      * @brief Start only the LED task (simplest, lowest risk)
      * @return true on success, false on failure
      */
     bool startLedTask();
     
     /**
      * @brief Start only the sensor task
      * @return true on success, false on failure
      */
     bool startSensorTask();
     
     /**
      * @brief Start only the communication task
      * @return true on success, false on failure
      */
     bool startCommTask();
     
     /**
      * @brief Check if all tasks are running
      * @return true if all tasks are running, false otherwise
      */
     bool areAllTasksRunning() const;
     
     /**
      * @brief Get the status of all tasks
      * @return String containing status information for all tasks
      */
     String getTaskStatusString() const;
     
     /**
      * @brief Get the memory usage of all tasks
      * @return String containing memory usage information for all tasks
      */
     String getTaskMemoryInfo() const;
     
     /**
      * @brief Get the FreeRTOS task state as a string
      * @param handle The task handle
      * @return String representing the task state
      */
     static String getTaskStateString(TaskHandle_t handle);
 
 private:
     /**
      * @brief Task handles
      * @{
      */
     TaskHandle_t sensorTaskHandle = nullptr;
     TaskHandle_t commTaskHandle = nullptr;
     TaskHandle_t ledTaskHandle = nullptr;
     /** @} */
     
     /**
      * @brief Manager references
      * @{
      */
     SensorManager* sensorManager = nullptr;
     CommunicationManager* commManager = nullptr;
     LedManager* ledManager = nullptr;
     ErrorHandler* errorHandler = nullptr;
     /** @} */
     
     /**
      * @brief Task status tracking
      */
     bool tasksInitialized = false;
     
     /**
      * @brief Static task functions that call the appropriate object method
      * @{
      */
     static void sensorTaskFunction(void* pvParameters);
     static void commTaskFunction(void* pvParameters);
     static void ledTaskFunction(void* pvParameters);
     /** @} */
     
     /**
      * @brief Instance task methods - actual implementations
      * @{
      */
     void sensorTask();
     void commTask();
     void ledTask();
     /** @} */
     
     /**
      * @brief Helper method to clean up all tasks
      */
     void cleanupTasks();
 };
 
 /** @} */ // End of task_management group

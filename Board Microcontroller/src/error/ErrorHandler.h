/**
 * @file ErrorHandler.h
 * @brief Error logging and reporting system with configurable output streams
 * @author Gabriel Avenia
 * @date May 2025
 * @defgroup error_handling Error Handling
 * @brief Components for error logging, reporting, and recovery
 * @{
 */

 #pragma once

 #include <Arduino.h>
 #include <vector>
 
 // Forward declaration to avoid circular dependency
 class LedManager;
 
 /**
  * @brief Error severity levels
  * Defines the different severity levels for error messages,
  * in order of increasing severity.
  */
 enum ErrorSeverity {
   INFO,     ///< Informational message, not an error
   WARNING,  ///< Warning that might require attention
   ERROR,    ///< Error that affects operation but allows recovery
   FATAL,    ///< Fatal error that prevents continued operation
 };
 
 /**
  * @brief Structure to hold error information
  * Contains all relevant information about an error event
  * including severity, message, and timestamp.
  */
 struct ErrorEntry {
   ErrorSeverity severity;     ///< Severity level of the error
   String message;             ///< Error message content
   unsigned long timestamp;    ///< Timestamp when error occurred (millis)
 };
 
 /**
  * @brief Structure to hold output configuration
  * Configures an output stream with the minimum severity level
  * to be routed to that output.
  */
 struct OutputConfig {
   Print* stream;              ///< Output stream pointer
   ErrorSeverity minSeverity;  ///< Minimum severity level to output
 };
 
 /**
  * @brief Handles error logging and reporting with configurable output streams
  * This class manages error reporting and logging throughout the system,
  * providing a centralized mechanism for error handling. It supports:
  * - Multiple configurable output streams with severity filtering
  * - Error log storage and retrieval
  * - Visual indication of errors via LED
  * - Customizable error routing for different destinations
  */
 class ErrorHandler {
 private:
   /**
    * @brief Vector of recent error entries
    */
   std::vector<ErrorEntry> errorLog;
   
   /**
    * @brief Maximum number of log entries to store
    */
   const int MAX_LOG_SIZE = 20;
   
   /**
    * @brief Default output stream (backward compatibility)
    */
   Print* defaultOutput;
   
   /**
    * @brief Output configurations for different destinations
    * @{
    */
   OutputConfig usbOutput;    ///< Configuration for USB serial output
   OutputConfig uartOutput;   ///< Configuration for UART debug output
   /** @} */
   
   /**
    * @brief Reference to UART debug serial
    */
   Print* uartDebugSerial;
   
   /**
    * @brief Reference to LED manager for visual indication
    */
   LedManager* ledManager = nullptr;
   
   /**
    * @brief Flag to indicate if custom routing is enabled
    */
   bool useCustomRouting;
   
 public:
   /**
    * @brief Get the LED manager pointer for debugging
    * @return The LED manager or nullptr if not set
    */
   LedManager* getLedManager() const { return ledManager; }
   
   /**
    * @brief Constructor for ErrorHandler with output stream routing
    * @param output Main output stream for errors (typically USB Serial)
    * @param debugOutput Secondary output stream for debug messages (typically UART)
    */
   ErrorHandler(Print* output = nullptr, Print* debugOutput = nullptr);
   
   /**
    * @brief Set the minimum severity level for a specific output stream
    * @param output The output stream to configure
    * @param minSeverity The minimum severity level to route to this output
    */
   void setOutputSeverity(Print* output, ErrorSeverity minSeverity);
   
   /**
    * @brief Set the LED manager for visual indication of errors
    * @param led Pointer to the LED manager
    */
   void setLedManager(LedManager* led);
 
   /**
    * @brief Enable or disable custom routing
    * When enabled, messages are routed to their configured streams.
    * When disabled, all messages go to the default output stream.
    * @param enable True to enable custom routing, false to disable
    */
   void enableCustomRouting(bool enable = true);
   
   /**
    * @brief Log a message with specified severity
    * Logs a message to the appropriate output streams based on severity
    * and routing configuration. Also triggers visual indication via LED
    * if available and if severity warrants it.
    * @param severity The severity level (INFO, WARNING, ERROR, FATAL)
    * @param message The message to log
    * @return true if a FATAL error was logged (for caller to take action)
    */
   bool logError(ErrorSeverity severity, String message);
 
   /**
    * @brief Get all logged error entries
    * @return Vector of error entries
    */
   std::vector<ErrorEntry> getErrorLog();
   
   /**
    * @brief Clear all logged error entries
    */
   void clearErrors();
   
   /**
    * @brief Get the current message routing configuration status
    * @return String containing routing status information
    */
   String getRoutingStatus() const;
   
   /**
    * @brief Convert severity level to string
    * @param severity The severity level
    * @return String representation of the severity
    */
   static String severityToString(ErrorSeverity severity);
   
   /**
    * @brief Convert string to severity level
    * @param severityStr The severity string
    * @return ErrorSeverity value (defaults to INFO if not recognized)
    */
   static ErrorSeverity stringToSeverity(const String& severityStr);
 };
 
 /** @} */ // End of error_handling group

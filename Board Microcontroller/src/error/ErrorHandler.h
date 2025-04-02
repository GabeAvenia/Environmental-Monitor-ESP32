#pragma once

#include <Arduino.h>
#include <vector>

// Error severity levels
enum ErrorSeverity {
  INFO,
  WARNING,
  ERROR,
  CRITICAL
};

// Structure to hold error information
struct ErrorEntry {
  ErrorSeverity severity;
  String message;
  unsigned long timestamp;
};

/**
 * @brief Handles error logging and reporting with configurable output streams
 *
 * Supports routing different severity levels to different output streams
 * and maintains a log of recent messages.
 */
class ErrorHandler {
private:
  std::vector<ErrorEntry> errorLog;
  const int MAX_LOG_SIZE = 20;
  
  // Output streams for different severity levels
  Print* outputStream;      // Default stream for backward compatibility
  Print* infoStream;        // Stream for INFO messages
  Print* warningStream;     // Stream for WARNING messages
  Print* errorStream;       // Stream for ERROR messages
  Print* criticalStream;    // Stream for CRITICAL messages
  Print* uartDebugSerial;   // Reference to UART debug serial
  
  // Flag to indicate if we're using per-severity routing
  bool useCustomRouting;
  
public:
  /**
   * @brief Constructor for ErrorHandler with output stream routing
   * 
   * @param output Main output stream for errors (typically USB Serial)
   * @param debugOutput Secondary output stream for debug messages (typically UART)
   */
  ErrorHandler(Print* output = nullptr, Print* debugOutput = nullptr);
  
  /**
   * @brief Configure output stream for INFO messages
   * @param output The stream to use
   */
  void setInfoOutput(Print* output);
  
  /**
   * @brief Configure output stream for WARNING messages
   * @param output The stream to use
   */
  void setWarningOutput(Print* output);
  
  /**
   * @brief Configure output stream for ERROR messages
   * @param output The stream to use
   */
  void setErrorOutput(Print* output);
  
  /**
   * @brief Configure output stream for CRITICAL messages
   * @param output The stream to use
   */
  void setCriticalOutput(Print* output);
  
  /**
   * @brief Enable or disable custom routing
   * 
   * When enabled, messages are routed to their configured streams.
   * When disabled, all messages go to the default output stream.
   * 
   * @param enable True to enable custom routing, false to disable
   */
  void enableCustomRouting(bool enable = true);
  
  /**
   * @brief Log an error message with specified severity
   * 
   * @param severity The severity level (INFO, WARNING, ERROR, CRITICAL)
   * @param message The message to log
   */
  void logError(ErrorSeverity severity, String message);
  
  /**
   * @brief Log an INFO level message
   * @param message The message to log
   */
  void logInfo(String message);
  
  /**
   * @brief Log a WARNING level message
   * @param message The message to log
   */
  void logWarning(String message);
  
  /**
   * @brief Log a CRITICAL level message
   * @param message The message to log
   */
  void logCritical(String message);
  
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
};
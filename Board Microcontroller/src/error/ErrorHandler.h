#pragma once

#include <Arduino.h>
#include <vector>

// Forward declaration to avoid circular dependency
class LedManager;

// Error severity levels
enum ErrorSeverity {
  INFO,
  WARNING,
  ERROR,
  FATAL,
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
  Print* fatalStream;       // Stream for FATAL messages
  Print* uartDebugSerial;   // Reference to UART debug serial
  
  // Reference to LED manager for visual indication
  LedManager* ledManager = nullptr;
  
  // Flag to indicate if we're using per-severity routing
  bool useCustomRouting;
  
public:
  /**
   * @brief Get the LED manager pointer for debugging
   * @return The LED manager or nullptr if not set
   */
  LedManager* getLedManager() const { return ledManager; }
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
   * @brief Configure output stream for FATAL messages
   * @param output The stream to use
   */
  void setFatalOutput(Print* output);
  
  /**
   * @brief Set the LED manager for visual indication of errors
   * @param led Pointer to the LED manager
   */
  void setLedManager(LedManager* led);

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
   * @brief Log a message with specified severity
   * 
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
};
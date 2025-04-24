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

// Structure to hold output configuration
struct OutputConfig {
  Print* stream;
  ErrorSeverity minSeverity;
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
  
  // Default output stream (backward compatibility)
  Print* defaultOutput;
  
  // Output stream configurations
  OutputConfig usbOutput;
  OutputConfig uartOutput;
  
  // Reference to UART debug serial
  Print* uartDebugSerial;
  
  // Reference to LED manager for visual indication
  LedManager* ledManager = nullptr;
  
  // Flag to indicate if we're using custom routing
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
   * @brief Set the minimum severity level for a specific output stream
   * 
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
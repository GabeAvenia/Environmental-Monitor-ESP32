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

class ErrorHandler {
private:
  std::vector<ErrorEntry> errorLog;
  const int MAX_LOG_SIZE = 20;
  Print* outputStream;
  
public:
  // Constructor
  ErrorHandler(Print* output = nullptr);
  
  // Error logging methods
  void logError(ErrorSeverity severity, String message);
  void logInfo(String message);
  void logWarning(String message);
  void logCritical(String message);
  
  // Error management
  std::vector<ErrorEntry> getErrorLog();
  void clearErrors();
};
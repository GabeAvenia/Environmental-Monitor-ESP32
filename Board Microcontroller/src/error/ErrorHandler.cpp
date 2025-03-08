#include "ErrorHandler.h"

ErrorHandler::ErrorHandler(Print* output) : outputStream(output) {
}

void ErrorHandler::logError(ErrorSeverity severity, String message) {
  // Create new error entry
  ErrorEntry entry;
  entry.severity = severity;
  entry.message = message;
  entry.timestamp = millis();
  
  // Manage log size
  if (errorLog.size() >= MAX_LOG_SIZE) {
    errorLog.erase(errorLog.begin());
  }
  
  // Add to log
  errorLog.push_back(entry);
  
  // Print to output if available
  if (outputStream != nullptr) {
    String severityStr;
    switch (severity) {
      case INFO: severityStr = "INFO"; break;
      case WARNING: severityStr = "WARNING"; break;
      case ERROR: severityStr = "ERROR"; break;
      case CRITICAL: severityStr = "CRITICAL"; break;
    }
    
    outputStream->print("[");
    outputStream->print(severityStr);
    outputStream->print("] ");
    outputStream->println(message);
  }
}

void ErrorHandler::logInfo(String message) {
  logError(INFO, message);
}

void ErrorHandler::logWarning(String message) {
  logError(WARNING, message);
}

void ErrorHandler::logCritical(String message) {
  logError(CRITICAL, message);
}

std::vector<ErrorEntry> ErrorHandler::getErrorLog() {
  return errorLog;
}

void ErrorHandler::clearErrors() {
  errorLog.clear();
}
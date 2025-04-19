#include "ErrorHandler.h"
#include "managers/LedManager.h"

ErrorHandler::ErrorHandler(Print* output, Print* debugOutput) 
  : outputStream(output), 
    infoStream(debugOutput ? debugOutput : output), 
    warningStream(output), 
    errorStream(output), 
    fatalStream(output),
    uartDebugSerial(debugOutput),
    useCustomRouting(debugOutput != nullptr) {
}

void ErrorHandler::setInfoOutput(Print* output) {
  infoStream = output;
}

void ErrorHandler::setWarningOutput(Print* output) {
  warningStream = output;
}

void ErrorHandler::setErrorOutput(Print* output) {
  errorStream = output;
}

void ErrorHandler::setFatalOutput(Print* output) {
  fatalStream = output;
}

void ErrorHandler::setLedManager(LedManager* led) {
  ledManager = led;
}

void ErrorHandler::enableCustomRouting(bool enable) {
  useCustomRouting = enable;
}

bool ErrorHandler::logError(ErrorSeverity severity, String message) {
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
  
  // Trigger LED indication if LED manager is available
  if (ledManager) {
    switch (severity) {
      case WARNING:
        ledManager->indicateWarning();
        break;
      case ERROR:
        ledManager->indicateError();
        break;
      case FATAL:
        ledManager->indicateFatalError();
        break;
      default:
        // No LED change for INFO
        break;
    }
  }
  
  // Determine which output stream to use
  Print* targetStream = outputStream;  // Default to old behavior
  
  if (useCustomRouting) {
    switch (severity) {
      case INFO:
        targetStream = infoStream;
        break;
      case WARNING:
        targetStream = warningStream;
        break;
      case ERROR:
        targetStream = errorStream;
        break;
      case FATAL:
        targetStream = fatalStream;
        break;
    }
  }
  
  // Print to output if available
  if (targetStream != nullptr) {
    String severityStr;
    switch (severity) {
      case INFO: severityStr = "INFO"; break;
      case WARNING: severityStr = "WARNING"; break;
      case ERROR: severityStr = "ERROR"; break;
      case FATAL: severityStr = "FATAL"; break;
    }
    
    // Add timestamp (seconds since boot)
    unsigned long currentTime = millis();
    float seconds = currentTime / 1000.0;
    
    // Safely check if we can write to the stream
    try {
      targetStream->print("[");
      targetStream->print(seconds, 3);  // Print with 3 decimal places (ms precision)
      targetStream->print("s][");
      targetStream->print(severityStr);
      targetStream->print("] ");
      targetStream->println(message);
    } catch (...) {
      // If we fail to write to this stream, disable it to prevent future errors
      if (targetStream == infoStream) infoStream = nullptr;
      if (targetStream == warningStream) warningStream = nullptr;
      if (targetStream == errorStream) errorStream = nullptr;
      if (targetStream == fatalStream) fatalStream = nullptr;
    }
  }
  
  // Return true if this was a FATAL error, allowing the caller to take action
  return (severity == FATAL);
}

std::vector<ErrorEntry> ErrorHandler::getErrorLog() {
  return errorLog;
}

void ErrorHandler::clearErrors() {
  errorLog.clear();
}

String ErrorHandler::getRoutingStatus() const {
  String status = "Message Routing:\n";
  status += "Custom Routing: " + String(useCustomRouting ? "Enabled" : "Disabled") + "\n";
  
  // Map each output to a description
  auto getStreamDesc = [this](Print* stream) -> String {
    if (stream == nullptr) return "Disabled";
    if (stream == outputStream) return "Default";
    if (stream == &Serial) return "USB";
    if (stream == uartDebugSerial) return "UART";
    return "Custom";
  };
  
  status += "INFO output: " + getStreamDesc(infoStream) + "\n";
  status += "WARNING output: " + getStreamDesc(warningStream) + "\n";
  status += "ERROR output: " + getStreamDesc(errorStream) + "\n";
  status += "FATAL output: " + getStreamDesc(fatalStream) + "\n";
  
  // Add statistics
  status += "Log entries: " + String(errorLog.size()) + "\n";
  
  // Add information about serial ports
  status += "USB Serial: " + String(&Serial ? "Available" : "Unavailable") + "\n";
  status += "UART Debug: " + String(uartDebugSerial ? "Available" : "Unavailable") + "\n";
  
  return status;
}
#include "ErrorHandler.h"
#include "managers/LedManager.h"

ErrorHandler::ErrorHandler(Print* output, Print* debugOutput) 
  : defaultOutput(output),
    uartDebugSerial(debugOutput),
    useCustomRouting(debugOutput != nullptr) {
  
  // Initialize USB output configuration
  usbOutput.stream = output;
  usbOutput.minSeverity = WARNING; // Default: USB gets WARNING and higher
  
  // Initialize UART output configuration
  uartOutput.stream = debugOutput;
  uartOutput.minSeverity = INFO;   // Default: UART gets everything
}

void ErrorHandler::setOutputSeverity(Print* output, ErrorSeverity minSeverity) {
  if (output == usbOutput.stream) {
    usbOutput.minSeverity = minSeverity;
  } else if (output == uartOutput.stream) {
    uartOutput.minSeverity = minSeverity;
  } else {
    // Unknown output stream - ignore or log an error?
  }
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
  
  // Format the message with severity and timestamp
  unsigned long currentTime = millis();
  float seconds = currentTime / 1000.0;
  String severityStr = severityToString(severity);
  String formattedMessage = "[" + String(seconds, 3) + "s][" + severityStr + "] " + message;
  
  // Route message based on severity and configuration
  if (!useCustomRouting) {
    // If custom routing is disabled, send all messages to default output
    if (defaultOutput != nullptr) {
      defaultOutput->println(formattedMessage);
    }
  } else {
    // Custom routing - check each configured output
    bool messageSent = false;
    
    // Check USB output
    if (usbOutput.stream != nullptr && severity >= usbOutput.minSeverity) {
      try {
        usbOutput.stream->println(formattedMessage);
        messageSent = true;
      } catch (...) {
        // If writing fails, disable this output
        usbOutput.stream = nullptr;
      }
    }
    
    // Check UART output
    if (uartOutput.stream != nullptr && severity >= uartOutput.minSeverity) {
      try {
        uartOutput.stream->println(formattedMessage);
        messageSent = true;
      } catch (...) {
        // If writing fails, disable this output
        uartOutput.stream = nullptr;
      }
    }
    
    // If no output received the message but defaultOutput exists, use it as fallback
    if (!messageSent && defaultOutput != nullptr) {
      try {
        defaultOutput->println(formattedMessage);
      } catch (...) {
        // If even this fails, disable default output
        defaultOutput = nullptr;
      }
    }
  }
  
  // Return true if this was a FATAL error, allowing the caller to take action
  return (severity == FATAL);
}

String ErrorHandler::severityToString(ErrorSeverity severity) {
  switch (severity) {
    case INFO: return "INFO";
    case WARNING: return "WARNING";
    case ERROR: return "ERROR";
    case FATAL: return "FATAL";
    default: return "UNKNOWN";
  }
}

ErrorSeverity ErrorHandler::stringToSeverity(const String& severityStr) {
  String upper = severityStr;
  upper.toUpperCase();
  
  if (upper == "WARNING") return WARNING;
  if (upper == "ERROR") return ERROR;
  if (upper == "FATAL") return FATAL;
  
  // Default to INFO for unrecognized values
  return INFO;
}

String ErrorHandler::getRoutingStatus() const {
  String status = "Message Routing:\n";
  status += "Custom Routing: " + String(useCustomRouting ? "Enabled" : "Disabled") + "\n";
  
  status += "USB output: " + 
            (usbOutput.stream ? "Enabled (min severity: " + severityToString(usbOutput.minSeverity) + ")" : "Disabled") + 
            "\n";
  
  status += "UART output: " + 
            (uartOutput.stream ? "Enabled (min severity: " + severityToString(uartOutput.minSeverity) + ")" : "Disabled") + 
            "\n";
  
  // Add statistics
  status += "Log entries: " + String(errorLog.size()) + "\n";
  
  return status;
}

ErrorSeverity ErrorHandler::getLastSeverity() const {
    if (errorLog.empty()) {
        return INFO;  // Default to INFO if no errors
    }
    return errorLog.back().severity;
}

String ErrorHandler::getLastMessage() const {
    if (errorLog.empty()) {
        return "";  // Return empty string if no errors
    }
    return errorLog.back().message;
}
#pragma once

#include <Arduino.h>
#include "interfaces/ISensor.h"
#include "../error/ErrorHandler.h"
#include "SensorTypes.h"

/**
 * @brief Base class for all sensor implementations.
 * 
 * This abstract class implements common functionality for all sensors
 * and serves as a base for concrete sensor implementations.
 */
class BaseSensor : public virtual ISensor {
protected:
    String name;                   ///< Unique identifier for this sensor
    SensorType type;               ///< Type of sensor
    bool connected;                ///< Connection status
    ErrorHandler* errorHandler;    ///< Error reporting mechanism
    
    /**
     * @brief Log an error message through the error handler.
     * 
     * @param message The error message.
     * @param severity The severity level of the error.
     */
    void logError(const String& message, ErrorSeverity severity = ERROR) {
        if (errorHandler) {
            errorHandler->logError(severity, message);
        }
    }

    /**
     * @brief Log an informational message through the error handler.
     * 
     * @param message The information message.
     */
    void logInfo(const String& message) {
        if (errorHandler) {
            errorHandler->logInfo(message);
        }
    }

public:
    /**
     * @brief Constructor for BaseSensor.
     * 
     * @param sensorName The name/identifier for this sensor instance.
     * @param sensorType The type of sensor.
     * @param err Pointer to the error handler for logging.
     */
    BaseSensor(const String& sensorName, SensorType sensorType, ErrorHandler* err)
        : name(sensorName), type(sensorType), connected(false), errorHandler(err) {}
    
    /**
     * @brief Virtual destructor.
     */
    virtual ~BaseSensor() {}
    
    /**
     * @brief Get the name of the sensor.
     * 
     * @return The sensor's name/identifier.
     */
    String getName() const override {
        return name;
    }
    
    /**
     * @brief Check if the sensor is currently connected and operational.
     * 
     * @return true if connected, false otherwise.
     */
    bool isConnected() const override {
        return connected;
    }
    
    /**
     * @brief Get the type of this sensor as an enum value.
     * 
     * @return The sensor type.
     */
    SensorType getType() const {
        return type;
    }
    
    /**
     * @brief Get the type of this sensor as a string.
     * 
     * @return The sensor type as a string.
     */
    String getTypeString() const {
        return sensorTypeToString(type);
    }
};

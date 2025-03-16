#pragma once

#include <Arduino.h>
#include "InterfaceTypes.h"

/**
 * @brief Base interface for all sensors.
 * 
 * This interface defines the common functionality that all sensors must implement,
 * regardless of the specific measurements they provide.
 */
class ISensor {
public:
    virtual ~ISensor() {}
    
    /**
     * @brief Get the name of the sensor.
     * @return The sensor's name/identifier.
     */
    virtual String getName() const = 0;
    
    /**
     * @brief Check if the sensor is currently connected and operational.
     * @return true if connected, false otherwise.
     */
    virtual bool isConnected() const = 0;
    
    /**
     * @brief Initialize the sensor hardware.
     * @return true if initialization succeeded, false otherwise.
     */
    virtual bool initialize() = 0;
    
    /**
     * @brief Perform a self-test to verify the sensor is functioning properly.
     * @return true if the self-test passed, false otherwise.
     */
    virtual bool performSelfTest() = 0;
    
    /**
     * @brief Get detailed information about the sensor.
     * @return A string containing sensor information.
     */
    virtual String getSensorInfo() const = 0;
    
    /**
     * @brief Get the type of this sensor as a string.
     * @return The sensor type as a string.
     */
    virtual String getTypeString() const = 0;
    
    /**
     * @brief Check if this sensor supports a specific interface.
     * 
     * @param type The interface type to check for.
     * @return true if the sensor supports the interface, false otherwise.
     */
    virtual bool supportsInterface(InterfaceType type) const = 0;
    
    /**
     * @brief Get the interface implementation for a specific type.
     * 
     * This allows safely casting to a specific interface without using dynamic_cast.
     * 
     * @param type The interface type to get.
     * @return Pointer to the interface, or nullptr if not supported.
     */
    virtual void* getInterface(InterfaceType type) const = 0;
};
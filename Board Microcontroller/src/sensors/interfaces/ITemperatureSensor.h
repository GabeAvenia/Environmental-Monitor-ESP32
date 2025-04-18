#pragma once

#include <Arduino.h>

/**
 * @brief Interface for sensors that can measure temperature
 */
class ITemperatureSensor {
public:
    virtual ~ITemperatureSensor() = default;
    
    /**
     * @brief Read current temperature from sensor
     * @return Temperature in degrees Celsius or NAN if reading failed
     */
    virtual float readTemperature() = 0;
    
    /**
     * @brief Get timestamp of last temperature reading
     * @return Timestamp in milliseconds from millis()
     */
    virtual unsigned long getTemperatureTimestamp() const = 0;
};
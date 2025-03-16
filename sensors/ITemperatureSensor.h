#pragma once

#include <Arduino.h>
#include "ISensor.h"

/**
 * @brief Interface for sensors that can measure temperature.
 */
class ITemperatureSensor : public virtual ISensor {
public:
    /**
     * @brief Read the current temperature value from the sensor.
     * @return The temperature in degrees Celsius, or NAN if reading failed.
     */
    virtual float readTemperature() = 0;
    
    /**
     * @brief Get the timestamp of the last temperature reading.
     * @return Timestamp in milliseconds (from millis()).
     */
    virtual unsigned long getTemperatureTimestamp() const = 0;
};

#pragma once

#include <Arduino.h>
#include "ISensor.h"

/**
 * @brief Interface for sensors that can measure barometric pressure.
 */
class IPressureSensor : public virtual ISensor {
public:
    /**
     * @brief Read the current pressure value from the sensor.
     * @return The pressure in hPa (hectopascals), or NAN if reading failed.
     */
    virtual float readPressure() = 0;
    
    /**
     * @brief Get the timestamp of the last pressure reading.
     * @return Timestamp in milliseconds (from millis()).
     */
    virtual unsigned long getPressureTimestamp() const = 0;
};

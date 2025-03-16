#pragma once

#include <Arduino.h>
#include "ISensor.h"

/**
 * @brief Interface for sensors that can measure CO2 levels.
 */
class ICO2Sensor : public virtual ISensor {
public:
    /**
     * @brief Read the current CO2 value from the sensor.
     * @return The CO2 level in ppm (parts per million), or NAN if reading failed.
     */
    virtual float readCO2() = 0;
    
    /**
     * @brief Get the timestamp of the last CO2 reading.
     * @return Timestamp in milliseconds (from millis()).
     */
    virtual unsigned long getCO2Timestamp() const = 0;
};

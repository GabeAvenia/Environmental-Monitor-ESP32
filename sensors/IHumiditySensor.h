#pragma once

#include <Arduino.h>
#include "ISensor.h"

/**
 * @brief Interface for sensors that can measure humidity.
 */
class IHumiditySensor : public virtual ISensor {
public:
    /**
     * @brief Read the current humidity value from the sensor.
     * @return The relative humidity as a percentage (0-100), or NAN if reading failed.
     */
    virtual float readHumidity() = 0;
    
    /**
     * @brief Get the timestamp of the last humidity reading.
     * @return Timestamp in milliseconds (from millis()).
     */
    virtual unsigned long getHumidityTimestamp() const = 0;
};

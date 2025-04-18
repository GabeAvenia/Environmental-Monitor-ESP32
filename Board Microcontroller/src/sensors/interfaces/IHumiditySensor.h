#pragma once

#include <Arduino.h>

/**
 * @brief Interface for sensors that can measure humidity
 */
class IHumiditySensor {
public:
    virtual ~IHumiditySensor() = default;
    
    /**
     * @brief Read current humidity from sensor
     * @return Relative humidity percentage (0-100) or NAN if reading failed
     */
    virtual float readHumidity() = 0;
    
    /**
     * @brief Get timestamp of last humidity reading
     * @return Timestamp in milliseconds from millis()
     */
    virtual unsigned long getHumidityTimestamp() const = 0;
};
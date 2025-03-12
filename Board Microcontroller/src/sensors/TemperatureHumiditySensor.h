#pragma once

#include <Adafruit_SHT4x.h>
#include "../error/ErrorHandler.h"
#include "Sensor.h"

class TemperatureHumiditySensor : public Sensor {
private:
    Adafruit_SHT4x sht4;
    ErrorHandler* errorHandler;
    int i2cAddress;
    
public:
    TemperatureHumiditySensor(String sensorName, int address, ErrorHandler* err);
    
    bool initialize() override;
    SensorReading readSensor() override;
    bool performSelfTest() override;
    String getSensorInfo() override;
};
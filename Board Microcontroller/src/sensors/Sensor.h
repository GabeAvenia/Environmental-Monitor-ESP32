#pragma once

#include <Arduino.h>

struct SensorReading {
    float temperature;
    float humidity;
    float co2;
    float pressure;
    unsigned long timestamp;
    
    SensorReading() : temperature(0), humidity(0), co2(0), pressure(0), timestamp(0) {}
};

class Sensor {
protected:
    String name;
    String type;
    SensorReading latestReading;
    bool isConnected;
    
public:
    Sensor(String sensorName, String sensorType) : 
        name(sensorName), 
        type(sensorType), 
        isConnected(false) {
    }
    
    virtual ~Sensor() {}
    
    virtual bool initialize() = 0;
    virtual SensorReading readSensor() = 0;
    virtual bool performSelfTest() = 0;
    virtual String getSensorInfo() = 0;
    
    String getName() { return name; }
    String getType() { return type; }
    bool getConnectionStatus() { return isConnected; }
    SensorReading getLatestReading() { return latestReading; }
};
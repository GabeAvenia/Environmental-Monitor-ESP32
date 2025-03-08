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
    TemperatureHumiditySensor(String sensorName, int address, ErrorHandler* err) : 
        Sensor(sensorName, "SHT41"), 
        errorHandler(err),
        i2cAddress(address) {
    }
    
    bool initialize() override {
        if (!sht4.begin()) {
            errorHandler->logError(ERROR, "Failed to initialize SHT41 sensor: " + name);
            isConnected = false;
            return false;
        }
        
        // Set up the sensor
        sht4.setPrecision(SHT4X_HIGH_PRECISION);
        sht4.setHeater(SHT4X_NO_HEATER);
        
        errorHandler->logInfo("SHT41 sensor initialized: " + name);
        isConnected = true;
        return true;
    }
    
    SensorReading readSensor() override {
        sensors_event_t humidity, temp;
        
        if (!isConnected) {
            errorHandler->logWarning("Attempted to read from disconnected sensor: " + name);
            return latestReading;
        }
        
        if (!sht4.getEvent(&humidity, &temp)) {
            errorHandler->logError(ERROR, "Failed to read from sensor: " + name);
            isConnected = false;
            return latestReading;
        }
        
        latestReading.temperature = temp.temperature;
        latestReading.humidity = humidity.relative_humidity;
        latestReading.timestamp = millis();
        
        return latestReading;
    }
    
    bool performSelfTest() override {
        sensors_event_t humidity, temp;
        bool success = sht4.getEvent(&humidity, &temp);
        
        if (!success) {
            errorHandler->logError(ERROR, "Self-test failed for sensor: " + name);
            isConnected = false;
        }
        
        return success;
    }
    
    String getSensorInfo() override {
        String info = "Sensor Name: " + name + "\n";
        info += "Type: " + type + "\n";
        info += "I2C Address: 0x" + String(i2cAddress, HEX) + "\n";
        info += "Connected: " + String(isConnected ? "Yes" : "No") + "\n";
        
        if (isConnected) {
            info += "Temperature: " + String(latestReading.temperature) + " C\n";
            info += "Humidity: " + String(latestReading.humidity) + " %\n";
        }
        
        return info;
    }
};
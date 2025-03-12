#include "TemperatureHumiditySensor.h"

TemperatureHumiditySensor::TemperatureHumiditySensor(String sensorName, int address, ErrorHandler* err) : 
    Sensor(sensorName, "SHT41"), 
    errorHandler(err),
    i2cAddress(address) {
}

bool TemperatureHumiditySensor::initialize() {
    // Try with Wire1 (our configured I2C bus)
    if (!sht4.begin(&Wire1)) {
        errorHandler->logError(ERROR, "Failed to initialize SHT41 sensor with Wire1: " + name);
        
        // As a fallback, try with default Wire
        Wire.begin();
        if (!sht4.begin()) {
            errorHandler->logError(ERROR, "Also failed with default Wire");
            isConnected = false;
            return false;
        } else {
            errorHandler->logInfo("SHT41 initialized with default Wire");
            isConnected = true;
        }
    } else {
        errorHandler->logInfo("SHT41 initialized with Wire1");
        isConnected = true;
    }
    
    // Set up the sensor
    sht4.setPrecision(SHT4X_HIGH_PRECISION);
    sht4.setHeater(SHT4X_NO_HEATER);
    
    return true;
}

SensorReading TemperatureHumiditySensor::readSensor() {
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

bool TemperatureHumiditySensor::performSelfTest() {
    sensors_event_t humidity, temp;
    bool success = sht4.getEvent(&humidity, &temp);
    
    if (!success) {
        errorHandler->logError(ERROR, "Self-test failed for sensor: " + name);
        isConnected = false;
    }
    
    return success;
}

String TemperatureHumiditySensor::getSensorInfo() {
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
#include "SHT41Sensor.h"

SHT41Sensor::SHT41Sensor(const String& sensorName, int address, TwoWire* i2cBus, 
                        I2CManager* i2cMgr, I2CPort port, ErrorHandler* err)
    : BaseSensor(sensorName, SensorType::SHT41, err),
      wire(i2cBus),
      i2cPort(port),
      i2cManager(i2cMgr),
      i2cAddress(address),
      lastTemperature(NAN),
      lastHumidity(NAN),
      tempTimestamp(0),
      humidityTimestamp(0) {
}

SHT41Sensor::~SHT41Sensor() {
    // Nothing to clean up
}

bool SHT41Sensor::initialize() {
    errorHandler->logError(INFO, "Initializing SHT41 sensor: " + name);
    
    // Add timeout for sensor initialization
    const unsigned long timeout = 1000; // 1 second timeout
    unsigned long startTime = millis();
    
    // The Adafruit SHT4x library supports passing a specific TwoWire instance
    bool initResult = false;
    
    // Try to initialize with timeout
    while (millis() - startTime < timeout) {
        initResult = sht4.begin(wire);
        if (initResult) {
            break;
        }
        // Short delay before retry
        delay(10);
        // Feed the watchdog
        yield();
    }
    
    if (!initResult) {
        errorHandler->logError(ERROR, "Failed to initialize SHT41 sensor: " + name + " (timed out)");
        connected = false;
        return false;
    }
    
    // Configure the sensor
    sht4.setPrecision(SHT4X_HIGH_PRECISION);
    sht4.setHeater(SHT4X_NO_HEATER);
    
    connected = true;
    errorHandler->logError(INFO, "SHT41 sensor initialized successfully: " + name);
    
    // Only try to get initial readings if we have time left
    if (millis() - startTime < timeout - 200) {
        updateReadings();
    }
    
    return true;
}

bool SHT41Sensor::updateReadings() const {
    if (!connected) {
        errorHandler->logError(ERROR, "Attempted to read from disconnected sensor: " + name);
        return false;
    }
    
    sensors_event_t humidity, temp;
    
    // Add timeout for sensor reading
    const unsigned long timeout = 500; // 500ms timeout
    unsigned long startTime = millis();
    
    bool readResult = false;
    
    // Try to read with timeout
    while (millis() - startTime < timeout) {
        // Non-const Adafruit API, we use mutable member and const_cast for internal state
        readResult = sht4.getEvent(&humidity, &temp);
        if (readResult) {
            break;
        }
        // Short delay before retry
        delay(5);
        // Feed the watchdog
        yield();
    }
    
    if (!readResult) {
        errorHandler->logError(ERROR, "Failed to read from SHT41 sensor: " + name + " (timed out)");
        const_cast<SHT41Sensor*>(this)->connected = false;
        return false;
    }
    
    // Update temperature
    lastTemperature = temp.temperature;
    tempTimestamp = millis();
    
    // Update humidity
    lastHumidity = humidity.relative_humidity;
    humidityTimestamp = tempTimestamp; // Same timestamp for both since they're read together
    
    return true;
}

float SHT41Sensor::readTemperature() {
    if (!updateReadings()) {
        return NAN;
    }
    return lastTemperature;
}

float SHT41Sensor::readHumidity() {
    if (!updateReadings()) {
        return NAN;
    }
    return lastHumidity;
}

unsigned long SHT41Sensor::getTemperatureTimestamp() const {
    return tempTimestamp;
}

unsigned long SHT41Sensor::getHumidityTimestamp() const {
    return humidityTimestamp;
}

bool SHT41Sensor::performSelfTest() {
    sensors_event_t humidity, temp;
    bool success = sht4.getEvent(&humidity, &temp);
    
    if (!success) {
        errorHandler->logError(ERROR, "Self-test failed for SHT41 sensor: " + name);
        connected = false;
    } else {
        connected = true;
        errorHandler->logError(INFO, "Self-test passed for SHT41 sensor: " + name);
    }
    
    return success;
}

String SHT41Sensor::getSensorInfo() const {
    String info = "Sensor Name: " + name + "\n";
    info += "Type: SHT41\n";
    info += "I2C Address: 0x" + String(i2cAddress, HEX) + "\n";
    info += "I2C Port: " + I2CManager::portToString(i2cPort) + "\n";
    info += "Connected: " + String(connected ? "Yes" : "No") + "\n";
    
    if (connected) {
        info += "Temperature: " + String(lastTemperature) + " °C\n";
        info += "Humidity: " + String(lastHumidity) + " %\n";
        
        // Calculate time since last reading
        unsigned long now = millis();
        unsigned long tempAge = now - tempTimestamp;
        info += "Last Reading: " + String(tempAge / 1000.0) + " seconds ago\n";
    }
    
    return info;
}

bool SHT41Sensor::supportsInterface(InterfaceType type) const {
    switch (type) {
        case InterfaceType::TEMPERATURE:
        case InterfaceType::HUMIDITY:
            return true;
        default:
            return false;
    }
}

void* SHT41Sensor::getInterface(InterfaceType type) const {
    switch (type) {
        case InterfaceType::TEMPERATURE:
            return const_cast<ITemperatureSensor*>(static_cast<const ITemperatureSensor*>(this));
        case InterfaceType::HUMIDITY:
            return const_cast<IHumiditySensor*>(static_cast<const IHumiditySensor*>(this));
        default:
            return nullptr;
    }
}
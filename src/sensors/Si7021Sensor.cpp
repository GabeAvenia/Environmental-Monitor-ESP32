#include "Si7021Sensor.h"

Si7021Sensor::Si7021Sensor(const String& sensorName, int address, TwoWire* i2cBus,
                          I2CManager* i2cMgr, I2CPort port, ErrorHandler* err)
    : BaseSensor(sensorName, SensorType::SI7021, err),
      si7021(i2cBus), // Pass wire directly to the constructor
      wire(i2cBus),
      i2cPort(port),
      i2cManager(i2cMgr),
      i2cAddress(address),
      lastTemperature(NAN),
      lastHumidity(NAN),
      tempTimestamp(0),
      humTimestamp(0) {
}

Si7021Sensor::~Si7021Sensor() {
    // Nothing to clean up
}

bool Si7021Sensor::initialize() {
    errorHandler->logError(INFO, "Initializing Si7021 sensor: " + name);
    
    if (!si7021.begin()) {
        errorHandler->logError(ERROR, "Failed to initialize Si7021 sensor: " + name);
        connected = false;
        return false;
    }
    
    connected = true;
    errorHandler->logError(INFO, "Si7021 sensor initialized successfully: " + name);
    
    // Read and log serial number for identification
    uint32_t serialNumber = si7021.sernum_a;
    errorHandler->logError(INFO, "Si7021 serial number: 0x" + String(serialNumber, HEX));
    
    // Get initial readings
    updateReadings();
    
    return true;
}

bool Si7021Sensor::updateReadings() const {
    if (!connected) {
        errorHandler->logError(ERROR, "Attempted to read from disconnected sensor: " + name);
        return false;
    }
    
    // Adding retries for more robustness
    const int MAX_RETRIES = 3;
    float temp = NAN;
    float humidity = NAN;
    bool success = false;
    
    for (int attempt = 0; attempt < MAX_RETRIES && !success; attempt++) {
        if (attempt > 0) {
            delay(50);
            errorHandler->logError(INFO, "Retrying Si7021 reading, attempt " + String(attempt+1) + " of " + String(MAX_RETRIES));
        }
        
        try {
            temp = si7021.readTemperature();
            
            if (!isnan(temp)) {
                delay(5);
                humidity = si7021.readHumidity();
                
                if (!isnan(humidity)) {
                    success = true;
                }
            }
        } catch (...) {
            errorHandler->logError(ERROR, "Exception during Si7021 reading for sensor: " + name);
            delay(20);
        }
    }
    
    if (!success) {
        errorHandler->logError(ERROR, "Failed to read from Si7021 sensor: " + name + " after " + String(MAX_RETRIES) + " retries");
        const_cast<Si7021Sensor*>(this)->connected = false;
        return false;
    }
    
    lastTemperature = temp;
    lastHumidity = humidity;
    unsigned long now = millis();
    tempTimestamp = now;
    humTimestamp = now;
    
    return true;
}

float Si7021Sensor::readTemperature() {
    if (!updateReadings()) {
        return NAN;
    }
    return lastTemperature;
}

float Si7021Sensor::readHumidity() {
    if (!updateReadings()) {
        return NAN;
    }
    return lastHumidity;
}

unsigned long Si7021Sensor::getTemperatureTimestamp() const {
    return tempTimestamp;
}

unsigned long Si7021Sensor::getHumidityTimestamp() const {
    return humTimestamp;
}

bool Si7021Sensor::performSelfTest() {
    // Reset the connected state before test
    connected = false;
    
    // Try to get both temperature and humidity readings
    bool success = updateReadings();
    
    if (success) {
        connected = true;
        errorHandler->logError(INFO, "Self-test passed for Si7021 sensor: " + name + 
                " (Temperature: " + String(lastTemperature) + "°C, Humidity: " + 
                String(lastHumidity) + "%)");
    } else {
        errorHandler->logError(ERROR, "Self-test failed for Si7021 sensor: " + name);
    }
    
    return success;
}

String Si7021Sensor::getSensorInfo() const {
    String info = "Sensor Name: " + name + "\n";
    info += "Type: Adafruit Si7021\n";
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
        
        // Additional information about the sensor if accessible
        try {
            info += "Hardware Revision: " + String(si7021.getRevision()) + "\n";
            info += "Serial Number: 0x" + String(si7021.sernum_a, HEX) + "\n";
        } catch (...) {
            info += "Detailed sensor info unavailable\n";
        }
    }
    
    return info;
}

bool Si7021Sensor::supportsInterface(InterfaceType type) const {
    switch (type) {
        case InterfaceType::TEMPERATURE:
        case InterfaceType::HUMIDITY:
            return true;
        default:
            return false;
    }
}

void* Si7021Sensor::getInterface(InterfaceType type) const {
    switch (type) {
        case InterfaceType::TEMPERATURE:
            return const_cast<ITemperatureSensor*>(static_cast<const ITemperatureSensor*>(this));
        case InterfaceType::HUMIDITY:
            return const_cast<IHumiditySensor*>(static_cast<const IHumiditySensor*>(this));
        default:
            return nullptr;
    }
}

bool Si7021Sensor::reinitialize() {
    errorHandler->logError(INFO, "Attempting to reinitialize Si7021 sensor: " + name);
    
    // Reset the connection state
    connected = false;
    
    // Add a small delay before reinitialization
    delay(50);
    
    // Try to initialize the sensor again
    bool success = si7021.begin();
    
    if (success) {
        connected = true;
        errorHandler->logError(INFO, "Successfully reinitialized Si7021 sensor: " + name);
        updateReadings();
    } else {
        errorHandler->logError(ERROR, "Failed to reinitialize Si7021 sensor: " + name);
    }
    
    return success;
}
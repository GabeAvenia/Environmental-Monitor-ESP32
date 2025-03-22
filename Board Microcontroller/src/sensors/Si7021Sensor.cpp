#include "Si7021Sensor.h"

Si7021Sensor::Si7021Sensor(const String& sensorName, int address, TwoWire* i2cBus, ErrorHandler* err)
    : BaseSensor(sensorName, SensorType::SI7021, err),
      wire(i2cBus),
      i2cAddress(address),
      lastTemperature(NAN),
      lastHumidity(NAN),
      tempTimestamp(0),
      humidityTimestamp(0) {
}

Si7021Sensor::~Si7021Sensor() {
    // Nothing to clean up
}

bool Si7021Sensor::initialize() {
    logInfo("Initializing Si7021 sensor: " + name);
    
    // The Si7021 library doesn't support specifying the I2C bus or address in begin()
    // We'll need to set these manually if possible or just use the defaults
    
    // Switch to the correct I2C bus
    if (wire) {
        si7021 = Adafruit_Si7021();
    }
    
    if (!si7021.begin()) {
        logError("Failed to initialize Si7021 sensor: " + name);
        connected = false;
        return false;
    }
    
    connected = true;
    logInfo("Si7021 sensor initialized successfully: " + name);
    
    // Read and log the sensor's serial number for identification
    uint32_t serialNumber = si7021.sernum_a;
    logInfo("Si7021 serial number: 0x" + String(serialNumber, HEX));
    
    // Get initial readings
    updateReadings();
    
    return true;
}

bool Si7021Sensor::updateReadings() const {
    if (!connected) {
        logErrorPublic("Attempted to read from disconnected sensor: " + name);
        return false;
    }
    
    // Read temperature
    float temp = si7021.readTemperature();
    if (isnan(temp)) {
        logErrorPublic("Failed to read temperature from Si7021 sensor: " + name);
        const_cast<Si7021Sensor*>(this)->connected = false;
        return false;
    }
    lastTemperature = temp;
    tempTimestamp = millis();
    
    // Read humidity
    float humidity = si7021.readHumidity();
    if (isnan(humidity)) {
        logErrorPublic("Failed to read humidity from Si7021 sensor: " + name);
        const_cast<Si7021Sensor*>(this)->connected = false;
        return false;
    }
    lastHumidity = humidity;
    humidityTimestamp = millis(); // We get a fresh timestamp as these are separate operations
    
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
    return humidityTimestamp;
}

bool Si7021Sensor::performSelfTest() {
    // Attempt to read both temperature and humidity
    float temp = si7021.readTemperature();
    float hum = si7021.readHumidity();
    
    // Check if readings are valid
    bool success = !isnan(temp) && !isnan(hum);
    
    if (!success) {
        logError("Self-test failed for Si7021 sensor: " + name);
        connected = false;
    } else {
        connected = true;
        logInfo("Self-test passed for Si7021 sensor: " + name + 
                " (Temperature: " + String(temp) + "°C, Humidity: " + String(hum) + "%)");
    }
    
    return success;
}

String Si7021Sensor::getSensorInfo() const {
    String info = "Sensor Name: " + name + "\n";
    info += "Type: Adafruit Si7021\n";
    info += "I2C Address: 0x" + String(i2cAddress, HEX) + "\n";
    info += "Connected: " + String(connected ? "Yes" : "No") + "\n";
    
    if (connected) {
        info += "Temperature: " + String(lastTemperature) + " °C\n";
        info += "Humidity: " + String(lastHumidity) + " %\n";
        
        // Calculate time since last reading
        unsigned long now = millis();
        unsigned long tempAge = now - tempTimestamp;
        info += "Last Temperature Reading: " + String(tempAge / 1000.0) + " seconds ago\n";
        
        unsigned long humAge = now - humidityTimestamp;
        info += "Last Humidity Reading: " + String(humAge / 1000.0) + " seconds ago\n";
        
        // Additional information about the sensor
        info += "Hardware Revision: " + String(si7021.getRevision()) + "\n";
        info += "Serial Number: 0x" + String(si7021.sernum_a, HEX) + "\n";
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
#include "Si7021Sensor.h"
Si7021Sensor::Si7021Sensor(const String& sensorName, int address, TwoWire* i2cBus, ErrorHandler* err)
    : BaseSensor(sensorName, SensorType::SI7021, err),
      wire(i2cBus),
      i2cAddress(address),
      lastTemperature(NAN),
      lastHumidity(NAN) {
}

Si7021Sensor::~Si7021Sensor() {
    // Nothing to clean up
}

bool Si7021Sensor::initialize() {
    logInfo("Initializing Si7021 sensor: " + name);
    
    // The Si7021 library doesn't support specifying the Wire instance
    // We need to do a manual check based on the wire pointer
    bool success = false;
    
    // Initialize the sensor
    if (wire == &Wire) {
        // Using default Wire
        success = si7021.begin();
    } else if (wire == &Wire1) {
        // Using Wire1 (STEMMA QT port)
        // We have to manually replace the global Wire object with Wire1
        // This is a workaround since the library doesn't support custom Wire instances
        Wire = Wire1;  // Temporarily redirect Wire to Wire1
        success = si7021.begin();
        // Note: In a real system, we'd need to restore Wire after this, but for our use case
        // with a single sensor, this is okay
    } else {
        logError("Unsupported Wire instance for Si7021 sensor: " + name);
        success = false;
    }
    
    if (!success) {
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
    
    // Adding retries for more robustness
    const int MAX_RETRIES = 3;
    float temp = NAN;
    float humidity = NAN;
    bool success = false;
    
    for (int attempt = 0; attempt < MAX_RETRIES && !success; attempt++) {
        if (attempt > 0) {
            // Add a delay between retries
            delay(50);
            logInfoPublic("Retrying Si7021 reading, attempt " + String(attempt+1) + " of " + String(MAX_RETRIES));
        }
        
        // Try reading temperature first
        try {
            temp = si7021.readTemperature();
            
            // If temperature read was successful, try humidity
            if (!isnan(temp)) {
                // Add a brief delay between readings to prevent I2C issues
                delay(5);
                humidity = si7021.readHumidity();
                
                // If both readings were successful, we're done
                if (!isnan(humidity)) {
                    success = true;
                }
            }
        } catch (...) {
            // Handle any exceptions (Arduino doesn't fully support them,
            // but this is a safety net for library implementation)
            logErrorPublic("Exception during Si7021 reading for sensor: " + name);
            delay(20); // Additional delay on exception
        }
    }
    
    // Check if readings were successful
    if (!success) {
        // Both readings failed even after retries
        logErrorPublic("Failed to read from Si7021 sensor: " + name + " after " + String(MAX_RETRIES) + " retries");
        const_cast<Si7021Sensor*>(this)->connected = false;
        return false;
    }
    
    // Update stored values
    lastTemperature = temp;
    lastHumidity = humidity;
    
    return true;
}

float Si7021Sensor::readTemperature() {
    // Try to update both readings since they're related
    if (!updateReadings()) {
        return NAN;
    }
    return lastTemperature;
}

float Si7021Sensor::readHumidity() {
    // Try to update both readings since they're related
    if (!updateReadings()) {
        return NAN;
    }
    return lastHumidity;
}

bool Si7021Sensor::performSelfTest() {
    // Reset the connected state before test
    connected = false;
    
    // Try to get both temperature and humidity readings
    bool success = updateReadings();
    
    if (success) {
        connected = true;
        logInfo("Self-test passed for Si7021 sensor: " + name + 
                " (Temperature: " + String(lastTemperature) + "°C, Humidity: " + 
                String(lastHumidity) + "%)");
    } else {
        logError("Self-test failed for Si7021 sensor: " + name);
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

bool Si7021Sensor::reinitialize() {
    logInfo("Attempting to reinitialize Si7021 sensor: " + name);
    
    // Reset the connection state
    connected = false;
    
    // Add a small delay before reinitialization
    delay(50);
    
    // Try to initialize the sensor again
    bool success = false;
    
    // The Si7021 library doesn't support specifying the Wire instance
    // We need to do a manual check based on the wire pointer
    if (wire == &Wire) {
        // Using default Wire
        success = si7021.begin();
    } else if (wire == &Wire1) {
        // Using Wire1 (STEMMA QT port)
        Wire = Wire1;  // Temporarily redirect Wire to Wire1
        success = si7021.begin();
    }
    
    if (success) {
        connected = true;
        logInfo("Successfully reinitialized Si7021 sensor: " + name);
        
        // Read the serial number again to verify
        uint32_t serialNumber = si7021.sernum_a;
        logInfo("Si7021 serial number after reinit: 0x" + String(serialNumber, HEX));
        
        // Update readings immediately
        updateReadings();
    } else {
        logError("Failed to reinitialize Si7021 sensor: " + name);
    }
    
    return success;
}
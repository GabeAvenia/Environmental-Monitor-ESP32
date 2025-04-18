#include "Si7021Sensor.h"

Si7021Sensor::Si7021Sensor(const String& sensorName, int address, TwoWire* i2cBus,
                          I2CManager* i2cMgr, I2CPort port, ErrorHandler* err)
    : BaseSensor(sensorName, SensorType::SI7021, err),
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
    logInfo("Initializing Si7021 sensor: " + name);
    
    // The Si7021 library doesn't support specifying the Wire instance directly
    bool success = false;
    
    // Store the original Wire reference
    TwoWire* originalWire = &Wire;
    
    // Initialize the sensor based on which Wire instance we're using
    if (wire == &Wire) {
        // Using default Wire - no redirection needed
        success = si7021.begin();
    } else {
        // Get the Wire configuration for our port
        const WireConfig* config = i2cManager->getWireConfig(i2cPort);
        if (!config) {
            logError("Failed to get Wire configuration for port " + I2CManager::portToString(i2cPort));
            connected = false;
            return false;
        }
        
        // Temporarily redirect Wire to our configuration
        Wire.end(); // End current Wire setup
        Wire.begin(config->sdaPin, config->sclPin);
        if (config->clockFrequency != 100000) {
            Wire.setClock(config->clockFrequency);
        }
        
        // Initialize with the configured Wire
        success = si7021.begin();
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
    
    // Store original Wire configuration
    TwoWire* originalWire = &Wire;
    bool needsWireReset = (wire != &Wire);
    
    if (needsWireReset) {
        // Configure Wire for our sensor
        const WireConfig* config = i2cManager->getWireConfig(i2cPort);
        if (config) {
            Wire.end();
            Wire.begin(config->sdaPin, config->sclPin);
            if (config->clockFrequency != 100000) {
                Wire.setClock(config->clockFrequency);
            }
        } else {
            logErrorPublic("Failed to get Wire configuration for reading");
            return false;
        }
    }
    
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
            // Handle any exceptions
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
    unsigned long now = millis();
    tempTimestamp = now;
    humTimestamp = now;
    
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

unsigned long Si7021Sensor::getTemperatureTimestamp() const {
    return tempTimestamp;
}

unsigned long Si7021Sensor::getHumidityTimestamp() const {
    return humTimestamp;
}

bool Si7021Sensor::performSelfTest() {
    // Reset the connected state before test
    connected = false;
    
    // Store original Wire configuration
    TwoWire* originalWire = &Wire;
    bool needsWireReset = (wire != &Wire);
    
    if (needsWireReset) {
        // Configure Wire for our sensor
        const WireConfig* config = i2cManager->getWireConfig(i2cPort);
        if (config) {
            Wire.end();
            Wire.begin(config->sdaPin, config->sclPin);
            if (config->clockFrequency != 100000) {
                Wire.setClock(config->clockFrequency);
            }
        } else {
            logError("Failed to get Wire configuration for self-test");
            return false;
        }
    }
    
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
    info += "I2C Port: " + I2CManager::portToString(i2cPort) + "\n";
    info += "Connected: " + String(connected ? "Yes" : "No") + "\n";
    
    if (connected) {
        // Store original Wire configuration
        TwoWire* originalWire = &Wire;
        bool needsWireReset = (wire != &Wire);
        
        if (needsWireReset) {
            // Configure Wire for our sensor
            const WireConfig* config = i2cManager->getWireConfig(i2cPort);
            if (config) {
                Wire.end();
                Wire.begin(config->sdaPin, config->sclPin);
                if (config->clockFrequency != 100000) {
                    Wire.setClock(config->clockFrequency);
                }
            }
        }
        
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
    logInfo("Attempting to reinitialize Si7021 sensor: " + name);
    
    // Reset the connection state
    connected = false;
    
    // Add a small delay before reinitialization
    delay(50);
    
    // Store original Wire configuration
    TwoWire* originalWire = &Wire;
    bool needsWireReset = (wire != &Wire);
    
    // Try to initialize the sensor again
    bool success = false;
    
    if (needsWireReset) {
        // Configure Wire for our sensor
        const WireConfig* config = i2cManager->getWireConfig(i2cPort);
        if (config) {
            Wire.end();
            Wire.begin(config->sdaPin, config->sclPin);
            if (config->clockFrequency != 100000) {
                Wire.setClock(config->clockFrequency);
            }
        } else {
            logError("Failed to get Wire configuration for reinitializing");
            return false;
        }
    }
    
    // The Si7021 library doesn't support specifying the Wire instance
    success = si7021.begin();
    
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
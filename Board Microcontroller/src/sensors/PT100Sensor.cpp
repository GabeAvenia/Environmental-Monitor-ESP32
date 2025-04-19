#include "PT100Sensor.h"

PT100Sensor::PT100Sensor(const String& sensorName, int ssPinNum, SPIManager* spiMgr, ErrorHandler* err,
                        float referenceResistor, int wireCount)
    : BaseSensor(sensorName, SensorType::PT100_RTD, err),
    max31865(ssPinNum), // Use the physical pin directly for MAX31865
    spiManager(spiMgr),
    ssPin(ssPinNum),    // Store the physical pin
    rRef(referenceResistor),
    numWires(wireCount),
    lastTemperature(NAN),
    tempTimestamp(0) {

    // Log the physical pin being used
    errorHandler->logError(INFO, "PT100 sensor using physical SS pin: " + String(ssPin));
}

PT100Sensor::~PT100Sensor() {
    // Nothing to clean up
}

bool PT100Sensor::initialize() {
    errorHandler->logError(INFO, "Initializing PT100 RTD sensor: " + name + " on SS pin " + String(ssPin));
    
    // Make sure SPI is initialized
    if (!spiManager || !spiManager->isInitialized()) {
        errorHandler->logError(ERROR, "SPI not initialized for PT100 sensor: " + name);
        connected = false;
        return false;
    }
    
    // Register the SS pin with the SPI manager
    spiManager->registerSSPin(ssPin);
    
    // Begin the MAX31865 with the appropriate wiring config
    max31865_numwires_t wiringMode;
    switch (numWires) {
        case 2:
            wiringMode = MAX31865_2WIRE;
            break;
        case 3:
            wiringMode = MAX31865_3WIRE;
            break;
        case 4:
        default:
            wiringMode = MAX31865_4WIRE;
            break;
    }
    
    // Initialize the MAX31865
    max31865.begin(wiringMode);
    
    // Check for any faults
    uint8_t fault = max31865.readFault();
    if (fault) {
        errorHandler->logError(ERROR, "MAX31865 fault detected during initialization: " + getFaultStatus());
        max31865.clearFault();
        // Only fail initialization for critical faults
        if (fault & (MAX31865_FAULT_OVUV | MAX31865_FAULT_REFINHIGH | MAX31865_FAULT_REFINLOW)) {
            connected = false;
            return false;
        }
        errorHandler->logError(ERROR, "Non-critical fault detected, attempting to continue");
    }
    
    // Read the RTD value directly for diagnostics
    uint16_t rtd = max31865.readRTD();
    float ratio = rtd / 32768.0;
    float resistance = ratio * rRef;
    
    errorHandler->logError(INFO, "Initial PT100 RTD value: " + String(rtd));
    errorHandler->logError(INFO, "Initial PT100 resistance: " + String(resistance) + " ohms (ratio: " + String(ratio, 8) + ")");
    
    // Check if RTD value is zero, which indicates a likely connection issue
    if (rtd == 0) {
        errorHandler->logError(ERROR, "RTD value is 0, suggesting a connection problem. Check wiring and SPI communication.");
        // Let's not mark it as disconnected yet, to allow for diagnostics
    }
    
    // Get initial temperature reading
    float temp = max31865.temperature(PT100_RTD_VALUE, rRef);
    errorHandler->logError(INFO, "Initial PT100 temperature: " + String(temp) + "°C");
    
    // Still mark as connected even with suspicious values for diagnostic purposes
    connected = true;
    
    // Store the initial reading
    lastTemperature = temp;
    tempTimestamp = millis();
    
    return true;
}

bool PT100Sensor::updateReading() const {
    if (!connected) {
        errorHandler->logError(ERROR, "Attempted to read from disconnected PT100 sensor: " + name);
        return false;
    }
    
    // Read the RTD value directly first for diagnostics
    uint16_t rtd = max31865.readRTD();
    float ratio = rtd / 32768.0;
    float resistance = ratio * rRef;
    
    // Only warn if RTD is zero, but don't disconnect
    if (rtd == 0) {
        errorHandler->logError(ERROR, "WARNING: PT100 RTD value is 0, suggesting a connection problem");
    }
    
    // Read the temperature
    float temp;
    try {
        // Calculate temperature using the RTD-to-temperature conversion
        temp = max31865.temperature(PT100_RTD_VALUE, rRef);
        
        // Check for any faults
        uint8_t fault = max31865.readFault();
        if (fault) {
            errorHandler->logError(ERROR, "MAX31865 fault detected during reading: " + getFaultStatus());
            max31865.clearFault();
            // Don't immediately disconnect for non-critical faults
        }
    } catch (...) {
        // Catch any exceptions that might occur during reading
        errorHandler->logError(ERROR, "Exception occurred while reading PT100 sensor: " + name);
        return false;
    }
    
    // Update our stored values regardless of validity
    lastTemperature = temp;
    tempTimestamp = millis();
    
    // No verbose logging here
    
    return true;
}

float PT100Sensor::readTemperature() {
    updateReading(); // Always update to get fresh readings
    return lastTemperature;
}

unsigned long PT100Sensor::getTemperatureTimestamp() const {
    return tempTimestamp;
}

bool PT100Sensor::performSelfTest() {
    errorHandler->logError(INFO, "Performing self-test on PT100 sensor: " + name);
    
    // Read the RTD value directly to check if the sensor is connected
    uint16_t rtd = max31865.readRTD();
    float ratio = rtd / 32768.0;
    float resistance = ratio * rRef;
    
    errorHandler->logError(INFO, "PT100 RTD value: " + String(rtd) + 
           ", Ratio: " + String(ratio, 8) + 
           ", Resistance: " + String(resistance, 3) + " ohms");
    
    // Check for any faults
    uint8_t fault = max31865.readFault();
    if (fault) {
        errorHandler->logError(ERROR, "MAX31865 fault detected during self-test: " + getFaultStatus());
        max31865.clearFault();
    }
    
    // Only consider it a failure if RTD is 0, suggesting no connection
    if (rtd == 0) {
        errorHandler->logError(ERROR, "Self-test failed: RTD value is 0, suggesting no connection");
        connected = false;
        return false;
    }
    
    // Try to read temperature to complete the test
    float temp = max31865.temperature(PT100_RTD_VALUE, rRef);
    errorHandler->logError(INFO, "PT100 temperature reading: " + String(temp) + "°C");
    
    // Keep connected true for diagnostics
    connected = true;
    errorHandler->logError(INFO, "Self-test passed for PT100 sensor: " + name);
    
    return true;
}

String PT100Sensor::getSensorInfo() const {
    String info = "Sensor Name: " + name + "\n";
    info += "Type: Adafruit PT100 RTD (MAX31865)\n";
    info += "SPI SS Pin: " + String(ssPin) + "\n";
    info += "Connected: " + String(connected ? "Yes" : "No") + "\n";
    info += "Wiring: " + String(numWires) + "-wire\n";
    info += "Reference Resistor: " + String(rRef) + " ohms\n";
    
    // Read live values for the info request
    uint16_t rtd = max31865.readRTD();
    float ratio = rtd / 32768.0;
    float resistance = ratio * rRef;
    
    info += "RTD Value: " + String(rtd) + "\n";
    info += "Ratio: " + String(ratio, 8) + "\n";
    info += "Resistance: " + String(resistance, 3) + " ohms\n";
    info += "Temperature: " + String(lastTemperature) + " °C\n";
    
    // Calculate time since last reading
    unsigned long now = millis();
    unsigned long age = now - tempTimestamp;
    info += "Last Reading: " + String(age / 1000.0) + " seconds ago\n";
    
    // Check for faults
    String faultStatus = getFaultStatus();
    info += "Fault Status: " + faultStatus + "\n";
    
    return info;
}

bool PT100Sensor::supportsInterface(InterfaceType type) const {
    switch (type) {
        case InterfaceType::TEMPERATURE:
            return true;
        default:
            return false;
    }
}

void* PT100Sensor::getInterface(InterfaceType type) const {
    switch (type) {
        case InterfaceType::TEMPERATURE:
            return const_cast<ITemperatureSensor*>(static_cast<const ITemperatureSensor*>(this));
        default:
            return nullptr;
    }
}

String PT100Sensor::getFaultStatus() const {
    uint8_t fault = max31865.readFault();
    
    if (!fault) {
        return "No Fault";
    }
    
    String faultStr = "";
    
    if (fault & MAX31865_FAULT_HIGHTHRESH) {
        faultStr += "RTD High Threshold, ";
    }
    if (fault & MAX31865_FAULT_LOWTHRESH) {
        faultStr += "RTD Low Threshold, ";
    }
    if (fault & MAX31865_FAULT_REFINLOW) {
        faultStr += "REFIN- > 0.85 x V_BIAS, ";
    }
    if (fault & MAX31865_FAULT_REFINHIGH) {
        faultStr += "REFIN- < 0.85 x V_BIAS (FORCE- open), ";
    }
    if (fault & MAX31865_FAULT_RTDINLOW) {
        faultStr += "RTDIN- < 0.85 x V_BIAS (FORCE- open), ";
    }
    if (fault & MAX31865_FAULT_OVUV) {
        faultStr += "Under/Over voltage";
    }
    
    // Trim trailing comma and space if present
    if (faultStr.endsWith(", ")) {
        faultStr = faultStr.substring(0, faultStr.length() - 2);
    }
    
    return faultStr;
}
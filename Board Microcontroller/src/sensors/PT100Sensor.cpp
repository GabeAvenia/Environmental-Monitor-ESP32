#include "PT100Sensor.h"

PT100Sensor::PT100Sensor(const String& sensorName, int ssPinNum, SPIManager* spiMgr, ErrorHandler* err,
                         float referenceResistor, int wireCount)
    : BaseSensor(sensorName, SensorType::PT100_RTD, err),
      max31865(ssPinNum), // Initialize MAX31865 with CS pin
      spiManager(spiMgr),
      ssPin(ssPinNum),
      rRef(referenceResistor),
      numWires(wireCount),
      lastTemperature(NAN),
      tempTimestamp(0) {
}

PT100Sensor::~PT100Sensor() {
    // Nothing to clean up
}

bool PT100Sensor::initialize() {
    logInfo("Initializing PT100 RTD sensor: " + name + " on SS pin " + String(ssPin));
    
    // Make sure SPI is initialized
    if (!spiManager || !spiManager->isInitialized()) {
        logError("SPI not initialized for PT100 sensor: " + name);
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
        logError("MAX31865 fault detected during initialization: " + getFaultStatus());
        max31865.clearFault();
        connected = false;
        return false;
    }
    
    connected = true;
    logInfo("PT100 RTD sensor initialized successfully: " + name + 
            " (" + String(numWires) + "-wire, reference: " + String(rRef) + " ohms)");
    
    // Get initial reading
    updateReading();
    
    return true;
}

bool PT100Sensor::updateReading() const {
    if (!connected) {
        logErrorPublic("Attempted to read from disconnected PT100 sensor: " + name);
        return false;
    }
    
    // Read the temperature
    float temp;
    try {
        // Read the temperature using the reference resistor value
        // For PT100, the RTD nominal value is 100 ohms at 0°C
        temp = max31865.temperature(PT100_RTD_VALUE, rRef);
        
        // Check for any faults
        uint8_t fault = max31865.readFault();
        if (fault) {
            logErrorPublic("MAX31865 fault detected during reading: " + getFaultStatus());
            max31865.clearFault();
            const_cast<PT100Sensor*>(this)->connected = false;
            return false;
        }
    } catch (...) {
        // Catch any exceptions that might occur during reading
        logErrorPublic("Exception occurred while reading PT100 sensor: " + name);
        const_cast<PT100Sensor*>(this)->connected = false;
        return false;
    }
    
    // Check if the reading is valid
    if (isnan(temp) || temp < -200 || temp > 850) {
        logErrorPublic("Invalid temperature reading from PT100 sensor: " + name + 
                   " (" + String(temp) + "°C)");
        const_cast<PT100Sensor*>(this)->connected = false;
        return false;
    }
    
    lastTemperature = temp;
    tempTimestamp = millis();
    
    return true;
}

float PT100Sensor::readTemperature() {
    if (!updateReading()) {
        return NAN;
    }
    return lastTemperature;
}

unsigned long PT100Sensor::getTemperatureTimestamp() const {
    return tempTimestamp;
}

bool PT100Sensor::performSelfTest() {
    // Read the RTD value directly to check if the sensor is connected
    uint16_t rtd = max31865.readRTD();
    float ratio = rtd / 32768.0;  // RTD / 2^15
    float resistance = ratio * rRef;
    
    // Check for any faults
    uint8_t fault = max31865.readFault();
    if (fault) {
        logError("MAX31865 fault detected during self-test: " + getFaultStatus());
        max31865.clearFault();
        connected = false;
        return false;
    }
    
    // A typical PT100 has 100 ohms at 0°C and about 138.5 ohms at 100°C
    // Check if the resistance is in a reasonable range
    if (resistance < 80.0 || resistance > 200.0) {
        logError("PT100 resistance out of range: " + String(resistance) + " ohms");
        connected = false;
        return false;
    }
    
    // Try to read temperature as final check
    float temp = max31865.temperature(PT100_RTD_VALUE, rRef);
    if (isnan(temp) || temp < -200 || temp > 850) {
        logError("Invalid temperature reading during self-test: " + String(temp) + "°C");
        connected = false;
        return false;
    }
    
    connected = true;
    logInfo("Self-test passed for PT100 sensor: " + name + 
            " (Resistance: " + String(resistance) + " ohms, Temperature: " + String(temp) + "°C)");
    
    return true;
}

String PT100Sensor::getSensorInfo() const {
    String info = "Sensor Name: " + name + "\n";
    info += "Type: Adafruit PT100 RTD (MAX31865)\n";
    info += "SPI SS Pin: " + String(ssPin) + "\n";
    info += "Connected: " + String(connected ? "Yes" : "No") + "\n";
    info += "Wiring: " + String(numWires) + "-wire\n";
    info += "Reference Resistor: " + String(rRef) + " ohms\n";
    
    if (connected) {
        info += "Temperature: " + String(lastTemperature) + " °C\n";
        
        // Calculate time since last reading
        unsigned long now = millis();
        unsigned long age = now - tempTimestamp;
        info += "Last Reading: " + String(age / 1000.0) + " seconds ago\n";
        
        // Get RTD value and resistance
        uint16_t rtd = max31865.readRTD();
        float ratio = rtd / 32768.0;
        float resistance = ratio * rRef;
        info += "RTD Value: " + String(rtd) + "\n";
        info += "Resistance: " + String(resistance) + " ohms\n";
        
        // Check for faults
        String faultStatus = getFaultStatus();
        info += "Fault Status: " + faultStatus + "\n";
    }
    
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
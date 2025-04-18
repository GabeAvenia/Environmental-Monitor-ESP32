#include "SensorFactory.h"
#include "SHT41Sensor.h"
#include "Si7021Sensor.h"
#include "PT100Sensor.h"

SensorFactory::SensorFactory(ErrorHandler* err, I2CManager* i2c, SPIManager* spi) 
    : errorHandler(err), i2cManager(i2c), spiManager(spi) {
}

void SensorFactory::setSPIManager(SPIManager* spi) {
    spiManager = spi;
}

// Template method for I2C sensor creation
template<typename SensorType>
ISensor* SensorFactory::createI2CSensor(const SensorConfig& config) {
    if (config.isSPI) {
        errorHandler->logError(ERROR, "Sensor type does not support SPI interface");
        return nullptr;
    }
    
    // Initialize the appropriate I2C bus if not already done
    if (!i2cManager->isPortInitialized(config.i2cPort)) {
        if (!i2cManager->beginPort(config.i2cPort)) {
            errorHandler->logError(ERROR, "Failed to initialize I2C port " + 
                                I2CManager::portToString(config.i2cPort) + 
                                " for sensor " + config.name);
            return nullptr;
        }
    }
    
    // Get the appropriate TwoWire instance
    TwoWire* wire = i2cManager->getWire(config.i2cPort);
    if (!wire) {
        errorHandler->logError(ERROR, "Failed to get I2C bus for port " + 
                            I2CManager::portToString(config.i2cPort));
        return nullptr;
    }
    
    // Create sensor with the specified configuration
    return new SensorType(config.name, config.address, wire, i2cManager, config.i2cPort, errorHandler);
}

// Explicit template instantiations for supported sensor types
template ISensor* SensorFactory::createI2CSensor<SHT41Sensor>(const SensorConfig& config);
template ISensor* SensorFactory::createI2CSensor<Si7021Sensor>(const SensorConfig& config);

// Updated createSensor method
ISensor* SensorFactory::createSensor(const SensorConfig& config) {
    SensorType type = sensorTypeFromString(config.type);
    
    // Log sensor creation
    errorHandler->logInfo("Creating sensor: " + config.name + " of type " + config.type + 
                     (config.isSPI ? 
                      " (SPI, SS Pin: " + String(config.address) + ")" : 
                      " (I2C, Port: " + I2CManager::portToString(config.i2cPort) + 
                      ", Address: 0x" + String(config.address, HEX) + ")"));
    
    // Check SPI manager for SPI sensors
    if (config.isSPI && !spiManager) {
        errorHandler->logError(ERROR, "SPI manager not provided for SPI sensor: " + config.name);
        return nullptr;
    }
    
    // Create sensor based on type
    switch (type) {
        case SensorType::SHT41:
            return createI2CSensor<SHT41Sensor>(config);
            
        case SensorType::SI7021:
            return createI2CSensor<Si7021Sensor>(config);
            
        case SensorType::PT100_RTD:
            return createPT100Sensor(config);  // Special case for PT100
  
        default:
            errorHandler->logError(ERROR, "Unsupported sensor type: " + config.type);
            return nullptr;
    }
}

// PT100 still needs special handling due to additional parameters
ISensor* SensorFactory::createPT100Sensor(const SensorConfig& config) {
    if (!config.isSPI) {
        errorHandler->logError(ERROR, "PT100 RTD requires SPI interface");
        return nullptr;
    }
    
    // Check SPI initialization
    if (!spiManager || !spiManager->isInitialized()) {
        errorHandler->logError(ERROR, "SPI not initialized for PT100 sensor: " + config.name);
        return nullptr;
    }
    
    // Default values
    float referenceResistor = 430.0;
    int wireMode = 3;
    
    // Parse additional settings if provided
    parseAdditionalPT100Settings(config.additional, referenceResistor, wireMode);
    
    // Map the logical pin to physical pin
    int physicalSsPin = config.address;
    if (config.address >= 0 && config.address < MAX_SS_PINS) {
        physicalSsPin = spiManager->mapLogicalToPhysicalPin(config.address);
    }
    
    // Create the sensor with hardware SPI
    errorHandler->logInfo("Creating PT100 sensor with physical SS pin: " + String(physicalSsPin) + 
                         ", Ref: " + String(referenceResistor) + 
                         ", Wire mode: " + String(wireMode));
    
    return new PT100Sensor(
        config.name,
        physicalSsPin,
        spiManager,
        errorHandler,
        referenceResistor,
        wireMode
    );
}

// Helper to parse PT100 additional settings
void SensorFactory::parseAdditionalPT100Settings(const String& additional, 
                                               float& refResistor, 
                                               int& wireMode) {
    if (additional.length() == 0) return;
    
    // Parse wire mode
    if (additional.indexOf("Wire mode:") >= 0 || 
        additional.indexOf("wire mode:") >= 0 ||
        additional.indexOf("Wire:") >= 0) {
        
        int pos = additional.indexOf("-wire");
        if (pos > 0) {
            for (int i = pos - 1; i >= 0; i--) {
                if (isDigit(additional.charAt(i))) {
                    char digitChar = additional.charAt(i);
                    if (digitChar >= '2' && digitChar <= '4') {
                        wireMode = digitChar - '0';
                        break;
                    }
                }
            }
        }
    }
    
    // Parse reference resistor value
    if (additional.indexOf("Ref:") >= 0 || 
        additional.indexOf("ref:") >= 0 ||
        additional.indexOf("Resistor:") >= 0) {
        
        int pos = additional.indexOf("Ref:") >= 0 ? additional.indexOf("Ref:") + 4 : 
                (additional.indexOf("ref:") >= 0 ? additional.indexOf("ref:") + 4 : 
                additional.indexOf("Resistor:") + 10);
        
        if (pos > 0) {
            String valStr = "";
            for (unsigned int i = pos; i < additional.length() && 
                (isDigit(additional.charAt(i)) || additional.charAt(i) == '.'); i++) {
                valStr += additional.charAt(i);
            }
            
            if (valStr.length() > 0) {
                float val = valStr.toFloat();
                if (val > 100.0 && val < 10000.0) {
                    refResistor = val;
                }
            }
        }
    }
}
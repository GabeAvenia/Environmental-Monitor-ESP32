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

ISensor* SensorFactory::createSensor(const SensorConfig& config) {
    SensorType type = sensorTypeFromString(config.type);
    
    // Log sensor creation attempt
    errorHandler->logInfo("Creating sensor: " + config.name + " of type " + config.type);
    if (!config.isSPI) {
        errorHandler->logInfo("I2C configuration: Port " + I2CManager::portToString(config.i2cPort) + 
                          ", Address 0x" + String(config.address, HEX));
    } else {
        errorHandler->logInfo("SPI configuration: SS Pin " + String(config.address));
        
        // Check if SPI manager is available
        if (!spiManager) {
            errorHandler->logError(ERROR, "SPI manager not provided for SPI sensor: " + config.name);
            return nullptr;
        }
    }
    
    // Create appropriate sensor based on type
    switch (type) {
        case SensorType::SHT41:
            return createSHT41Sensor(config);
            
        case SensorType::BME280:
            return createBME280Sensor(config);
            
        case SensorType::TMP117:
            return createTMP117Sensor(config);
            
        case SensorType::SI7021:
            return createSi7021Sensor(config);
            
        case SensorType::PT100_RTD:
            return createPT100Sensor(config);
            
        default:
            errorHandler->logError(ERROR, "Unknown sensor type: " + config.type);
            return nullptr;
    }
}

ISensor* SensorFactory::createSHT41Sensor(const SensorConfig& config) {
    if (config.isSPI) {
        errorHandler->logError(ERROR, "SHT41 does not support SPI interface");
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
    
    // Create an SHT41 sensor with the specified configuration
    SHT41Sensor* sensor = new SHT41Sensor(
        config.name,
        config.address,
        wire,
        errorHandler
    );
    
    return sensor;
}

ISensor* SensorFactory::createBME280Sensor(const SensorConfig& config) {
    // For now, not implemented
    errorHandler->logError(ERROR, "BME280 sensor support not implemented yet");
    return nullptr;
}

ISensor* SensorFactory::createTMP117Sensor(const SensorConfig& config) {
    // For now, not implemented
    errorHandler->logError(ERROR, "TMP117 sensor support not implemented yet");
    return nullptr;
}

ISensor* SensorFactory::createSi7021Sensor(const SensorConfig& config) {
    if (config.isSPI) {
        errorHandler->logError(ERROR, "Si7021 does not support SPI interface");
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
    
    // Create a Si7021 sensor with the specified configuration
    Si7021Sensor* sensor = new Si7021Sensor(
        config.name,
        config.address,
        wire,
        errorHandler
    );
    
    return sensor;
}

ISensor* SensorFactory::createPT100Sensor(const SensorConfig& config) {
    if (!config.isSPI) {
        errorHandler->logError(ERROR, "PT100 RTD requires SPI interface");
        return nullptr;
    }
    
    // Make sure SPI is initialized
    if (!spiManager || !spiManager->isInitialized()) {
        errorHandler->logError(ERROR, "SPI not initialized for PT100 sensor: " + config.name);
        return nullptr;
    }
    
    // Default values
    float referenceResistor = 430.0; // Default reference resistor in ohms
    int wireMode = 3;               // Default to 3-wire mode
    
    // Parse additional settings if provided
    if (config.additional.length() > 0) {
        // Parse wire mode
        if (config.additional.indexOf("Wire mode:") >= 0 || 
            config.additional.indexOf("wire mode:") >= 0 ||
            config.additional.indexOf("Wire:") >= 0) {
            
            // Extract the number before "wire"
            int pos = config.additional.indexOf("-wire");
            if (pos > 0) {
                // Look for a digit before "-wire"
                char digitChar = '0';
                for (int i = pos - 1; i >= 0; i--) {
                    if (isDigit(config.additional.charAt(i))) {
                        digitChar = config.additional.charAt(i);
                        break;
                    }
                }
                if (digitChar >= '2' && digitChar <= '4') {
                    wireMode = digitChar - '0';
                    errorHandler->logInfo("PT100 wire mode set to " + String(wireMode) + "-wire from additional settings");
                }
            }
        }
        
        // Parse reference resistor value
        if (config.additional.indexOf("Ref:") >= 0 || 
            config.additional.indexOf("ref:") >= 0 ||
            config.additional.indexOf("Resistor:") >= 0) {
            
            int pos = config.additional.indexOf("Ref:") >= 0 ? config.additional.indexOf("Ref:") + 4 : 
                    (config.additional.indexOf("ref:") >= 0 ? config.additional.indexOf("ref:") + 4 : 
                    (config.additional.indexOf("Resistor:") >= 0 ? config.additional.indexOf("Resistor:") + 10 : -1));
            
            if (pos > 0) {
                String valStr = "";
                for (unsigned int i = pos; i < config.additional.length() && (isDigit(config.additional.charAt(i)) || config.additional.charAt(i) == '.'); i++) {
                    valStr += config.additional.charAt(i);
                }
                
                if (valStr.length() > 0) {
                    float val = valStr.toFloat();
                    if (val > 100.0 && val < 10000.0) { // Sanity check for resistor value
                        referenceResistor = val;
                        errorHandler->logInfo("PT100 reference resistor set to " + String(referenceResistor) + " ohms from additional settings");
                    }
                }
            }
        }
    }
    
    // Get the logical SS pin from config
    int logicalSsPin = config.address;
    
    // Map the logical pin to physical pin
    int physicalSsPin = logicalSsPin;
    if (logicalSsPin >= 0 && logicalSsPin < MAX_SS_PINS) {
        physicalSsPin = spiManager->mapLogicalToPhysicalPin(logicalSsPin);
    }
    
    // Create with hardware SPI
    errorHandler->logInfo("Creating PT100 sensor with hardware SPI: CS:" + String(physicalSsPin) + 
                        " (logical index: " + String(logicalSsPin) + ")");
    
    PT100Sensor* sensor = new PT100Sensor(
        config.name,
        physicalSsPin,  // Pass the PHYSICAL pin to the constructor
        spiManager,
        errorHandler,
        referenceResistor,
        wireMode
    );
    
    return sensor;
}
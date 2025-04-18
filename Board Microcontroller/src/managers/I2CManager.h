#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <vector>
#include <map>
#include "../error/ErrorHandler.h"

// Define I2C port identifiers - expandable enum for future buses
enum class I2CPort {
    I2C0 = 0,   // Default I2C bus (typically Arduino pins)
    I2C1 = 1,   // Secondary I2C bus (typically STEMMA QT)
    
    I2C_MULTIPLEXED_START = 100,  // Range for multiplexed buses
    // Multiplexed buses would be I2C_MULTIPLEXED_START + multiplexer_channel
};

// Structure to hold configuration for a TwoWire instance
struct WireConfig {
    TwoWire* wire;            // Pointer to the TwoWire instance
    int sdaPin;               // SDA pin number
    int sclPin;               // SCL pin number
    bool initialized;         // Whether this bus has been initialized
    uint32_t clockFrequency;  // I2C clock frequency in Hz
    
    WireConfig() : wire(nullptr), sdaPin(-1), sclPin(-1), 
                   initialized(false), clockFrequency(100000) {}
                   
    WireConfig(TwoWire* w, int sda, int scl, uint32_t freq = 100000) 
        : wire(w), sdaPin(sda), sclPin(scl), initialized(false), clockFrequency(freq) {}
};

class I2CManager {
private:
    std::map<I2CPort, WireConfig> wireBuses;
    ErrorHandler* errorHandler;
    
public:
    /**
     * @brief Constructor for I2CManager.
     * 
     * @param err Pointer to the error handler for logging.
     */
    I2CManager(ErrorHandler* err);
    
    /**
     * @brief Destructor.
     */
    ~I2CManager();
    
    /**
     * @brief Register a TwoWire instance for a specific I2C port
     * 
     * @param port The I2C port identifier
     * @param wire Pointer to the TwoWire instance
     * @param sdaPin The SDA pin number for this bus
     * @param sclPin The SCL pin number for this bus
     * @param clockFreq The I2C clock frequency in Hz (default: 100kHz)
     * @return true if registration succeeded, false otherwise
     */
    bool registerWire(I2CPort port, TwoWire* wire, int sdaPin, int sclPin, uint32_t clockFreq = 100000);
    
    /**
     * @brief Initialize default I2C buses.
     * 
     * @return true if at least one bus was initialized, false if all failed.
     */
    bool begin();
    
    /**
     * @brief Initialize a specific I2C bus.
     * 
     * @param port The I2C port to initialize.
     * @return true if initialization succeeded, false otherwise.
     */
    bool beginPort(I2CPort port);
    
    /**
     * @brief Check if a specific I2C port is initialized.
     * 
     * @param port The I2C port to check.
     * @return true if the port is initialized, false otherwise.
     */
    bool isPortInitialized(I2CPort port) const;
    
    /**
     * @brief Scan an I2C bus for devices.
     * 
     * @param port The I2C port to scan.
     * @param addresses Output vector that will be filled with found addresses.
     * @return true if at least one device was found, false otherwise.
     */
    bool scanBus(I2CPort port, std::vector<int>& addresses);
    
    /**
     * @brief Get the TwoWire object for a specific I2C port.
     * 
     * @param port The I2C port.
     * @return Pointer to the TwoWire object, or nullptr if not initialized.
     */
    TwoWire* getWire(I2CPort port);
    
    /**
     * @brief Get the configuration for a specific I2C port
     * 
     * @param port The I2C port
     * @return The WireConfig structure, or nullptr if not registered
     */
    const WireConfig* getWireConfig(I2CPort port) const;
    
    /**
     * @brief Check if a device is present at the specified address on a specific I2C port.
     * 
     * @param port The I2C port to check.
     * @param address The I2C address to check.
     * @return true if a device is present, false otherwise.
     */
    bool devicePresent(I2CPort port, int address);
    
    /**
     * @brief Convert a string port name to I2CPort enum.
     * 
     * @param portName The string port name (e.g., "I2C0", "I2C1").
     * @return The corresponding I2CPort enum value.
     */
    static I2CPort stringToPort(const String& portName);
    
    /**
     * @brief Convert I2CPort enum to string.
     * 
     * @param port The I2CPort enum value.
     * @return The string representation of the port.
     */
    static String portToString(I2CPort port);
};
#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <vector>
#include "../error/ErrorHandler.h"

// Define I2C port identifiers
enum class I2CPort {
    I2C0,   // Default I2C bus (GPIO7-SDA, GPIO6-SCL)
    I2C1    // Secondary I2C bus (GPIO40-SDA, GPIO41-SCL)
};

class I2CManager {
private:
    TwoWire* wire0;   // Primary I2C bus (Wire)
    TwoWire* wire1;   // Secondary I2C bus (Wire1)
    ErrorHandler* errorHandler;
    bool initialized0;
    bool initialized1;
    
    // Pin definitions for I2C buses
    // I2C0
    int sda0Pin;  // GPIO7 (A3/SDA)
    int scl0Pin;  // GPIO6 (A2/SCL)
    
    // I2C1
    int sda1Pin;  // GPIO40 (SCL1, marked as MTDO)
    int scl1Pin;  // GPIO41 (SDA1, marked as MTDI)
    
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
     * @brief Initialize both I2C buses.
     * 
     * @return true if at least one bus was initialized, false if both failed.
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
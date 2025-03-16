#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <vector>
#include "../error/ErrorHandler.h"

class I2CManager {
private:
    TwoWire* wire;
    ErrorHandler* errorHandler;
    bool initialized;
    int sdaPin;
    int sclPin;
    
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
     * @brief Initialize the I2C bus.
     * 
     * @param sda SDA pin number.
     * @param scl SCL pin number.
     * @return true if initialization succeeded, false otherwise.
     */
    bool begin(int sda = 41, int scl = 40);
    
    /**
     * @brief Check if the I2C manager is initialized.
     * 
     * @return true if initialized, false otherwise.
     */
    bool isInitialized() const;
    
    /**
     * @brief Scan the I2C bus for devices.
     * 
     * @param addresses Output vector that will be filled with found addresses.
     * @return true if at least one device was found, false otherwise.
     */
    bool scanBus(std::vector<int>& addresses);
    
    /**
     * @brief Get the I2C bus object.
     * 
     * @return Pointer to the TwoWire object.
     */
    TwoWire* getWire();
    
    /**
     * @brief Check if a device is present at the specified address.
     * 
     * @param address The I2C address to check.
     * @return true if a device is present, false otherwise.
     */
    bool devicePresent(int address);
    
    /**
     * @brief Get the SDA pin number.
     * 
     * @return The SDA pin number.
     */
    int getSdaPin() const;
    
    /**
     * @brief Get the SCL pin number.
     * 
     * @return The SCL pin number.
     */
    int getSclPin() const;
};

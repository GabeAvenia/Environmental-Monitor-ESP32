#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <vector>
#include "../error/ErrorHandler.h"

/**
 * @brief Class for managing SPI communications with sensors.
 */
class SPIManager {
private:
    ErrorHandler* errorHandler;
    bool initialized;
    
    // SPI pin definitions
    int mosiPin;
    int misoPin;
    int sckPin;
    std::vector<int> ssPins; // List of all slave select pins
    
    // Settings for different devices
    SPISettings defaultSettings;
    
public:
    /**
     * @brief Constructor for SPIManager.
     * 
     * @param err Pointer to the error handler for logging.
     */
    SPIManager(ErrorHandler* err);
    
    /**
     * @brief Destructor.
     */
    ~SPIManager();
    
    /**
     * @brief Initialize the SPI bus.
     * 
     * @param mosi MOSI pin number.
     * @param miso MISO pin number.
     * @param sck SCK pin number.
     * @return true if initialization succeeded, false otherwise.
     */
    bool begin(int mosi = 37, int miso = 35, int sck = 36);
    
    /**
     * @brief Check if the SPI manager is initialized.
     * 
     * @return true if initialized, false otherwise.
     */
    bool isInitialized() const;
    
    /**
     * @brief Register a slave select pin.
     * 
     * @param ssPin The slave select pin number.
     * @return true if registration succeeded, false otherwise.
     */
    bool registerSSPin(int ssPin);
    
    /**
     * @brief Begin a transaction with a specific device.
     * 
     * @param ssPin The slave select pin for the device.
     * @param settings Optional SPI settings for this transaction.
     * @return true if transaction setup succeeded, false otherwise.
     */
    bool beginTransaction(int ssPin, SPISettings settings = SPISettings());
    
    /**
     * @brief End the current transaction.
     * 
     * @param ssPin The slave select pin for the device.
     */
    void endTransaction(int ssPin);
    
    /**
     * @brief Transfer a single byte over SPI.
     * 
     * @param data The byte to send.
     * @return The byte received.
     */
    uint8_t transfer(uint8_t data);
    
    /**
     * @brief Transfer multiple bytes over SPI.
     * 
     * @param buffer The buffer to send and receive data.
     * @param size The size of the buffer.
     */
    void transfer(uint8_t* buffer, size_t size);
    
    /**
     * @brief Test if a device is responding on the specified SS pin.
     * 
     * This may need to be customized depending on the specific device protocol.
     * 
     * @param ssPin The slave select pin for the device.
     * @return true if device responds, false otherwise.
     */
    bool testDevice(int ssPin);
    
    /**
     * @brief Get the SPI object.
     * 
     * @return Reference to the SPI object.
     */
    SPIClass& getSPI();
};
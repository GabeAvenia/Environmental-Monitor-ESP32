#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <vector>
#include "../error/ErrorHandler.h"
#include "Constants.h"

constexpr int DEFAULT_SPI_MOSI_PIN = Constants::Pins::SPI::MOSI;
constexpr int DEFAULT_SPI_MISO_PIN = Constants::Pins::SPI::MISO;
constexpr int DEFAULT_SPI_SCK_PIN = Constants::Pins::SPI::SCK;
constexpr int DEFAULT_SPI_SS_PIN = -1;

constexpr int SS_PIN_A0 = Constants::Pins::SPI::SS_A0;
constexpr int SS_PIN_A1 = Constants::Pins::SPI::SS_A1;
constexpr int SS_PIN_A2 = Constants::Pins::SPI::SS_A2;
constexpr int SS_PIN_A3 = Constants::Pins::SPI::SS_A3;

// Number of supported SS pins
#define MAX_SS_PINS 4

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
    static int ssPinMapping[MAX_SS_PINS]; // Mapping array for SS pins
    
    // Settings for different devices
    SPISettings defaultSettings;
    
public:
    /**
     * @brief Map a logical SS pin index to a physical pin number.
     * 
     * @param logicalPin The logical SS pin index.
     * @return The physical pin number.
     */
    int mapLogicalToPhysicalPin(int logicalPin) {
        if (logicalPin >= 0 && logicalPin < MAX_SS_PINS) {
            return ssPinMapping[logicalPin];
        }
        return logicalPin; // Return unchanged if out of range
    }

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
    bool begin(int mosi = DEFAULT_SPI_MOSI_PIN, int miso = DEFAULT_SPI_MISO_PIN, 
               int sck = DEFAULT_SPI_SCK_PIN);
    
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
    
    /**
     * @brief Get the MOSI pin.
     * 
     * @return The MOSI pin number.
     */
    int getMosiPin() const { return mosiPin; }
    
    /**
     * @brief Get the MISO pin.
     * 
     * @return The MISO pin number.
     */
    int getMisoPin() const { return misoPin; }
    
    /**
     * @brief Get the SCK pin.
     * 
     * @return The SCK pin number.
     */
    int getSckPin() const { return sckPin; }
};
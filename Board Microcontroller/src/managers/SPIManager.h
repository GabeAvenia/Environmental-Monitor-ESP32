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
     * @brief Map a logical SS pin index to a physical pin number.
     * 
     * @param logicalPin The logical SS pin index.
     * @return The physical pin number.
     */
    int mapLogicalToPhysicalPin(int logicalPin) {
        size_t arraySize = sizeof(Constants::Pins::SPI::SS_PINS) / sizeof(int);
        if (logicalPin >= 0 && logicalPin < arraySize) {
            return Constants::Pins::SPI::SS_PINS[logicalPin];
        }
        return logicalPin; // Return unchanged if out of range
    }

    // Other methods as before
    SPIManager(ErrorHandler* err);
    ~SPIManager();
    bool begin(int mosi = DEFAULT_SPI_MOSI_PIN, int miso = DEFAULT_SPI_MISO_PIN, 
               int sck = DEFAULT_SPI_SCK_PIN);
    bool isInitialized() const;
    bool registerSSPin(int ssPin);
    bool beginTransaction(int ssPin, SPISettings settings = SPISettings());
    void endTransaction(int ssPin);
    uint8_t transfer(uint8_t data);
    void transfer(uint8_t* buffer, size_t size);
    bool testDevice(int ssPin);
    SPIClass& getSPI();
    int getMosiPin() const { return mosiPin; }
    int getMisoPin() const { return misoPin; }
    int getSckPin() const { return sckPin; }
};
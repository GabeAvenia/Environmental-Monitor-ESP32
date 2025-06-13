/**
 * @file SPIManager.h
 * @brief Manager for SPI bus communications
 * @author Gabriel Avenia
 * @date May 2025
 * @defgroup spi_management SPI Management
 * @ingroup communication
 * @brief Components for managing SPI bus and device communications
 * @{
 */

 #pragma once

 #include <Arduino.h>
 #include <SPI.h>
 #include <vector>
 #include "../error/ErrorHandler.h"
 #include "Constants.h"
 
 /**
  * @brief Default SPI pin definitions
  * @{
  */
 constexpr int DEFAULT_SPI_MOSI_PIN = Constants::Pins::SPI::MOSI;
 constexpr int DEFAULT_SPI_MISO_PIN = Constants::Pins::SPI::MISO;
 constexpr int DEFAULT_SPI_SCK_PIN = Constants::Pins::SPI::SCK;
 constexpr int DEFAULT_SPI_SS_PIN = -1;   ///< No default SS pin
 /** @} */
 
 /**
  * @brief Class for managing SPI communications with sensors
  * This class provides a centralized management system for SPI communications,
  * handling bus initialization, device selection, and transaction management.
  */
 class SPIManager {
 private:
     /**
      * @brief Error handler for logging and error reporting
      */
     ErrorHandler* errorHandler;
     
     /**
      * @brief Flag indicating if SPI has been initialized
      */
     bool initialized;
     
     /**
      * @brief SPI pin definitions
      * @{
      */
     int mosiPin;
     int misoPin;
     int sckPin;
     std::vector<int> ssPins; ///< List of all slave select pins
     /** @} */
     
     /**
      * @brief Default settings for SPI communications
      */
     SPISettings defaultSettings;
     
 public:
     /**
      * @brief Map a logical SS pin index to a physical pin number
      * Converts a logical pin index (0, 1, 2, etc.) to the corresponding
      * physical GPIO pin number from the Constants::Pins::SPI::SS_PINS array.
      * @param logicalPin The logical SS pin index
      * @return The physical pin number
      */
     int mapLogicalToPhysicalPin(int logicalPin) {
         size_t arraySize = sizeof(Constants::Pins::SPI::SS_PINS) / sizeof(int);
         if (logicalPin >= 0 && logicalPin < arraySize) {
             return Constants::Pins::SPI::SS_PINS[logicalPin];
         }
         return logicalPin; // Return unchanged if out of range
     }
 
     /**
      * @brief Constructor for SPIManager
      * @param err Pointer to the error handler for logging
      */
     SPIManager(ErrorHandler* err);
     
     /**
      * @brief Destructor
      */
     ~SPIManager();
     
     /**
      * @brief Initialize the SPI bus
      * Initializes the SPI bus with the specified pin assignments.
      * @param mosi The MOSI pin number (default: Constants::Pins::SPI::MOSI)
      * @param miso The MISO pin number (default: Constants::Pins::SPI::MISO)
      * @param sck The SCK pin number (default: Constants::Pins::SPI::SCK)
      * @return true if initialization succeeded, false otherwise
      */
     bool begin(int mosi = DEFAULT_SPI_MOSI_PIN, int miso = DEFAULT_SPI_MISO_PIN, 
                int sck = DEFAULT_SPI_SCK_PIN);
     
     /**
      * @brief Check if SPI bus is initialized
      * @return true if SPI is initialized, false otherwise
      */
     bool isInitialized() const;
     
     /**
      * @brief Register an SS pin for use with SPI
      * Configures the specified pin as an output for use as an SPI slave select.
      * @param ssPin The SS pin to register (logical index or physical pin)
      * @return true if registration succeeded, false otherwise
      */
     bool registerSSPin(int ssPin);
     
     /**
      * @brief Begin an SPI transaction
      * Begins an SPI transaction with the specified SS pin and settings.
      * @param ssPin The SS pin to use (logical index or physical pin)
      * @param settings The SPI settings to use (default: SPISettings())
      * @return true if transaction initialization succeeded, false otherwise
      */
     bool beginTransaction(int ssPin, SPISettings settings = SPISettings());
     
     /**
      * @brief End an SPI transaction
      * Ends the current SPI transaction and deselects the specified SS pin.
      * @param ssPin The SS pin to deselect (logical index or physical pin)
      */
     void endTransaction(int ssPin);
     
     /**
      * @brief Transfer a single byte over SPI
      * @param data The byte to send
      * @return The byte received during the transfer
      */
     uint8_t transfer(uint8_t data);
     
     /**
      * @brief Transfer multiple bytes over SPI
      * @param buffer Pointer to the data buffer (in-place transfer)
      * @param size Number of bytes to transfer
      */
     void transfer(uint8_t* buffer, size_t size);
     
     /**
      * @brief Test if an SPI device is present
      * Performs a simple test to check if a device is responding on the specified SS pin.
      * @param ssPin The SS pin to test (logical index or physical pin)
      * @return true if device appears to be present, false otherwise
      * @note This test may not be reliable for all devices
      */
     bool testDevice(int ssPin);
     
     /**
      * @brief Get the SPI class instance
      * @return Reference to the SPI instance
      */
     SPIClass& getSPI();
     
     /**
      * @brief Get the MOSI pin
* @return The MISO pin number
     */
    int getMisoPin() const { return misoPin; }
    
    /**
     * @brief Get the SCK pin
     * @return The SCK pin number
     */
    int getSckPin() const { return sckPin; }
};

/** @} */ // End of spi_management group

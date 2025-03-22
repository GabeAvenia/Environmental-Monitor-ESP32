#pragma once

#include <Adafruit_MAX31865.h>
#include <SPI.h>
#include "BaseSensor.h"
#include "interfaces/ITemperatureSensor.h"
#include "readings/TemperatureReading.h"
#include "../managers/SPIManager.h"

// Define PT100 RTD value constant (100 ohms at 0°C)
#define PT100_RTD_VALUE 100

/**
 * @brief Implementation of the PT100 RTD temperature sensor with MAX31865 ADC.
 * 
 * This class provides access to the PT100 RTD temperature sensor,
 * using the MAX31865 ADC to convert resistance to temperature.
 */
class PT100Sensor : public BaseSensor, 
                    public ITemperatureSensor {
                        private:
                        mutable Adafruit_MAX31865 max31865;   ///< Underlying Adafruit driver
                        SPIManager* spiManager;               ///< SPI manager for communication
                        int ssPin;                            ///< SPI slave select pin
                        float rRef;                           ///< Reference resistor value (430.0 ohms by default)
                        int numWires;                         ///< Number of wires (2, 3, or 4)
                        mutable float lastTemperature;        ///< Last temperature reading
                        mutable unsigned long tempTimestamp;  ///< Timestamp of last temperature reading
    
    /**
     * @brief Update temperature reading from the sensor.
     * 
     * @return True if the reading was successfully updated, false otherwise.
     */
    bool updateReading() const;

public:
    /**
     * @brief Constructor for PT100Sensor.
     * 
     * @param sensorName Name/identifier for this sensor instance.
     * @param ssPinNum Slave select pin for SPI communication.
     * @param spiMgr Pointer to the SPI manager.
     * @param err Pointer to the error handler for logging.
     * @param referenceResistor Value of the reference resistor in ohms (default: 430.0 for Adafruit board).
     * @param wireCount Number of wires in the PT100 connection (default: 3 for three-wire).
     */
    PT100Sensor(const String& sensorName, int ssPinNum, SPIManager* spiMgr, ErrorHandler* err,
                float referenceResistor = 430.0, int wireCount = 3);
    
    /**
     * @brief Destructor.
     */
    ~PT100Sensor();
    
    /**
     * @brief Initialize the sensor hardware.
     * 
     * @return true if initialization succeeded, false otherwise.
     */
    bool initialize() override;
    
    /**
     * @brief Read the current temperature value from the sensor.
     * 
     * @return The temperature in degrees Celsius, or NAN if reading failed.
     */
    float readTemperature() override;
    
    /**
     * @brief Get the timestamp of the last temperature reading.
     * 
     * @return Timestamp in milliseconds (from millis()).
     */
    unsigned long getTemperatureTimestamp() const override;
    
    /**
     * @brief Perform a self-test to verify the sensor is functioning properly.
     * 
     * @return true if the self-test passed, false otherwise.
     */
    bool performSelfTest() override;
    
    /**
     * @brief Get detailed information about the sensor.
     * 
     * @return A string containing sensor information.
     */
    String getSensorInfo() const override;
    
    /**
     * @brief Check if this sensor supports a specific interface.
     * 
     * @param type The interface type to check for.
     * @return true if the sensor supports the interface, false otherwise.
     */
    bool supportsInterface(InterfaceType type) const override;
    
    /**
     * @brief Get the interface implementation for a specific type.
     * 
     * @param type The interface type to get.
     * @return Pointer to the interface, or nullptr if not supported.
     */
    void* getInterface(InterfaceType type) const override;

    /**
     * @brief Get the MAX31865 fault status and description.
     * 
     * @return String description of any faults, or "No Fault" if none.
     */
    String getFaultStatus() const;
};
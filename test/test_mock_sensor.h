/**
 * @file test_mock_sensor.h
 * @brief Test suite for sensor interface implementation using mock sensor
 * @author Gabriel Avenia
 * @date May 2025
 * @defgroup mock_sensor_tests Mock Sensor Tests
 * @brief Tests for the ISensor interface using a mock implementation
 * @{
 */

#ifndef TEST_MOCK_SENSOR_H
#define TEST_MOCK_SENSOR_H

#include <unity.h>
#include "../src/sensors/BaseSensor.h"
#include "../src/sensors/interfaces/ITemperatureSensor.h"
#include "../src/sensors/interfaces/IHumiditySensor.h"

/**
 * @brief Mock sensor implementation for testing
 * @details Implements both ISensor, ITemperatureSensor and IHumiditySensor
 *          interfaces for testing sensor management functionality.
 */
class MockSensor : public BaseSensor, 
                   public ITemperatureSensor,
                   public IHumiditySensor {
private:
    float mockTemperature;          ///< Mock temperature value
    float mockHumidity;             ///< Mock humidity value
    unsigned long tempTimestamp;    ///< Temperature reading timestamp
    unsigned long humTimestamp;     ///< Humidity reading timestamp
    bool shouldInitializeSuccessfully; ///< Flag to control init behavior
    bool shouldConnectSuccessfully;    ///< Flag to control connection behavior

public:
    /**
     * @brief Constructor for MockSensor
     * @param name Sensor name/identifier
     * @param err Error handler pointer
     * @param initSuccess Whether initialization should succeed
     * @param connectSuccess Whether connection should succeed
     */
    MockSensor(const String& name, ErrorHandler* err, bool initSuccess = true, bool connectSuccess = true)
        : BaseSensor(name, SensorType::UNKNOWN, err),
          mockTemperature(25.0),
          mockHumidity(50.0),
          tempTimestamp(0),
          humTimestamp(0),
          shouldInitializeSuccessfully(initSuccess),
          shouldConnectSuccessfully(connectSuccess) {
    }
    
    /**
     * @brief Initialize the sensor
     * @return true if initialization succeeded (based on shouldInitializeSuccessfully)
     */
    bool initialize() override {
        connected = shouldConnectSuccessfully;
        return shouldInitializeSuccessfully;
    }
    
    /**
     * @brief Read the current temperature value
     * @return The mock temperature value, or NAN if not connected
     */
    float readTemperature() override {
        if (!connected) return NAN;
        tempTimestamp = millis();
        return mockTemperature;
    }
    
    /**
     * @brief Read the current humidity value
     * @return The mock humidity value, or NAN if not connected
     */
    float readHumidity() override {
        if (!connected) return NAN;
        humTimestamp = millis();
        return mockHumidity;
    }
    
    /**
     * @brief Get the timestamp of the last temperature reading
     * @return Temperature reading timestamp
     */
    unsigned long getTemperatureTimestamp() const override {
        return tempTimestamp;
    }
    
    /**
     * @brief Get the timestamp of the last humidity reading
     * @return Humidity reading timestamp
     */
    unsigned long getHumidityTimestamp() const override {
        return humTimestamp;
    }
    
    /**
     * @brief Perform a sensor self-test
     * @return true if the self-test passed (based on shouldConnectSuccessfully)
     */
    bool performSelfTest() override {
        connected = shouldConnectSuccessfully;
        return shouldConnectSuccessfully;
    }
    
    /**
     * @brief Get detailed information about the sensor
     * @return String with sensor information
     */
    String getSensorInfo() const override {
        return "MockSensor: " + name;
    }
    
    /**
     * @brief Check if this sensor supports the specified interface
     * @param type The interface type to check
     * @return true if supported (temperature and humidity only)
     */
    bool supportsInterface(InterfaceType type) const override {
        return (type == InterfaceType::TEMPERATURE || 
                type == InterfaceType::HUMIDITY);
    }
    
    /**
     * @brief Get the interface implementation for a specific type
     * @param type The interface type to get
     * @return Pointer to the interface, or nullptr if not supported
     */
    void* getInterface(InterfaceType type) const override {
        switch (type) {
            case InterfaceType::TEMPERATURE:
                return const_cast<ITemperatureSensor*>(
                    static_cast<const ITemperatureSensor*>(this));
            case InterfaceType::HUMIDITY:
                return const_cast<IHumiditySensor*>(
                    static_cast<const IHumiditySensor*>(this));
            default:
                return nullptr;
        }
    }
    
    // Test helper methods
    
    /**
     * @brief Set the mock temperature value
     * @param temp New temperature value
     */
    void setMockTemperature(float temp) {
        mockTemperature = temp;
    }
    
    /**
     * @brief Set the mock humidity value
     * @param hum New humidity value
     */
    void setMockHumidity(float hum) {
        mockHumidity = hum;
    }
    
    /**
     * @brief Manually set the connected state
     * @param isConnected New connected state
     */
    void setConnected(bool isConnected) {
        connected = isConnected;
    }
};

/**
 * @brief Test basic mock sensor creation and initialization
 * @details Verifies that the MockSensor correctly handles
 *          initialization and connection state.
 */
void test_mock_sensor_creation() {
    ErrorHandler errorHandler(nullptr);
    MockSensor sensor("TestMockSensor", &errorHandler);
    
    TEST_ASSERT_EQUAL_STRING("TestMockSensor", sensor.getName().c_str());
    TEST_ASSERT_FALSE(sensor.isConnected()); // Not connected before initialize
    
    bool initResult = sensor.initialize();
    TEST_ASSERT_TRUE(initResult);
    TEST_ASSERT_TRUE(sensor.isConnected()); // Connected after successful init
}

/**
 * @brief Test reading temperature and humidity from mock sensor
 * @details Verifies that the MockSensor correctly provides temperature
 *          and humidity readings according to its current state.
 */
void test_mock_sensor_readings() {
    ErrorHandler errorHandler(nullptr);
    MockSensor sensor("TestMockSensor", &errorHandler);
    sensor.initialize();
    
    // Test default values
    TEST_ASSERT_EQUAL_FLOAT(25.0, sensor.readTemperature());
    TEST_ASSERT_EQUAL_FLOAT(50.0, sensor.readHumidity());
    
    // Test setting and reading custom values
    sensor.setMockTemperature(30.5);
    sensor.setMockHumidity(75.8);
    
    TEST_ASSERT_EQUAL_FLOAT(30.5, sensor.readTemperature());
    TEST_ASSERT_EQUAL_FLOAT(75.8, sensor.readHumidity());
    
    // Test disconnected state
    sensor.setConnected(false);
    TEST_ASSERT_TRUE(isnan(sensor.readTemperature()));
    TEST_ASSERT_TRUE(isnan(sensor.readHumidity()));
}

/**
 * @brief Test interface querying for mock sensor
 * @details Verifies that the MockSensor correctly implements
 *          the interface querying system for supported interfaces.
 */
void test_mock_sensor_interfaces() {
    ErrorHandler errorHandler(nullptr);
    MockSensor sensor("TestMockSensor", &errorHandler);
    
    // Test interface support
    TEST_ASSERT_TRUE(sensor.supportsInterface(InterfaceType::TEMPERATURE));
    TEST_ASSERT_TRUE(sensor.supportsInterface(InterfaceType::HUMIDITY));
    TEST_ASSERT_FALSE(sensor.supportsInterface(InterfaceType::CO2));
    
    // Test interface retrieval
    void* tempInterface = sensor.getInterface(InterfaceType::TEMPERATURE);
    TEST_ASSERT_NOT_NULL(tempInterface);
    ITemperatureSensor* tempSensor = static_cast<ITemperatureSensor*>(tempInterface);
    
    void* humInterface = sensor.getInterface(InterfaceType::HUMIDITY);
    TEST_ASSERT_NOT_NULL(humInterface);
    IHumiditySensor* humSensor = static_cast<IHumiditySensor*>(humInterface);
    
    void* co2Interface = sensor.getInterface(InterfaceType::CO2);
    TEST_ASSERT_NULL(co2Interface);
}

/**
 * @brief Run all mock sensor tests
 */
void run_mock_sensor_tests() {
    RUN_TEST(test_mock_sensor_creation);
    RUN_TEST(test_mock_sensor_readings);
    RUN_TEST(test_mock_sensor_interfaces);
}

#endif // TEST_MOCK_SENSOR_H

/** @} */ // End of mock_sensor_tests group
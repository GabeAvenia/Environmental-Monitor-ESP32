/**
 * @file test_sensor_registry.h
 * @brief Test suite for the SensorRegistry component
 * @author Gabriel Avenia
 * @date May 2025
 * @defgroup registry_tests Sensor Registry Tests
 * @brief Tests for sensor registration and management
 * @{
 */

#ifndef TEST_SENSOR_REGISTRY_H
#define TEST_SENSOR_REGISTRY_H

#include <unity.h>
#include "../src/managers/SensorRegistry.h"
#include "test_mock_sensor.h"

/**
 * @brief Test sensor registration and lookup functionality
 * @details Verifies that sensors can be registered, counted, and
 *          retrieved by name from the registry.
 */
void test_sensor_registry_registration() {
    ErrorHandler errorHandler(nullptr);
    SensorRegistry registry(&errorHandler);
    
    // Verify initially empty
    TEST_ASSERT_EQUAL(0, registry.count());
    
    // Create and register sensors
    MockSensor* sensor1 = new MockSensor("Sensor1", &errorHandler);
    MockSensor* sensor2 = new MockSensor("Sensor2", &errorHandler);
    
    registry.registerSensor(sensor1);
    registry.registerSensor(sensor2);
    
    // Verify count and lookup
    TEST_ASSERT_EQUAL(2, registry.count());
    TEST_ASSERT_EQUAL(sensor1, registry.getSensorByName("Sensor1"));
    TEST_ASSERT_EQUAL(sensor2, registry.getSensorByName("Sensor2"));
    TEST_ASSERT_NULL(registry.getSensorByName("NonExistentSensor"));
    
    // Verify hasSensor
    TEST_ASSERT_TRUE(registry.hasSensor("Sensor1"));
    TEST_ASSERT_TRUE(registry.hasSensor("Sensor2"));
    TEST_ASSERT_FALSE(registry.hasSensor("NonExistentSensor"));
    
    // Cleanup
    std::vector<ISensor*> sensors = registry.clear();
    
    // Verify clear
    TEST_ASSERT_EQUAL(0, registry.count());
    TEST_ASSERT_EQUAL(2, sensors.size());
    
    // Cleanup resources
    delete sensor1;
    delete sensor2;
}

/**
 * @brief Test retrieving sensors by interface type
 * @details Verifies that sensors can be retrieved by capability
 *          (temperature, humidity) rather than by name.
 */
void test_sensor_registry_interface_lookup() {
    ErrorHandler errorHandler(nullptr);
    SensorRegistry registry(&errorHandler);
    
    // Create and register sensors
    MockSensor* tempHumSensor = new MockSensor("TempHumSensor", &errorHandler);
    registry.registerSensor(tempHumSensor);
    
    // Initialize sensors before interface queries
    tempHumSensor->initialize();
    
    // Get sensors by interface type
    auto tempSensors = registry.getTemperatureSensors();
    auto humSensors = registry.getHumiditySensors();
    
    // Verify counts
    TEST_ASSERT_EQUAL(1, tempSensors.size());
    TEST_ASSERT_EQUAL(1, humSensors.size());
    
    // Verify returned sensors are correct
    TEST_ASSERT_EQUAL(static_cast<ITemperatureSensor*>(tempHumSensor), tempSensors[0]);
    TEST_ASSERT_EQUAL(static_cast<IHumiditySensor*>(tempHumSensor), humSensors[0]);
    
    // Cleanup
    std::vector<ISensor*> sensors = registry.clear();
    delete tempHumSensor;
}

/**
 * @brief Test registry behavior with duplicate sensor names
 * @details Verifies that the registry enforces unique names and
 *          properly handles duplicate registration attempts.
 */
void test_sensor_registry_duplicates() {
    ErrorHandler errorHandler(nullptr);
    SensorRegistry registry(&errorHandler);
    
    // Create and register sensors
    MockSensor* sensor1 = new MockSensor("SameName", &errorHandler);
    MockSensor* sensor2 = new MockSensor("SameName", &errorHandler);
    
    // Log a message to clear any previous errors
    errorHandler.logError(INFO, "Starting duplicate test");
    
    registry.registerSensor(sensor1);
    registry.registerSensor(sensor2);
    
    // Verify only first sensor was registered (since names must be unique)
    TEST_ASSERT_EQUAL(1, registry.count());
    TEST_ASSERT_EQUAL(sensor1, registry.getSensorByName("SameName"));
    
    // Verify warning was logged about duplicate
    TEST_ASSERT_EQUAL(WARNING, errorHandler.getLastSeverity());
    TEST_ASSERT_TRUE(errorHandler.getLastMessage().indexOf("already exists") >= 0);
    
    // Cleanup
    std::vector<ISensor*> sensors = registry.clear();
    delete sensor1;
    delete sensor2;
}

/**
 * @brief Test unregistration of sensors
 * @details Verifies that sensors can be selectively removed from
 *          the registry while preserving other entries.
 */
void test_sensor_registry_unregistration() {
    ErrorHandler errorHandler(nullptr);
    SensorRegistry registry(&errorHandler);
    
    MockSensor* sensor1 = new MockSensor("Sensor1", &errorHandler);
    MockSensor* sensor2 = new MockSensor("Sensor2", &errorHandler);
    
    registry.registerSensor(sensor1);
    registry.registerSensor(sensor2);
    
    // Verify initial state
    TEST_ASSERT_EQUAL(2, registry.count());
    
    // Unregister sensor1
    ISensor* unregistered = registry.unregisterSensor("Sensor1");
    
    // Verify unregistration
    TEST_ASSERT_EQUAL(sensor1, unregistered);
    TEST_ASSERT_EQUAL(1, registry.count());
    TEST_ASSERT_NULL(registry.getSensorByName("Sensor1"));
    TEST_ASSERT_EQUAL(sensor2, registry.getSensorByName("Sensor2"));
    
    // Clear previous messages
    errorHandler.logError(INFO, "Testing non-existent sensor unregistration");
    
    // Try unregistering non-existent sensor
    unregistered = registry.unregisterSensor("NonExistent");
    
    // Verify no change and null return
    TEST_ASSERT_NULL(unregistered);
    TEST_ASSERT_EQUAL(1, registry.count());
    
    // Verify warning was logged
    TEST_ASSERT_EQUAL(WARNING, errorHandler.getLastSeverity());
    TEST_ASSERT_TRUE(errorHandler.getLastMessage().indexOf("non-existent") >= 0);
    
    // Cleanup
    std::vector<ISensor*> sensors = registry.clear();
    delete sensor1;
    delete sensor2;
}

/**
 * @brief Run all sensor registry tests
 */
void run_sensor_registry_tests() {
    RUN_TEST(test_sensor_registry_registration);
    RUN_TEST(test_sensor_registry_interface_lookup);
    RUN_TEST(test_sensor_registry_duplicates);
    RUN_TEST(test_sensor_registry_unregistration);
}

#endif // TEST_SENSOR_REGISTRY_H

/** @} */ // End of registry_tests group
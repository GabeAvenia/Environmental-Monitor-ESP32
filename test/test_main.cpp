/**
 * @file test_main.cpp
 * @brief Main test runner file
 * @author Gabriel Avenia
 * @date May 2025
 * @defgroup test_main Test System
 * @brief Main entry point for unit testing
 * @{
 */

#include <Arduino.h>
#include <unity.h>

// Include all test headers
#include "test_config.h"
#include "test_error_handler.h"
#include "test_readings.h"
#include "test_mock_sensor.h"
#include "test_sensor_registry.h"
#include "test_double_buffering.h"
#include "test_sensor_types.h"

// Function declarations for the test groups
void run_config_tests();
void run_error_handler_tests();
void run_reading_tests();
void run_mock_sensor_tests();
void run_sensor_registry_tests();
void run_double_buffering_tests();
void run_sensor_type_tests();

/**
 * @brief Setup function runs before each test
 */
void setUp(void) {
    // Global setup code runs before each test
}

/**
 * @brief Teardown function runs after each test
 */
void tearDown(void) {
    // Global teardown code runs after each test
}

/**
 * @brief Arduino setup function - initializes tests
 */
void setup() {
    // Wait for serial connection
    delay(2000);
    Serial.begin(115200);
    
    UNITY_BEGIN();
    
    // Run all test groups
    run_config_tests();
    run_error_handler_tests();
    run_reading_tests();
    run_mock_sensor_tests();
    run_sensor_registry_tests();
    run_double_buffering_tests();
    run_sensor_type_tests();
    
    UNITY_END();
}

/**
 * @brief Arduino loop function - empty for tests
 */
void loop() {
    // Tests complete, nothing to do here
}

/** @} */ // End of test_main group
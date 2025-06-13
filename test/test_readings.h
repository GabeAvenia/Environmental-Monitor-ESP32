/**
 * @file test_readings.h
 * @brief Test suite for temperature and humidity reading structures
 * @author Gabriel Avenia
 * @date May 2025
 * @defgroup reading_tests Reading Structure Tests
 * @brief Tests for sensor reading data structures
 * @{
 */

#ifndef TEST_READINGS_H
#define TEST_READINGS_H

#include <unity.h>
#include "../src/sensors/readings/TemperatureReading.h"
#include "../src/sensors/readings/HumidityReading.h"

/**
 * @brief Test TemperatureReading default constructor
 * @details Verifies that the default constructor creates an
 *          invalid reading with appropriate values.
 */
void test_temperature_reading_default_constructor() {
    TemperatureReading reading;
    
    TEST_ASSERT_FALSE(reading.valid);
    TEST_ASSERT_TRUE(isnan(reading.value));
    TEST_ASSERT_EQUAL(0, reading.timestamp);
    TEST_ASSERT_EQUAL_STRING("Invalid", reading.toString().c_str());
}

/**
 * @brief Test TemperatureReading parametrized constructor with valid value
 * @details Verifies that the parametrized constructor correctly
 *          initializes a valid reading with the provided values.
 */
void test_temperature_reading_valid_constructor() {
    float testTemp = 25.5;
    unsigned long testTime = 12345;
    TemperatureReading reading(testTemp, testTime);
    
    TEST_ASSERT_TRUE(reading.valid);
    TEST_ASSERT_EQUAL_FLOAT(testTemp, reading.value);
    TEST_ASSERT_EQUAL(testTime, reading.timestamp);
    TEST_ASSERT_EQUAL_STRING("25.50 °C", reading.toString().c_str());
}

/**
 * @brief Test TemperatureReading with invalid value
 * @details Verifies that the parametrized constructor with
 *          an invalid value (NAN) creates an invalid reading.
 */
void test_temperature_reading_invalid_value() {
    float testTemp = NAN;
    TemperatureReading reading(testTemp);
    
    TEST_ASSERT_FALSE(reading.valid);
    TEST_ASSERT_TRUE(isnan(reading.value));
    TEST_ASSERT_EQUAL_STRING("Invalid", reading.toString().c_str());
}

/**
 * @brief Test HumidityReading default constructor
 * @details Verifies that the default constructor creates an
 *          invalid reading with appropriate values.
 */
void test_humidity_reading_default_constructor() {
    HumidityReading reading;
    
    TEST_ASSERT_FALSE(reading.valid);
    TEST_ASSERT_TRUE(isnan(reading.value));
    TEST_ASSERT_EQUAL(0, reading.timestamp);
    TEST_ASSERT_EQUAL_STRING("Invalid", reading.toString().c_str());
}

/**
 * @brief Test HumidityReading parametrized constructor with valid value
 * @details Verifies that the parametrized constructor correctly
 *          initializes a valid reading with the provided values.
 */
void test_humidity_reading_valid_constructor() {
    float testHumidity = 65.7;
    unsigned long testTime = 67890;
    HumidityReading reading(testHumidity, testTime);
    
    TEST_ASSERT_TRUE(reading.valid);
    TEST_ASSERT_EQUAL_FLOAT(testHumidity, reading.value);
    TEST_ASSERT_EQUAL(testTime, reading.timestamp);
    TEST_ASSERT_EQUAL_STRING("65.70 %", reading.toString().c_str());
}

/**
 * @brief Test HumidityReading with invalid value
 * @details Verifies that the parametrized constructor with
 *          an invalid value (NAN) creates an invalid reading.
 */
void test_humidity_reading_invalid_value() {
    float testHumidity = NAN;
    HumidityReading reading(testHumidity);
    
    TEST_ASSERT_FALSE(reading.valid);
    TEST_ASSERT_TRUE(isnan(reading.value));
    TEST_ASSERT_EQUAL_STRING("Invalid", reading.toString().c_str());
}

/**
 * @brief Run all reading structure tests
 */
void run_reading_tests() {
    RUN_TEST(test_temperature_reading_default_constructor);
    RUN_TEST(test_temperature_reading_valid_constructor);
    RUN_TEST(test_temperature_reading_invalid_value);
    RUN_TEST(test_humidity_reading_default_constructor);
    RUN_TEST(test_humidity_reading_valid_constructor);
    RUN_TEST(test_humidity_reading_invalid_value);
}

#endif // TEST_READINGS_H

/** @} */ // End of reading_tests group
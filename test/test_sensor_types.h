/**
 * @file test_sensor_types.h
 * @brief Test suite for sensor type enumeration and conversion utilities
 * @author Gabriel Avenia
 * @date May 2025
 * @defgroup sensor_type_tests Sensor Type Tests
 * @brief Tests for sensor type identification and conversion
 * @{
 */

#ifndef TEST_SENSOR_TYPES_H
#define TEST_SENSOR_TYPES_H

#include <Arduino.h>
#include <unity.h>
#include "../src/sensors/SensorTypes.h"

/**
 * @brief Test conversion from string to SensorType enum
 * @details Verifies that string representations are correctly
 *          converted to the corresponding SensorType enum values.
 */
void test_sensor_type_from_string() {
    // Test known types
    TEST_ASSERT_EQUAL(SensorType::SHT41, sensorTypeFromString("SHT41"));
    TEST_ASSERT_EQUAL(SensorType::SI7021, sensorTypeFromString("SI7021"));
    TEST_ASSERT_EQUAL(SensorType::PT100_RTD, sensorTypeFromString("PT100_RTD"));
    
    // Test case-insensitive
    TEST_ASSERT_EQUAL(SensorType::SHT41, sensorTypeFromString("sht41"));
    TEST_ASSERT_EQUAL(SensorType::SI7021, sensorTypeFromString("si7021"));
    
    // Test alternate names
    TEST_ASSERT_EQUAL(SensorType::SI7021, sensorTypeFromString("Adafruit SI7021"));
    TEST_ASSERT_EQUAL(SensorType::PT100_RTD, sensorTypeFromString("Adafruit PT100 RTD"));
    
    // Test unknown type
    TEST_ASSERT_EQUAL(SensorType::UNKNOWN, sensorTypeFromString("NonExistentType"));
    TEST_ASSERT_EQUAL(SensorType::UNKNOWN, sensorTypeFromString(""));
}

/**
 * @brief Test conversion from SensorType enum to string
 * @details Verifies that SensorType enum values are correctly
 *          converted to their string representations.
 */
void test_sensor_type_to_string() {
    // Test all enum values
    TEST_ASSERT_EQUAL_STRING("SHT41", sensorTypeToString(SensorType::SHT41).c_str());
    TEST_ASSERT_EQUAL_STRING("Adafruit SI7021", sensorTypeToString(SensorType::SI7021).c_str());
    TEST_ASSERT_EQUAL_STRING("Adafruit PT100 RTD", sensorTypeToString(SensorType::PT100_RTD).c_str());
    TEST_ASSERT_EQUAL_STRING("UNKNOWN", sensorTypeToString(SensorType::UNKNOWN).c_str());
}

/**
 * @brief Test roundtrip conversion (string -> enum -> string)
 * @details Verifies that converting from string to enum and back
 *          produces the expected canonical string representation.
 */
void test_sensor_type_roundtrip_conversion() {
    // For SHT41
    SensorType type1 = sensorTypeFromString("SHT41");
    String str1 = sensorTypeToString(type1);
    TEST_ASSERT_EQUAL_STRING("SHT41", str1.c_str());
    
    // For SI7021
    SensorType type2 = sensorTypeFromString("SI7021");
    String str2 = sensorTypeToString(type2);
    TEST_ASSERT_EQUAL_STRING("Adafruit SI7021", str2.c_str());
    
    // For PT100_RTD
    SensorType type3 = sensorTypeFromString("PT100_RTD");
    String str3 = sensorTypeToString(type3);
    TEST_ASSERT_EQUAL_STRING("Adafruit PT100 RTD", str3.c_str());
}

/**
 * @brief Run all sensor type tests
 */
void run_sensor_type_tests() {
    RUN_TEST(test_sensor_type_from_string);
    RUN_TEST(test_sensor_type_to_string);
    RUN_TEST(test_sensor_type_roundtrip_conversion);
}

#endif // TEST_SENSOR_TYPES_H

/** @} */ // End of sensor_type_tests group
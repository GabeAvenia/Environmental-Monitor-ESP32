/**
 * @file test_config.h
 * @brief Test suite for the ConfigManager and SensorConfig components
 * @author Gabriel Avenia
 * @date May 2025
 * @defgroup config_tests Configuration Tests
 * @brief Tests for configuration component functionality
 * @{
 */

#ifndef TEST_CONFIG_H
#define TEST_CONFIG_H

#include <unity.h>
#include "../src/config/ConfigManager.h"

/**
 * @brief Test SensorConfig equality operator implementation
 * @details Verifies that the equality operator correctly identifies
 *          identical configurations and detects differences in all fields.
 */
void test_sensor_config_equality() {
    SensorConfig config1;
    config1.name = "TestSensor";
    config1.type = "SHT41";
    config1.communicationType = CommunicationType::I2C;
    config1.portNum = 0;
    config1.address = 0x44;
    config1.pollingRate = 1000;
    config1.additional = "";

    // Create a copy that should be equal
    SensorConfig config2 = config1;
    
    // Test equality operator
    TEST_ASSERT_TRUE(config1 == config2);
    
    // Modify one field and test inequality
    config2.name = "DifferentSensor";
    TEST_ASSERT_FALSE(config1 == config2);
    
    // Reset and modify another field
    config2 = config1;
    config2.type = "SI7021";
    TEST_ASSERT_FALSE(config1 == config2);
    
    // Reset and modify communication type
    config2 = config1;
    config2.communicationType = CommunicationType::SPI;
    TEST_ASSERT_FALSE(config1 == config2);
    
    // Reset and modify port number
    config2 = config1;
    config2.portNum = 1;
    TEST_ASSERT_FALSE(config1 == config2);
}

/**
 * @brief Test SensorConfig inequality operator implementation
 * @details Verifies that the inequality operator correctly identifies
 *          differing configurations and returns false for identical ones.
 */
void test_sensor_config_inequality() {
    SensorConfig config1;
    config1.name = "SensorA";
    config1.type = "SHT41";
    config1.communicationType = CommunicationType::I2C;
    config1.portNum = 0;
    config1.address = 0x44;
    
    SensorConfig config2;
    config2.name = "SensorB";
    config2.type = "SI7021";
    config2.communicationType = CommunicationType::SPI;
    config2.portNum = 0;
    config2.address = 0x40;
    
    // Test inequality operator
    TEST_ASSERT_TRUE(config1 != config2);
    
    // Make them equal and test
    config2 = config1;
    TEST_ASSERT_FALSE(config1 != config2);
}

/**
 * @brief Test communication type string/enum conversion
 * @details Verifies conversion functions between string representations
 *          and enum values for the CommunicationType enum.
 */
void test_communication_type_conversion() {
    // Test string to enum conversion
    TEST_ASSERT_EQUAL(CommunicationType::I2C, stringToCommunicationType("I2C"));
    TEST_ASSERT_EQUAL(CommunicationType::SPI, stringToCommunicationType("SPI"));
    
    // Test case insensitivity
    TEST_ASSERT_EQUAL(CommunicationType::I2C, stringToCommunicationType("i2c"));
    TEST_ASSERT_EQUAL(CommunicationType::SPI, stringToCommunicationType("spi"));
    
    // Test unknown string defaults to I2C
    TEST_ASSERT_EQUAL(CommunicationType::I2C, stringToCommunicationType("UNKNOWN"));
    
    // Test enum to string conversion
    TEST_ASSERT_EQUAL_STRING("I2C", communicationTypeToString(CommunicationType::I2C).c_str());
    TEST_ASSERT_EQUAL_STRING("SPI", communicationTypeToString(CommunicationType::SPI).c_str());
}

/**
 * @brief Run all configuration component tests
 */
void run_config_tests() {
    RUN_TEST(test_sensor_config_equality);
    RUN_TEST(test_sensor_config_inequality);
    RUN_TEST(test_communication_type_conversion);
}

#endif // TEST_CONFIG_H

/** @} */ // End of config_tests group
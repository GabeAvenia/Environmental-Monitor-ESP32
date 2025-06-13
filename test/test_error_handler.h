/**
 * @file test_error_handler.h
 * @brief Test suite for the error handling system
 * @author Gabriel Avenia
 * @date May 2025
 * @defgroup error_tests Error Handling Tests
 * @brief Tests for error reporting and severity management
 * @{
 */

#ifndef TEST_ERROR_HANDLER_H
#define TEST_ERROR_HANDLER_H

#include <unity.h>
#include "../src/error/ErrorHandler.h"

/**
 * @brief Test error severity enum ordering
 * @details Verifies that the error severity levels are correctly
 *          ordered from least severe (INFO) to most severe (FATAL).
 */
void test_error_severity_enum() {
    // Test that the enums are ordered correctly by severity
    TEST_ASSERT_TRUE(INFO < WARNING);
    TEST_ASSERT_TRUE(WARNING < ERROR);
    TEST_ASSERT_TRUE(ERROR < FATAL);
}

/**
 * @brief Test error entry structure
 * @details Verifies that the ErrorEntry structure correctly
 *          stores the error severity, message, and timestamp.
 */
void test_error_entry() {
    // Create an error entry
    ErrorEntry entry;
    entry.severity = WARNING;
    entry.message = "Test warning message";
    entry.timestamp = 12345;
    
    // Test the values
    TEST_ASSERT_EQUAL(WARNING, entry.severity);
    TEST_ASSERT_EQUAL_STRING("Test warning message", entry.message.c_str());
    TEST_ASSERT_EQUAL(12345, entry.timestamp);
}

/**
 * @brief Test severity string conversion
 * @details Verifies that error severity levels are correctly
 *          converted to and from string representations.
 */
void test_error_severity_conversion() {
    // Test conversion to string
    TEST_ASSERT_EQUAL_STRING("INFO", ErrorHandler::severityToString(INFO).c_str());
    TEST_ASSERT_EQUAL_STRING("WARNING", ErrorHandler::severityToString(WARNING).c_str());
    TEST_ASSERT_EQUAL_STRING("ERROR", ErrorHandler::severityToString(ERROR).c_str());
    TEST_ASSERT_EQUAL_STRING("FATAL", ErrorHandler::severityToString(FATAL).c_str());
    
    // Test conversion from string
    TEST_ASSERT_EQUAL(INFO, ErrorHandler::stringToSeverity("INFO"));
    TEST_ASSERT_EQUAL(WARNING, ErrorHandler::stringToSeverity("WARNING"));
    TEST_ASSERT_EQUAL(ERROR, ErrorHandler::stringToSeverity("ERROR"));
    TEST_ASSERT_EQUAL(FATAL, ErrorHandler::stringToSeverity("FATAL"));
    
    // Test case insensitivity
    TEST_ASSERT_EQUAL(INFO, ErrorHandler::stringToSeverity("info"));
    TEST_ASSERT_EQUAL(WARNING, ErrorHandler::stringToSeverity("Warning"));
    
    // Test default for unknown strings
    TEST_ASSERT_EQUAL(INFO, ErrorHandler::stringToSeverity("UNKNOWN"));
}

/**
 * @brief Run all error handler tests
 */
void run_error_handler_tests() {
    RUN_TEST(test_error_severity_enum);
    RUN_TEST(test_error_entry);
    RUN_TEST(test_error_severity_conversion);
}

#endif // TEST_ERROR_HANDLER_H

/** @} */ // End of error_tests group
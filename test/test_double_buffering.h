/**
 * @file test_double_buffering.h
 * @brief Test suite for the thread-safe double buffering mechanism
 * @author Gabriel Avenia
 * @date May 2025
 * @defgroup double_buffer_tests Double Buffering Tests
 * @brief Tests for thread-safe data exchange between cores
 * @{
 */

#ifndef TEST_DOUBLE_BUFFERING_H
#define TEST_DOUBLE_BUFFERING_H

#include <unity.h>
#include <atomic>
#include <map>
#include <string>

/**
 * @brief Simplified cache structure for testing double buffering
 */
struct TestCache {
    float value;            ///< Test value
    unsigned long timestamp; ///< Timestamp for test value
    
    /**
     * @brief Default constructor
     */
    TestCache() : value(0.0), timestamp(0) {}
    
    /**
     * @brief Constructor with parameters
     * @param v The test value
     * @param t The timestamp
     */
    TestCache(float v, unsigned long t) : value(v), timestamp(t) {}
};

/**
 * @brief Simplified manager for testing double buffering
 * @details This test class mimics the SensorManager's double-buffering
 *          mechanism that allows thread-safe data exchange between
 *          the sensor task and communication task.
 */
class DoubleBufferingManager {
private:
    std::atomic<bool> currentBufferIndex{false}; ///< false = bufferA active, true = bufferB active
    std::map<std::string, TestCache> bufferA;    ///< First buffer for readings
    std::map<std::string, TestCache> bufferB;    ///< Second buffer for readings
    
public:
    /**
     * @brief Get the active buffer being written to
     * @return Reference to the active cache buffer
     */
    std::map<std::string, TestCache>& getActiveBuffer() {
        return currentBufferIndex.load() ? bufferB : bufferA;
    }
    
    /**
     * @brief Get the buffer for safe reading by other threads
     * @return Const reference to the read cache buffer
     */
    const std::map<std::string, TestCache>& getReadBuffer() const {
        return currentBufferIndex.load() ? bufferA : bufferB;
    }
    
    /**
     * @brief Atomically swap active and read buffers
     */
    void swapBuffers() {
        currentBufferIndex.store(!currentBufferIndex.load());
    }
    
    /**
     * @brief Write a value to the active buffer
     * @param key Identifier for the value
     * @param value The value to store
     * @param timestamp Timestamp for the value
     */
    void writeToActiveBuffer(const std::string& key, float value, unsigned long timestamp) {
        getActiveBuffer()[key] = TestCache(value, timestamp);
    }
    
    /**
     * @brief Read a value from the read buffer
     * @param key Identifier for the value to read
     * @param value Output parameter for the value
     * @param timestamp Output parameter for the timestamp
     * @return true if the key was found, false otherwise
     */
    bool readFromReadBuffer(const std::string& key, float& value, unsigned long& timestamp) const {
        const auto& readBuffer = getReadBuffer();
        auto it = readBuffer.find(key);
        if (it != readBuffer.end()) {
            value = it->second.value;
            timestamp = it->second.timestamp;
            return true;
        }
        return false;
    }
};

/**
 * @brief Test basic double buffering operation
 * @details Verifies the core functionality of the double buffering system:
 *          writing to active buffer, swapping buffers, and reading from read buffer.
 */
void test_double_buffering_basic_operation() {
    DoubleBufferingManager manager;
    
    // Write to active buffer
    manager.writeToActiveBuffer("sensor1", 25.5, 12345);
    
    // Initially, read buffer should be empty
    float value = 0.0;
    unsigned long timestamp = 0;
    TEST_ASSERT_FALSE(manager.readFromReadBuffer("sensor1", value, timestamp));
    
    // Swap buffers
    manager.swapBuffers();
    
    // Now we should be able to read the value
    TEST_ASSERT_TRUE(manager.readFromReadBuffer("sensor1", value, timestamp));
    TEST_ASSERT_EQUAL_FLOAT(25.5, value);
    TEST_ASSERT_EQUAL(12345, timestamp);
    
    // Write a new value to the active buffer (which is now the other buffer)
    manager.writeToActiveBuffer("sensor1", 30.0, 67890);
    
    // Read buffer should still have the old value
    TEST_ASSERT_TRUE(manager.readFromReadBuffer("sensor1", value, timestamp));
    TEST_ASSERT_EQUAL_FLOAT(25.5, value);
    TEST_ASSERT_EQUAL(12345, timestamp);
    
    // Swap again
    manager.swapBuffers();
    
    // Now read buffer should have the new value
    TEST_ASSERT_TRUE(manager.readFromReadBuffer("sensor1", value, timestamp));
    TEST_ASSERT_EQUAL_FLOAT(30.0, value);
    TEST_ASSERT_EQUAL(67890, timestamp);
}

/**
 * @brief Test multiple keys in the double buffer
 * @details Verifies the double buffering system can handle multiple
 *          different keys simultaneously.
 */
void test_double_buffering_multiple_keys() {
    DoubleBufferingManager manager;
    
    // Write multiple values
    manager.writeToActiveBuffer("temp1", 25.5, 12345);
    manager.writeToActiveBuffer("hum1", 60.0, 12346);
    manager.writeToActiveBuffer("temp2", 26.0, 12347);
    
    // Swap buffers
    manager.swapBuffers();
    
    // Read values
    float value1 = 0.0, value2 = 0.0, value3 = 0.0;
    unsigned long timestamp1 = 0, timestamp2 = 0, timestamp3 = 0;
    
    TEST_ASSERT_TRUE(manager.readFromReadBuffer("temp1", value1, timestamp1));
    TEST_ASSERT_TRUE(manager.readFromReadBuffer("hum1", value2, timestamp2));
    TEST_ASSERT_TRUE(manager.readFromReadBuffer("temp2", value3, timestamp3));
    
    TEST_ASSERT_EQUAL_FLOAT(25.5, value1);
    TEST_ASSERT_EQUAL_FLOAT(60.0, value2);
    TEST_ASSERT_EQUAL_FLOAT(26.0, value3);
    
    TEST_ASSERT_EQUAL(12345, timestamp1);
    TEST_ASSERT_EQUAL(12346, timestamp2);
    TEST_ASSERT_EQUAL(12347, timestamp3);
}

/**
 * @brief Test updating values in the double buffer
 * @details Verifies that updating values in the active buffer
 *          doesn't affect the read buffer until the next swap.
 */
void test_double_buffering_updates() {
    DoubleBufferingManager manager;
    
    // Write initial value
    manager.writeToActiveBuffer("sensor1", 25.5, 12345);
    manager.swapBuffers();
    
    // Update value in active buffer
    manager.writeToActiveBuffer("sensor1", 26.0, 12350);
    
    // Read buffer should still have old value
    float value = 0.0;
    unsigned long timestamp = 0;
    TEST_ASSERT_TRUE(manager.readFromReadBuffer("sensor1", value, timestamp));
    TEST_ASSERT_EQUAL_FLOAT(25.5, value);
    TEST_ASSERT_EQUAL(12345, timestamp);
    
    // Swap buffers
    manager.swapBuffers();
    
    // Read buffer should now have new value
    TEST_ASSERT_TRUE(manager.readFromReadBuffer("sensor1", value, timestamp));
    TEST_ASSERT_EQUAL_FLOAT(26.0, value);
    TEST_ASSERT_EQUAL(12350, timestamp);
}

/**
 * @brief Run all double buffering tests
 */
void run_double_buffering_tests() {
    RUN_TEST(test_double_buffering_basic_operation);
    RUN_TEST(test_double_buffering_multiple_keys);
    RUN_TEST(test_double_buffering_updates);
}

#endif // TEST_DOUBLE_BUFFERING_H

/** @} */ // End of double_buffer_tests group
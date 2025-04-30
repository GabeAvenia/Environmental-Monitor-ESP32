#pragma once

namespace Constants {
    // Product identification
    static const char* PRODUCT_NAME = "GPower Environmental Monitor";
    static const char* FIRMWARE_VERSION = "1.0.0";
    
    // File system paths
    static const char* CONFIG_FILE_PATH = "/config.json";
    
    // Configuration keys
    static const char* CONFIG_MONITOR_ID = "Environmental Monitor ID";
    static const char* CONFIG_I2C_SENSORS = "I2C Sensors";
    static const char* CONFIG_SPI_SENSORS = "SPI Sensors";
    
    // SCPI command tokens
    namespace SCPI {
        // System identification
        static const char* IDN = "*IDN?";
        
        // Measurement commands
        static const char* MEASURE_QUERY = "MEAS?";
        
        // System commands
        static const char* LIST_SENSORS = "SYST:SENS:LIST?";
        static const char* GET_CONFIG = "SYST:CONF?";
        static const char* SET_BOARD_ID = "SYST:CONF:BOARD:ID";
        static const char* UPDATE_CONFIG = "SYST:CONF:UPDATE";
        static const char* UPDATE_SENSOR_CONFIG = "SYST:CONF:SENS:UPDATE";
        static const char* UPDATE_ADDITIONAL_CONFIG = "SYST:CONF:ADD:UPDATE";
        
        // Message routing commands - simplified
        static const char* LOG_ROUTE = "SYST:LOG";        // Format: SYST:LOG <destination>,<severity>
        static const char* LOG_STATUS = "SYST:LOG?";      // Get current log routing settings
        
        // Test commands
        static const char* TEST_FILESYSTEM = "TEST:FS";
        static const char* TEST_UPDATE = "TEST:UPDATE";
        static const char* TEST = "TEST";
        static const char* ECHO = "ECHO";
        static const char* RESET = "RESET";
        static const char* TEST_INFO = "TEST:INFO";
        static const char* TEST_WARNING = "TEST:WARNING";
        static const char* TEST_ERROR = "TEST:ERROR";
        static const char* TEST_FATAL = "TEST:FATAL";
        
        // LED control commands
        static const char* LED_IDENTIFY = "SYST:LED:IDENT";
    }
    namespace System {
        static const uint32_t DEFAULT_POLLING_RATE_MS = 1000;
        static const uint32_t MIN_POLLING_RATE_MS = 50;
        static const uint32_t MAX_POLLING_RATE_MS = 300000; // 5 minutes
        
        static const size_t MAX_COMMAND_BUFFER_SIZE = 4096;
    }
    
    // Task management constants
    namespace Tasks {
        // Stack sizes (in words)
        static const uint32_t STACK_SIZE_SENSOR = 6144;
        static const uint32_t STACK_SIZE_COMM = 6144;
        static const uint32_t STACK_SIZE_LED = 3072;
        
        // Task priorities
        static const UBaseType_t PRIORITY_SENSOR = 3;
        static const UBaseType_t PRIORITY_COMM = 2;
        static const UBaseType_t PRIORITY_LED = 1;
        
        // Core assignments
        static const BaseType_t CORE_SENSOR = 0;
        static const BaseType_t CORE_COMM = 1;
        static const BaseType_t CORE_LED = 0;
    }
    
    // LED-related constants
    namespace LED {
        // Colors (WRGB format)
        static const uint32_t COLOR_OFF = 0x000000;
        static const uint32_t COLOR_GREEN = 0x00FF00;
        static const uint32_t COLOR_YELLOW = 0xFFFF00;
        static const uint32_t COLOR_BLUE = 0x0000FF;
        static const uint32_t COLOR_RED = 0xFF0000;
        static const uint32_t COLOR_ORANGE = 0xFF8000;
        
        // Brightness levels (0-255)
        static const uint8_t DIM_BRIGHTNESS = 30;
        static const uint8_t FULL_BRIGHTNESS = 40;
        
        // Timing constants
        static const unsigned long IDENTIFY_FLASH_RATE_MS = 250;
        static const unsigned long IDENTIFY_DURATION_MS = 5000;
        static const unsigned long WARNING_ERROR_DURATION_MS = 2000;
        static const unsigned long PULSE_DURATION_MS = 100;
    }
    
    // Sensor-related constants
    namespace Sensors {
        // Retry logic
        static const int MAX_RETRIES = 3;
        static const int RETRY_DELAY_MS = 50;
        static const int MAX_I2C_RECOVERY_ATTEMPTS = 5;
        
        // Caching
        static const unsigned long DEFAULT_MAX_CACHE_AGE_MS = 5000;
        static const unsigned long MIN_CACHE_AGE_MS = 50;
        
        // I2C specific
        static const uint32_t DEFAULT_I2C_CLOCK_FREQ = 100000;
        static const uint32_t I2C_RECOVERY_INTERVAL_MS = 5000;
    }
    
    // Communication-related constants
    namespace Communication {
        // Serial settings
        static const long DEFAULT_BAUD_RATE = 115200;
        
        // Buffer sizes
        static const size_t MAX_BUFFER_SIZE = 4096;
        static const size_t MAX_RESPONSE_SIZE = 1024;
        
        // Reading retry constants
        static const int MAX_READING_RETRIES = 4;
        static const int READING_RETRY_DELAY_MS = 5;
        static const int COMMAND_TIMEOUT_MS = 200;
    }
    namespace Pins {
        // LED pins
        constexpr int NEOPIXEL_DATA = 39;      // Data pin for NeoPixel
        #define NEOPIXEL_POWER 38      // Power pin for NeoPixel
        // I2C buses
        namespace I2C {
            // Default I2C bus (I2C0)
            static const int I2C0_SDA = 7;        // GPIO7
            static const int I2C0_SCL = 6;        // GPIO6
            
            // STEMMA QT / Qwiic bus (I2C1)
            static const int I2C1_SDA = 41;       // GPIO41
            static const int I2C1_SCL = 40;       // GPIO40
        }
        
        // SPI bus
        namespace SPI {
            static const int MOSI = 35;           // GPIO35
            static const int MISO = 37;           // GPIO37
            static const int SCK = 36;            // GPIO36
            
            // Logical to physical SS pin mapping
            static const int SS_A0 = 18;          // GPIO18
            static const int SS_A1 = 17;          // GPIO17
            static const int SS_A2 = 9;           // GPIO9
            static const int SS_A3 = 8;           // GPIO8
        }
        
        // Debug UART
        namespace UART {
            static const int TX = 5;              // GPIO5
            static const int RX = 16;             // GPIO16
        }
    }
}
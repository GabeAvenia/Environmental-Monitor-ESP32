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
        static const char* MEASURE = "MEAS";
        static const char* MEASURE_QUERY = "MEAS?";
        
        // System commands
        static const char* LIST_SENSORS = "SYST:SENS:LIST?";
        static const char* GET_CONFIG = "SYST:CONF?";
        static const char* SET_BOARD_ID = "SYST:CONF:BOARD:ID";
        static const char* UPDATE_CONFIG = "SYST:CONF:UPDATE";
        static const char* UPDATE_SENSOR_CONFIG = "SYST:CONF:SENS:UPDATE";
        static const char* UPDATE_ADDITIONAL_CONFIG = "SYST:CONF:ADD:UPDATE";
        
        // Message routing commands
        static const char* MSG_ROUTE_STATUS = "SYST:LOG:ROUTE?";
        static const char* MSG_ROUTE_SET = "SYST:LOG:ROUTE";
        static const char* MSG_ROUTE_INFO = "SYST:LOG:INFO:ROUTE";
        static const char* MSG_ROUTE_WARNING = "SYST:LOG:WARN:ROUTE";
        static const char* MSG_ROUTE_ERROR = "SYST:LOG:ERR:ROUTE";
        
        // Test commands
        static const char* TEST_FILESYSTEM = "TEST:FS";
        static const char* TEST_UPDATE = "TEST:UPDATE";
        static const char* TEST = "TEST";
        static const char* ECHO = "ECHO";
        static const char* RESET = "RESET";
        
        // LED control commands
        static const char* LED_IDENTIFY = "SYST:LED:IDENT";
    }
}
#pragma once

namespace Constants {
    // Alternative approach for constants that works in C++14
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
        static const char* IDN = "*IDN?";
        static const char* MEASURE_SINGLE = "MEAS:SINGLE";
        static const char* LIST_SENSORS = "SYST:SENS:LIST?";
        static const char* GET_CONFIG = "SYST:CONF?";
        static const char* SET_BOARD_ID = "SYST:CONF:BOARD:ID";
        static const char* STREAM_START = "STREAM:START";
        static const char* STREAM_STOP = "STREAM:STOP";
        static const char* STREAM_STATUS = "STREAM:STATUS?";

        static const char* VERBOSE_LOG = "SYST:LOG:VERB";
        static const char* UPDATE_CONFIG = "SYST:CONF:UPDATE";
    }
}
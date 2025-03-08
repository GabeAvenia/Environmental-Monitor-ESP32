#pragma once

namespace Constants {
    // Alternative approach for constants that works in C++14
    // Product identification
    static const char* PRODUCT_NAME = "GPower Environmental Monitor";
    static const char* FIRMWARE_VERSION = "1.0.0";
    
    // File system paths
    static const char* CONFIG_FILE_PATH = "/config.json";
    
    // SCPI command tokens
    namespace SCPI {
        static const char* IDN = "*IDN?";
        static const char* MEASURE_TEMP = "MEAS:TEMP?";
        static const char* MEASURE_HUM = "MEAS:HUM?";
        static const char* LIST_SENSORS = "SYST:SENS:LIST?";
        static const char* GET_CONFIG = "SYST:CONF?";
        static const char* SET_BOARD_ID = "SYST:CONF:BOARD:ID";
    }
}
#pragma once

/**
 * @brief Enumeration of supported sensor types.
 * 
 * This enum defines the sensor models that the system can work with.
 * When adding support for a new sensor model, add it here.
 */
enum class SensorType {
    UNKNOWN,
    SHT41,
    BME280,
    TMP117,
    SCD40,
    AHT20
};

/**
 * @brief Convert a string sensor type to the enum representation.
 * 
 * @param typeStr The string representation of the sensor type.
 * @return The corresponding SensorType enum value, or UNKNOWN if not recognized.
 */
inline SensorType sensorTypeFromString(const String& typeStr) {
    if (typeStr.equalsIgnoreCase("SHT41")) return SensorType::SHT41;
    if (typeStr.equalsIgnoreCase("BME280")) return SensorType::BME280;
    if (typeStr.equalsIgnoreCase("TMP117")) return SensorType::TMP117;
    if (typeStr.equalsIgnoreCase("SCD40")) return SensorType::SCD40;
    if (typeStr.equalsIgnoreCase("AHT20")) return SensorType::AHT20;
    
    return SensorType::UNKNOWN;
}

/**
 * @brief Convert a SensorType enum to its string representation.
 * 
 * @param type The SensorType enum value.
 * @return The string representation of the sensor type.
 */
inline String sensorTypeToString(SensorType type) {
    switch (type) {
        case SensorType::SHT41: return "SHT41";
        case SensorType::BME280: return "BME280";
        case SensorType::TMP117: return "TMP117";
        case SensorType::SCD40: return "SCD40";
        case SensorType::AHT20: return "AHT20";
        default: return "UNKNOWN";
    }
}

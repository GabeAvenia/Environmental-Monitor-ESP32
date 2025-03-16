#include "SensorRegistry.h"

SensorRegistry::SensorRegistry(ErrorHandler* err) : errorHandler(err) {
}

SensorRegistry::~SensorRegistry() {
    // Note: We don't own the sensors, so we don't delete them
    allSensors.clear();
}

void SensorRegistry::registerSensor(ISensor* sensor) {
    if (!sensor) {
        errorHandler->logError(ERROR, "Attempted to register null sensor");
        return;
    }
    
    // Check if sensor already exists
    if (hasSensor(sensor->getName())) {
        errorHandler->logWarning("Sensor with name " + sensor->getName() + " already exists in registry");
        return;
    }
    
    // Add to the general sensor list
    allSensors.push_back(sensor);
    
    errorHandler->logInfo("Registered sensor: " + sensor->getName());
}

ISensor* SensorRegistry::unregisterSensor(const String& sensorName) {
    ISensor* sensorToRemove = nullptr;
    
    // Find the sensor
    for (auto it = allSensors.begin(); it != allSensors.end(); ++it) {
        if ((*it)->getName() == sensorName) {
            sensorToRemove = *it;
            allSensors.erase(it);
            break;
        }
    }
    
    if (!sensorToRemove) {
        errorHandler->logWarning("Attempted to unregister non-existent sensor: " + sensorName);
        return nullptr;
    }
    
    errorHandler->logInfo("Unregistered sensor: " + sensorName);
    return sensorToRemove;
}

std::vector<ISensor*> SensorRegistry::clear() {
    std::vector<ISensor*> sensors = allSensors;
    allSensors.clear();
    
    errorHandler->logInfo("Cleared all sensors from registry");
    return sensors;
}

std::vector<ISensor*> SensorRegistry::getAllSensors() const {
    return allSensors;
}

std::vector<ITemperatureSensor*> SensorRegistry::getTemperatureSensors() const {
    std::vector<ITemperatureSensor*> result;
    
    for (auto sensor : allSensors) {
        ITemperatureSensor* tempSensor = getInterfaceFrom<ITemperatureSensor>(
            sensor, InterfaceType::TEMPERATURE);
        
        if (tempSensor) {
            result.push_back(tempSensor);
        }
    }
    
    return result;
}

std::vector<IHumiditySensor*> SensorRegistry::getHumiditySensors() const {
    std::vector<IHumiditySensor*> result;
    
    for (auto sensor : allSensors) {
        IHumiditySensor* humSensor = getInterfaceFrom<IHumiditySensor>(
            sensor, InterfaceType::HUMIDITY);
        
        if (humSensor) {
            result.push_back(humSensor);
        }
    }
    
    return result;
}

ISensor* SensorRegistry::getSensorByName(const String& name) const {
    for (auto sensor : allSensors) {
        if (sensor->getName() == name) {
            return sensor;
        }
    }
    return nullptr;
}

bool SensorRegistry::hasSensor(const String& name) const {
    return getSensorByName(name) != nullptr;
}

size_t SensorRegistry::count() const {
    return allSensors.size();
}
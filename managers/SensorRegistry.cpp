#include "SensorRegistry.h"

SensorRegistry::SensorRegistry(ErrorHandler* err) : errorHandler(err) {
}

SensorRegistry::~SensorRegistry() {
    // Note: We don't own the sensors, so we don't delete them
    allSensors.clear();
    temperatureSensors.clear();
    humiditySensors.clear();
    pressureSensors.clear();
    co2Sensors.clear();
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
    
    // Add to specialized lists based on interfaces
    ITemperatureSensor* tempSensor = dynamic_cast<ITemperatureSensor*>(sensor);
    if (tempSensor) {
        temperatureSensors.push_back(tempSensor);
    }
    
    IHumiditySensor* humSensor = dynamic_cast<IHumiditySensor*>(sensor);
    if (humSensor) {
        humiditySensors.push_back(humSensor);
    }
    
    IPressureSensor* presSensor = dynamic_cast<IPressureSensor*>(sensor);
    if (presSensor) {
        pressureSensors.push_back(presSensor);
    }
    
    ICO2Sensor* co2Sensor = dynamic_cast<ICO2Sensor*>(sensor);
    if (co2Sensor) {
        co2Sensors.push_back(co2Sensor);
    }
    
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
    
    // Remove from specialized lists
    removeFromVector(temperatureSensors, sensorToRemove);
    removeFromVector(humiditySensors, sensorToRemove);
    removeFromVector(pressureSensors, sensorToRemove);
    removeFromVector(co2Sensors, sensorToRemove);
    
    errorHandler->logInfo("Unregistered sensor: " + sensorName);
    return sensorToRemove;
}

std::vector<ISensor*> SensorRegistry::clear() {
    std::vector<ISensor*> sensors = allSensors;
    
    allSensors.clear();
    temperatureSensors.clear();
    humiditySensors.clear();
    pressureSensors.clear();
    co2Sensors.clear();
    
    errorHandler->logInfo("Cleared all sensors from registry");
    return sensors;
}

std::vector<ISensor*> SensorRegistry::getAllSensors() const {
    return allSensors;
}

std::vector<ITemperatureSensor*> SensorRegistry::getTemperatureSensors() const {
    return temperatureSensors;
}

std::vector<IHumiditySensor*> SensorRegistry::getHumiditySensors() const {
    return humiditySensors;
}

std::vector<IPressureSensor*> SensorRegistry::getPressureSensors() const {
    return pressureSensors;
}

std::vector<ICO2Sensor*> SensorRegistry::getCO2Sensors() const {
    return co2Sensors;
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

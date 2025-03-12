#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <vector>
#include "../error/ErrorHandler.h"

class I2CManager {
private:
    TwoWire* wire;
    ErrorHandler* errorHandler;
    
public:
    I2CManager(ErrorHandler* err);
    ~I2CManager();
    
    bool begin(int sda = 41, int scl = 40);
    bool scanBus(std::vector<int>& addresses);
    TwoWire* getWire();
};
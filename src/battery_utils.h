#ifndef BATTERY_UTILS_H_
#define BATTERY_UTILS_H_

#include <Arduino.h>


namespace BATTERY_Utils {

    float checkBattery();
    float checkExternalVoltage();
    bool checkIfShouldSleep();

}

#endif
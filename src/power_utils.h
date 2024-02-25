#ifndef POWER_UTILS_H_
#define POWER_UTILS_H_

#include <Arduino.h>
#include "XPowersLib.h"


namespace POWER_Utils {

    void activateMeasurement();
    void activateLoRa();
    void deactivateLoRa();
    bool begin(TwoWire &port);
    void setup();
    //void lowerCpuFrequency();

}

#endif

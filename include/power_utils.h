#ifndef POWER_UTILS_H_
#define POWER_UTILS_H_

#include <Arduino.h>
#if defined(HAS_AXP192) || defined(HAS_AXP2101)
    #include "XPowersLib.h"
#else
    #include <Wire.h>
#endif


namespace POWER_Utils {

    double  getBatteryVoltage();
    bool    isBatteryConnected();
    void    activateMeasurement();
    void    activateGPS();
    void    deactivateGPS();
    void    activateLoRa();
    void    deactivateLoRa();
    bool    begin(TwoWire &port);
    void    setup();

}

#endif
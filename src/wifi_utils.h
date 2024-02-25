#ifndef WIFI_UTILS_H_
#define WIFI_UTILS_H_

#include <Arduino.h>

namespace WIFI_Utils {

    void checkWiFi();
    void startWiFi();
    void checkIfAutoAPShouldPowerOff();
    void setup();
    

}

#endif
#ifndef WIFI_UTILS_H_
#define WIFI_UTILS_H_

#include <Arduino.h>


namespace WIFI_Utils {

    void checkWiFi();
    void startAutoAP();
    void startWiFi();
    void checkAutoAPTimeout();
    void setup();
    
}

#endif
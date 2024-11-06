#ifndef A7670_UTILS_H_
#define A7670_UTILS_H_

#include <Arduino.h>


namespace A7670_Utils {

    bool checkModemOn();
    void setup();
    bool checkATResponse(const String& ATMessage);
    void APRS_IS_connect();
    void uploadToAPRSIS(const String& packet);
    void listenAPRSIS();

}

#endif
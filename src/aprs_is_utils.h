#ifndef APRS_IS_UTILS_H_
#define APRS_IS_UTILS_H_

#include <Arduino.h>

//#define PinPointApp //uncomment this line when using PinPoint App ( https://www.pinpointaprs.com )

namespace APRS_IS_Utils {

    void connect();
    void checkStatus();
    String createPacket(String unprocessedPacket);
    void processLoRaPacket(String packet);
    void processAPRSISPacket(String packet);
    void loop();

}

#endif
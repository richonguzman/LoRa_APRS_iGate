#ifndef APRS_IS_UTILS_H_
#define APRS_IS_UTILS_H_

#include <Arduino.h>


//#define TextSerialOutputForApp
/*  uncomment the previous line to get text from Serial-Output over USB into PC for:
    - PinPoint App ( https://www.pinpointaprs.com )
    - APRSIS32 App ( http://aprsisce.wikidot.com )
*/


namespace APRS_IS_Utils {

    void upload(String line);
    void connect();
    void checkStatus();
    String createPacket(String unprocessedPacket);
    void processLoRaPacket(String packet);
    void processAPRSISPacket(String packet);
    void loop(String packet);

}

#endif
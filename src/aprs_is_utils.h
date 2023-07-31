#ifndef APRS_IS_UTILS_H_
#define APRS_IS_UTILS_H_

#include <Arduino.h>

namespace APRS_IS_Utils {

void connect();
void checkStatus();
String createPacket(String unprocessedPacket);
void processLoRaPacket(String packet);
void processAPRSISPacket(String packet);
void loop();

}

#endif
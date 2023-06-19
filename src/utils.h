#ifndef UTILS_H_
#define UTILS_H_

#include <Arduino.h>

namespace Utils {

void processStatus();
String getLocalIP();
void setupDiplay();
void activeStations();
void checkBeaconInterval();
void checkDisplayInterval();
void validateDigiFreqs();
void typeOfPacket(String packet, String packetType);
void startOTAServer();

}

#endif
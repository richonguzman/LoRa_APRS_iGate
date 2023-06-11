#ifndef UTILS_H_
#define UTILS_H_

#include <Arduino.h>

namespace utils {

void processStatus();
void setupDiplay();
String getLocalIP();
void checkBeaconInterval();
void checkDisplayInterval();
void validateDigiFreqs();
void typeOfPacket(String packet);
void startOTAServer();

}

#endif
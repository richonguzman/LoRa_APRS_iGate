#ifndef UTILS_H_
#define UTILS_H_

#include <Arduino.h>

namespace Utils {

    void processStatus();
    String getLocalIP();
    void setupDisplay();
    void activeStations();
    void checkBeaconInterval();
    void checkDisplayInterval();
    void checkWiFiInterval();
    void validateDigiFreqs();
    void typeOfPacket(String packet, String packetType);
    void onOTAStart();
    void onOTAProgress(size_t current, size_t final);
    void onOTAEnd(bool success);

}

#endif
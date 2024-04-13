#ifndef UTILS_H_
#define UTILS_H_

#include <Arduino.h>

class ReceivedPacket {
public:
    long    millis;
    String  packet;
    int     RSSI;
    float     SNR;
};

namespace Utils {

    void processStatus();
    String getLocalIP();
    void setupDisplay();
    void activeStations();
    void checkBeaconInterval();
    void checkDisplayInterval();
    void checkWiFiInterval();
    void validateFreqs();
    void typeOfPacket(String packet, String packetType);
    void print(String text);
    void println(String text);

}

#endif
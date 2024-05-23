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
    void validateFreqs();
    void typeOfPacket(const String& packet, uint8_t packetType);
    void print(const String& text);
    void println(const String& text);
    void checkRebootMode();
    void checkRebootTime();

}

#endif
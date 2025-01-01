#ifndef STATION_UTILS_H_
#define STATION_UTILS_H_


#include <Arduino.h>


struct Packet25SegBuffer {
    uint32_t    receivedTime;
    String      station;
    String      payload;
};

struct LastHeardStation {
    uint32_t    lastHeardTime;
    String      station;
};

namespace STATION_Utils {

    void loadBlackList();
    bool checkBlackList(const String& callsign);
    void deleteNotHeard();
    void updateLastHeard(const String& station);
    bool wasHeard(const String& station);
    void clean25SegBuffer();
    bool check25SegBuffer(const String& station, const String& textMessage);
    void processOutputPacketBuffer();
    void addToOutputPacketBuffer(const String& packet);

}

#endif
#ifndef STATION_UTILS_H_
#define STATION_UTILS_H_

#include <Arduino.h>


namespace STATION_Utils {

    void deleteNotHeard();
    void updateLastHeard(const String& station);
    bool wasHeard(const String& station);
    void clean25SegBuffer();
    bool check25SegBuffer(const String& station, const String& textMessage);
    void processOutputPacketBuffer();
    void addToOutputPacketBuffer(const String& packet);

}

#endif
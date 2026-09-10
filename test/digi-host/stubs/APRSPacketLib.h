#pragma once

#include <Arduino.h>

struct APRSPacket {
    String header;
    String sender;
    String path;
    int type = 0;
    String addressee;
    String payload;
};

class APRSPacketLib {
public:
    static String checkForStartingBytes(const String& packet) { return packet; }
};

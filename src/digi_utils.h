#ifndef DIGI_UTILS_H_
#define DIGI_UTILS_H_

#include <Arduino.h>


namespace DIGI_Utils {

    String generateDigiRepeatedPacket(const String& packet);
    void processLoRaPacket(const String& packet);

}

#endif
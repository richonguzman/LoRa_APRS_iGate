#ifndef DIGI_UTILS_H_
#define DIGI_UTILS_H_

#include <Arduino.h>


namespace DIGI_Utils {

    String  buildPacket(const String& path, const String& packet, bool thirdParty);
    String  generateDigiRepeatedPacket(const String& packet, bool thirdParty);
    void    processLoRaPacket(const String& packet);

}

#endif
#pragma once

#include <Arduino.h>
#include <APRSPacketLib.h>

namespace Utils {
bool callsignIsValid(const String& callsign);
void updateLoRaPacketDisplayInfo(const APRSPacket& packet, uint8_t type);
}

#pragma once

#include <Arduino.h>

namespace STATION_Utils {
bool isIn25SegHashBuffer(const String& station, const String& textMessage);
void updateLastHeard(const String& station);
void addToOutputPacketBuffer(const String& packet, bool flag = false);
}

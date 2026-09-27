#pragma once

#include <Arduino.h>

namespace APRS_IS_Utils {
String checkForStartingBytes(const String& packet);
bool processReceivedLoRaMessage(const String& sender,
                                const String& addresseeAndMessage);
}

#pragma once
#include <Arduino.h>

// Builds the iGate position beacon (fixed lat/lon from Config.beacon), using
// APRSPacketLib's Base91-compressed position — same format as the ESP32 build.
namespace Beacon {
    String buildAprsisLine();   // CALL>APLRG1,PATH,qAC:=<overlay><base91><comment>
    String buildRfLine();       // CALL>APLRG1,PATH:=<overlay><base91><comment>
}

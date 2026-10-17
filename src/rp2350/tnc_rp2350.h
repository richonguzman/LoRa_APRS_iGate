#pragma once
#include <Arduino.h>

// KISS TNC server for the RP2350 iGate (TCP :8001 over the W5500). Lets external
// APRS clients (APRSdroid, Xastir, YAAC, ...) use the iGate's LoRa radio:
//   client -> KISS frame -> decode -> RF TX (and optional APRS-IS bridge)
//   RF RX  -> KISS-encode -> all connected clients
// Reuses the validated KISS codec (src/kiss_utils.cpp). All calls run from
// netTask (the W5500 owner); RF TX is handed to loraTask via enqueueRfFrame().
namespace Tnc {
    void setup();                          // start the server if Config.tnc.enableServer
    void poll();                           // accept clients + read their KISS frames
    void broadcast(const String& tnc2frame); // a received RF frame -> KISS clients
}

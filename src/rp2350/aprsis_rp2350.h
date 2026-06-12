#pragma once
#include <Arduino.h>

// Lean APRS-IS uplink for the RP2350 iGate core path (RX LoRa -> APRS-IS),
// over the W5500 (EthernetClient). The full feature set (queries, digi, TNC,
// station tracking) of the original aprs_is_utils.cpp is deferred. All calls
// touch the W5500/SPI0, so run them from the same task that owns Ethernet.
namespace AprsIs {
    bool connect();                       // TCP connect + APRS-IS login
    bool connected();
    void poll();                          // drain incoming server lines (keepalive)
    void send(const String &line);        // send a raw APRS-IS line (e.g. beacon)
    void forward(const String &loraPacket); // RX LoRa packet -> APRS-IS (qAR)
}

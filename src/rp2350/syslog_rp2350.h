#pragma once
#include <Arduino.h>

// Remote syslog (RFC5424 over UDP) for the RP2350 iGate. Replaces the ESP32
// syslog_utils (WiFiUDP) with EthernetUDP. The server is resolved ONCE at setup
// (literal IP or a single DNS lookup) and cached, so logging never does per-call
// DNS — that blocking-per-LOG behaviour is what reboot-looped the W5500 boards.
// netTask only (W5500 owner); fire-and-forget UDP.
namespace Syslog {
    void setup();                                          // resolve server once + begin UDP
    void logRx(const String &frame, int rssi, float snr);  // a received RF packet
    void logTx(const String &info);                        // a TX / APRS-IS event line
}

#pragma once
#include <Arduino.h>

// Lean SNTP client over the W5500 (EthernetUDP) — replaces the ESP32 ntp_utils
// (NTPClient + WiFiUDP). Non-blocking: poll() fires a request and picks up the
// reply on a later call. Time is local (Config.ntp.gmtCorrection applied). Used
// for real timestamps in the web views / heartbeat. netTask only (W5500 owner).
namespace Ntp {
    void     poll();                 // sync periodically (~15 min); call each net loop
    bool     synced();               // true once a reply has been received
    uint32_t nowEpoch();             // current LOCAL unix epoch, 0 if not synced
    String   hms(uint32_t epoch);    // "HH:MM:SS" from a local epoch
}

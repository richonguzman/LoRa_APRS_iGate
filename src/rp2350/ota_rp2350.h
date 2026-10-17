#pragma once
#include <Arduino.h>
#include <Ethernet.h>

// Network firmware update for the RP2350 iGate. Replaces the ESP32 ota_utils
// (ElegantOTA + ESPAsyncWebServer) with the arduino-pico Updater: the raw
// firmware.bin is streamed into flash and staged for the bootloader, which
// applies it on the next reboot. Served at POST /update over the W5500
// (netTask). HTTP Basic auth against Config.ota.username/password (when set).
//
//   curl -u user:pass --data-binary @firmware.bin http://<ip>/update
namespace Ota {
    void handleUpdate(EthernetClient &c, const String &authHeader, long contentLength);
}

#pragma once
#include <Arduino.h>

// GPS reader for the RP2350 iGate port: hardware UART (UART1 / Serial2) parsed
// with TinyGPS++. Compiles to no-ops unless built with -D HAS_GPS. When
// Config.beacon.gpsActive is set and a valid fix is present, the beacon uses the
// live GPS position (see beacon_rp2350.cpp) instead of the fixed config position.
namespace Gps {
    void     setup();          // begin the UART (HAS_GPS only); call from setup()
    void     poll();           // feed available UART bytes to TinyGPS++; call each net loop
    bool     hasFix();         // true when a valid, non-zero position fix is available
    double   lat();            // last valid latitude  (0 if no fix)
    double   lng();            // last valid longitude (0 if no fix)
    double   altMeters();      // last valid altitude in metres (NAN if unknown)
    uint32_t satellites();     // satellites in use (0 if unknown)
}

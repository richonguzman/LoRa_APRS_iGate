#pragma once
#include <Arduino.h>

// Supply-voltage monitor for the RP2350 iGate. Reads VSYS via the on-board 3:1
// divider on GP29/ADC3 (Pico2 / WIZnet W5500-EVB-Pico2). Reported in the beacon
// (Batt=) and/or as APRS telemetry, per Config.battery. No sleep handling — this
// is a mains/Ethernet iGate.
namespace Battery {
    void  setup();    // ADC resolution
    float vsys();     // supply voltage in volts (0 if unavailable)
}

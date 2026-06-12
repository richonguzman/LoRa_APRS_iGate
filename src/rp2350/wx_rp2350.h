#pragma once
#include <Arduino.h>

// Lean weather module for the RP2350 iGate: a single BMP280 on I2C0 (Wire),
// producing the APRS weather field appended to the beacon. BMP280 gives
// temperature + barometric pressure (no humidity, so the 'h' field is "..").
namespace Wx {
    // Init the BMP280 on Wire (PIN_WX_SDA/PIN_WX_SCL). Safe to call when the
    // sensor is absent — readAprs() then returns "". Call from setup() only.
    void setup();

    // True once a BMP280 answered on the bus at setup().
    bool present();

    // APRS weather payload: ".../...g...t<TTT>h..b<PPPPP>"  (temp degF, pres 0.1 hPa).
    // Returns "" if no sensor was detected. Call from netTask only (single Wire owner).
    String readAprs();
}

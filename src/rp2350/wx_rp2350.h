#pragma once
#include <Arduino.h>

// Lean weather module for the RP2350 iGate on I2C0 (Wire). Auto-detects a
// BMP280 (0x76/0x77 -> temperature + barometric pressure) and/or an SHT40
// (0x44 -> temperature + relative humidity), and builds the APRS weather field
// from whatever is present.
namespace Wx {
    // Probe the BMP280 and SHT40 on Wire (PIN_WX_SDA/PIN_WX_SCL). Safe to call
    // when no sensor is present — readAprs() then returns "". Call from setup().
    void setup();

    // True once a BMP280 or SHT40 answered on the bus at setup().
    bool present();

    // APRS weather payload: ".../...g...t<TTT>[h<HH>][b<PPPPP>]" (temp degF,
    // humidity %, pressure 0.1 hPa; absent fields omitted). Returns "" if no
    // sensor was detected. Call from netTask only (single Wire owner).
    String readAprs();
}

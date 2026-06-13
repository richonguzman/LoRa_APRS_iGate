#pragma once
#include <Arduino.h>

// APRS structured telemetry for the RP2350 iGate. Reports operational metrics
// (not battery — the upstream's channels — which this board doesn't have):
//   A1 RxPkts  RF packets received since last report
//   A2 Heard   stations heard (rememberStationTime window)
//   A3 RSSI    last RX signal (dBm)
//   A4 SNR     last RX signal-to-noise (dB)
// The Base91-compressed data goes appended to the APRS-IS position beacon; the
// EQNS./UNIT./PARM. definition messages (addressed to self) go out at boot and
// then ~daily so viewers (aprs.fi) can label/scale the channels. netTask only.
namespace Telemetry {
    String compressed();        // "|<seq><A1><A2><A3><A4>|" Base91; advances the sequence
    bool   dueDefinitions();    // true at boot and every ~24 h
    void   sendDefinitions();   // AprsIs::send the EQNS./UNIT./PARM. messages
}

#pragma once
#include <Arduino.h>

// APRS structured telemetry for the RP2350 iGate, classic "T#" data packets
// (universally parsed by aprs.fi; the Base91 "|...|"-in-comment form did not
// register there). Operational channels (no battery — this board has none):
//   A1 RxPkts  RF packets since last report   A2 Heard  stations heard
//   A3 RSSI    last RX dBm                     A4 SNR    last RX dB   (A5 spare)
// Sent as its own packet alongside the APRS-IS beacon; EQNS./UNIT./PARM. define
// the channels (sent to self at boot + ~daily). netTask only.
namespace Telemetry {
    String dataPacket();        // "CALL>APLRG1,TCPIP,qAC:T#nnn,a1..a5,bits"; advances seq
    bool   dueDefinitions();    // true at boot and every ~24 h
    void   sendDefinitions();   // AprsIs::send the EQNS./UNIT./PARM. messages
}

/*
 * RP2350 APRS structured telemetry. Operational channels (RxPkts/Heard/RSSI/SNR)
 * encoded Base91 and appended to the APRS-IS beacon, plus EQNS/UNIT/PARM
 * definition messages addressed to self. netTask only (reads the APRS-IS socket
 * via AprsIs and the netTask-owned counters).
 */
#include "telemetry_rp2350.h"
#include "configuration.h"
#include "station_rp2350.h"
#include "aprsis_rp2350.h"

extern Configuration Config;
extern int   rssi;          // last RX signal stats (lora_utils_rp2350.cpp)
extern float snr;

namespace {

uint16_t sequence = 0;
uint32_t lastDefs = 0;      // millis() of last EQNS/UNIT/PARM (0 = never)

// Base91 2-char encoding of a value clamped to [0, 8280] (= 91*91 - 1).
String b91(int raw) {
    if (raw < 0) raw = 0;
    if (raw > 8280) raw = 8280;
    String s;
    s += char(raw / 91 + 33);
    s += char(raw % 91 + 33);
    return s;
}

// the self-addressed message envelope used for EQNS/UNIT/PARM (TCPIP*, qAC)
String defEnvelope(const String &call, const String &payload) {
    String s = call;
    s += ">APLRG1,TCPIP,qAC::";
    String padded = call;
    for (int i = call.length(); i < 9; i++) padded += ' ';
    s += padded;
    s += ':';
    s += payload;
    return s;
}

}  // namespace

namespace Telemetry {

String compressed() {
    int rxPkts = (int)Station::takeRxCount();
    int heard  = (int)Station::activeCount();
    String t = "|";
    t += b91(sequence);
    t += b91(rxPkts);                 // A1 EQNS 0,1,0
    t += b91(heard);                  // A2 EQNS 0,1,0
    t += b91(rssi + 150);             // A3 EQNS 0,1,-150  (dBm)
    t += b91((int)(snr * 10) + 200);  // A4 EQNS 0,0.1,-20 (dB)
    t += "|";
    sequence++;
    if (sequence >= 8281) sequence = 0;
    return t;
}

bool dueDefinitions() {
    return lastDefs == 0 || (millis() - lastDefs) > 24UL * 60UL * 60UL * 1000UL;
}

void sendDefinitions() {
    String call = Config.tacticalCallsign.length() ? Config.tacticalCallsign : Config.callsign;
    AprsIs::send(defEnvelope(call, "PARM.RxPkts,Heard,RSSI,SNR"));
    AprsIs::send(defEnvelope(call, "UNIT.Pkt,Stn,dBm,dB"));
    AprsIs::send(defEnvelope(call, "EQNS.0,1,0,0,1,0,0,1,-150,0,0.1,-20"));
    lastDefs = millis() ? millis() : 1;
    Serial.println("[telemetry] sent EQNS/UNIT/PARM definitions");
}

}  // namespace Telemetry

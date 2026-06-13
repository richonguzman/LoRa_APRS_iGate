/*
 * RP2350 APRS structured telemetry — classic "T#seq,a1..a5,bits" data packets
 * plus EQNS/UNIT/PARM definitions addressed to self. Operational channels
 * (RxPkts/Heard/RSSI/SNR). netTask only (reads netTask-owned counters + the
 * APRS-IS socket via AprsIs). Values are 3-digit 0..255 with the EQNS below
 * mapping them back to engineering units.
 */
#include "telemetry_rp2350.h"
#include "configuration.h"
#include "station_rp2350.h"
#include "aprsis_rp2350.h"

extern Configuration Config;
extern int   rssi;          // last RX signal stats (lora_utils_rp2350.cpp)
extern float snr;

namespace {

uint16_t sequence = 0;      // 000..999
uint32_t lastDefs = 0;      // millis() of last EQNS/UNIT/PARM (0 = never)

int clamp255(long v) { return v < 0 ? 0 : (v > 255 ? 255 : (int)v); }

String pad3(int v) {
    char b[4];
    snprintf(b, sizeof(b), "%03d", clamp255(v));
    return String(b);
}

String selfCall() {
    return Config.tacticalCallsign.length() ? Config.tacticalCallsign : Config.callsign;
}

// self-addressed message envelope for EQNS/UNIT/PARM (TCPIP, qAC)
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

String dataPacket() {
    int a1 = clamp255(Station::takeRxCount());          // RxPkts   EQNS 0,1,0
    int a2 = clamp255(Station::activeCount());           // Heard    EQNS 0,1,0
    int a3 = clamp255(rssi + 150);                       // RSSI     EQNS 0,1,-150  -> dBm
    int a4 = clamp255((long)((snr + 20.0f) * 5.0f));     // SNR      EQNS 0,0.2,-20 -> dB
    int a5 = 0;                                           // spare

    char seq[4];
    snprintf(seq, sizeof(seq), "%03u", sequence);
    sequence = (sequence + 1) % 1000;

    String line = selfCall();
    line += ">APLRG1,TCPIP,qAC:T#";
    line += seq;  line += ',';
    line += pad3(a1); line += ',';
    line += pad3(a2); line += ',';
    line += pad3(a3); line += ',';
    line += pad3(a4); line += ',';
    line += pad3(a5); line += ",00000000";
    return line;
}

bool dueDefinitions() {
    return lastDefs == 0 || (millis() - lastDefs) > 24UL * 60UL * 60UL * 1000UL;
}

void sendDefinitions() {
    String call = selfCall();
    AprsIs::send(defEnvelope(call, "PARM.RxPkts,Heard,RSSI,SNR,Aux"));
    AprsIs::send(defEnvelope(call, "UNIT.Pkt,Stn,dBm,dB,-"));
    AprsIs::send(defEnvelope(call, "EQNS.0,1,0,0,1,0,0,1,-150,0,0.2,-20,0,0,0"));
    lastDefs = millis() ? millis() : 1;
    Serial.println("[telemetry] sent EQNS/UNIT/PARM definitions");
}

}  // namespace Telemetry

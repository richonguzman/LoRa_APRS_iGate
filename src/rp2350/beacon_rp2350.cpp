#include "beacon_rp2350.h"
#include <APRSPacketLib.h>
#include "configuration.h"
#include "wx_rp2350.h"
#include "battery_rp2350.h"
#include "gps_rp2350.h"

extern Configuration Config;

namespace Beacon {

// " Batt=X.XXV" appended to the comment when supply-voltage reporting goes in the
// beacon (not as telemetry). Empty otherwise.
static String battField() {
    if (Config.battery.sendInternalVoltage && !Config.battery.sendVoltageAsTelemetry) {
        char b[12];
        snprintf(b, sizeof(b), " Batt=%.2fV", (double)Battery::vsys());
        return String(b);
    }
    return "";
}

// Base91-compressed position payload: =<overlay><base91>  (mirrors gps_utils.cpp).
// Uses the live GPS fix when GPS is active and locked, else the fixed config
// coordinates (a stationary iGate keeps its saved position until a fix arrives).
static String encodedPosition() {
    double lat = Config.beacon.latitude;
    double lng = Config.beacon.longitude;
#ifdef HAS_GPS
    if (Config.beacon.gpsActive && Gps::hasFix()) {
        lat = Gps::lat();
        lng = Gps::lng();
    }
#endif
    return APRSPacketLib::encodeGPSIntoBase91(lat, lng, 0, 0, Config.beacon.symbol,
                                              false, 0, true, Config.beacon.ambiguityLevel);
}

// Weather field appended after the position when a sensor is active (matches
// utils.cpp: position + wxData + comment).
static String wxField() {
    if (!Config.wxsensor.active) return "";
    String wx = Wx::readAprs();
    return wx.length() ? wx : ".../...g...t...";
}

String buildAprsisLine() {
    String line = APRSPacketLib::generateBasePacket(Config.callsign, "APLRG1", Config.beacon.path);
    line += ",qAC:=";                 // q-construct for APRS-IS-injected, position w/ messaging
    line += Config.beacon.overlay;
    line += encodedPosition();
    line += wxField();
    line += Config.beacon.comment;
    line += battField();
    return line;
}

String buildRfLine() {
    // Built from loraTask — must NOT touch the BMP280 (Wire/I2C0 is owned by
    // netTask via buildAprsisLine), so no WX field here. Position + comment only.
    String line = APRSPacketLib::generateBasePacket(Config.callsign, "APLRG1", Config.beacon.path);
    line += ":=";
    line += Config.beacon.overlay;
    line += encodedPosition();
    line += Config.beacon.comment;
    line += battField();
    return line;
}

}  // namespace Beacon

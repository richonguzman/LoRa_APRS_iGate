#include "beacon_rp2350.h"
#include <APRSPacketLib.h>
#include "configuration.h"
#include "wx_rp2350.h"

extern Configuration Config;

namespace Beacon {

// Base91-compressed position payload: =<overlay><base91>  (mirrors gps_utils.cpp)
static String encodedPosition() {
    return APRSPacketLib::encodeGPSIntoBase91(Config.beacon.latitude, Config.beacon.longitude,
                                              0, 0, Config.beacon.symbol, false, 0, true,
                                              Config.beacon.ambiguityLevel);
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
    return line;
}

}  // namespace Beacon

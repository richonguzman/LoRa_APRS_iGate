#include "beacon_rp2350.h"
#include <APRSPacketLib.h>
#include "configuration.h"

extern Configuration Config;

namespace Beacon {

// Base91-compressed position payload: =<overlay><base91>  (mirrors gps_utils.cpp)
static String encodedPosition() {
    return APRSPacketLib::encodeGPSIntoBase91(Config.beacon.latitude, Config.beacon.longitude,
                                              0, 0, Config.beacon.symbol, false, 0, true,
                                              Config.beacon.ambiguityLevel);
}

String buildAprsisLine() {
    String line = APRSPacketLib::generateBasePacket(Config.callsign, "APLRG1", Config.beacon.path);
    line += ",qAC:=";                 // q-construct for APRS-IS-injected, position w/ messaging
    line += Config.beacon.overlay;
    line += encodedPosition();
    line += Config.beacon.comment;
    return line;
}

String buildRfLine() {
    String line = APRSPacketLib::generateBasePacket(Config.callsign, "APLRG1", Config.beacon.path);
    line += ":=";
    line += Config.beacon.overlay;
    line += encodedPosition();
    line += Config.beacon.comment;
    return line;
}

}  // namespace Beacon

/*
 * RP2350 APRS message originator. Builds a standard message packet addressed to
 * another station. Pure (read-only on Config); the caller enqueues the result
 * into the Station output buffer from loraTask.
 */
#include "message_rp2350.h"
#include "configuration.h"

extern Configuration Config;

namespace Message {

String buildRF(const String& to, const String& text) {
    if (to.length() == 0 || text.length() == 0) return "";
    String pkt = Config.callsign;
    pkt += ">APLRG1";
    if (Config.beacon.path.length()) { pkt += ','; pkt += Config.beacon.path; }
    pkt += "::";
    String padded = to;
    for (int i = to.length(); i < 9; i++) padded += ' ';   // APRS addressee is 9 chars
    pkt += padded;
    pkt += ':';
    pkt += text;
    return pkt;
}

}  // namespace Message

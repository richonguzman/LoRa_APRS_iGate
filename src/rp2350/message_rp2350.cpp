/*
 * RP2350 APRS message originator. Builds a standard message packet addressed to
 * another station. Pure (read-only on Config); the caller enqueues the result
 * into the Station output buffer from loraTask.
 */
#include "message_rp2350.h"
#include "configuration.h"

extern Configuration Config;

namespace Message {

static String envelope(const String& tocallPath, const String& to, const String& text) {
    String pkt = Config.callsign;
    pkt += ">APLRG1";
    pkt += tocallPath;                                      // ",PATH" (RF) or ",TCPIP*" (APRS-IS)
    pkt += "::";
    String padded = to;
    for (int i = to.length(); i < 9; i++) padded += ' ';   // APRS addressee is 9 chars
    pkt += padded;
    pkt += ':';
    pkt += text;
    return pkt;
}

String buildRF(const String& to, const String& text) {
    if (to.length() == 0 || text.length() == 0) return "";
    String path = Config.beacon.path.length() ? ("," + Config.beacon.path) : "";
    return envelope(path, to, text);
}

String buildAprsis(const String& to, const String& text) {
    if (to.length() == 0 || text.length() == 0) return "";
    return envelope(",TCPIP*", to, text);
}

}  // namespace Message

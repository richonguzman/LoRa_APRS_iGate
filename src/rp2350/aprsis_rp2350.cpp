#include "aprsis_rp2350.h"
#include <Ethernet.h>
#include "configuration.h"

extern Configuration Config;

static EthernetClient aprsClient;
static const char *IGATE_VERS = "RP2350iGate 0.1";

namespace AprsIs {

bool connected() { return aprsClient.connected(); }

bool connect() {
    if (aprsClient.connected()) return true;
    aprsClient.stop();
    Serial.printf("[aprsis] connecting %s:%d ...\n",
                  Config.aprs_is.server.c_str(), Config.aprs_is.port);
    if (!aprsClient.connect(Config.aprs_is.server.c_str(), Config.aprs_is.port)) {
        Serial.println("[aprsis] connect FAILED");
        return false;
    }
    // login line: "user CALL pass PASSCODE vers SW VER [filter F]"
    String login = "user ";
    login += Config.callsign;
    login += " pass ";
    login += Config.aprs_is.passcode;
    login += " vers ";
    login += IGATE_VERS;
    if (Config.aprs_is.filter.length()) {
        login += " filter ";
        login += Config.aprs_is.filter;
    }
    aprsClient.print(login + "\r\n");
    Serial.println("[aprsis] connected, logged in as " + Config.callsign);
    return true;
}

void poll() {
    // discard server traffic (keepalives / messages) so the RX buffer drains
    while (aprsClient.available()) aprsClient.read();
}

void send(const String &line) {
    if (aprsClient.connected()) aprsClient.print(line + "\r\n");
}

void forward(const String &packet) {
    if (!aprsClient.connected()) return;
    if (packet.length() < 5) return;
    if (packet.indexOf("NOGATE") >= 0 || packet.indexOf("RFONLY") >= 0) return;
    if (packet.indexOf("TCPIP") >= 0) return;     // already gated, don't loop it back
    int gt    = packet.indexOf('>');
    int colon = packet.indexOf(':');
    if (gt <= 3 || colon <= gt) return;           // not a valid "SENDER>PATH:payload"
    // skip the 3-byte LoRa-APRS header; build  SENDER>PATH,qAR,IGATECALL:payload
    String line = packet.substring(3, colon);     // SENDER>PATH
    line += ",qAR,";
    line += Config.callsign;
    line += packet.substring(colon);              // ":payload"
    aprsClient.print(line + "\r\n");
    Serial.println("[aprsis] up: " + line);
}

}  // namespace AprsIs

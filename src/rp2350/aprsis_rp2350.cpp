#include "aprsis_rp2350.h"
#include <Ethernet.h>
#include "configuration.h"

extern Configuration Config;
// Inbox store (eth_web.cpp) — poll() runs in netTask, same task that owns it.
extern void ethWebAddMessage(const String &from, const String &text, const String &via);

static EthernetClient aprsClient;
static const char *IGATE_VERS = "RP2350iGate 0.1";

// Parse one APRS-IS line; if it's a message addressed to our callsign, record it
// in the inbox and ack it (if it carries a {msgid}).
static void processAprsisLine(const String &line) {
    if (line.length() == 0 || line[0] == '#') return;       // server comment / keepalive
    int gt = line.indexOf('>');
    int dc = line.indexOf("::");
    if (gt <= 0 || dc <= gt) return;                        // not a message packet
    String from = line.substring(0, gt);
    if (from == Config.callsign) return;                    // ignore our own echoes
    String rest = line.substring(dc + 2);                   // "ADDRESSEE :text{id}"
    int fc = rest.indexOf(':');
    if (fc < 0) return;
    String addressee = rest.substring(0, fc);
    addressee.trim();
    if (addressee != Config.callsign) return;               // not for us

    String text  = rest.substring(fc + 1);
    int    brace = text.indexOf('{');
    String msg   = (brace > 0) ? text.substring(0, brace) : text;
    ethWebAddMessage(from, msg, "IS");

    if (brace > 0 && aprsClient.connected()) {              // ack it over APRS-IS
        String id = text.substring(brace + 1);
        id.trim();
        String ack = Config.callsign;
        ack += ">APLRG1,TCPIP*::";
        String padded = from;
        for (int i = from.length(); i < 9; i++) padded += ' ';
        ack += padded;
        ack += ":ack";
        ack += id;
        aprsClient.print(ack + "\r\n");
    }
}

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
    // read line by line; record messages addressed to us, drop the rest
    static String lineBuf;
    while (aprsClient.available()) {
        char ch = (char)aprsClient.read();
        if (ch == '\n') { processAprsisLine(lineBuf); lineBuf = ""; }
        else if (ch != '\r') { if (lineBuf.length() < 512) lineBuf += ch; }
    }
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

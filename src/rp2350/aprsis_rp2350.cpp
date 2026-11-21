#include "aprsis_rp2350.h"
#include <Ethernet.h>
#include <APRSPacketLib.h>
#include "configuration.h"
#include "station_rp2350.h"
#include "syslog_rp2350.h"

extern Configuration Config;
// Inbox store (eth_web.cpp) — poll() runs in netTask, same task that owns it.
extern void ethWebAddMessage(const String &from, const String &text, const String &via);

static EthernetClient aprsClient;
static const char *IGATE_VERS = "RP2350iGate 0.1";
static uint32_t lastLineMs  = 0;     // last line of any kind from the server ("#" keepalives too)
static uint32_t connectedAt = 0;     // when the current connection was opened

// APRS-IS servers send a "#" keepalive about every 20 s. A connection that stays
// quiet this long is dead even if the socket still reports it as connected.
static const uint32_t SILENT_DROP_MS = 60000;

// Parse one APRS-IS line; if it's a message addressed to our callsign, record it
// in the inbox and ack it (if it carries a {msgid}).
static void processAprsisLine(const String &line) {
    // Login answer: "# logresp CALL verified, server T2XXX" / "... unverified, ...".
    // Like the ESP32 (APRS_IS_Utils::processAPRSISPacket), a rejected passcode stops APRS-IS
    // (until the next boot or config save) instead of reconnecting every 10 s and uploading
    // packets the server ignores. Only an explicit "unverified" blocks: if a server never
    // answers, behaviour stays as before. "unverified" contains "verified", so test it first.
    if (line.startsWith("# logresp") && line.indexOf(Config.callsign) >= 0 && line.indexOf("unverified") >= 0) {
        Serial.println("\n****APRS PASSCODE NOT VALID****\n");
        Syslog::logTx("*** APRS-IS passcode NOT valid -> APRS-IS stopped ***");
        aprsClient.stop();
        Config.aprs_is.active = false;      // in memory only: the saved config keeps APRS-IS enabled
        return;
    }
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

    // ack id = text after the LAST '{'; empty = no ack requested (generateAckMessage)
    String id = (brace > 0) ? text.substring(text.lastIndexOf('{') + 1) : "";
    id.trim();
    if (id.length() && aprsClient.connected()) {            // ack it over APRS-IS
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
    connectedAt = millis();
    Serial.println("[aprsis] connected, logged in as " + Config.callsign);
    return true;
}

void poll() {
    // read line by line; record messages addressed to us, drop the rest
    static String lineBuf;
    while (aprsClient.available()) {
        char ch = (char)aprsClient.read();
        if (ch == '\n') { lastLineMs = millis(); processAprsisLine(lineBuf); lineBuf = ""; }
        else if (ch != '\r') { if (lineBuf.length() < 512) lineBuf += ch; }
    }
    // a server that went quiet leaves the socket "connected" forever: drop it so
    // netTask reconnects
    if (aprsClient.connected() && millis() - lastLineMs > SILENT_DROP_MS &&
        millis() - connectedAt > SILENT_DROP_MS) {
        Serial.println("[aprsis] server silent, reconnecting");
        aprsClient.stop();
    }
}

bool silentFor(uint32_t ms) { return millis() - lastLineMs > ms; }

void send(const String &line) {
    if (line.length() == 0) return;                         // never send an empty line to APRS-IS
    if (aprsClient.connected()) aprsClient.print(line + "\r\n");
}

// Gate rules of APRS_IS_Utils::processLoRaPacket. This build never gates APRS-IS
// back to RF (no messagesToRF/objectsToRF), so it is a receive-only iGate and
// uses qAO, like the ESP32 with messagesToRF off (buildPacketToUpload).
// IS2PLAIN metadata line sent right before the gated packet (same as buildIS2Meta()
// in the ESP32 aprs_is_utils.cpp). rx_modem=2 is LoRa; the bit rate is derived from
// the RX SF/BW/CR, which only change with a config save (= reboot), so it is cached.
static String buildIS2Meta(int rssi, float snr) {
    static String fixedTail;
    if (fixedTail == "") {
        // LoRa bit rate = SF * (BW / 2^SF) * 4/CR   (CR = denominator: 5..8)
        uint32_t bitrate = (uint32_t)(Config.loramodule.rxSpreadingFactor
                           * ((float)Config.loramodule.rxSignalBandwidth / (1 << Config.loramodule.rxSpreadingFactor))
                           * (4.0 / Config.loramodule.rxCodingRate4));
        fixedTail = " rx_modem=2 rx_bitrate=" + String(bitrate) + " rx_frequency=" + String(Config.loramodule.rxFreq);
    }
    return "#is2-meta rx_rssi_dbm=" + String(rssi) + " rx_snr_db=" + String(snr, 1) + fixedTail;
}

void forward(const String &packet, int rssi, float snr) {
    if (!aprsClient.connected()) return;
    if (packet.length() < 5) return;
    String body = packet.substring(3);            // skip the 3-byte LoRa-APRS header
    int gt    = body.indexOf('>');
    int colon = body.indexOf(':');
    if (gt <= 0 || colon <= gt) return;           // not a valid "SENDER>PATH:payload"
    if (colon + 1 < (int)body.length() && body[colon + 1] == '}') return;   // third-party packet

    String sender = body.substring(0, gt);
    if (sender == Config.callsign || !Station::callsignIsValid(sender)) return;

    // NOGATE / RFONLY / TCPIP count only in the path, not in the payload
    String hdr  = body.substring(gt + 1, colon);  // "TOCALL,PATH"
    int comma   = hdr.indexOf(',');
    String path = (comma > 2) ? hdr.substring(comma + 1) : "";
    if (path.indexOf("NOGATE") >= 0 || path.indexOf("RFONLY") >= 0 || path.indexOf("TCPIP") >= 0) return;

    // SENDER>TOCALL,PATH,qAO,IGATECALL:payload
    String line = body.substring(0, colon);
    line += ",qAO,";
    line += Config.callsign;
    line += APRSPacketLib::checkForStartingBytes(body.substring(colon));
    if (Config.aprs_is.is2Metadata) aprsClient.print(buildIS2Meta(rssi, snr) + "\r\n" + line + "\r\n");   // meta line + packet in one write
    else                            aprsClient.print(line + "\r\n");
    Serial.println("[aprsis] up: " + line);
}

}  // namespace AprsIs

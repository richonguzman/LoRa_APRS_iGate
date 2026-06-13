/*
 * RP2350 query responder — lean port of QUERY_Utils (src/query_utils.cpp).
 * Answers RF-borne APRS message queries directed at our callsign. Replies/acks
 * go through the Station anti-collision output buffer. loraTask context only.
 */
#include "query_rp2350.h"
#include "configuration.h"
#include "station_rp2350.h"

extern Configuration Config;

// Signal stats of the last RX, set by lora_utils_rp2350.cpp.
extern int   rssi;
extern float snr;
extern int   freqError;

// Records an incoming APRS message addressed to us (main.cpp -> netTask store,
// served at GET /messages.json). Called from loraTask.
extern void onIncomingMessage(const String &from, const String &text);

static const char *IGATE_VERSION = "RP2350 LoRa iGate v0.1";

namespace {

// Build the APRS message envelope addressed back to `station`:
//   CALL>APLRG1,RFONLY[,PATH]::STATION   :   (9-char padded addressee, RFONLY so
// the reply is not gated). Caller appends the message body.
String envelope(const String& station) {
    String s = Config.callsign;
    s += ">APLRG1,RFONLY";
    if (Config.beacon.path.length()) { s += ','; s += Config.beacon.path; }
    s += "::";
    String padded = station;
    for (int i = station.length(); i < 9; i++) padded += ' ';
    s += padded;
    s += ':';
    return s;
}

// Compose the answer text for a query, or "" if unsupported. Manager-only remote
// management commands (Config.remoteManagement managers) are honored when `sender`
// is a manager. Our queries arrive over RF only, so rfOnly is always satisfied.
String answerFor(const String& query, const String& sender) {
    String q = query;
    q.toUpperCase();
    // --- public queries ---
    if (q == "?APRS?" || q == "H" || q == "HELP" || q == "?")
        return "?APRSV ?APRSP ?APRSL ?APRSSR";
    if (q == "?APRSV")
        return String(IGATE_VERSION);
    if (q == "?APRSP")
        return "iGate QTH: " + String(Config.beacon.latitude, 2) + " " + String(Config.beacon.longitude, 2);
    if (q == "?APRSL") {
        String list = Station::heardListString();
        if (list.length() == 0)
            return "No Station Listened in the last " + String(Config.rememberStationTime) + " min.";
        return list;
    }
    if (q == "?APRSSR") {
        char buf[40];
        snprintf(buf, sizeof(buf), " %ddBm / %.2fdB / %dHz", rssi, (double)snr, freqError);
        return String(buf);
    }
    // --- manager-only remote management ---
    if (Station::isManager(sender)) {
        if (q.startsWith("?TX=ON")) {
            if (Config.loramodule.txActive) return "TX was ON";
            Config.loramodule.txActive = true;
            return "TX=ON";
        }
        if (q.startsWith("?TX=OFF")) {
            if (!Config.loramodule.txActive) return "TX was OFF";
            Config.loramodule.txActive = false;
            return "TX=OFF";
        }
        if (q.startsWith("?TX=?"))
            return Config.loramodule.txActive ? "TX=ON" : "TX=OFF";
        if (q.startsWith("?COMMIT")) {
            Config.writeFile();
            return "New Config Saved";
        }
    }
    return "";
}

}  // namespace

namespace Query {

bool handleMessage(const String& body, const String& sender) {
    int dc = body.indexOf("::");
    if (dc <= 10) return false;                              // not an APRS message

    String addresseeAndMsg = body.substring(dc + 2);
    int firstColon = addresseeAndMsg.indexOf(':');
    if (firstColon < 0) return false;
    String addressee = addresseeAndMsg.substring(0, firstColon);
    addressee.trim();
    if (addressee != Config.callsign) return false;          // not for us

    String text = addresseeAndMsg.substring(firstColon + 1); // message text (+ optional {msgid})

    // ack any message that carries a {msgid}
    int brace = text.indexOf('{');
    String msgText = (brace > 0) ? text.substring(0, brace) : text;

    onIncomingMessage(sender, msgText);                      // log it for the web UI
    if (brace > 0) {
        String ackId = text.substring(brace + 1);
        ackId.trim();
        String ack = envelope(sender);
        ack += "ack";
        ack += ackId;
        Station::enqueueTx(ack, false);
        Serial.println("[query] ack -> " + sender + " (" + ackId + ")");
    }

    if (msgText.indexOf('?') == 0) {                         // it's a query
        String answer = answerFor(msgText, sender);
        if (answer.length()) {
            String reply = envelope(sender);
            reply += answer;
            reply += " *";                                  // random 2-char tag (matches upstream)
            reply += char(random(97, 123));
            reply += char(random(97, 123));
            reply += '*';
            Station::enqueueTx(reply, false);
            Serial.println("[query] " + msgText + " -> " + sender + ": " + answer);
        }
        return true;                                        // query handled: don't gate/digipeat
    }
    // Plain message addressed to us: consumed locally (recorded + acked above).
    // Return true so it is NOT gated to APRS-IS nor digipeated — a message "for
    // me" is not pass-through traffic. Messages to OTHER stations still gate/digi
    // normally (this function returns false early for them). To put a message on
    // APRS-IS, the sender uses the "Send via APRS-IS (TCP)" path explicitly.
    return true;
}

}  // namespace Query

/*
 * RP2350 TNC server — replaces the ESP32 tnc_utils.cpp (WiFiServer/mDNS) with an
 * EthernetServer on the W5500. Speaks KISS (codec reused as-is from
 * src/kiss_utils.cpp) or, with tnc.kissProtocol = false, plain TNC2 text lines
 * terminated by CR LF, as tnc_utils.cpp does. Runs entirely in netTask (W5500
 * owner); frames bound for RF are handed to loraTask via enqueueRfFrame()
 * (txMsgQueue -> Station buffer).
 */
#include "tnc_rp2350.h"
#include <Ethernet.h>
#include "configuration.h"
#include "kiss_utils.h"
#include "kiss_protocol.h"
#include "aprsis_rp2350.h"

extern Configuration Config;
extern void enqueueRfFrame(const String &frame);   // main.cpp: push to txMsgQueue -> loraTask

#define TNC_PORT        8001
#define TNC_MAX_CLIENTS 2
#define TNC_MAX_BUF     512

static EthernetServer tncServer(TNC_PORT);
static EthernetClient tncClients[TNC_MAX_CLIENTS];
static String         tncBuf[TNC_MAX_CLIENTS];
static bool           tncStarted = false;

namespace {

const char *protocolLabel() {
    return Config.tnc.kissProtocol ? "KISS" : "TNC2";
}

// Act on one complete TNC2 frame from a client: TX over RF (honoring acceptOwn /
// txActive) and optionally bridge to APRS-IS.
void processFrame(const String &frame) {
    int gt = frame.indexOf('>');
    String sender = (gt > 0) ? frame.substring(0, gt) : "";
    if (!Config.tnc.acceptOwn && sender == Config.callsign) {
        Serial.printf("[tnc] ignored own frame from %s client\n", protocolLabel());
        return;
    }
    Serial.printf("[tnc] <- (%s) ", protocolLabel());
    Serial.println(frame);
    if (Config.loramodule.txActive) enqueueRfFrame(frame);  // -> loraTask -> RF

    if (Config.tnc.aprsBridgeActive && Config.aprs_is.active && AprsIs::connected()) {
        int colon = frame.indexOf(':');
        if (gt > 0 && colon > gt) {                     // SENDER>PATH,qAO,IGATE:payload
            String line = frame.substring(0, colon);
            line += ",qAO,";
            line += Config.callsign;
            line += frame.substring(colon);
            AprsIs::send(line);
            Serial.println("[tnc] -> APRS-IS: " + line);
        }
    }
}

// KISS: collect bytes between FENDs, then decode.
void handleByteKISS(int idx, char ch) {
    String &buf = tncBuf[idx];
    if (buf.length() == 0 && ch != (char)FEND) return;     // wait for a frame start
    buf += ch;

    if (ch == (char)FEND && buf.length() > 3) {
        bool isData = false;
        String frame = decodeKISS(buf, isData);
        buf = "";
        if (isData && frame.length() > 0) processFrame(frame);
    }
    if (buf.length() > TNC_MAX_BUF) buf = "";
}

// TNC2: one frame per line; CR is ignored, LF ends it. Lines that are not
// SENDER>PATH:payload are dropped, as tnc_utils.cpp does.
void handleByteTNC2(int idx, char ch) {
    String &buf = tncBuf[idx];
    if (ch == '\r') return;

    if (ch == '\n') {
        String frame = buf;
        frame.trim();
        buf = "";
        if (frame.length() > 0 && frame.indexOf(':') != -1 && frame.indexOf('>') != -1) processFrame(frame);
        return;
    }

    buf += ch;
    if (buf.length() > TNC_MAX_BUF) buf = "";
}

void handleByte(int idx, char ch) {
    if (Config.tnc.kissProtocol) handleByteKISS(idx, ch);
    else                         handleByteTNC2(idx, ch);
}

}  // namespace

namespace Tnc {

void setup() {
    if (!Config.tnc.enableServer) return;
    tncServer.begin();
    tncStarted = true;
    Serial.printf("[tnc] %s server on :%d\n", protocolLabel(), TNC_PORT);
}

void poll() {
    if (!tncStarted) return;

    EthernetClient nc = tncServer.accept();
    if (nc) {
        bool placed = false;
        for (int i = 0; i < TNC_MAX_CLIENTS; i++) {
            if (!tncClients[i] || !tncClients[i].connected()) {
                tncClients[i].stop();
                tncClients[i] = nc;
                tncBuf[i] = "";
                placed = true;
                Serial.printf("[tnc] client %d connected\n", i);
                break;
            }
        }
        if (!placed) { Serial.println("[tnc] no free slots, refused"); nc.stop(); }
    }

    for (int i = 0; i < TNC_MAX_CLIENTS; i++) {
        if (tncClients[i] && tncClients[i].connected()) {
            while (tncClients[i].available() > 0) handleByte(i, (char)tncClients[i].read());
        }
    }
}

void broadcast(const String &tnc2frame) {
    if (!tncStarted || tnc2frame.length() == 0) return;
    String encoded = Config.tnc.kissProtocol ? encodeKISS(tnc2frame) : (tnc2frame + "\r\n");
    for (int i = 0; i < TNC_MAX_CLIENTS; i++) {
        if (tncClients[i] && tncClients[i].connected()) {
            tncClients[i].print(encoded);
            tncClients[i].flush();
        }
    }
}

}  // namespace Tnc

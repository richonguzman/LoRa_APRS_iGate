/*
 * RP2350 KISS TNC server — replaces the ESP32 tnc_utils.cpp (WiFiServer/mDNS)
 * with an EthernetServer on the W5500. The KISS framing/codec is reused as-is
 * from src/kiss_utils.cpp. Runs entirely in netTask (W5500 owner); frames bound
 * for RF are handed to loraTask via enqueueRfFrame() (txMsgQueue -> Station buffer).
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

// Decode a complete KISS frame from client `idx` and act on it: TX over RF
// (honoring acceptOwn / txActive) and optionally bridge to APRS-IS.
void handleByte(int idx, char ch) {
    String &buf = tncBuf[idx];
    if (buf.length() == 0 && ch != (char)FEND) return;     // wait for a frame start
    buf += ch;

    if (ch == (char)FEND && buf.length() > 3) {
        bool isData = false;
        String frame = decodeKISS(buf, isData);
        buf = "";
        if (!isData || frame.length() == 0) return;

        int gt = frame.indexOf('>');
        String sender = (gt > 0) ? frame.substring(0, gt) : "";
        if (!Config.tnc.acceptOwn && sender == Config.callsign) {
            Serial.println("[tnc] ignored own frame from client");
            return;
        }
        Serial.println("[tnc] <- " + frame);
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
    if (buf.length() > TNC_MAX_BUF) buf = "";
}

}  // namespace

namespace Tnc {

void setup() {
    if (!Config.tnc.enableServer) return;
    tncServer.begin();
    tncStarted = true;
    Serial.printf("[tnc] KISS server on :%d\n", TNC_PORT);
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
    String kiss = encodeKISS(tnc2frame);
    for (int i = 0; i < TNC_MAX_CLIENTS; i++) {
        if (tncClients[i] && tncClients[i].connected()) {
            tncClients[i].print(kiss);
            tncClients[i].flush();
        }
    }
}

}  // namespace Tnc

#include "eth_web.h"
#include <Arduino.h>
#include <Ethernet.h>
#include <LittleFS.h>
#include <vector>
#include "web_assets.h"   // gzipped SPA assets embedded in flash
#include "ntp_rp2350.h"
#include "ota_rp2350.h"
#include "map_utils.h"    // stations heard with a decoded position (web "Map" view)
#include "configuration.h"

extern Configuration Config;

// Apply a posted config form (urlencoded or multipart) to Config + persist.
extern bool applyConfigForm(const String &contentType, const String &body);

// Set by POST /action?type=send-beacon; netTask sends an APRS-IS beacon and clears it.
extern volatile bool g_beaconNow;
// Set by POST /action?type=send-rf-beacon; loraTask sends an RF beacon and clears it.
extern volatile bool g_rfBeaconNow;
// Originate an APRS message (RF via loraTask and/or APRS-IS via the socket).
extern void webSendMessage(const String &to, const String &text, bool viaRF, bool viaTCP);

static const int    WEB_PORT        = 80;
static const size_t MAX_LINE_LEN    = 512;
static const size_t MAX_HEADER_LINES = 40;
static const char  *CONFIG_PATH     = "/igate_conf.json";

static EthernetServer webServer(WEB_PORT);
static bool webStarted = false;

// --- incoming APRS messages addressed to us (shown in the Messages view) ---
struct RxMsg { String from; String text; String via; uint32_t when; };
static std::vector<RxMsg> rxMessages;          // netTask-owned, newest pushed to back
static const size_t RXMSG_MAX = 20;

void ethWebAddMessage(const String &from, const String &text, const String &via) {
    if (rxMessages.size() >= RXMSG_MAX) rxMessages.erase(rxMessages.begin());
    rxMessages.push_back({from, text, via, millis()});
    Serial.println("[msg] from " + from + " (" + via + "): " + text);
}

// --- received LoRa packets log (shown in the Received packets view) ---
struct RxPkt { String frame; int rssi; float snr; uint32_t when; };
static std::vector<RxPkt> rxPackets;           // netTask-owned, newest pushed to back
static const size_t RXPKT_MAX = 10;

void ethWebAddPacket(const String &frame, int rssi, float snr) {
    if (rxPackets.size() >= RXPKT_MAX) rxPackets.erase(rxPackets.begin());
    rxPackets.push_back({frame, rssi, snr, millis()});
}

struct Request {
    String method;
    String path;
    String query;
    String contentType;
    String authorization;
    long   contentLength = 0;
};

// minimal percent-decoder for query-string values
static String urlDecodeQ(const String &s) {
    String out; out.reserve(s.length());
    for (size_t i = 0; i < s.length(); i++) {
        char ch = s[i];
        if (ch == '+') out += ' ';
        else if (ch == '%' && i + 2 < s.length()) {
            auto hx = [](char h) -> int {
                if (h >= '0' && h <= '9') return h - '0';
                if (h >= 'a' && h <= 'f') return h - 'a' + 10;
                if (h >= 'A' && h <= 'F') return h - 'A' + 10;
                return 0; };
            out += (char)((hx(s[i + 1]) << 4) | hx(s[i + 2])); i += 2;
        } else out += ch;
    }
    return out;
}

// extract a query-string parameter ("key=value", value up to '&'), url-decoded
static String getQueryParam(const String &query, const char *key) {
    String needle = String(key) + "=";
    int i = query.indexOf(needle);
    if (i < 0) return "";
    i += needle.length();
    int amp = query.indexOf('&', i);
    if (amp < 0) amp = query.length();
    return urlDecodeQ(query.substring(i, amp));
}

// standard base64 of a string (for HTTP Basic auth comparison)
static String b64(const String &data) {
    static const char *T = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    String r;
    int v = 0, c = 0;
    for (size_t i = 0; i < data.length(); i++) {
        v = (v << 8) | (uint8_t)data[i];
        c += 8;
        while (c >= 6) { c -= 6; r += T[(v >> c) & 0x3F]; }
    }
    if (c > 0) r += T[(v << (6 - c)) & 0x3F];
    while (r.length() % 4) r += '=';
    return r;
}

// web-interface HTTP Basic auth: allowed unless webadmin is active with creds and
// the request lacks the matching "Authorization: Basic <base64(user:pass)>".
static bool webAuthOk(const String &authHeader) {
    if (!Config.webadmin.active) return true;
    if (Config.webadmin.username.length() == 0 && Config.webadmin.password.length() == 0) return true;
    if (!authHeader.startsWith("Basic ")) return false;
    String token = authHeader.substring(6);
    token.trim();
    return token == b64(Config.webadmin.username + ":" + Config.webadmin.password);
}

// escape a string for embedding in a JSON string literal
static String jsonEscape(const String &s) {
    String out; out.reserve(s.length() + 4);
    for (size_t i = 0; i < s.length(); i++) {
        char ch = s[i];
        if (ch == '"' || ch == '\\') { out += '\\'; out += ch; }
        else if (ch == '\n') out += "\\n";
        else if (ch == '\r') out += "\\r";
        else if (ch == '\t') out += "\\t";
        else if ((uint8_t)ch < 0x20) { /* drop other control chars */ }
        else out += ch;
    }
    return out;
}

// ---- HTTP request parsing (adapted from mesh/eth/ethApiHandlers.cpp) ----
static bool readLine(EthernetClient &c, String &out, uint32_t deadlineMs) {
    out = "";
    while ((int32_t)(millis() - deadlineMs) < 0) {
        if (!c.connected()) return false;
        if (!c.available()) { delay(1); continue; }
        int ch = c.read();
        if (ch < 0) continue;
        if (ch == '\n') return true;
        if (ch == '\r') continue;
        if (out.length() >= MAX_LINE_LEN) return false;
        out += (char)ch;
    }
    return false;
}

static bool headerIs(const String &line, const char *prefix, String &valueOut) {
    size_t pl = strlen(prefix);
    if (line.length() < pl + 1) return false;
    for (size_t i = 0; i < pl; i++) {
        char a = line.charAt(i), b = prefix[i];
        if (a >= 'A' && a <= 'Z') a += 32;
        if (b >= 'A' && b <= 'Z') b += 32;
        if (a != b) return false;
    }
    if (line.charAt(pl) != ':') return false;
    valueOut = line.substring(pl + 1);
    valueOut.trim();
    return true;
}

static bool parseRequest(EthernetClient &c, Request &req, uint32_t deadlineMs) {
    String line;
    if (!readLine(c, line, deadlineMs) || line.length() == 0) return false;
    int sp1 = line.indexOf(' ');
    int sp2 = line.indexOf(' ', sp1 + 1);
    if (sp1 <= 0 || sp2 <= sp1) return false;
    req.method = line.substring(0, sp1);
    String full = line.substring(sp1 + 1, sp2);
    int q = full.indexOf('?');
    if (q >= 0) { req.path = full.substring(0, q); req.query = full.substring(q + 1); }
    else        { req.path = full; req.query = ""; }
    size_t n = 0; String val;
    while (n++ < MAX_HEADER_LINES) {
        if (!readLine(c, line, deadlineMs)) return false;
        if (line.length() == 0) return true;  // end of headers
        if (headerIs(line, "Content-Length", val)) req.contentLength = val.toInt();
        else if (headerIs(line, "Content-Type", val)) req.contentType = val;
        else if (headerIs(line, "Authorization", val)) req.authorization = val;
    }
    return false;
}

// ---- response helpers ----
static void sendStatus(EthernetClient &c, int code, const char *reason) {
    c.print("HTTP/1.1 "); c.print(code); c.print(' '); c.print(reason); c.print("\r\n");
}
static void cors(EthernetClient &c) {
    c.print("Access-Control-Allow-Origin: *\r\n");
    c.print("Access-Control-Allow-Headers: Content-Type\r\n");
    c.print("Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n");
}
static void sendText(EthernetClient &c, int code, const char *reason, const char *ctype, const String &body) {
    sendStatus(c, code, reason);
    c.print("Content-Type: "); c.print(ctype); c.print("\r\n");
    cors(c);
    c.print("Content-Length: "); c.print(body.length()); c.print("\r\n");
    c.print("Connection: close\r\n\r\n");
    c.print(body);
}

static void serveConfig(EthernetClient &c) {
    File f = LittleFS.open(CONFIG_PATH, "r");
    if (!f) { sendText(c, 200, "OK", "application/json", "{}"); return; }
    sendStatus(c, 200, "OK");
    c.print("Content-Type: application/json\r\n");
    cors(c);
    c.print("Content-Length: "); c.print((uint32_t)f.size()); c.print("\r\n");
    c.print("Connection: close\r\n\r\n");
    uint8_t buf[256];
    size_t n;
    while ((n = f.read(buf, sizeof(buf))) > 0) c.write(buf, n);
    f.close();
}

// Serve a gzipped flash asset (chunked — the W5500 TX buffer is ~2 KB/socket).
static void serveAsset(EthernetClient &c, const WebAsset &a) {
    sendStatus(c, 200, "OK");
    c.print("Content-Type: "); c.print(a.ctype); c.print("\r\n");
    c.print("Content-Encoding: gzip\r\n");
    c.print("Cache-Control: max-age=3600\r\n");
    cors(c);
    c.print("Content-Length: "); c.print((uint32_t)a.len); c.print("\r\n");
    c.print("Connection: close\r\n\r\n");
    size_t sent = 0;
    while (sent < a.len) {
        size_t chunk = a.len - sent;
        if (chunk > 1024) chunk = 1024;
        size_t w = c.write(a.data + sent, chunk);   // RP2350: flash is directly addressable
        if (w == 0) break;
        sent += w;
    }
}

// POST /configuration.json : read the urlencoded body, apply to Config, persist,
// then redirect + reboot (mirrors the original web_utils flow).
static void handleConfigPost(EthernetClient &c, const Request &req) {
    String body;
    if (req.contentLength > 0 && req.contentLength < 65536) body.reserve(req.contentLength);
    uint32_t dl = millis() + 8000;
    while ((long)body.length() < req.contentLength && (int32_t)(millis() - dl) < 0) {
        int avail = c.available();
        if (avail > 0) {
            while (avail-- > 0 && (long)body.length() < req.contentLength) body += (char)c.read();
        } else if (!c.connected()) break;
        else delay(1);
    }
    Serial.printf("[web] POST /configuration.json: %d bytes (%s)\n",
                  (int)body.length(), req.contentType.c_str());
    if (applyConfigForm(req.contentType, body)) {
        sendStatus(c, 302, "Found");
        c.print("Location: /?success=1\r\n");
        cors(c);
        c.print("Content-Length: 0\r\nConnection: close\r\n\r\n");
        c.flush();
        delay(500);
        Serial.println("[web] config saved -> reboot");
        rp2040.restart();
    } else {
        sendText(c, 500, "Internal Server Error", "text/plain", "could not save config");
        c.flush();
    }
}

static void handleClient(EthernetClient &c) {
    Request req;
    uint32_t deadline = millis() + 3000;
    if (!parseRequest(c, req, deadline)) return;
    Serial.printf("[web] %s %s\n", req.method.c_str(), req.path.c_str());

    if (req.method == "OPTIONS") {
        sendStatus(c, 204, "No Content");
        cors(c);
        c.print("Content-Length: 0\r\nConnection: close\r\n\r\n");
        c.flush();
        return;
    }
    // web-interface auth (HTTP Basic) when enabled. /status (version probe) and
    // /update (its own OTA auth) are exempt to avoid breaking probes / double-auth.
    if (req.path != "/status" && req.path != "/update" && !webAuthOk(req.authorization)) {
        sendStatus(c, 401, "Unauthorized");
        c.print("WWW-Authenticate: Basic realm=\"iGate Admin\"\r\n");
        cors(c);
        c.print("Content-Length: 0\r\nConnection: close\r\n\r\n");
        c.flush();
        return;
    }
    if (req.path == "/status") {
        extern const char *FW_BUILD;
        sendText(c, 200, "OK", "text/plain", String("LoRa APRS iGate RP2350 — built ") + FW_BUILD);
        c.flush();
        return;
    }
    if (req.path == "/stations.json") {   // stations with a position, for the Map view
        sendText(c, 200, "OK", "application/json", MAP_Utils::getStationsJson());
        c.flush();
        return;
    }
    if (req.path == "/messages.json") {
        String body = "[";
        uint32_t now = millis();
        for (size_t i = rxMessages.size(); i-- > 0; ) {        // newest first
            const RxMsg &m = rxMessages[i];
            body += "{\"from\":\"";  body += jsonEscape(m.from);
            body += "\",\"text\":\""; body += jsonEscape(m.text);
            body += "\",\"via\":\"";  body += jsonEscape(m.via);
            body += "\",\"age\":";    body += String((now - m.when) / 1000);
            body += "}";
            if (i != 0) body += ",";
        }
        body += "]";
        sendText(c, 200, "OK", "application/json", body);
        c.flush();
        return;
    }
    if (req.path == "/received-packets.json") {
        String body = "[";
        uint32_t now = millis();
        for (size_t i = rxPackets.size(); i-- > 0; ) {         // newest first
            const RxPkt &p = rxPackets[i];
            uint32_t age = (now - p.when) / 1000;
            String ago = age < 60 ? String(age) + "s" : String(age / 60) + "m";
            // real wall-clock = NTP-now minus this packet's age (works even for
            // packets received before the NTP sync); fall back to "ago" if no NTP
            String rxTime = Ntp::synced() ? Ntp::hms(Ntp::nowEpoch() - age) : ago;
            body += "{\"rxTime\":\""; body += rxTime;
            body += "\",\"packet\":\""; body += jsonEscape(p.frame);
            body += "\",\"RSSI\":";     body += String(p.rssi);
            body += ",\"SNR\":";        body += String(p.snr, 1);
            body += "}";
            if (i != 0) body += ",";
        }
        body += "]";
        sendText(c, 200, "OK", "application/json", body);
        c.flush();
        return;
    }
    if (req.path == "/update") {            // POST raw firmware.bin -> arduino-pico Updater
        if (req.method == "POST") { Ota::handleUpdate(c, req.authorization, req.contentLength); return; }
        sendText(c, 405, "Method Not Allowed", "text/plain", "POST firmware.bin");
        c.flush();
        return;
    }
    if (req.path == "/configuration.json") {
        if (req.method == "GET") { serveConfig(c); c.flush(); return; }
        if (req.method == "POST") { handleConfigPost(c, req); return; }  // reboots on success
        sendText(c, 405, "Method Not Allowed", "text/plain", "GET or POST");
        c.flush();
        return;
    }
    if (req.path == "/action") {            // POST /action?type=send-beacon | send-rf-beacon | send-message | reboot
        if (req.query.indexOf("send-message") >= 0) {
            String to   = getQueryParam(req.query, "to");
            String text = getQueryParam(req.query, "text");
            bool viaRF  = req.query.indexOf("rf=1") >= 0;
            bool viaTCP = req.query.indexOf("tcp=1") >= 0;
            if (!viaRF && !viaTCP) viaRF = true;               // default to RF if unspecified
            if (to.length() == 0 || text.length() == 0) {
                sendText(c, 400, "Bad Request", "text/plain", "need to= and text=");
            } else {
                webSendMessage(to, text, viaRF, viaTCP);
                sendText(c, 200, "OK", "text/plain", "message queued");
            }
        } else if (req.query.indexOf("send-rf-beacon") >= 0) {
            g_rfBeaconNow = true;
            sendText(c, 200, "OK", "text/plain", "rf beacon queued");
        } else if (req.query.indexOf("send-beacon") >= 0) {
            g_beaconNow = true;
            sendText(c, 200, "OK", "text/plain", "beacon queued");
        } else if (req.query.indexOf("reboot") >= 0) {
            sendText(c, 200, "OK", "text/plain", "rebooting");
            c.flush();
            delay(300);
            rp2040.restart();
        } else {
            sendText(c, 404, "Not Found", "text/plain", "unknown action");
        }
        c.flush();
        return;
    }
    if (req.method == "GET") {  // static SPA assets
        for (size_t i = 0; i < WEB_ASSETS_N; i++) {
            if (req.path == WEB_ASSETS[i].path) {
                serveAsset(c, WEB_ASSETS[i]);
                c.flush();
                return;
            }
        }
    }
    sendText(c, 404, "Not Found", "text/plain", "not found");
    c.flush();
}

void ethWebSetup() {
    if (webStarted) return;
    LittleFS.begin();
    webServer.begin();
    webStarted = true;
    Serial.println("[web] HTTP config server on :80  (/status, /configuration.json)");
}

void ethWebPoll() {
    if (!webStarted) return;
    EthernetClient client = webServer.accept();
    if (client) {
        handleClient(client);
        client.stop();
    }
}

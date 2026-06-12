#include "eth_web.h"
#include <Arduino.h>
#include <Ethernet.h>
#include <LittleFS.h>
#include "web_assets.h"   // gzipped SPA assets embedded in flash

// Apply a posted config form (urlencoded or multipart) to Config + persist.
extern bool applyConfigForm(const String &contentType, const String &body);

static const int    WEB_PORT        = 80;
static const size_t MAX_LINE_LEN    = 512;
static const size_t MAX_HEADER_LINES = 40;
static const char  *CONFIG_PATH     = "/igate_conf.json";

static EthernetServer webServer(WEB_PORT);
static bool webStarted = false;

struct Request {
    String method;
    String path;
    String query;
    String contentType;
    long   contentLength = 0;
};

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
    if (req.path == "/status") {
        sendText(c, 200, "OK", "text/plain", "OK");
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

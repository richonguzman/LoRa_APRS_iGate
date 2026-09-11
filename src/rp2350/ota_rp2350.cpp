/*
 * RP2350 network OTA. Streams the posted raw firmware.bin into the arduino-pico
 * Updater (Update.begin/write/end); Update.end(true) stages it via picoOTA for
 * the bootloader, which copies it on the next reboot. netTask only (W5500 owner;
 * the transfer blocks the net loop for a few seconds — acceptable).
 */
#include "ota_rp2350.h"
#include <Updater.h>
#include "configuration.h"

extern Configuration Config;

namespace {

String base64(const String &data) {
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

// No credentials configured -> open. Otherwise require "Basic <base64(user:pass)>".
bool authOk(const String &authHeader) {
    if (Config.ota.username.length() == 0 && Config.ota.password.length() == 0) return true;
    if (!authHeader.startsWith("Basic ")) return false;
    String token = authHeader.substring(6);
    token.trim();
    return token == base64(Config.ota.username + ":" + Config.ota.password);
}

void reply(EthernetClient &c, int code, const char *reason, const char *msg) {
    c.print("HTTP/1.1 "); c.print(code); c.print(' '); c.print(reason); c.print("\r\n");
    c.print("Connection: close\r\nContent-Type: text/plain\r\nContent-Length: ");
    c.print((int)strlen(msg)); c.print("\r\n\r\n"); c.print(msg);
    c.flush();
}

}  // namespace

namespace Ota {

void handleUpdate(EthernetClient &c, const String &authHeader, long contentLength) {
    if (!authOk(authHeader)) {
        c.print("HTTP/1.1 401 Unauthorized\r\nWWW-Authenticate: Basic realm=\"iGate OTA\"\r\n"
                "Connection: close\r\nContent-Length: 0\r\n\r\n");
        c.flush();
        Serial.println("[ota] auth failed");
        return;
    }
    if (contentLength <= 0) { reply(c, 400, "Bad Request", "need Content-Length"); return; }

    Serial.printf("[ota] starting update, %ld bytes\n", contentLength);
    if (!Update.begin((size_t)contentLength)) {
        Serial.printf("[ota] Update.begin failed err=%u\n", Update.getError());
        reply(c, 500, "Internal Server Error", "begin failed (not enough space?)");
        return;
    }

    uint8_t buf[1024];
    long remaining = contentLength;
    uint32_t lastActivity = millis();
    while (remaining > 0) {
        if (!c.connected()) { Serial.println("[ota] client disconnected"); Update.end(false); return; }
        int avail = c.available();
        if (avail <= 0) {
            if (millis() - lastActivity > 10000) {
                Serial.println("[ota] timeout");
                Update.end(false);
                reply(c, 408, "Request Timeout", "timeout");
                return;
            }
            delay(1);
            continue;
        }
        size_t cap = sizeof(buf);
        if ((long)cap > remaining) cap = (size_t)remaining;
        if ((size_t)avail < cap) cap = (size_t)avail;
        int got = c.read(buf, cap);
        if (got <= 0) continue;
        if (Update.write(buf, (size_t)got) != (size_t)got) {
            Serial.printf("[ota] write failed err=%u\n", Update.getError());
            Update.end(false);
            reply(c, 500, "Internal Server Error", "write failed");
            return;
        }
        remaining -= got;
        lastActivity = millis();
    }

    if (!Update.end(true)) {
        Serial.printf("[ota] end failed err=%u\n", Update.getError());
        reply(c, 500, "Internal Server Error", "finalize failed");
        return;
    }

    Serial.println("[ota] update staged OK, rebooting...");
    reply(c, 200, "OK", "update ok, rebooting");
    delay(400);
    rp2040.restart();
}

}  // namespace Ota

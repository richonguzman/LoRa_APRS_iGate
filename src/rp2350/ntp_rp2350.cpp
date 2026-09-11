/*
 * RP2350 SNTP client over EthernetUDP (W5500). Non-blocking request/reply; keeps
 * a (baseEpoch, baseMillis) pair so nowEpoch() interpolates with millis() between
 * syncs. Local time = UTC + Config.ntp.gmtCorrection hours. netTask only.
 */
#include "ntp_rp2350.h"
#include <Ethernet.h>
#include <EthernetUdp.h>
#include "configuration.h"

extern Configuration Config;

static EthernetUDP udp;
static bool     started     = false;
static bool     isSynced    = false;
static uint32_t baseEpoch   = 0;     // local unix epoch at last sync
static uint32_t baseMillis  = 0;     // millis() at last sync
static uint32_t lastAttempt = 0;
static bool     pending     = false;
static uint32_t sentAt      = 0;

static const uint16_t LOCAL_PORT     = 2390;
static const uint32_t SYNC_INTERVAL  = 15UL * 60UL * 1000UL;  // resync every 15 min
static const uint32_t RETRY_INTERVAL = 5UL * 1000UL;          // retry quickly while unsynced (DNS may not be ready at boot)
static const uint32_t NTP_UNIX_DELTA = 2208988800UL;          // seconds 1900 -> 1970

namespace Ntp {

static void sendRequest() {
    byte pkt[48] = {0};
    pkt[0] = 0x1B;                                  // LI=0, VN=3, Mode=3 (client)
    if (udp.beginPacket(Config.ntp.server.c_str(), 123) == 1) {
        udp.write(pkt, 48);
        udp.endPacket();
        pending = true;
        sentAt  = millis();
    }
}

void poll() {
    if (!started) { udp.begin(LOCAL_PORT); started = true; }
    uint32_t now = millis();

    bool due = !isSynced ? (lastAttempt == 0 || now - lastAttempt > RETRY_INTERVAL)
                         : (now - baseMillis > SYNC_INTERVAL);
    if (!pending && due) { lastAttempt = now; sendRequest(); }

    if (pending) {
        if (udp.parsePacket() >= 48) {
            byte buf[48];
            udp.read(buf, 48);
            uint32_t secs1900 = ((uint32_t)buf[40] << 24) | ((uint32_t)buf[41] << 16) |
                                ((uint32_t)buf[42] << 8) | (uint32_t)buf[43];
            baseEpoch  = (secs1900 - NTP_UNIX_DELTA) + (int32_t)(Config.ntp.gmtCorrection * 3600.0f);
            baseMillis = millis();
            isSynced   = true;
            pending    = false;
            Serial.println("[ntp] synced " + hms(baseEpoch) + " (" + Config.ntp.server + ")");
        } else if (now - sentAt > 3000) {
            pending = false;                        // timeout — retry next interval
        }
    }
}

bool synced() { return isSynced; }

uint32_t nowEpoch() {
    if (!isSynced) return 0;
    return baseEpoch + (millis() - baseMillis) / 1000;
}

String hms(uint32_t epoch) {
    uint32_t d = epoch % 86400UL;
    char b[9];
    snprintf(b, sizeof(b), "%02u:%02u:%02u",
             (unsigned)(d / 3600), (unsigned)((d % 3600) / 60), (unsigned)(d % 60));
    return String(b);
}

}  // namespace Ntp

/*
 * RP2350 SNTP client over EthernetUDP (W5500). Non-blocking request/reply; keeps
 * a (baseEpoch, baseMillis) pair so nowEpoch() interpolates with millis() between
 * syncs. Local time = UTC + Config.ntp.gmtCorrection hours. netTask only.
 *
 * The UDP socket is only held from the request to the reply (or its timeout):
 * the W5100S has 4 sockets, and one kept open here for good left the web server
 * a single one, so a browser loading the page got connections refused.
 */
#include "ntp_rp2350.h"
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <Dns.h>
#include "configuration.h"

extern Configuration Config;

static EthernetUDP udp;
static bool     isSynced    = false;
static uint32_t baseEpoch   = 0;     // local unix epoch at last sync
static uint32_t baseMillis  = 0;     // millis() at last sync
static uint32_t lastAttempt = 0;
static bool     pending     = false;
static uint32_t sentAt      = 0;

static const uint16_t LOCAL_PORT     = 2390;
static const uint32_t SYNC_INTERVAL  = 15UL * 60UL * 1000UL;  // resync every 15 min
static const uint32_t RETRY_INTERVAL = 5UL * 1000UL;          // retry quickly while unsynced (DNS may not be ready at boot)
static const uint32_t RESYNC_RETRY   = 60UL * 1000UL;         // a failed resync is retried this often, not on every poll
static const uint32_t NTP_UNIX_DELTA = 2208988800UL;          // seconds 1900 -> 1970

namespace Ntp {

static void sendRequest() {
    // Resolve BEFORE taking our socket: the DNS lookup needs a socket of its own.
    IPAddress server;
    if (!server.fromString(Config.ntp.server)) {
        DNSClient dns;
        dns.begin(Ethernet.dnsServerIP());
        if (dns.getHostByName(Config.ntp.server.c_str(), server) != 1) return;
    }
    if (udp.begin(LOCAL_PORT) != 1) return;         // no free socket now: try again later
    byte pkt[48] = {0};
    pkt[0] = 0x1B;                                  // LI=0, VN=3, Mode=3 (client)
    if (udp.beginPacket(server, 123) == 1) {
        udp.write(pkt, 48);
        udp.endPacket();
        pending = true;
        sentAt  = millis();
    } else {
        udp.stop();
    }
}

void poll() {
    uint32_t now = millis();

    bool due = !isSynced ? (lastAttempt == 0 || now - lastAttempt > RETRY_INTERVAL)
                         : (now - baseMillis > SYNC_INTERVAL && now - lastAttempt > RESYNC_RETRY);
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
            udp.stop();
            Serial.println("[ntp] synced " + hms(baseEpoch) + " (" + Config.ntp.server + ")");
        } else if (millis() - sentAt > 3000) {
            // timeout — retry next interval. Not `now - sentAt`: sentAt is stamped after
            // `now`, past the DNS lookup, so that difference wrapped and every request
            // timed out in the same poll it was sent, before the reply could arrive.
            pending = false;
            udp.stop();
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

String ymd(uint32_t epoch) {
    // days since 1970-01-01 -> civil date (Howard Hinnant's civil_from_days)
    int32_t  z   = (int32_t)(epoch / 86400UL) + 719468;
    int32_t  era = z / 146097;
    uint32_t doe = (uint32_t)(z - era * 146097);
    uint32_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    uint32_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    uint32_t mp  = (5 * doy + 2) / 153;
    uint32_t d   = doy - (153 * mp + 2) / 5 + 1;
    uint32_t m   = mp < 10 ? mp + 3 : mp - 9;
    int32_t  y   = (int32_t)yoe + era * 400 + (m <= 2 ? 1 : 0);
    char b[11];
    snprintf(b, sizeof(b), "%04d-%02u-%02u", (int)y, (unsigned)m, (unsigned)d);
    return String(b);
}

}  // namespace Ntp

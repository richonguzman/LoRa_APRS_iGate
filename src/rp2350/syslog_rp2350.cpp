/*
 * RP2350 remote syslog (RFC5424 / UDP over EthernetUDP). The server IP is
 * resolved once at setup (literal or one DNS lookup) and cached; logging is
 * fire-and-forget UDP to that IP — never a per-call DNS lookup (the per-LOG
 * DNS/ARP block is what reboot-looped the W5500 boards). netTask only.
 */
#include "syslog_rp2350.h"
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <Dns.h>
#include "configuration.h"

extern Configuration Config;

static EthernetUDP udp;
static IPAddress   serverIp;
static bool        ready = false;

static void send(const String &message) {
    if (!ready) return;
    String pkt = "<165>1 - ";
    pkt += Config.callsign;
    pkt += " RP2350iGate - - - ";
    pkt += message;
    udp.beginPacket(serverIp, Config.syslog.port);          // resolved IP -> no DNS
    udp.write((const uint8_t *)pkt.c_str(), pkt.length());
    udp.endPacket();
}

namespace Syslog {

void setup() {
    if (!Config.syslog.active) return;
    udp.begin(0);
    if (serverIp.fromString(Config.syslog.server)) {        // literal IP
        ready = true;
    } else {                                                // resolve hostname once
        DNSClient dns;
        dns.begin(Ethernet.dnsServerIP());
        IPAddress ip;
        if (dns.getHostByName(Config.syslog.server.c_str(), ip) == 1) { serverIp = ip; ready = true; }
    }
    if (ready)
        Serial.printf("[syslog] -> %d.%d.%d.%d:%d\n", serverIp[0], serverIp[1], serverIp[2], serverIp[3],
                      Config.syslog.port);
    else
        Serial.println("[syslog] could not resolve server " + Config.syslog.server);
}

void logRx(const String &frame, int rssi, float snr) {
    if (!ready) return;
    int gt    = frame.indexOf('>');
    int colon = frame.indexOf(':');
    String sender  = (gt > 0) ? frame.substring(0, gt) : "?";
    String payload = (colon >= 0) ? frame.substring(colon + 1) : frame;
    char sig[40];
    snprintf(sig, sizeof(sig), " / %ddBm / %.2fdB", rssi, (double)snr);
    send("RX / " + sender + " ---> " + payload + sig);
}

void logTx(const String &info) {
    send(info);
}

}  // namespace Syslog

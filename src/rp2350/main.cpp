/*
 * LoRa APRS iGate — RP2350 (WIZnet W5500-EVB-Pico2 + E22).
 * Core path: receive LoRa-APRS packets (SPI1, LoRa_Utils) and forward them to
 * APRS-IS over Ethernet (W5500/SPI0, AprsIs), plus the web config server.
 *
 * Concurrency (FreeRTOS SMP):
 *   - loraTask  : owns the radio (SPI1); RX packets -> rxQueue.
 *   - netTask   : owns the W5500 (SPI0); DHCP + web server + APRS-IS connect/
 *                 poll/forward. ALL Ethernet/W5500 access lives here (no race).
 *   - loop()    : LED heartbeat + status line.
 * The two SPI buses are separate, so radio and network run truly in parallel.
 */
#include <Arduino.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <SPI.h>
#include <Ethernet.h>
#include "configuration.h"
#include "lora_utils.h"
#include "eth_web.h"
#include "aprsis_rp2350.h"
#include "beacon_rp2350.h"
#include "wx_rp2350.h"
#include "digi_rp2350.h"
#include "station_rp2350.h"
#include "query_rp2350.h"
#include "message_rp2350.h"
#include "tnc_rp2350.h"
#include "telemetry_rp2350.h"
#include "ntp_rp2350.h"
#include "mqtt_rp2350.h"
#include "syslog_rp2350.h"
#include "pico/unique_id.h"   // pico_get_unique_board_id (cached at boot — SMP-safe)

#ifndef HB_LED
#define HB_LED 25          // onboard LED (GP25 on the WIZnet W5500-EVB-Pico2)
#endif

// Firmware build timestamp — shown in the boot banner and served at GET /status
// (lets you confirm which build is running, e.g. after an OTA).
const char *FW_BUILD = __DATE__ " " __TIME__;

// Global iGate config instance (lives in LoRa_APRS_iGate.cpp on ESP32, excluded
// from the RP2350 build; the modules extern it).
Configuration Config;

static byte mac[6];
static QueueHandle_t rxQueue;        // heap String* handoff: loraTask -> netTask
static QueueHandle_t txMsgQueue;     // heap String* handoff: netTask -> loraTask (outgoing msgs)
static QueueHandle_t rxMsgQueue;     // heap String* handoff: loraTask -> netTask (incoming msgs for us)
static QueueHandle_t rxPktQueue;     // heap String* handoff: loraTask -> netTask (RX packets log)

// Signal stats of the last RX (set in lora_utils_rp2350.cpp's receivePacket()).
extern int   rssi;
extern float snr;
volatile bool g_beaconNow = false;   // set by POST /action?type=send-beacon (eth_web)
volatile bool g_rfBeaconNow = false; // set by POST /action?type=send-rf-beacon (eth_web)

// Called from netTask (eth_web /action handler) to originate an APRS message.
// We build here (Config is read-only) and hand the packet to loraTask, the sole
// owner of the Station output buffer (no cross-task race on outBuffer).
// Called from netTask (eth_web /action). RF goes via the queue to loraTask; the
// APRS-IS copy is sent right here (netTask owns the socket).
void webSendMessage(const String &to, const String &text, bool viaRF, bool viaTCP) {
    if (viaRF) {
        String pkt = Message::buildRF(to, text);
        if (pkt.length()) {
            String *p = new String(pkt);
            if (xQueueSend(txMsgQueue, &p, 0) != pdTRUE) delete p;
        }
    }
    if (viaTCP && AprsIs::connected()) {
        String line = Message::buildAprsis(to, text);
        if (line.length()) { AprsIs::send(line); Serial.println("[msg] APRS-IS: " + line); }
    }
}

// Called from loraTask (Query::handleMessage) when an APRS message addressed to
// us is received. Hand it to netTask (the W5500 owner) for the /messages.json
// store via a queue — no cross-task race on the message list.
void onIncomingMessage(const String &from, const String &text) {
    String *p = new String(from + "\t" + text);
    if (xQueueSend(rxMsgQueue, &p, 0) != pdTRUE) delete p;
}

// Called from netTask (TNC KISS client, MQTT downlink) to transmit a frame over
// RF. Hands it to loraTask via txMsgQueue (Station output buffer owner).
void enqueueRfFrame(const String &frame) {
    if (frame.length() == 0) return;
    String *p = new String(frame);
    if (xQueueSend(txMsgQueue, &p, 0) != pdTRUE) delete p;
}

// --------------------------------------------------------------- LoRa RX/TX task
// Owns the radio (SPI1): transmits the RF beacon (when Config.beacon.sendViaRF)
// and receives LoRa-APRS packets. Both run from THIS task so the radio is never
// touched concurrently.
void loraTask(void *) {
    LoRa_Utils::setup();
    uint32_t lastRfBeacon = 0;
    for (;;) {
        // --- RF beacon TX: enqueue into the anti-collision output buffer ---
        if (Config.beacon.sendViaRF) {
            bool locOk = !(Config.beacon.latitude == 0.0 && Config.beacon.longitude == 0.0);
            if (locOk && (lastRfBeacon == 0 || g_rfBeaconNow ||
                (millis() - lastRfBeacon >= (uint32_t)Config.beacon.interval * 60000UL))) {
                lastRfBeacon = millis();
                g_rfBeaconNow = false;
                String rf = Beacon::buildRfLine();
                Serial.println("[beacon] RF queued: " + rf);
                Station::enqueueTx(rf, true);
            }
        }
        // --- RX ---
        String pkt = LoRa_Utils::receivePacket();   // raw packet (with 3-byte hdr), "" if none
        if (pkt.length() > 0) {
            Station::noteRx();
            String body = pkt.length() > 3 ? pkt.substring(3) : pkt;   // strip LoRa-APRS header
            Serial.println("[lora] RX: " + body);

            // log every RX packet for the web "Received packets" view (frame +
            // signal stats captured together, before policy filtering)
            String *lp = new String(String(rssi) + "\t" + String(snr, 1) + "\t" + body);
            if (xQueueSend(rxPktQueue, &lp, 0) != pdTRUE) delete lp;

            // packet policy (shared by gate + digi): parse SENDER + payload
            int gt    = body.indexOf('>');
            int colon = body.indexOf(':');
            String sender  = (gt > 0) ? body.substring(0, gt) : "";
            String payload = (colon >= 0) ? body.substring(colon) : body;

            // our own packet heard back (e.g. digipeated by another station): do
            // not gate/digi it, nor count it as a heard station (matches upstream).
            String selfCall = Config.tacticalCallsign.length() ? Config.tacticalCallsign : Config.callsign;
            bool isSelf      = sender.length() && sender == selfCall;
            bool blacklisted = sender.length() && Station::isBlacklisted(sender);
            bool dup         = !isSelf && sender.length() && Station::isDuplicate(sender, payload);
            if (sender.length() && !isSelf) Station::updateLastHeard(sender);

            if (isSelf) {
                Serial.println("[lora] ignored own packet heard back: " + sender);
            } else if (blacklisted) {
                Serial.println("[lora] dropped (blacklist): " + sender);
            } else if (dup) {
                Serial.println("[lora] dropped (dup <25s): " + sender);
            } else if (sender.length() && Query::handleMessage(body, sender)) {
                // it was a query addressed to us — answered over RF, don't gate/digi
            } else {
                // gate to APRS-IS (handed to netTask, the W5500 owner)
                String *p = new String(pkt);
                if (xQueueSend(rxQueue, &p, 0) != pdTRUE) delete p;   // drop if full
                // digipeat: rewrite WIDEn-N and enqueue for anti-collision TX
                if (Digi::enabled()) {
                    String repeat = Digi::process(pkt);
                    if (repeat.length() > 0) {
                        Serial.println("[digi] queued: " + repeat);
                        Station::enqueueTx(repeat, false);
                    }
                }
            }
        }
        // --- outgoing messages handed over by netTask (web /action) ---
        String *m;
        while (xQueueReceive(txMsgQueue, &m, 0) == pdTRUE) {
            Serial.println("[msg] queued: " + *m);
            Station::enqueueTx(*m, false);
            delete m;
        }
        // --- pump the RF output buffer (spaces TX, backs off after RX) ---
        Station::processTxBuffer();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ------------------------------------------------------------- Network task
void netTask(void *) {
    pinMode(PIN_ETH_RST, OUTPUT);
    digitalWrite(PIN_ETH_RST, LOW);  vTaskDelay(pdMS_TO_TICKS(10));
    digitalWrite(PIN_ETH_RST, HIGH); vTaskDelay(pdMS_TO_TICKS(60));
    SPI.setRX(PIN_ETH_MISO);
    SPI.setSCK(PIN_ETH_SCK);
    SPI.setTX(PIN_ETH_MOSI);
    Ethernet.init(PIN_ETH_CS);

    // MAC derived from the chip's unique board ID (like Meshtastic's getMacAddr).
    // pico_get_unique_board_id() returns a value cached at boot by an SDK ctor
    // (single-core, before the scheduler) — it does NOT touch flash here, so it's
    // safe under FreeRTOS SMP. mac[0]=0x02 => locally administered + unicast; the
    // 4 chip bytes make two identical boards differ on the LAN (no DHCP clash).
    pico_unique_board_id_t uid;
    pico_get_unique_board_id(&uid);
    mac[0] = 0x02; mac[1] = 0x00;
    mac[2] = uid.id[4]; mac[3] = uid.id[5]; mac[4] = uid.id[6]; mac[5] = uid.id[7];
    Serial.printf("[ETH] MAC %02X:%02X:%02X:%02X:%02X:%02X\n",
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Serial.println("[ETH] DHCP...");
    if (Ethernet.begin(mac) == 0) Serial.println("[ETH] DHCP FAILED (check link/cable)");
    else { Serial.print("[ETH] IP: "); Serial.println(Ethernet.localIP()); }

    ethWebSetup();
    Tnc::setup();                            // KISS TNC server on :8001 (if enabled)
    Mqtt::setup();                           // MQTT bridge (if Config.mqtt.active)
    Syslog::setup();                         // remote syslog (if Config.syslog.active)

    uint32_t lastMaintain = millis();
    uint32_t lastAprsTry  = 0;
    uint32_t lastHb       = 0;
    uint32_t lastBeacon   = 0;
    for (;;) {
        ethWebPoll();
        Tnc::poll();                         // accept KISS clients + read their frames
        Ntp::poll();                         // SNTP time sync (real timestamps)
        Mqtt::loop();                        // MQTT (re)connect + service
        if (Config.aprs_is.active) {
            if (!AprsIs::connected() && (millis() - lastAprsTry > 10000)) {
                lastAprsTry = millis();
                AprsIs::connect();
            }
            AprsIs::poll();
            // telemetry channel definitions (EQNS/UNIT/PARM) at boot + ~daily
            if (AprsIs::connected() && Telemetry::dueDefinitions()) Telemetry::sendDefinitions();
            // periodic position beacon over APRS-IS (config has sendViaAPRSIS)
            bool locOk = !(Config.beacon.latitude == 0.0 && Config.beacon.longitude == 0.0);
            if (Config.beacon.sendViaAPRSIS && AprsIs::connected() && locOk &&
                (lastBeacon == 0 || g_beaconNow ||
                 (millis() - lastBeacon >= (uint32_t)Config.beacon.interval * 60000UL))) {
                lastBeacon = millis();
                g_beaconNow = false;
                String b = Beacon::buildAprsisLine();
                AprsIs::send(b);
                Serial.println("[beacon] APRS-IS: " + b);
                String t = Telemetry::dataPacket();  // classic T# telemetry packet
                AprsIs::send(t);
                Serial.println("[telemetry] " + t);
                Mqtt::publishBeacon(b);              // mirror beacon to MQTT (if beaconOverMqtt)
                if (Config.syslog.logBeaconOverTCPIP) Syslog::logTx("APRSIS BEACON / " + b);
            }
        }
        // forward any LoRa packets handed over by loraTask
        String *p;
        while (xQueueReceive(rxQueue, &p, 0) == pdTRUE) {
            if (Config.aprs_is.active) AprsIs::forward(*p);
            delete p;
        }
        // store any incoming messages addressed to us (for GET /messages.json)
        String *m;
        while (xQueueReceive(rxMsgQueue, &m, 0) == pdTRUE) {
            int tab = m->indexOf('\t');
            if (tab > 0) ethWebAddMessage(m->substring(0, tab), m->substring(tab + 1), "RF");
            delete m;
        }
        // store RX packets log (for GET /received-packets.json): "rssi\tsnr\tframe"
        // and forward each received frame to any connected KISS TNC clients.
        String *lp;
        while (xQueueReceive(rxPktQueue, &lp, 0) == pdTRUE) {
            int t1 = lp->indexOf('\t');
            int t2 = lp->indexOf('\t', t1 + 1);
            if (t1 > 0 && t2 > t1) {
                String frame = lp->substring(t2 + 1);
                int prssi = lp->substring(0, t1).toInt();
                float psnr = lp->substring(t1 + 1, t2).toFloat();
                ethWebAddPacket(frame, prssi, psnr);
                Tnc::broadcast(frame);
                Mqtt::publishRx(frame);
                Syslog::logRx(frame, prssi, psnr);
            }
            delete lp;
        }
        if (millis() - lastMaintain > 5000) { Ethernet.maintain(); lastMaintain = millis(); }
        // heartbeat printed HERE (netTask owns the W5500 — no SPI race)
        if (millis() - lastHb > 3000) {
            lastHb = millis();
            IPAddress ip = Ethernet.localIP();
            String clock = Ntp::synced() ? Ntp::hms(Ntp::nowEpoch()) : "--:--:--";
            Serial.printf("[hb] up=%lus  %s  ip=%d.%d.%d.%d  callsign=%s  aprsis=%s  stations=%u\n",
                          (unsigned long)(millis() / 1000), clock.c_str(), ip[0], ip[1], ip[2], ip[3],
                          Config.callsign.c_str(), AprsIs::connected() ? "up" : "down",
                          (unsigned)Station::activeCount());
        }
        // scheduled auto-reboot (Config "Reboot Time", in hours of uptime)
        if (Config.rebootMode && Config.rebootModeTime > 0 &&
            millis() > (uint32_t)Config.rebootModeTime * 3600UL * 1000UL) {
            Serial.println("[reboot] scheduled reboot time reached, restarting...");
            delay(100);
            rp2040.restart();
        }
        vTaskDelay(pdMS_TO_TICKS(15));
    }
}

// ------------------------------------------------------------------- setup
void setup() {
    Serial.begin(115200);
    uint32_t t0 = millis();
    while (!Serial && (millis() - t0 < 3000)) {}
    Serial.println(String("\n=== LoRa APRS iGate — RP2350 (built ") + FW_BUILD + ") ===");

    pinMode(HB_LED, OUTPUT);
    Config.setup();                          // LittleFS + /igate_conf.json (defaults on first boot)
    if (Config.rebootMode && Config.rebootModeTime > 0)
        Serial.printf("[reboot] auto-reboot every %d h\n", Config.rebootModeTime);
    Station::setup();                        // load blacklist/managers from Config
    Wx::setup();                             // BMP280 on I2C0 (Wire) — single-threaded init here

    rxQueue    = xQueueCreate(8, sizeof(String *));
    txMsgQueue = xQueueCreate(4, sizeof(String *));
    rxMsgQueue = xQueueCreate(8, sizeof(String *));
    rxPktQueue = xQueueCreate(8, sizeof(String *));
    xTaskCreate(loraTask, "lora", 4096, nullptr, 3, nullptr);
    xTaskCreate(netTask,  "net",  4096, nullptr, 2, nullptr);
}

void loop() {
    // LED blink ONLY — no W5500/Ethernet access here (that lives in netTask),
    // otherwise concurrent SPI0 access corrupts both readers.
    static uint32_t tick = 0;
    digitalWrite(HB_LED, (tick++ & 1) ? HIGH : LOW);   // ~2 Hz proof-of-life
    vTaskDelay(pdMS_TO_TICKS(250));
}

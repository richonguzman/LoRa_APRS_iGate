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
#include "pico/unique_id.h"   // pico_get_unique_board_id (cached at boot — SMP-safe)

#ifndef HB_LED
#define HB_LED 25          // onboard LED (GP25 on the WIZnet W5500-EVB-Pico2)
#endif

// Global iGate config instance (lives in LoRa_APRS_iGate.cpp on ESP32, excluded
// from the RP2350 build; the modules extern it).
Configuration Config;

static byte mac[6];
static QueueHandle_t rxQueue;        // heap String* handoff: loraTask -> netTask
static QueueHandle_t txMsgQueue;     // heap String* handoff: netTask -> loraTask (outgoing msgs)
volatile bool g_beaconNow = false;   // set by POST /action?type=send-beacon (eth_web)
volatile bool g_rfBeaconNow = false; // set by POST /action?type=send-rf-beacon (eth_web)

// Called from netTask (eth_web /action handler) to originate an APRS message.
// We build here (Config is read-only) and hand the packet to loraTask, the sole
// owner of the Station output buffer (no cross-task race on outBuffer).
void webSendMessage(const String &to, const String &text) {
    String pkt = Message::buildRF(to, text);
    if (pkt.length() == 0) return;
    String *p = new String(pkt);
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

            // packet policy (shared by gate + digi): parse SENDER + payload
            int gt    = body.indexOf('>');
            int colon = body.indexOf(':');
            String sender  = (gt > 0) ? body.substring(0, gt) : "";
            String payload = (colon >= 0) ? body.substring(colon) : body;

            bool blacklisted = sender.length() && Station::isBlacklisted(sender);
            bool dup         = sender.length() && Station::isDuplicate(sender, payload);
            if (sender.length()) Station::updateLastHeard(sender);

            if (blacklisted) {
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

    uint32_t lastMaintain = millis();
    uint32_t lastAprsTry  = 0;
    uint32_t lastHb       = 0;
    uint32_t lastBeacon   = 0;
    for (;;) {
        ethWebPoll();
        if (Config.aprs_is.active) {
            if (!AprsIs::connected() && (millis() - lastAprsTry > 10000)) {
                lastAprsTry = millis();
                AprsIs::connect();
            }
            AprsIs::poll();
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
            }
        }
        // forward any LoRa packets handed over by loraTask
        String *p;
        while (xQueueReceive(rxQueue, &p, 0) == pdTRUE) {
            if (Config.aprs_is.active) AprsIs::forward(*p);
            delete p;
        }
        if (millis() - lastMaintain > 5000) { Ethernet.maintain(); lastMaintain = millis(); }
        // heartbeat printed HERE (netTask owns the W5500 — no SPI race)
        if (millis() - lastHb > 3000) {
            lastHb = millis();
            IPAddress ip = Ethernet.localIP();
            Serial.printf("[hb] up=%lus  ip=%d.%d.%d.%d  callsign=%s  aprsis=%s  stations=%u\n",
                          (unsigned long)(millis() / 1000), ip[0], ip[1], ip[2], ip[3],
                          Config.callsign.c_str(), AprsIs::connected() ? "up" : "down",
                          (unsigned)Station::activeCount());
        }
        vTaskDelay(pdMS_TO_TICKS(15));
    }
}

// ------------------------------------------------------------------- setup
void setup() {
    Serial.begin(115200);
    uint32_t t0 = millis();
    while (!Serial && (millis() - t0 < 3000)) {}
    Serial.println("\n=== LoRa APRS iGate — RP2350 ===");

    pinMode(HB_LED, OUTPUT);
    Config.setup();                          // LittleFS + /igate_conf.json (defaults on first boot)
    Station::setup();                        // load blacklist/managers from Config
    Wx::setup();                             // BMP280 on I2C0 (Wire) — single-threaded init here

    rxQueue    = xQueueCreate(8, sizeof(String *));
    txMsgQueue = xQueueCreate(4, sizeof(String *));
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

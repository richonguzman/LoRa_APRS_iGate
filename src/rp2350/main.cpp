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

// Last MAC octet — per build env so two boards never collide on DHCP. We do NOT
// read the chip unique-ID at runtime (faults under FreeRTOS SMP -> USB hang).
#ifndef MAC_ID
#define MAC_ID 0x10
#endif
#ifndef HB_LED
#define HB_LED 25          // onboard LED (GP25 on the WIZnet W5500-EVB-Pico2)
#endif

// Global iGate config instance (lives in LoRa_APRS_iGate.cpp on ESP32, excluded
// from the RP2350 build; the modules extern it).
Configuration Config;

static byte mac[6];
static QueueHandle_t rxQueue;       // heap String* handoff: loraTask -> netTask
volatile bool g_beaconNow = false;  // set by POST /action?type=send-beacon (eth_web)

// --------------------------------------------------------------- LoRa RX task
void loraTask(void *) {
    LoRa_Utils::setup();
    for (;;) {
        String pkt = LoRa_Utils::receivePacket();   // raw packet (with 3-byte hdr), "" if none
        if (pkt.length() > 0) {
            Serial.println("[lora] RX: " + pkt.substring(pkt.length() >= 3 ? 3 : 0));
            String *p = new String(pkt);
            if (xQueueSend(rxQueue, &p, 0) != pdTRUE) delete p;   // drop if full
        }
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

    mac[0] = 0x02; mac[1] = 0x00; mac[2] = 0x4C; mac[3] = 0x47; mac[4] = 0x00; mac[5] = MAC_ID;
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
            Serial.printf("[hb] up=%lus  ip=%d.%d.%d.%d  callsign=%s  aprsis=%s\n",
                          (unsigned long)(millis() / 1000), ip[0], ip[1], ip[2], ip[3],
                          Config.callsign.c_str(), AprsIs::connected() ? "up" : "down");
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

    rxQueue = xQueueCreate(8, sizeof(String *));
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

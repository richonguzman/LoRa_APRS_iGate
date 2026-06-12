/*
 * LoRa APRS iGate — RP2350 + W5500 + E22P-433M30S
 * =====================================================================
 * PHASE 1 — platform bring-up validator
 *   - arduino-pico (earlephilhower) + FreeRTOS SMP across both Cortex-M33
 *   - RadioLib SX1262 RX on SPI1; DIO1 IRQ -> binary semaphore -> loraTask
 *   - W5500 (arduino-libraries/Ethernet) DHCP bring-up on SPI0 -> netTask
 *
 * SUCCESS CRITERIA:
 *   1) Serial prints a decoded LoRa-APRS frame from a real beacon
 *      (EA4GLO-10 transmits its position over RF every ~15 min).
 *   2) Board pulls a DHCP lease from the LAN.
 *   -> validates the platform + the FreeRTOS concurrency model in one shot.
 *
 * RF params are the live config of the user's iGate (OE5BPA LoRa-APRS std).
 */
#include <Arduino.h>
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <RadioLib.h>
#include <SPI.h>
#include <Ethernet.h>
#include "eth_web.h"
#include "configuration.h"

// Last MAC octet — set per build env so two boards never collide on DHCP.
// (We deliberately do NOT read the chip unique-ID at runtime: that flash read
//  faults under FreeRTOS SMP while the other core executes from XIP -> USB hang.)
#ifndef MAC_ID
#define MAC_ID 0x10
#endif

// Global iGate config instance. In the ESP32 build this lives in
// LoRa_APRS_iGate.cpp (excluded from the RP2350 build); the modules extern it.
Configuration Config;

// Onboard LED — GP25 on the WIZnet W5500-EVB-Pico2 (same as Pico 2).
// Blinks as a monitor-independent proof of life.
#ifndef HB_LED
#define HB_LED 25
#endif

// ---- LoRa RF parameters (3 build modes) ----
#if defined(LINK_TEST)
// Two-board bench LINK test (both boards = E22P-868M30S). ANTENNAS REQUIRED on
// BOTH boards — the E22P is a +30 dBm/1 W PA and TX without antenna kills it.
// Controlled channel away from the Meshtastic net; identical params both ends.
// LOW power + short packets; keep boards ~1-2 m apart.
static const float    LORA_FREQ_MHZ = 868.500f;
static const float    LORA_BW_KHZ   = 125.0f;
static const uint8_t  LORA_SF       = 12;
static const uint8_t  LORA_CR       = 5;
static const uint8_t  LORA_SYNC     = 0x12;
static const uint16_t LORA_PREAMB   = 8;
static const int8_t   LORA_TX_DBM   = 2;     // LOW (the module PA still boosts it)
#elif defined(TEST_BAND_868)
// Listen to the user's own Meshtastic EU_868 LongFast traffic (hops 3 channels)
// to validate the RX chain. SF11/BW250/CR4:5 ; sync 0x2b ; preamble 16.
static const float    TEST_FREQS[3] = { 869.442f, 869.525f, 869.608f };
static const float    LORA_FREQ_MHZ = TEST_FREQS[0];
static const float    LORA_BW_KHZ   = 250.0f;
static const uint8_t  LORA_SF       = 11;
static const uint8_t  LORA_CR       = 5;
static const uint8_t  LORA_SYNC     = 0x2b;
static const uint16_t LORA_PREAMB   = 16;
static const int8_t   LORA_TX_DBM   = 20;    // unused (RX only)
#else
// 433 MHz LoRa-APRS (EA4GLO-10 live config — needs the E22P-433M30S module)
static const float    LORA_FREQ_MHZ = 433.775f;
static const float    LORA_BW_KHZ   = 125.0f;
static const uint8_t  LORA_SF       = 12;
static const uint8_t  LORA_CR       = 5;     // 4/5
static const uint8_t  LORA_SYNC     = 0x12;  // LoRa-APRS private sync word
static const uint16_t LORA_PREAMB   = 8;
static const int8_t   LORA_TX_DBM   = 20;    // unused in Phase-1 RX
#endif
static const float    LORA_TCXO_V   = 1.8f;  // E22P TCXO fed from DIO3

// ---- Radio on SPI1 (Module: CS, DIO1/IRQ, RST, BUSY, bus) ----
SX1262 radio = new Module(PIN_LORA_CS, PIN_LORA_DIO1, PIN_LORA_RST, PIN_LORA_BUSY, SPI1);

// ---- W5500 on SPI0 ----
byte mac[6];  // derived from the RP2350 unique board ID in netTask() (unique per board)

static SemaphoreHandle_t rxSem;
volatile int g_radioSt = -999;   // latched radio.begin() result (shown in heartbeat)

// DIO1 fired in IRQ context -> wake loraTask. (No IRAM_ATTR needed on RP2350.)
void onDio1() {
  BaseType_t hpw = pdFALSE;
  xSemaphoreGiveFromISR(rxSem, &hpw);
  portYIELD_FROM_ISR(hpw);
}

// ---------------------------------------------------------------- LoRa RX task
void loraTask(void *) {
  uint8_t buf[256];
#ifdef TEST_BAND_868
  uint8_t  fidx = 0;
  const TickType_t waitTicks = pdMS_TO_TICKS(12000);  // hop channel if idle 12 s
#else
  const TickType_t waitTicks = portMAX_DELAY;
#endif
  for (;;) {
    if (xSemaphoreTake(rxSem, waitTicks) == pdTRUE) {
      int len = radio.getPacketLength();
      int st  = radio.readData(buf, len);
      radio.startReceive();  // re-arm immediately
      if (st == RADIOLIB_ERR_NONE) {
        Serial.printf("\n[LoRa] RX %d B  RSSI=%.1f dBm  SNR=%.1f dB\n",
                      len, radio.getRSSI(), radio.getSNR());
#if defined(LINK_TEST)
        Serial.print("  MSG: ");
        for (int i = 0; i < len; i++)
          Serial.write((buf[i] >= 32 && buf[i] < 127) ? buf[i] : '.');
        Serial.println();
#elif defined(TEST_BAND_868)
        Serial.print("  (Meshtastic frame, encrypted) hex: ");
        for (int i = 0; i < len && i < 16; i++) Serial.printf("%02X ", buf[i]);
        Serial.println(len > 16 ? "..." : "");
#else
        // LoRa-APRS frames are prefixed with 3 bytes: '<' 0xFF 0x01
        int off = (len >= 3 && buf[0] == '<' && buf[1] == 0xFF && buf[2] == 0x01) ? 3 : 0;
        Serial.print("  APRS: ");
        for (int i = off; i < len; i++)
          Serial.write((buf[i] >= 32 && buf[i] < 127) ? buf[i] : '.');
        Serial.println();
#endif
      } else {
        Serial.printf("[LoRa] readData err %d\n", st);
      }
    }
#ifdef TEST_BAND_868
    else {  // idle timeout -> hop to next EU868 channel (all radio ops in this task)
      fidx = (fidx + 1) % 3;
      radio.standby();
      radio.setFrequency(TEST_FREQS[fidx]);
      radio.startReceive();
      Serial.printf("[LoRa] listening %.3f MHz (SF11 BW250 — Meshtastic test)\n",
                    TEST_FREQS[fidx]);
    }
#endif
  }
}

// ------------------------------------------------------------- Network task
void netTask(void *) {
  // Reset the on-board W5500
  pinMode(PIN_ETH_RST, OUTPUT);
  digitalWrite(PIN_ETH_RST, LOW);  vTaskDelay(pdMS_TO_TICKS(10));
  digitalWrite(PIN_ETH_RST, HIGH); vTaskDelay(pdMS_TO_TICKS(60));

  // arduino-libraries/Ethernet uses the global `SPI` (= SPI0 on arduino-pico)
  SPI.setRX(PIN_ETH_MISO);
  SPI.setSCK(PIN_ETH_SCK);
  SPI.setTX(PIN_ETH_MOSI);
  Ethernet.init(PIN_ETH_CS);

  // Locally-administered MAC; last octet differs per build env (MAC_ID).
  mac[0] = 0x02;  // locally administered, unicast
  mac[1] = 0x00; mac[2] = 0x4C; mac[3] = 0x47; mac[4] = 0x00; mac[5] = MAC_ID;
  Serial.printf("[ETH] MAC %02X:%02X:%02X:%02X:%02X:%02X\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  Serial.println("[ETH] DHCP...");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("[ETH] DHCP FAILED (check link/cable)");
  } else {
    Serial.print("[ETH] IP: ");
    Serial.println(Ethernet.localIP());
  }
  ethWebSetup();   // HTTP config server on :80 (same task owns the W5500/SPI0)
  uint32_t lastMaintain = millis();
  for (;;) {
    ethWebPoll();
    if (millis() - lastMaintain > 5000) { Ethernet.maintain(); lastMaintain = millis(); }
    vTaskDelay(pdMS_TO_TICKS(15));
  }
}

// ---------------------------------------------- LoRa TX task (LINK test only)
#if defined(LINK_TEST) && defined(ROLE_TX)
void txTask(void *) {
  uint32_t n = 0;
  char msg[64];
  for (;;) {
    int len = snprintf(msg, sizeof(msg), "iGate RP2350 link test #%lu", (unsigned long)n++);
    Serial.printf("[LoRa] TX \"%s\" ... ", msg);
    int st = radio.transmit((uint8_t *)msg, len);   // blocks for the airtime
    Serial.printf("%s (%d)\n", st == RADIOLIB_ERR_NONE ? "sent" : "ERR", st);
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}
#endif

// ------------------------------------------------------------------- setup
void setup() {
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && (millis() - t0 < 3000)) {}
  Serial.println("\n=== iGate RP2350 — Phase 1 bring-up ===");

  Config.setup();   // mount LittleFS + create/read /igate_conf.json (defaults on first boot)

  pinMode(HB_LED, OUTPUT);   // onboard LED — life indicator (blinks in loop())

  // E22P RFEN must be HIGH for the RF front-end (LNA on RX, PA on TX)
  pinMode(PIN_LORA_RFEN, OUTPUT);
  digitalWrite(PIN_LORA_RFEN, HIGH);

  // Radio bus on SPI1 — match Meshtastic's init exactly (pins + explicit begin)
  SPI1.setSCK(PIN_LORA_SCK);
  SPI1.setTX(PIN_LORA_MOSI);
  SPI1.setRX(PIN_LORA_MISO);
  SPI1.begin(false);   // RadioLib expects the bus already begun (false = no HW CS)

  Serial.print("[LoRa] init... ");
  int st = radio.begin(LORA_FREQ_MHZ, LORA_BW_KHZ, LORA_SF, LORA_CR,
                       LORA_SYNC, LORA_TX_DBM, LORA_PREAMB, LORA_TCXO_V, false);
  if (st != RADIOLIB_ERR_NONE) {     // DC-DC failed -> some E22 modules need LDO
    Serial.printf("DC-DC begin failed (%d), retrying LDO... ", st);
    st = radio.begin(LORA_FREQ_MHZ, LORA_BW_KHZ, LORA_SF, LORA_CR,
                     LORA_SYNC, LORA_TX_DBM, LORA_PREAMB, LORA_TCXO_V, true);
  }
  g_radioSt = st;
  if (st == RADIOLIB_ERR_NONE) {
    radio.setDio2AsRfSwitch(true);   // E22P DIO2->TXEN bridge (enables PA on TX)
#if defined(LINK_TEST) && defined(ROLE_TX)
    Serial.printf("OK — TX role @%.3f MHz, %d dBm (chip)\n", LORA_FREQ_MHZ, LORA_TX_DBM);
#else
    radio.setDio1Action(onDio1);
    radio.startReceive();
    Serial.printf("OK — RX listening %.3f MHz\n", LORA_FREQ_MHZ);
#endif
  } else {
    // common: -2 (chip not found / SPI), -707 (SPI cmd timeout), TCXO issues
    Serial.printf("FAILED code %d  (check wiring / TCXO / useRegulatorLDO)\n", st);
  }

  rxSem = xSemaphoreCreateBinary();
#if defined(LINK_TEST) && defined(ROLE_TX)
  xTaskCreate(txTask,   "tx",   4096, nullptr, 3, nullptr);
#else
  xTaskCreate(loraTask, "lora", 4096, nullptr, 3, nullptr);
#endif
  xTaskCreate(netTask,  "net",  4096, nullptr, 2, nullptr);
}

void loop() {
  // Blink the onboard LED ~2 Hz as a monitor-independent proof of life, and
  // print a serial heartbeat every ~3 s (the boot banner is printed once and
  // the RX role is otherwise silent until a packet arrives).
  static uint32_t tick = 0;
  digitalWrite(HB_LED, (tick & 1) ? HIGH : LOW);
  if (++tick % 12 == 0) {
    IPAddress ip = Ethernet.localIP();
    Serial.printf("[hb #%lu] up=%lus  ip=%d.%d.%d.%d  radio=%d\n",
                  (unsigned long)(tick / 12), (unsigned long)(millis() / 1000),
                  ip[0], ip[1], ip[2], ip[3], g_radioSt);
  }
  vTaskDelay(pdMS_TO_TICKS(250));
}

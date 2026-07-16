/*
 * RP2350 implementation of the LoRa_Utils API (include/lora_utils.h), reusing
 * the validated E22P / SX1262 radio bring-up (SPI1, DIO2-as-RF-switch, TCXO).
 * Replaces the ESP32-centric src/lora_utils.cpp (excluded from the RP2350
 * build_src_filter). LoRa-APRS framing: on-air packets are prefixed with the
 * 3 bytes '<' 0xFF 0x01 (OE5BPA standard).
 */
#include "lora_utils.h"
#include <RadioLib.h>
#include <SPI.h>
#include "configuration.h"
#include "board_pinout.h"

extern Configuration Config;

// File-scope (static) so it doesn't clash with the inline radio still in
// rp2350/main.cpp until the iGate loop replaces it.
// HAS_SX1268 selects the EBYTE E22-400M30S (433 MHz, SX1268 silicon); the
// default is the SX1262 used by the E22(P)-868/900M30S modules. Both RadioLib
// classes share the SX126x command set, so the rest of this file is identical.
#if defined(HAS_SX1268)
static SX1268 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN, SPI1);
#else
static SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN, SPI1);
#endif
static volatile bool rxFlag = false;

// How often to warm-reset the SX126x analog frontend / AGC state (see resetAGC).
// Ported from upstream PR #440 (ndoo / Andrew Yong), itself from Meshtastic.
#define AGC_RESET_INTERVAL_MS 60000

// Signal stats of the last received packet (read by the ?APRSSR query; the
// original firmware keeps these as globals, so the query module externs them).
int   rssi      = 0;
float snr       = 0;
int   freqError = 0;
static float rxFreqMHz = 433.775f;
static float txFreqMHz = 433.775f;

static void onLoraDio1() { rxFlag = true; }

namespace LoRa_Utils {

// Undocumented Heltec/Semtech-recommended SX126x register patch (bit 0 of 0x8B5)
// that measurably reduces packet loss. Needs RADIOLIB_LOW_LEVEL=1 for getMod().
// Calibrate(0x7F) clears it, so resetAGC() re-applies it after every calibration.
static void applyRxSensitivityPatch() {
    if (radio.getMod()->SPIsetRegValue(0x8B5, 0x01, 0, 0) == RADIOLIB_ERR_NONE)
        Serial.println("[lora] applied SX126x 0x8B5 RX-sensitivity patch");
    else
        Serial.println("[lora] FAILED to apply SX126x 0x8B5 RX-sensitivity patch");
}

// SX126x has no true AGC; the frontend gain can get stuck low after prolonged RX.
// Periodically warm-reset it (warm sleep + full calibration), skipping if a packet
// is actively arriving. Called from receivePacket() only (loraTask owns SPI1).
static void resetAGC() {
    uint32_t irqFlags = radio.getIrqFlags();
    if (irqFlags & (RADIOLIB_SX126X_IRQ_HEADER_VALID | RADIOLIB_SX126X_IRQ_PREAMBLE_DETECTED))
        return;  // packet actively arriving, don't disturb it

    radio.sleep(true);                                  // warm sleep - resets the analog frontend / AGC state
    radio.standby(RADIOLIB_SX126X_STANDBY_RC, true);    // wake to RC standby for stable calibration

    uint8_t calData = RADIOLIB_SX126X_CALIBRATE_ALL;
    radio.getMod()->SPIwriteStream(RADIOLIB_SX126X_CMD_CALIBRATE, &calData, 1, true, false);

    radio.getMod()->hal->delay(5);
    uint32_t calibrationStart = millis();
    while (radio.getMod()->hal->digitalRead(radio.getMod()->getGpio())) {
        if (millis() - calibrationStart > 50) {
            Serial.println("[lora] SX126x AGC reset: calibration did not complete within 50ms");
            break;
        }
        radio.getMod()->hal->yield();
    }

    float freq = (float)Config.loramodule.rxFreq / 1000000;
    radio.calibrateImage(freq);         // Calibrate(0x7F) defaults image cal to the wrong band otherwise

    radio.setRxBoostedGainMode(true);   // re-apply settings that Calibrate(0x7F) resets
    applyRxSensitivityPatch();

    radio.startReceive();
}

void setup() {
    // RF front-end control depends on how TXEN is wired (see board_pinout.h):
    //   RADIO_TXEN >= 0  -> canonical E22-400M30S / E22P-433M30S design (datasheet
    //                       §4.1/§5.2): TXEN and RXEN are separate MCU-driven lines,
    //                       DIO2 floating. RadioLib toggles both switch pins around
    //                       every RX/TX transition (LNA off in TX, PA off in RX).
    //   RADIO_TXEN <  0  -> bridged front-end (on-module DIO2->TXEN): a single RFEN
    //                       (RXEN) line held HIGH, dropped by hand during transmit.
    //                       Used on carriers where TXEN can't get its own MCU pin
    //                       (e.g. the W5100S-EVB carrier, which can't be rewired).
#if RADIO_TXEN < 0
    pinMode(RADIO_RXEN, OUTPUT);
    digitalWrite(RADIO_RXEN, HIGH);            // bridged: RFEN held HIGH while active
#endif
    SPI1.setSCK(RADIO_SCLK_PIN);
    SPI1.setTX(RADIO_MOSI_PIN);
    SPI1.setRX(RADIO_MISO_PIN);
    SPI1.begin(false);

    // Deliberate E22 reset with the RF front-end held OFF (PA + LNA), so a *software*
    // reboot (config save -> rp2040.restart, which does NOT power-cycle the module)
    // recovers a module left in a bad state instead of needing a physical power
    // cycle. RadioLib's begin() also resets, but a longer, front-end-safe pulse here
    // is more reliable after a glitchy restart.
#if RADIO_TXEN >= 0
    pinMode(RADIO_TXEN, OUTPUT); digitalWrite(RADIO_TXEN, LOW);   // PA off during reset
    pinMode(RADIO_RXEN, OUTPUT); digitalWrite(RADIO_RXEN, LOW);   // LNA off during reset
#endif
    pinMode(RADIO_RST_PIN, OUTPUT);
    digitalWrite(RADIO_RST_PIN, LOW);  delay(5);
    digitalWrite(RADIO_RST_PIN, HIGH); delay(5);

#ifdef LORA_DIAG
    Serial.println("[diag] ==== LoRa bring-up diagnostic ====");
    // Low-level reach-the-chip probe (separates "dead/stuck chip" from "bad MISO/SPI").
    pinMode(RADIO_BUSY_PIN, INPUT);
    pinMode(RADIO_RST_PIN, OUTPUT);
    pinMode(RADIO_CS_PIN, OUTPUT);
    digitalWrite(RADIO_CS_PIN, HIGH);
    Serial.printf("[diag] pins CS=%d SCK=%d MOSI=%d MISO=%d RST=%d BUSY=%d DIO1=%d\n", RADIO_CS_PIN,
                  RADIO_SCLK_PIN, RADIO_MOSI_PIN, RADIO_MISO_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN, RADIO_DIO1_PIN);
    Serial.printf("[diag] BUSY before reset = %d\n", digitalRead(RADIO_BUSY_PIN));
    digitalWrite(RADIO_RST_PIN, LOW);
    delay(2);
    Serial.printf("[diag] BUSY while NRST low = %d\n", digitalRead(RADIO_BUSY_PIN));
    digitalWrite(RADIO_RST_PIN, HIGH);
    delay(1);
    int bA = digitalRead(RADIO_BUSY_PIN);
    delay(10);
    int bB = digitalRead(RADIO_BUSY_PIN);
    // A live SX126x pulses BUSY high during boot then drops it LOW (ready) ~1-2 ms
    // after NRST is released. BUSY stuck HIGH => chip not running (dead/power/clock).
    Serial.printf("[diag] BUSY after reset: +1ms=%d +11ms=%d  (expect ->0 if chip alive)\n", bA, bB);
    // Raw GetStatus (opcode 0xC0): clock out the status byte. 0x00 or 0xFF on MISO
    // = no chip / dead MISO path; anything else = SPI link to the die is alive.
    SPI1.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
    digitalWrite(RADIO_CS_PIN, LOW);
    uint8_t op = SPI1.transfer(0xC0);
    uint8_t stbyte = SPI1.transfer(0x00);
    digitalWrite(RADIO_CS_PIN, HIGH);
    SPI1.endTransaction();
    Serial.printf("[diag] SX126x GetStatus raw: op=%02X status=%02X  (00/FF = mudo)\n", op, stbyte);
    // Version string (REG 0x0320, 16 bytes). RadioLib findChip() strncmp's the
    // first 6 chars vs the class name -> a "SX1262" here is why the SX1268 class
    // rejected this E22-400M30S with -2.
    {
        uint8_t ver[17] = {0};
        SPI1.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
        digitalWrite(RADIO_CS_PIN, LOW);
        SPI1.transfer(0x1D);  // ReadRegister opcode
        SPI1.transfer(0x03);  // addr hi (0x0320)
        SPI1.transfer(0x20);  // addr lo
        SPI1.transfer(0x00);  // status NOP
        for (int k = 0; k < 16; k++) ver[k] = SPI1.transfer(0x00);
        digitalWrite(RADIO_CS_PIN, HIGH);
        SPI1.endTransaction();
        Serial.printf("[diag] chip version string = '%s'\n", (char *)ver);
    }
#endif

    rxFreqMHz = (float)Config.loramodule.rxFreq / 1000000.0f;
    txFreqMHz = (float)Config.loramodule.txFreq / 1000000.0f;
    float bw  = (float)Config.loramodule.rxSignalBandwidth / 1000.0f;

    // The E22(P)-xxxM30S carries its own 30 dBm PA; driving the SX126x above 20 dBm
    // overdrives it -> ~690 mA current spike -> supply brownout that wedges the
    // module in a low-sensitivity state until a physical power cycle (verified on
    // the .243 W5100S: power=22 went deaf, cold-boot recovered ~10 dB). Cap the
    // SX126x drive at 20 dBm — no useful range is gained above it on this front-end.
    if (Config.loramodule.power > 20) Config.loramodule.power = 20;

    int st = radio.begin(rxFreqMHz, bw, Config.loramodule.rxSpreadingFactor,
                         Config.loramodule.rxCodingRate4, 0x12, Config.loramodule.power,
                         8, SX126X_DIO3_TCXO_VOLTAGE, false);
    if (st != RADIOLIB_ERR_NONE) {            // some E22 modules need the LDO regulator
        Serial.printf("[lora] begin %d -> retry LDO\n", st);
        st = radio.begin(rxFreqMHz, bw, Config.loramodule.rxSpreadingFactor,
                         Config.loramodule.rxCodingRate4, 0x12, Config.loramodule.power,
                         8, SX126X_DIO3_TCXO_VOLTAGE, true);
    }
#if RADIO_TXEN < 0
    radio.setDio2AsRfSwitch(true);             // bridged: DIO2 -> TXEN on-module (DIO2 wired)
#else
    // Canonical front-end: DIO2 is left floating; RadioLib drives the external
    // TXEN/RXEN switch pins itself (mutually exclusive) on every RX/TX transition.
    radio.setRfSwitchPins(RADIO_RXEN, RADIO_TXEN);
#endif
    radio.setRxBoostedGainMode(true);          // boosted LNA gain (higher RX sensitivity)
    applyRxSensitivityPatch();                 // undocumented 0x8B5 RX patch (upstream PR #440)
    radio.setDio1Action(onLoraDio1);
    radio.startReceive();
    Serial.printf("[lora] %s @%.4f MHz SF%d BW%.0f CR4:%d (state %d)\n",
                  st == RADIOLIB_ERR_NONE ? "RX listening" : "FAILED", rxFreqMHz,
                  Config.loramodule.rxSpreadingFactor, bw, Config.loramodule.rxCodingRate4, st);
}

String receivePacket() {
    // Periodic AGC warm-reset (upstream PR #440). Only when no packet is pending —
    // resetAGC() sleeps/calibrates the radio, which would drop an already-received
    // (RxDone) packet waiting in the FIFO. It also bails internally if one is
    // mid-arrival (HEADER_VALID / PREAMBLE_DETECTED).
#ifdef LORA_RX_POLL
    bool rxPending = radio.getIrqFlags() & RADIOLIB_SX126X_IRQ_RX_DONE;
#else
    bool rxPending = rxFlag;
#endif
    static uint32_t lastAgcResetTime = 0;
    if (!rxPending && millis() - lastAgcResetTime > AGC_RESET_INTERVAL_MS) {
        resetAGC();
        lastAgcResetTime = millis();
    }

#ifdef LORA_RX_POLL
    // Optional fallback for a carrier that genuinely doesn't route the SX126x
    // DIO1 RxDone interrupt to the MCU: poll the IRQ register over SPI instead of
    // relying on the DIO1 interrupt / rxFlag. Not needed on the current boards —
    // the W5100S carrier was verified (isr fires on TxDone and RxDone, GP14 goes
    // high) to deliver the DIO1 IRQ once the DIO2->TXEN bridge was cut and the
    // canonical RF switch (setRfSwitchPins) is used.
    if (!(radio.getIrqFlags() & RADIOLIB_SX126X_IRQ_RX_DONE)) return "";
#else
    if (!rxFlag) return "";
#endif
    rxFlag = false;
    String str;
    int st = radio.readData(str);
    rssi      = (int)radio.getRSSI();
    snr       = radio.getSNR();
    freqError = (int)radio.getFrequencyError();
    radio.startReceive();                       // re-arm
    if (st != RADIOLIB_ERR_NONE) return "";
    // Return the RAW packet WITH its 3-byte LoRa-APRS header ('<' 0xFF 0x01) —
    // the iGate processing/qAR build does packet.substring(3) to skip it.
    return str;
}

String receivePacketFromSleep() { return receivePacket(); }

void changeFreqTx() { radio.standby(); radio.setFrequency(txFreqMHz); }
void changeFreqRx() { radio.standby(); radio.setFrequency(rxFreqMHz); radio.startReceive(); }

void sendNewPacket(const String& newPacket) {
    if (!Config.loramodule.txActive) return;   // RF TX disabled (RX-only iGate / ?TX=OFF)
    changeFreqTx();
#if RADIO_TXEN < 0
    digitalWrite(RADIO_RXEN, LOW);             // bridged: RX path off; DIO2 drives TXEN during transmit
#endif
    String tx;
    tx.reserve(newPacket.length() + 3);
    tx += '<';
    tx += (char)0xFF;
    tx += (char)0x01;
    tx += newPacket;
    int st = radio.transmit(tx);
    Serial.printf("[lora] TX %d B (state %d)\n", (int)tx.length(), st);
#if RADIO_TXEN < 0
    digitalWrite(RADIO_RXEN, HIGH);            // bridged: restore RX path
#endif
    changeFreqRx();
    // DIO1 fires on TxDone as well as RxDone, so transmit() just set rxFlag.
    // Clear it (after re-arming RX) so the next receivePacket() doesn't read the
    // stale TX FIFO back as a bogus "received" packet (self-RX -> duplicate gate).
    rxFlag = false;
}

void wakeRadio()  { radio.standby(); radio.startReceive(); }
void sleepRadio() { radio.sleep(); }

}  // namespace LoRa_Utils

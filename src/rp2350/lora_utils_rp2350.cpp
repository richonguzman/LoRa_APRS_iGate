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

// Signal stats of the last received packet (read by the ?APRSSR query; the
// original firmware keeps these as globals, so the query module externs them).
int   rssi      = 0;
float snr       = 0;
int   freqError = 0;
static float rxFreqMHz = 433.775f;
static float txFreqMHz = 433.775f;

static void onLoraDio1() { rxFlag = true; }

namespace LoRa_Utils {

void setup() {
    pinMode(RADIO_RXEN, OUTPUT);
    digitalWrite(RADIO_RXEN, HIGH);            // E22P RFEN: held HIGH while active
    SPI1.setSCK(RADIO_SCLK_PIN);
    SPI1.setTX(RADIO_MOSI_PIN);
    SPI1.setRX(RADIO_MISO_PIN);
    SPI1.begin(false);

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

    int st = radio.begin(rxFreqMHz, bw, Config.loramodule.rxSpreadingFactor,
                         Config.loramodule.rxCodingRate4, 0x12, Config.loramodule.power,
                         8, SX126X_DIO3_TCXO_VOLTAGE, false);
    if (st != RADIOLIB_ERR_NONE) {            // some E22 modules need the LDO regulator
        Serial.printf("[lora] begin %d -> retry LDO\n", st);
        st = radio.begin(rxFreqMHz, bw, Config.loramodule.rxSpreadingFactor,
                         Config.loramodule.rxCodingRate4, 0x12, Config.loramodule.power,
                         8, SX126X_DIO3_TCXO_VOLTAGE, true);
    }
    radio.setDio2AsRfSwitch(true);             // E22P DIO2 -> TXEN bridge
    radio.setDio1Action(onLoraDio1);
    radio.startReceive();
    Serial.printf("[lora] %s @%.4f MHz SF%d BW%.0f CR4:%d (state %d)\n",
                  st == RADIOLIB_ERR_NONE ? "RX listening" : "FAILED", rxFreqMHz,
                  Config.loramodule.rxSpreadingFactor, bw, Config.loramodule.rxCodingRate4, st);
}

String receivePacket() {
    if (!rxFlag) return "";
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
    digitalWrite(RADIO_RXEN, LOW);             // E22 RX path off; DIO2 drives TXEN during transmit
    String tx;
    tx.reserve(newPacket.length() + 3);
    tx += '<';
    tx += (char)0xFF;
    tx += (char)0x01;
    tx += newPacket;
    int st = radio.transmit(tx);
    Serial.printf("[lora] TX %d B (state %d)\n", (int)tx.length(), st);
    digitalWrite(RADIO_RXEN, HIGH);            // restore RX path
    changeFreqRx();
    // DIO1 fires on TxDone as well as RxDone, so transmit() just set rxFlag.
    // Clear it (after re-arming RX) so the next receivePacket() doesn't read the
    // stale TX FIFO back as a bogus "received" packet (self-RX -> duplicate gate).
    rxFlag = false;
}

void wakeRadio()  { radio.standby(); radio.startReceive(); }
void sleepRadio() { radio.sleep(); }

}  // namespace LoRa_Utils

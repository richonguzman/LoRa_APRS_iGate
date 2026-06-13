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
static SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN, SPI1);
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

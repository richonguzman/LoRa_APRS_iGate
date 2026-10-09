/* Copyright (C) 2026 Ricardo Guzman - CA2RXU
 *
 * This file is part of LoRa APRS iGate.
 *
 * LoRa APRS iGate is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LoRa APRS iGate is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LoRa APRS iGate. If not, see <https://www.gnu.org/licenses/>.
 */

#include <APRSPacketLib.h>
#include <RadioLib.h>
#include "configuration.h"
#include "network_manager.h"
#include "aprs_is_utils.h"
#include "station_utils.h"
#include "board_pinout.h"
#include "syslog_utils.h"
#include "map_utils.h"
#include "ntp_utils.h"
#include "display.h"
#include "utils.h"
#include "thermal_utils.h"


extern Configuration    Config;
extern NetworkManager   *networkManager;
extern bool             packetIsBeacon;

extern std::vector<ReceivedPacket> receivedPackets;

bool operationDone      = true;
bool transmitFlag       = true;

#define DIFS_SLOTS      2       // Number of secuential CAD slots to consider a free channel to Tx
int  backoffMax         = 4;    // Max Backoff value (number of CAD slots to wait before Tx)
#define CAD_MAX_WAIT_MS 10000   // Max total time waiting for a free channel, then the packet is dropped
unsigned long cadStartTime = 0;

#ifdef HAS_SX1262
    SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
#endif
#ifdef HAS_SX1268
    #if defined(LIGHTGATEWAY_1_0) || defined(LIGHTGATEWAY_PLUS_1_0)
        SPIClass loraSPI(FSPI);
        SX1268 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN, loraSPI);
    #else
        SX1268 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
    #endif
#endif
#ifdef HAS_SX1278
    SX1278 radio = new Module(RADIO_CS_PIN, RADIO_BUSY_PIN, RADIO_RST_PIN);
#endif
#ifdef HAS_SX1276
    SX1276 radio = new Module(RADIO_CS_PIN, RADIO_BUSY_PIN, RADIO_RST_PIN);
#endif
#if defined(HAS_LLCC68)         //LLCC68 supports spreading factor only in range of 5-11!
    LLCC68 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
#endif

int rssi, freqError;
float snr;
APRSPacket lastAprsPacket;

#if defined(HAS_SX1278) || defined(HAS_SX1276)
    #define CHIP_MIN_POWER  2       // PA_BOOST (below 2 RadioLib switches to RFO, not wired on most modules)
    #define CHIP_MAX_POWER  20
#else                               // SX1262 / SX1268 / LLCC68
    #define CHIP_MIN_POWER  -9
    #define CHIP_MAX_POWER  22
#endif

#ifndef RADIO_MAX_POWER             // optional per board in board_pinout.h (e.g. 1W PA modules)
    #define RADIO_MAX_POWER CHIP_MAX_POWER
#endif


namespace LoRa_Utils {

    static String sanitizeForWeb(const String& s) {     // replaces non-printable ASCII with '.' for WebUI display
        String out = s;
        for (int i = 0; i < (int)out.length(); i++) {
            uint8_t c = (uint8_t)out[i];
            if (c < 32 || c == 127) out.setCharAt(i, '.');
        }
        return out;
    }

    void setFlag(void) {
        operationDone = true;
    }

    int validPower(int requested) {
        const int maxPower = (RADIO_MAX_POWER < CHIP_MAX_POWER) ? RADIO_MAX_POWER : CHIP_MAX_POWER;
        int power = constrain(requested, CHIP_MIN_POWER, maxPower);
        #if defined(HAS_SX1278) || defined(HAS_SX1276)
            if (power > 17 && power < 20) power = 17;   // SX127x PA_BOOST: only 2-17 or 20
        #endif
        return power;
    }

    void setup() {
        #ifdef RADIO_VCC_PIN        // LoRa module power switch (QRP Labs LightGateway, T-Beam 1W)
            pinMode(RADIO_VCC_PIN, OUTPUT);
            digitalWrite(RADIO_VCC_PIN, HIGH);
        #endif
        #if defined (LIGHTGATEWAY_1_0) || defined(LIGHTGATEWAY_PLUS_1_0)
            loraSPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN, RADIO_CS_PIN);
        #else
            SPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN);
        #endif
        #ifdef RADIO_ANT_SW_PIN     // RAK3312 antenna switch needs power
            pinMode(RADIO_ANT_SW_PIN, OUTPUT);
            digitalWrite(RADIO_ANT_SW_PIN, RADIO_ANT_SW_ON_STATE);
        #endif
        float freq = (float)Config.loramodule.rxFreq / 1000000;
        #if defined(RADIO_HAS_XTAL)
            radio.XTAL = true;
        #endif
        #if (defined(RADIO_RXEN) && defined(RADIO_TXEN))    // before begin() so RF switch is driven from start (Ebyte E22/E32 1W, QRP Labs LightGateway)
            radio.setRfSwitchPins(RADIO_RXEN, RADIO_TXEN);
        #elif defined(RADIO_RXEN)                           // T-Beam 1W: RXEN = LNA only, DIO2 drives the PA
            radio.setRfSwitchPins(RADIO_RXEN, RADIOLIB_NC);
        #endif
        int state = radio.begin(freq);
        if (state != RADIOLIB_ERR_NONE) {
            Utils::println("Starting LoRa failed! State: " + String(state));
            while (true);
        }
        #if defined(HAS_SX1262) || defined(HAS_SX1268) || defined(HAS_LLCC68)
            radio.setDio1Action(setFlag);
        #endif
        #if defined(HAS_SX1278) || defined(HAS_SX1276)
            radio.setDio0Action(setFlag, RISING);
        #endif

        /*#ifdef SX126X_DIO3_TCXO_VOLTAGE
            if (radio.setTCXO(float(SX126X_DIO3_TCXO_VOLTAGE)) == RADIOLIB_ERR_NONE) {
                Utils::println("Set LoRa Module TCXO Voltage to:" + String(SX126X_DIO3_TCXO_VOLTAGE));
            } else {
                Utils::println("Set LoRa Module TCXO Voltage failed! State: " + String(state));
                while (true);
        }
         #endif*/

        radio.setSpreadingFactor(Config.loramodule.rxSpreadingFactor);
        radio.setCodingRate(Config.loramodule.rxCodingRate4);
        float signalBandwidth = Config.loramodule.rxSignalBandwidth / 1000;
        radio.setBandwidth(signalBandwidth);
        radio.setCRC(true);

        int power = validPower(Config.loramodule.power);
        if (power != Config.loramodule.power) {
            Utils::println("LoRa power adjusted: " + String(Config.loramodule.power) + " -> " + String(power));
        }
        state = radio.setOutputPower(power);
        if (state != RADIOLIB_ERR_NONE) {
            Utils::println("LoRa setOutputPower failed! State: " + String(state));     // log and keep going
        }
        #if defined(HAS_SX1278) || defined(HAS_SX1276)
            radio.setCurrentLimit(120); // OCP ceiling for SX127x: ~120mA needed at +20dBm (not a fixed consumption)
        #else                           // SX1262 / SX1268 / LLCC68 (also 1W Ebyte E22 / E220 modules)
            radio.setCurrentLimit(140);
        #endif

        #if defined(HAS_SX1262) || defined(HAS_SX1268) || defined(HAS_LLCC68)
            radio.setRxBoostedGainMode(true);
        #endif

        #if defined(HAS_TCXO) && !defined(HAS_1W_LORA)
            radio.setDio2AsRfSwitch();
        #endif
        #if defined(TTGO_T_BEAM_1W)
            radio.setDio2AsRfSwitch(true);                          // DIO2 drives the 1W PA
            radio.setPaRampTime(RADIOLIB_SX126X_PA_RAMP_800U);      // PA needs >800us to settle (default 200us)
        #endif
        #ifdef HAS_TCXO
            radio.setTCXO(1.8);
        #endif

        Utils::println("init : LoRa Module    ...     done!");
    }

    void changeFreqTx() {
        float freq = (float)Config.loramodule.txFreq / 1000000;
        radio.setFrequency(freq);
        radio.setSpreadingFactor(Config.loramodule.txSpreadingFactor);
        radio.setCodingRate(Config.loramodule.txCodingRate4);
        float signalBandwidth = Config.loramodule.txSignalBandwidth / 1000;
        radio.setBandwidth(signalBandwidth);
    }

    void changeFreqRx() {
        float freq = (float)Config.loramodule.rxFreq / 1000000;
        radio.setFrequency(freq);
        radio.setSpreadingFactor(Config.loramodule.rxSpreadingFactor);
        radio.setCodingRate(Config.loramodule.rxCodingRate4);
        float signalBandwidth = Config.loramodule.rxSignalBandwidth / 1000;
        radio.setBandwidth(signalBandwidth);
    }

    bool doCAD() {      // CAD (Channel Activity Detection)
        return radio.scanChannel() != RADIOLIB_CHANNEL_FREE;    // false=channel free | true=RADIOLIB_LORA_DETECTED or CAD failed
    }

    bool doDIFS() {
        for (uint8_t i = DIFS_SLOTS; i > 0; i--) {
            if (doCAD()) return false;
        }
        return true;
    }

    bool cadTimedOut() {
        return millis() - cadStartTime > CAD_MAX_WAIT_MS;
    }

    void waitForDIFS() {
        while (!doDIFS()) {
            if (cadTimedOut()) return;
            Serial.println("CAD/DIFS failed, retry...");
        }
    }

    void doBEB() {
        int backoffCounter = random(1, backoffMax + 1);
        while (backoffCounter > 0) {
            if (cadTimedOut()) return;
            if (doCAD()) {
                waitForDIFS();  // busy channel: freeze backoff and restart DIFS
            } else {
                backoffCounter--;
            }
        }
    }

    void sendNewPacket(const String& newPacket) {
        if (!Config.loramodule.txActive) return;
        #ifdef FAN_CTRL_PIN
            if (THERMAL_Utils::isTxBlocked()) {
                Utils::println("Thermal: Tx blocked (over-temperature), packet dropped: " + newPacket);
                return;
            }
        #endif

        if (Config.loramodule.txFreq != Config.loramodule.rxFreq) {
            if (!packetIsBeacon || (packetIsBeacon && Config.beacon.beaconFreq == 1)) {
                changeFreqTx();
            }
        }

        #ifdef INTERNAL_LED_PIN
            if (Config.digi.ecoMode != 1) digitalWrite(INTERNAL_LED_PIN, HIGH);     // disabled in Ultra Eco Mode
        #endif

        bool cadDropped = false;
        if (Config.loramodule.cadActive) {
            cadStartTime = millis();
            waitForDIFS();  // DIFS (Distributed Inter-Frame Space)
            doBEB();        // BEB  (Binary Exponential Backoff)
            if (cadTimedOut()) {
                Utils::println("CAD timeout, packet dropped: " + newPacket);
                cadDropped = true;
            }
        }

        if (!cadDropped) {
            #ifdef FAN_CTRL_PIN
                THERMAL_Utils::onTxStart();
            #endif
            int state = radio.transmit("\x3c\xff\x01" + newPacket);
            #ifdef FAN_CTRL_PIN
                THERMAL_Utils::onTxEnd();
            #endif
            transmitFlag = true;
            if (state == RADIOLIB_ERR_NONE) {
                if (Config.syslog.active && networkManager->isConnected()) {
                    SYSLOG_Utils::logLoRaTx(newPacket);
                }
                Utils::print("---> LoRa Packet Tx : ");
                Utils::println(newPacket);
            } else {
                Utils::print(F("failed, code "));
                Utils::println(String(state));
            }
        }
        #ifdef INTERNAL_LED_PIN
            if (Config.digi.ecoMode != 1) digitalWrite(INTERNAL_LED_PIN, LOW);      // disabled in Ultra Eco Mode
        #endif
        if (Config.loramodule.txFreq != Config.loramodule.rxFreq) {
            if (!packetIsBeacon || (packetIsBeacon && Config.beacon.beaconFreq == 1)) {
                changeFreqRx();
            }
        }
        if (cadDropped) radio.startReceive();   // no Tx end IRQ will restart Rx
    }

    String receivePacketFromSleep() {
        String packet = "";
        int state = radio.readData(packet);
        if (state == RADIOLIB_ERR_NONE) {
            Utils::println("<--- LoRa Packet Rx : " + packet.substring(3));
        } else {
            packet = "";
        }
        return packet;
    }

    String receivePacket() {
        String packet = "";
        if (operationDone) {
            operationDone = false;
            if (transmitFlag) {
                radio.startReceive();
                transmitFlag = false;
            } else {
                int state = radio.readData(packet);
                if (state == RADIOLIB_ERR_NONE) {
                    if (packet != "") {

                        String sender   = packet.substring(3, packet.indexOf(">"));
                        if (packet.substring(0,3) == "\x3c\xff\x01" && !STATION_Utils::isBlacklisted(sender)) {     // avoid processing BlackListed stations
                            rssi        = radio.getRSSI();
                            snr         = radio.getSNR();
                            freqError   = radio.getFrequencyError();
                            Utils::println("<--- LoRa Packet Rx (RSSI:" + String(rssi) + " | SNR:" + String(snr) + " | FreqErr:" + String(freqError) + ") : " + packet.substring(3));

                            lastAprsPacket = APRSPacketLib::processReceivedPacket(packet.substring(3), rssi, snr, freqError);
                            if (Config.digi.ecoMode == 0) {
                                if (receivedPackets.size() >= 10) {
                                    receivedPackets.erase(receivedPackets.begin());
                                }
                                ReceivedPacket receivedPacket;
                                receivedPacket.rxDate   = NTP_Utils::getFormatedDate();
                                receivedPacket.rxTime   = NTP_Utils::getFormatedTime();
                                receivedPacket.packet   = sanitizeForWeb(packet.substring(3));
                                receivedPacket.RSSI     = rssi;
                                receivedPacket.SNR      = snr;
                                receivedPackets.push_back(receivedPacket);

                                if (lastAprsPacket.type == 0 || lastAprsPacket.type == 4) {   // 0 = GPS, 4 = Mic-E (only ones with position)
                                    MAP_Utils::upsert(lastAprsPacket.sender, lastAprsPacket.latitude, lastAprsPacket.longitude, lastAprsPacket.path, lastAprsPacket.overlay + lastAprsPacket.symbol, lastAprsPacket.rssi, lastAprsPacket.snr);
                                }
                            }

                            if (Config.syslog.active && networkManager->isConnected()) {
                                SYSLOG_Utils::logLoRaRx(lastAprsPacket, packet, rssi, snr, freqError); // RX
                            }
                        } else {
                            packet = "";
                        }
                        return packet;
                    }
                } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
                    rssi        = radio.getRSSI();
                    snr         = radio.getSNR();
                    freqError   = radio.getFrequencyError();
                    Utils::println(F("CRC error!"));
                    if (Config.syslog.active && networkManager->isConnected()) {
                        SYSLOG_Utils::logCRCError(packet, rssi, snr, freqError);
                    }
                    packet = "";
                } else {
                    Utils::print(F("failed, code "));
                    Utils::println(String(state));
                    packet = "";
                }
            }
        }
        return packet;
    }

    void wakeRadio() {
        radio.startReceive();
    }

    void sleepRadio() {
        radio.sleep();
        #ifdef RADIO_ANT_SW_PIN
            digitalWrite(RADIO_ANT_SW_PIN, !RADIO_ANT_SW_ON_STATE);
        #endif
    }

}
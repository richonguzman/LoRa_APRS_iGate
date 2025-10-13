/* Copyright (C) 2025 Ricardo Guzman - CA2RXU
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

#include <RadioLib.h>
#include <WiFi.h>
#include "configuration.h"
#include "aprs_is_utils.h"
#include "station_utils.h"
#include "board_pinout.h"
#include "syslog_utils.h"
#include "ntp_utils.h"
#include "display.h"
#include "utils.h"


extern Configuration    Config;
extern uint32_t         lastRxTime;
extern bool             packetIsBeacon;

extern std::vector<ReceivedPacket> receivedPackets;

bool operationDone   = true;
bool transmitFlag    = true;

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


namespace LoRa_Utils {

    void setFlag(void) {
        operationDone = true;
    }

    void setup() {
        #if defined (LIGHTGATEWAY_1_0) || defined(LIGHTGATEWAY_PLUS_1_0)
            pinMode(RADIO_VCC_PIN,OUTPUT);
            digitalWrite(RADIO_VCC_PIN,HIGH);
            loraSPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN, RADIO_CS_PIN);
        #else
            SPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN);
        #endif
        float freq = (float)Config.loramodule.rxFreq / 1000000;
        #if defined(RADIO_HAS_XTAL)
            radio.XTAL = true;
        #endif
        int state = radio.begin(freq);
        if (state == RADIOLIB_ERR_NONE) {
            Utils::println("Initializing LoRa Module");
        } else {
            Utils::println("Starting LoRa failed! State: " + String(state));
            while (true);
        }
        #if defined(HAS_SX1262) || defined(HAS_SX1268) || defined(HAS_LLCC68)
            radio.setDio1Action(setFlag);
        #endif
        #if defined(HAS_SX1278) || defined(HAS_SX1276)
            radio.setDio0Action(setFlag, RISING);
        #endif
        radio.setSpreadingFactor(Config.loramodule.rxSpreadingFactor);
        radio.setCodingRate(Config.loramodule.rxCodingRate4);
        float signalBandwidth = Config.loramodule.rxSignalBandwidth/1000;
        radio.setBandwidth(signalBandwidth);        
        radio.setCRC(true);

        #if (defined(RADIO_RXEN) && defined(RADIO_TXEN))    // QRP Labs LightGateway has 400M22S (SX1268)
            radio.setRfSwitchPins(RADIO_RXEN, RADIO_TXEN);
        #endif

        #ifdef HAS_1W_LORA  // Ebyte E22 400M30S (SX1268) / 900M30S (SX1262) / Ebyte E220 400M30S (LLCC68)
            state = radio.setOutputPower(Config.loramodule.power); // max value 20dB for 1W modules as they have Low Noise Amp
            radio.setCurrentLimit(140); // to be validated (100 , 120, 140)?
        #endif
        #if defined(HAS_SX1278) || defined(HAS_SX1276)
            state = radio.setOutputPower(Config.loramodule.power); // max value 20dB for 400M30S as it has Low Noise Amp
            radio.setCurrentLimit(100); // to be validated (80 , 100)?
        #endif
        #if (defined(HAS_SX1268) || defined(HAS_SX1262)) && !defined(HAS_1W_LORA)
            state = radio.setOutputPower(Config.loramodule.power + 2); // values available: 10, 17, 22 --> if 20 in tracker_conf.json it will be updated to 22.
            radio.setCurrentLimit(140);
        #endif

        #if defined(HAS_SX1262) || defined(HAS_SX1268) || defined(HAS_LLCC68)
            radio.setRxBoostedGainMode(true);
        #endif

        if (state == RADIOLIB_ERR_NONE) {
            Utils::println("init : LoRa Module    ...     done!");
        } else {
            Utils::println("Starting LoRa failed! State: " + String(state));
            while (true);
        }
    }

    void changeFreqTx() {
        delay(500);
        float freq = (float)Config.loramodule.txFreq / 1000000;
        radio.setFrequency(freq);
        radio.setSpreadingFactor(Config.loramodule.txSpreadingFactor);
        radio.setCodingRate(Config.loramodule.txCodingRate4);
        radio.setBandwidth(Config.loramodule.txSignalBandwidth);
    }

    void changeFreqRx() {
        delay(500);
        float freq = (float)Config.loramodule.rxFreq / 1000000;
        radio.setFrequency(freq);
        radio.setSpreadingFactor(Config.loramodule.rxSpreadingFactor);
        radio.setCodingRate(Config.loramodule.rxCodingRate4);
        radio.setBandwidth(Config.loramodule.rxSignalBandwidth);
    }

    void sendNewPacket(const String& newPacket) {
        if (!Config.loramodule.txActive) return;

        if (Config.loramodule.txFreq != Config.loramodule.rxFreq) {
            if (!packetIsBeacon || (packetIsBeacon && !Config.digi.beaconOnRxFreq)) {
                changeFreqTx();
            }
        }
        
        #ifdef INTERNAL_LED_PIN
            if (Config.digi.ecoMode != 1) digitalWrite(INTERNAL_LED_PIN, HIGH);     // disabled in Ultra Eco Mode
        #endif
        int state = radio.transmit("\x3c\xff\x01" + newPacket);
        transmitFlag = true;
        if (state == RADIOLIB_ERR_NONE) {
            if (Config.syslog.active && WiFi.status() == WL_CONNECTED) {
                SYSLOG_Utils::log(3, newPacket, 0, 0.0, 0);    // TX
            }
            Utils::print("---> LoRa Packet Tx : ");
            Utils::println(newPacket);
        } else {
            Utils::print(F("failed, code "));
            Utils::println(String(state));
        }
        #ifdef INTERNAL_LED_PIN
            if (Config.digi.ecoMode != 1) digitalWrite(INTERNAL_LED_PIN, LOW);      // disabled in Ultra Eco Mode
        #endif
        if (Config.loramodule.txFreq != Config.loramodule.rxFreq) {
            if (!packetIsBeacon || (packetIsBeacon && !Config.digi.beaconOnRxFreq)) {
                changeFreqRx();
            }
        }
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
                        if (packet.substring(0,3) == "\x3c\xff\x01" && !STATION_Utils::isBlacklisted(sender)){   // avoid processing BlackListed stations
                            rssi        = radio.getRSSI();
                            snr         = radio.getSNR();
                            freqError   = radio.getFrequencyError();
                            Utils::println("<--- LoRa Packet Rx : " + packet.substring(3));
                            Utils::println("(RSSI:" + String(rssi) + " / SNR:" + String(snr) + " / FreqErr:" + String(freqError) + ")");

                            if (Config.digi.ecoMode == 0) {
                                if (receivedPackets.size() >= 10) {
                                    receivedPackets.erase(receivedPackets.begin());
                                }
                                ReceivedPacket receivedPacket;
                                receivedPacket.rxTime   = NTP_Utils::getFormatedTime();
                                receivedPacket.packet   = packet.substring(3);
                                receivedPacket.RSSI     = rssi;
                                receivedPacket.SNR      = snr;
                                receivedPackets.push_back(receivedPacket);
                            }

                            if (Config.syslog.active && WiFi.status() == WL_CONNECTED) {
                                SYSLOG_Utils::log(1, packet, rssi, snr, freqError); // RX
                            }
                        } else {
                            packet = "";
                        }
                        lastRxTime = millis();
                        return packet;
                    }
                } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
                    rssi        = radio.getRSSI();
                    snr         = radio.getSNR();
                    freqError   = radio.getFrequencyError();
                    Utils::println(F("CRC error!"));
                    if (Config.syslog.active && WiFi.status() == WL_CONNECTED) {
                        SYSLOG_Utils::log(0, packet, rssi, snr, freqError); // CRC
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
    }

}
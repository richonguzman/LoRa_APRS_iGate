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
extern std::vector<ReceivedPacket> receivedPackets;

// OPTIMIERUNG: Volatile für ISR-Zugriff, IRAM_ATTR für ESP32
volatile bool operationDone = true;
volatile bool transmitFlag = true;
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

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
#if defined(HAS_LLCC68)
    LLCC68 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
#endif

int rssi, freqError;
float snr;

// String Pool für häufig verwendete Strings (OPTIMIERUNG)
namespace {
    const char PREAMBLE[] PROGMEM = "\x3c\xff\x01";
}

namespace LoRa_Utils {

    // OPTIMIERUNG: ISR-sichere Flag-Behandlung
    void IRAM_ATTR setFlag(void) {
        portENTER_CRITICAL_ISR(&mux);
        operationDone = true;
        portEXIT_CRITICAL_ISR(&mux);
    }

    void setup() {
        #if defined (LIGHTGATEWAY_1_0) || defined(LIGHTGATEWAY_PLUS_1_0)
            pinMode(RADIO_VCC_PIN, OUTPUT);
            digitalWrite(RADIO_VCC_PIN, HIGH);
            loraSPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN, RADIO_CS_PIN);
        #else
            SPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN);
        #endif
        
        float freq = (float)Config.loramodule.rxFreq / 1000000.0f;
        
        #if defined(RADIO_HAS_XTAL)
            radio.XTAL = true;
        #endif
        
        int state = radio.begin(freq);
        if (state == RADIOLIB_ERR_NONE) {
            Utils::println("Initializing LoRa Module");
        } else {
            Utils::print("Starting LoRa failed! State: ");
            Utils::println(String(state));
            while (true) { delay(1000); }
        }
        
        #if defined(HAS_SX1262) || defined(HAS_SX1268) || defined(HAS_LLCC68)
            radio.setDio1Action(setFlag);
        #endif
        #if defined(HAS_SX1278) || defined(HAS_SX1276)
            radio.setDio0Action(setFlag, RISING);
        #endif
        
        radio.setSpreadingFactor(Config.loramodule.spreadingFactor);
        float signalBandwidth = Config.loramodule.signalBandwidth / 1000.0f;
        radio.setBandwidth(signalBandwidth);
        radio.setCodingRate(Config.loramodule.codingRate4);
        radio.setCRC(true);

        #if (defined(RADIO_RXEN) && defined(RADIO_TXEN))
            radio.setRfSwitchPins(RADIO_RXEN, RADIO_TXEN);
        #endif

        #ifdef HAS_1W_LORA
            state = radio.setOutputPower(Config.loramodule.power);
            radio.setCurrentLimit(140);
        #endif

        #if (defined(HAS_SX1268) || defined(HAS_SX1262)) && !defined(HAS_1W_LORA)
            state = radio.setOutputPower(Config.loramodule.power + 2);
            radio.setCurrentLimit(140);
        #endif
        
        #if defined(HAS_SX1278) || defined(HAS_SX1276)
            state = radio.setOutputPower(Config.loramodule.power);
            radio.setCurrentLimit(100);
        #endif

        #if defined(HAS_SX1262) || defined(HAS_SX1268) || defined(HAS_LLCC68)
            radio.setRxBoostedGainMode(true);
        #endif

        if (state == RADIOLIB_ERR_NONE) {
            Utils::println("init : LoRa Module    ...     done!");
        } else {
            Utils::print("Starting LoRa failed! State: ");
            Utils::println(String(state));
            while (true) { delay(1000); }
        }
    }

    void changeFreqTx() {
        delay(500);
        float freq = (float)Config.loramodule.txFreq / 1000000.0f;
        radio.setFrequency(freq);
    }

    void changeFreqRx() {
        delay(500);
        float freq = (float)Config.loramodule.rxFreq / 1000000.0f;
        radio.setFrequency(freq);
    }

    void sendNewPacket(const String& newPacket) {
        if (!Config.loramodule.txActive) return;

        if (Config.loramodule.txFreq != Config.loramodule.rxFreq) {
            changeFreqTx();
        }
        
        #ifdef INTERNAL_LED_PIN
            if (Config.digi.ecoMode != 1) {
                digitalWrite(INTERNAL_LED_PIN, HIGH);
            }
        #endif
        
        // OPTIMIERUNG: String Reserve statt mehrfacher Konkatenation
        String txPacket;
        txPacket.reserve(newPacket.length() + 3);
        txPacket = FPSTR(PREAMBLE);
        txPacket += newPacket;
        
        int state = radio.transmit(txPacket);
        
        // OPTIMIERUNG: Atomic flag set
        portENTER_CRITICAL(&mux);
        transmitFlag = true;
        portEXIT_CRITICAL(&mux);
        
        if (state == RADIOLIB_ERR_NONE) {
            if (Config.syslog.active && WiFi.status() == WL_CONNECTED) {
                SYSLOG_Utils::log(3, newPacket, 0, 0.0, 0);
            }
            Utils::print("---> LoRa Packet Tx : ");
            Utils::println(newPacket);
        } else {
            Utils::print(F("failed, code "));
            Utils::println(String(state));
        }
        
        #ifdef INTERNAL_LED_PIN
            if (Config.digi.ecoMode != 1) {
                digitalWrite(INTERNAL_LED_PIN, LOW);
            }
        #endif
        
        if (Config.loramodule.txFreq != Config.loramodule.rxFreq) {
            changeFreqRx();
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
        
        // OPTIMIERUNG: Atomic flag read
        bool isDone = false;
        portENTER_CRITICAL(&mux);
        isDone = operationDone;
        if (isDone) {
            operationDone = false;
        }
        portEXIT_CRITICAL(&mux);
        
        if (isDone) {
            bool isTransmit = false;
            portENTER_CRITICAL(&mux);
            isTransmit = transmitFlag;
            portEXIT_CRITICAL(&mux);
            
            if (isTransmit) {
                radio.startReceive();
                portENTER_CRITICAL(&mux);
                transmitFlag = false;
                portEXIT_CRITICAL(&mux);
            } else {
                int state = radio.readData(packet);
                if (state == RADIOLIB_ERR_NONE) {
                    if (packet.length() > 3) {
                        String sender = packet.substring(3, packet.indexOf(">"));
                        
                        // OPTIMIERUNG: Substring nur einmal
                        const char* preambleCheck = packet.c_str();
                        if (preambleCheck[0] == '\x3c' && 
                            preambleCheck[1] == '\xff' && 
                            preambleCheck[2] == '\x01' && 
                            !STATION_Utils::isBlacklisted(sender)) {
                            
                            rssi = radio.getRSSI();
                            snr = radio.getSNR();
                            freqError = radio.getFrequencyError();
                            
                            Utils::println("<--- LoRa Packet Rx : " + packet.substring(3));
                            Utils::print("(RSSI:");
                            Utils::print(String(rssi));
                            Utils::print(" / SNR:");
                            Utils::print(String(snr));
                            Utils::print(" / FreqErr:");
                            Utils::print(String(freqError));
                            Utils::println(")");
                            
                            if (Config.syslog.active && WiFi.status() == WL_CONNECTED) {
                                SYSLOG_Utils::log(0, packet.substring(3), rssi, snr, freqError);
                            }
                            
                            lastRxTime = millis();
                        } else {
                            packet = "";
                        }
                    } else {
                        packet = "";
                    }
                } else if (state != RADIOLIB_ERR_RX_TIMEOUT) {
                    Utils::print(F("failed, code "));
                    Utils::println(String(state));
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

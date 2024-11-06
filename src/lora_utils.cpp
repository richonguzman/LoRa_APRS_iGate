#include <RadioLib.h>
#include <WiFi.h>
#include "configuration.h"
#include "aprs_is_utils.h"
#include "board_pinout.h"
#include "syslog_utils.h"
#include "ntp_utils.h"
#include "display.h"
#include "utils.h"

extern Configuration    Config;
extern uint32_t         lastRxTime;

extern std::vector<ReceivedPacket> receivedPackets;

bool operationDone   = true;
bool transmitFlag    = true;

#ifdef HAS_SX1262
    SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
#endif
#ifdef HAS_SX1268
    #if defined(LIGHTGATEWAY_1_0)
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
        #ifdef LIGHTGATEWAY_1_0
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
            if (!Config.lowPowerMode) {
                radio.setDio1Action(setFlag);
            } else {
                radio.setDIOMapping(1, RADIOLIB_SX126X_IRQ_RX_DONE);
            }
        #endif
        #if defined(HAS_SX1278) || defined(HAS_SX1276)
            radio.setDio0Action(setFlag, RISING);
        #endif
        radio.setSpreadingFactor(Config.loramodule.spreadingFactor);
        float signalBandwidth = Config.loramodule.signalBandwidth/1000;
        radio.setBandwidth(signalBandwidth);
        radio.setCodingRate(Config.loramodule.codingRate4);
        radio.setCRC(true);

        #if (defined(RADIO_RXEN) && defined(RADIO_TXEN)) || defined(LIGHTGATEWAY_1_0)   // QRP Labs LightGateway has 400M22S (SX1268)
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
    }

    void changeFreqRx() {
        delay(500);
        float freq = (float)Config.loramodule.rxFreq / 1000000;
        radio.setFrequency(freq);
    }

    void sendNewPacket(const String& newPacket) {
        if (!Config.loramodule.txActive) return;

        if (Config.loramodule.txFreq != Config.loramodule.rxFreq) {
            changeFreqTx();
        }
        
        #ifdef INTERNAL_LED_PIN
            if (!Config.digi.ecoMode) digitalWrite(INTERNAL_LED_PIN, HIGH);
        #endif
        int state = radio.transmit("\x3c\xff\x01" + newPacket);
        transmitFlag = true;
        if (state == RADIOLIB_ERR_NONE) {
            if (Config.syslog.active && WiFi.status() == WL_CONNECTED) {
                SYSLOG_Utils::log(3, newPacket, 0, 0.0, 0);    // TX
            }
            Utils::print("---> LoRa Packet Tx    : ");
            Utils::println(newPacket);
        } else {
            Utils::print(F("failed, code "));
            Utils::println(String(state));
        }
        #ifdef INTERNAL_LED_PIN
            if (!Config.digi.ecoMode) digitalWrite(INTERNAL_LED_PIN, LOW);
        #endif
        if (Config.loramodule.txFreq != Config.loramodule.rxFreq) {
            changeFreqRx();
        }
    }

    /*String packetSanitization(const String& packet) {
        String sanitizedPacket = packet;
        if (packet.indexOf("\0") > 0) {
            sanitizedPacket.replace("\0", "");
        }
        if (packet.indexOf("\r") > 0) {
            sanitizedPacket.replace("\r", "");
        }
        if (packet.indexOf("\n") > 0) {
            sanitizedPacket.replace("\n", "");
        }
        return sanitizedPacket;
    }*/

    void startReceive() {
        radio.startReceive();
    }

    String receivePacket() {
        String packet = "";
        if (operationDone || Config.lowPowerMode) {
            operationDone = false;
            if (transmitFlag && !Config.lowPowerMode) {
                radio.startReceive();
                transmitFlag = false;
            } else {
                int state = radio.readData(packet);
                if (state == RADIOLIB_ERR_NONE) {
                    if (packet != "") {
                        rssi        = radio.getRSSI();
                        snr         = radio.getSNR();
                        freqError   = radio.getFrequencyError();
                        Utils::println("<--- LoRa Packet Rx    : " + packet.substring(3));
                        Utils::println("(RSSI:" + String(rssi) + " / SNR:" + String(snr) + " / FreqErr:" + String(freqError) + ")");

                        if (!Config.lowPowerMode && !Config.digi.ecoMode) {
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

    void sleepRadio() {
        radio.sleep();
    }

}
#include <RadioLib.h>
#include <WiFi.h>
#include "configuration.h"
#include "aprs_is_utils.h"
#include "syslog_utils.h"
#include "pins_config.h"
#include "display.h"
#include "utils.h"

extern Configuration    Config;
extern uint32_t         lastRxTime;

extern std::vector<ReceivedPacket> receivedPackets;

bool transmissionFlag   = true;
bool enableInterrupt    = true;

#ifdef HAS_SX1262
    SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
#endif

#ifdef HAS_SX1268
    SX1268 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
#endif

#ifdef HAS_SX1278
    SX1278 radio = new Module(RADIO_CS_PIN, RADIO_BUSY_PIN, RADIO_RST_PIN);
#endif

#ifdef HAS_SX1276
    SX1276 radio = new Module(RADIO_CS_PIN, RADIO_BUSY_PIN, RADIO_RST_PIN);
#endif

int rssi, freqError;
float snr;

namespace LoRa_Utils {

    void setFlag(void) {
        if(!enableInterrupt) {
            return;
        }
        transmissionFlag = true;
    }

    void setup() {
        SPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN);
        float freq = (float)Config.loramodule.rxFreq / 1000000;
        int state = radio.begin(freq);
        if (state == RADIOLIB_ERR_NONE) {
            Utils::println("Initializing LoRa Module");
        } else {
            Utils::println("Starting LoRa failed!");
            while (true);
        }
        #if defined(HAS_SX1262) || defined(HAS_SX1268)
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

        #if defined(ESP32_DIY_1W_LoRa)
            radio.setRfSwitchPins(RADIO_RXEN, RADIO_TXEN);
        #endif

        #if defined(HAS_SX1278) || defined(HAS_SX1276) || ESP32_DIY_1W_LoRa
            state = radio.setOutputPower(Config.loramodule.power); // max value 20dB for 400M30S as it has Low Noise Amp
        #endif   
        #if defined(HELTEC_V3)  || defined(HELTEC_WSL_V3) || defined(HELTEC_WS) || defined(TTGO_T_Beam_V1_0_SX1268) || defined(TTGO_T_Beam_V1_2_SX1262)
            state = radio.setOutputPower(Config.loramodule.power + 2); // values available: 10, 17, 22 --> if 20 in tracker_conf.json it will be updated to 22.
        #endif

        #if defined(HAS_SX1262) || defined(HAS_SX1268)
            radio.setRxBoostedGainMode(true);
        #endif

        if (state == RADIOLIB_ERR_NONE) {
            Utils::println("init : LoRa Module    ...     done!");
            delay(500);
            radio.startReceive();
        } else {
            Utils::println("Starting LoRa failed!");
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
            digitalWrite(INTERNAL_LED_PIN, HIGH);
        #endif
        enableInterrupt = false;
        int state = radio.transmit("\x3c\xff\x01" + newPacket);
        if (state == RADIOLIB_ERR_NONE) {
            if (Config.syslog.active && WiFi.status() == WL_CONNECTED) {
                SYSLOG_Utils::log(3, newPacket, 0, 0, 0);    // TX
            }
            Utils::print("---> LoRa Packet Tx    : ");
            Utils::println(newPacket);
        } else {
            Utils::print(F("failed, code "));
            Utils::println(String(state));
        }
        #ifdef INTERNAL_LED_PIN
            digitalWrite(INTERNAL_LED_PIN, LOW);
        #endif
        if (Config.loramodule.txFreq != Config.loramodule.rxFreq) {
            changeFreqRx();
        }
        enableInterrupt = true;
        radio.startReceive();
    }

    String packetSanitization(const String& packet) {
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
    }

    void startReceive() {
        radio.startReceive();
    }

    String receivePacket() {
        String packet = "";
        if (transmissionFlag) {
            transmissionFlag = false;
            int state = radio.readData(packet);
            if (state == RADIOLIB_ERR_NONE) {
                if (packet != "") {
                    rssi        = radio.getRSSI();
                    snr         = radio.getSNR();
                    freqError   = radio.getFrequencyError();
                    Utils::println("<--- LoRa Packet Rx    : " + packet);
                    Utils::println("(RSSI:" + String(rssi) + " / SNR:" + String(snr) + " / FreqErr:" + String(freqError) + ")");

                    if (!Config.lowPowerMode) {
                        ReceivedPacket receivedPacket;
                        receivedPacket.millis   = millis();
                        receivedPacket.packet   = packet.substring(3);
                        receivedPacket.RSSI     = rssi;
                        receivedPacket.SNR      = snr;
                        if (receivedPackets.size() >= 20) {
                            receivedPackets.erase(receivedPackets.begin());
                        }
                        receivedPackets.push_back(receivedPacket);
                    }

                    if (Config.syslog.active && WiFi.status() == WL_CONNECTED) {
                        SYSLOG_Utils::log(1, packet, rssi, snr, freqError); // RX
                    }
                    lastRxTime = millis();
                    return packet;
                }                
            } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
                rssi = radio.getRSSI();
                snr = radio.getSNR();
                freqError = radio.getFrequencyError();
                Utils::println(F("CRC error!"));
                if (Config.syslog.active && WiFi.status() == WL_CONNECTED) {
                    SYSLOG_Utils::log(0, packet, rssi, snr, freqError); // CRC
                }
                //packet = "";
                Utils::println(packet);
            } else {
                Utils::print(F("failed, code "));
                Utils::println(String(state));
                packet = "";
            }
        }
        return packet;
    }

}
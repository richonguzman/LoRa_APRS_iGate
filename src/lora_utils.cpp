#include <RadioLib.h>
#include <LoRa.h>
#include <WiFi.h>
#include "configuration.h"
#include "aprs_is_utils.h"
#include "syslog_utils.h"
#include "pins_config.h"
#include "display.h"

extern Configuration  Config;

#if defined(HELTEC_V3) || defined(HELTEC_WS) || defined(TTGO_T_Beam_V1_2_SX1262)
SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
bool transmissionFlag = true;
bool enableInterrupt = true;
#endif
#if defined(ESP32_DIY_1W_LoRa) || defined(TTGO_T_Beam_V1_0_SX1268) || defined(OE5HWN_MeshCom)
SX1268 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
bool transmissionFlag = true;
bool enableInterrupt = true;
#endif

int rssi, freqError;
float snr;


namespace LoRa_Utils {

    void setFlag(void) {
        #ifdef HAS_SX126X
        transmissionFlag = true;
        #endif
    }

    void setup() {
        #ifdef HAS_SX127X
        SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
        LoRa.setPins(LORA_CS, LORA_RST, LORA_IRQ);
        long freq = Config.loramodule.rxFreq;
        if (!LoRa.begin(freq)) {
            Serial.println("Starting LoRa failed!");
            show_display("ERROR", "Starting LoRa failed!");
            while (true) {
                delay(1000);
            }
        }
        LoRa.setSpreadingFactor(Config.loramodule.spreadingFactor);
        LoRa.setSignalBandwidth(Config.loramodule.signalBandwidth);
        LoRa.setCodingRate4(Config.loramodule.codingRate4);
        LoRa.enableCrc();
        LoRa.setTxPower(Config.loramodule.power);
        Serial.print("init : LoRa Module    ...     done!");
        #endif
        #ifdef HAS_SX126X
        SPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN);
        float freq = (float)Config.loramodule.rxFreq/1000000;
        int state = radio.begin(freq);
        if (state == RADIOLIB_ERR_NONE) {
            Serial.print("Initializing SX126X LoRa Module");
        } else {
            Serial.println("Starting LoRa failed!");
        while (true);
        }
        radio.setDio1Action(setFlag);
        radio.setSpreadingFactor(Config.loramodule.spreadingFactor);
        float signalBandwidth = Config.loramodule.signalBandwidth/1000;
        radio.setBandwidth(signalBandwidth);
        radio.setCodingRate(Config.loramodule.codingRate4);
        radio.setCRC(true);
        #if defined(ESP32_DIY_1W_LoRa)
        radio.setRfSwitchPins(RADIO_RXEN, RADIO_TXEN);
        #endif
        #if defined(HELTEC_V3)  || defined(HELTEC_WS) || defined(TTGO_T_Beam_V1_0_SX1268) || defined(TTGO_T_Beam_V1_2_SX1262)
        state = radio.setOutputPower(Config.loramodule.power + 2); // values available: 10, 17, 22 --> if 20 in tracker_conf.json it will be updated to 22.
        #endif
        #ifdef ESP32_DIY_1W_LoRa_GPS
        state = radio.setOutputPower(Config.loramodule.power); // max value 20 (when 20dB in setup 30dB in output as 400M30S has Low Noise Amp) 
        #endif
        if (state == RADIOLIB_ERR_NONE) {
            Serial.println("init : LoRa Module    ...     done!");
        } else {
            Serial.println("Starting LoRa failed!");
            while (true);
        }
        #endif
    }

    void changeFreqTx() {
        delay(500);
        #ifdef HAS_SX127X
        LoRa.setFrequency(Config.loramodule.txFreq);
        #endif
        #ifdef HAS_SX126X
        float freq = (float)Config.loramodule.txFreq/1000000;
        radio.setFrequency(freq);
        #endif
    }

    void changeFreqRx() {
        delay(500);
        #ifdef HAS_SX127X
        LoRa.setFrequency(Config.loramodule.rxFreq);
        #endif
        #ifdef HAS_SX126X
        float freq = (float)Config.loramodule.rxFreq/1000000;
        radio.setFrequency(freq);
        #endif
    }
    
    void sendNewPacket(const String &typeOfMessage, const String &newPacket) {
        if (!Config.loramodule.txActive) return;

        if (Config.loramodule.txFreq != Config.loramodule.rxFreq) {
            changeFreqTx();    
        }

        #ifdef HAS_INTERNAL_LED
        digitalWrite(internalLedPin,HIGH);
        #endif
        #ifdef HAS_SX127X
        LoRa.beginPacket();
        LoRa.write('<');
        if (typeOfMessage == "APRS")  {
            LoRa.write(0xFF);
        } else if (typeOfMessage == "LoRa") {
            LoRa.write(0xF8);
        }
        LoRa.write(0x01);
        LoRa.write((const uint8_t *)newPacket.c_str(), newPacket.length());
        LoRa.endPacket();
        #endif
        #ifdef HAS_SX126X
        int state = radio.transmit("\x3c\xff\x01" + newPacket);
        if (state == RADIOLIB_ERR_NONE) {
            //Serial.println(F("success!"));
        } else if (state == RADIOLIB_ERR_PACKET_TOO_LONG) {
            Serial.println(F("too long!"));
        } else if (state == RADIOLIB_ERR_TX_TIMEOUT) {
            Serial.println(F("timeout!"));
        } else {
            Serial.print(F("failed, code "));
            Serial.println(state);
        }
        #endif
        #ifdef HAS_INTERNAL_LED
        digitalWrite(internalLedPin,LOW);
        #endif
        SYSLOG_Utils::log("Tx", newPacket,0,0,0);
        Serial.print("---> LoRa Packet Tx    : ");
        Serial.println(newPacket);

        if (Config.loramodule.txFreq != Config.loramodule.rxFreq) {
            changeFreqRx();
        }
    }

    String generatePacket(String aprsisPacket) {
        String firstPart, messagePart;
        aprsisPacket.trim();
        firstPart = aprsisPacket.substring(0, aprsisPacket.indexOf(","));
        messagePart = aprsisPacket.substring(aprsisPacket.indexOf("::")+2);
        return firstPart + ",TCPIP,WIDE1-1," + Config.callsign + "::" + messagePart;
    }

    String packetSanitization(String packet) {
        //Serial.println(packet);
        if (packet.indexOf("\0")>0) {
            packet.replace("\0","");
        }
        if (packet.indexOf("\r")>0) {
            packet.replace("\r","");
        }
        if (packet.indexOf("\n")>0) {
            packet.replace("\n","");
        }
        return packet;
    }

    String receivePacket() {
        String loraPacket = "";
        #ifdef HAS_SX127X
        int packetSize = LoRa.parsePacket();
        if (packetSize) {
            while (LoRa.available()) {
                int inChar = LoRa.read();
                loraPacket += (char)inChar;
            }
            rssi      = LoRa.packetRssi();
            snr       = LoRa.packetSnr();
            freqError = LoRa.packetFrequencyError();
        }
        #endif
        #ifdef HAS_SX126X
        if (transmissionFlag) {
            transmissionFlag = false;
            radio.startReceive();
            int state = radio.readData(loraPacket);
            if (state == RADIOLIB_ERR_NONE) {
                if(!loraPacket.isEmpty()) {
                    Serial.println("LoRa Rx ---> " + loraPacket);
                }                
                rssi      = radio.getRSSI();
                snr       = radio.getSNR();
                freqError = radio.getFrequencyError();
            } else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
                // timeout occurred while waiting for a packet
            } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
                Serial.println(F("CRC error!"));
            } else {
                Serial.print(F("failed, code "));
                Serial.println(state);
            }
        }
        #endif

        if ((loraPacket.indexOf("\0")!=-1) || (loraPacket.indexOf("\r")!=-1) || (loraPacket.indexOf("\n")!=-1)) {
            loraPacket = packetSanitization(loraPacket);
        }

        #ifndef TextSerialOutputForApp
        if (loraPacket!="") {
            Serial.println("(RSSI:" +String(rssi) + " / SNR:" + String(snr) +  " / FreqErr:" + String(freqError) + ")");
        }
        #endif
        
        if (Config.syslog.active && WiFi.status() == WL_CONNECTED && loraPacket != "") {
            SYSLOG_Utils::log("Rx", loraPacket, rssi, snr, freqError);
        }
        return loraPacket;
    }

}
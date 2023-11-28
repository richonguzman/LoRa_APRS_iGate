#include <WiFi.h>
#include "configuration.h"
#include "station_utils.h"
#include "aprs_is_utils.h"
#include "lora_utils.h"
#include "digi_utils.h"
#include "wifi_utils.h"
#include "gps_utils.h"
#include "display.h"
#include "utils.h"

extern Configuration    Config;
extern WiFiClient       espClient;
extern int              stationMode;
extern uint32_t         lastScreenOn;
extern int              lastStationModeState;
extern String           iGateBeaconPacket;
extern String           firstLine;
extern String           secondLine;
extern String           thirdLine;
extern String           fourthLine;
extern String           fifthLine;
extern String           sixthLine;
extern String           seventhLine;

namespace DIGI_Utils {

    void processPacket(String packet) {
        String loraPacket;
        if (packet != "") {
            Serial.print("Received Lora Packet   : " + String(packet));
            if ((packet.substring(0, 3) == "\x3c\xff\x01") && (packet.indexOf("NOGATE") == -1)) {
                Serial.println("   ---> APRS LoRa Packet");
                String sender = packet.substring(3,packet.indexOf(">"));
                STATION_Utils::updateLastHeard(sender);
                STATION_Utils::updatePacketBuffer(packet);
                Utils::typeOfPacket(packet, "Digi");
                if ((stationMode==3 || stationMode==5 || stationMode==6) && (packet.indexOf("WIDE1-1") > 10)) { // ver lo de WIDE para sM=6
                    if (stationMode==6 && ((WiFi.status()==WL_CONNECTED) && espClient.connected())) {
                        espClient.write(APRS_IS_Utils::createPacket(packet).c_str());
                        Serial.print("(Uploaded to APRS-IS)");
                    }
                    if (stationMode==6) {
                        loraPacket = packet.substring(3) + " test sM6";
                        delay(5000);
                    } else { 
                        loraPacket = packet.substring(3);
                        delay(500);
                    }
                    loraPacket.replace("WIDE1-1", Config.callsign + "*");
                    LoRa_Utils::sendNewPacket("APRS", loraPacket);
                    display_toggle(true);
                    lastScreenOn = millis();
                } else if (stationMode==4){
                    if (packet.indexOf("WIDE1-1") == -1) {
                        loraPacket = packet.substring(3,packet.indexOf(":")) + "," + Config.callsign + "*" + packet.substring(packet.indexOf(":"));
                    } else {
                        loraPacket = packet.substring(3,packet.indexOf(",")+1) + Config.callsign + "*" + packet.substring(packet.indexOf(","));
                    }
                    delay(500);
                    if (stationMode==4) {
                        LoRa_Utils::changeFreqTx();
                    }
                    LoRa_Utils::sendNewPacket("APRS", loraPacket);
                    if (stationMode==4) {
                        LoRa_Utils::changeFreqRx();
                    }
                    display_toggle(true);
                    lastScreenOn = millis();
                }
            } else {
                Serial.println("   ---> LoRa Packet Ignored (first 3 bytes or NOGATE)\n");
            }
        }
    }

    void loop() {
        if (stationMode==3 || stationMode==4 || stationMode==5) {
            if (lastStationModeState==0 && stationMode==5) {
                iGateBeaconPacket = GPS_Utils::generateBeacon();
                lastStationModeState = 1;
                String Tx = String(Config.loramodule.digirepeaterTxFreq);
                secondLine = "Rx:" + String(Tx.substring(0,3)) + "." + String(Tx.substring(3,6));
                secondLine += " Tx:" + String(Tx.substring(0,3)) + "." + String(Tx.substring(3,6));
                thirdLine = "<<   DigiRepeater  >>";
            }
            Utils::checkDisplayInterval();
            Utils::checkBeaconInterval();
            show_display(firstLine, secondLine, thirdLine, fourthLine, fifthLine, sixthLine, seventhLine, 0);
            processPacket(LoRa_Utils::receivePacket());
        } else if (stationMode==6) {
            if (WiFi.status() != WL_CONNECTED) {
                WIFI_Utils::startWiFi();
            }
            if (!espClient.connected()) {
                APRS_IS_Utils::connect();
            }
            String Tx = String(Config.loramodule.digirepeaterTxFreq);
            secondLine = "Rx:" + String(Tx.substring(0,3)) + "." + String(Tx.substring(3,6));
            secondLine += " Tx:" + String(Tx.substring(0,3)) + "." + String(Tx.substring(3,6));
            thirdLine = "<<   Digi + iGate  >>";
            Utils::checkDisplayInterval();
            Utils::checkBeaconInterval();
            show_display(firstLine, secondLine, thirdLine, fourthLine, fifthLine, sixthLine, seventhLine, 0);
            processPacket(LoRa_Utils::receivePacket());
        }
    }

}
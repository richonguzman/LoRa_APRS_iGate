#include <WiFi.h>
#include "configuration.h"
#include "aprs_is_utils.h"
#include "station_utils.h"
#include "syslog_utils.h"
#include "query_utils.h"
#include "A7670_utils.h"
#include "digi_utils.h"
#include "display.h"
#include "utils.h"

extern Configuration        Config;
extern WiFiClient           espClient;
extern uint32_t             lastScreenOn;
extern String               firstLine;
extern String               secondLine;
extern String               thirdLine;
extern String               fourthLine;
extern String               fifthLine;
extern String               sixthLine;
extern String               seventhLine;

extern std::vector<String>  outputPacketBuffer;
extern uint32_t             lastRxTime;

extern bool                 modemLoggedToAPRSIS;

#ifdef ESP32_DIY_LoRa_A7670
extern bool                 stationBeacon;
#endif


namespace APRS_IS_Utils {

    void upload(String line) {
        espClient.print(line + "\r\n");
    }

    void connect() {
        uint8_t count = 0;
        String aprsauth;
        Serial.print("Connecting to APRS-IS ...     ");
        while (!espClient.connect(Config.aprs_is.server.c_str(), Config.aprs_is.port) && count < 20) {
            Serial.println("Didn't connect with server...");
            delay(1000);
            espClient.stop();
            espClient.flush();
            Serial.println("Run client.stop");
            Serial.println("Trying to connect with Server: " + String(Config.aprs_is.server) + " AprsServerPort: " + String(Config.aprs_is.port));
            count++;
            Serial.println("Try: " + String(count));
        }
        if (count == 20) {
            Serial.println("Tried: " + String(count) + " FAILED!");
        }
        else {
            Serial.println("Connected!\n(Server: " + String(Config.aprs_is.server) + " / Port: " + String(Config.aprs_is.port) + ")");

            // String filter = "t/m/" + Config.callsign + "/" + (String)Config.aprs_is.reportingDistance;

            aprsauth = "user " + Config.callsign + " pass " + Config.aprs_is.passcode + " vers CA2RXU_LoRa_iGate 1.3 filter " + Config.aprs_is.filter;
            upload(aprsauth);
            delay(200);
        }
    }

    void checkStatus() {
        String wifiState, aprsisState;
        if (WiFi.status() == WL_CONNECTED) {
            wifiState = "OK";
        } else {
            wifiState = "AP";
            if (!Config.display.alwaysOn && Config.display.timeout != 0) {
                display_toggle(true);
            }
            lastScreenOn = millis();
        }

        if (!Config.aprs_is.active) {
            aprsisState = "OFF";
        } else {
            #ifdef ESP32_DIY_LoRa_A7670
            if (modemLoggedToAPRSIS) {
                aprsisState = "OK";
            } else {
                aprsisState = "--";
            }
            #else
            if (espClient.connected()) {
                aprsisState = "OK";
            } else {
                aprsisState = "--";
            }
            #endif
            if(aprsisState == "--" && !Config.display.alwaysOn && Config.display.timeout != 0) {
                display_toggle(true);
                lastScreenOn = millis();
            }
        }
        secondLine = "WiFi: " + wifiState + " APRS-IS: " + aprsisState;
    }

    String buildPacketToUpload(String packet) {
        if (!(Config.aprs_is.active && Config.digi.mode == 0)) { // Check if NOT only IGate
            return packet.substring(3, packet.indexOf(":")) + ",qAR," + Config.callsign + packet.substring(packet.indexOf(":"));
        }
        else {
            return packet.substring(3, packet.indexOf(":")) + ",qAO," + Config.callsign + packet.substring(packet.indexOf(":"));
        }
    }

    String buildPacketToTx(String aprsisPacket) {
        String firstPart, messagePart;
        aprsisPacket.trim();
        firstPart = aprsisPacket.substring(0, aprsisPacket.indexOf(","));
        messagePart = aprsisPacket.substring(aprsisPacket.indexOf("::") + 2);
        return firstPart + ",TCPIP,WIDE1-1," + Config.callsign + "::" + messagePart;
    }

    bool processReceivedLoRaMessage(String sender, String packet) {
        String ackMessage, receivedMessage;
        if (packet.indexOf("{") > 0) {     // ack?
            ackMessage = "ack" + packet.substring(packet.indexOf("{") + 1);
            ackMessage.trim();
            //Serial.println(ackMessage);
            for (int i = sender.length(); i < 9; i++) {
                sender += ' ';
            }
            if (Config.beacon.path == "") {
                STATION_Utils::addToOutputPacketBuffer(Config.callsign + ">APLRG1,RFONLY::" + sender + ":" + ackMessage);
            } else {
                STATION_Utils::addToOutputPacketBuffer(Config.callsign + ">APLRG1,RFONLY," + Config.beacon.path + "::" + sender + ":" + ackMessage);
            }

            receivedMessage = packet.substring(packet.indexOf(":") + 1, packet.indexOf("{"));
        } else {
            receivedMessage = packet.substring(packet.indexOf(":") + 1);
        }
        if (receivedMessage.indexOf("?") == 0) {
            delay(2000);
            if (!Config.display.alwaysOn && Config.display.timeout != 0) {
                display_toggle(true);
            }
            STATION_Utils::addToOutputPacketBuffer(QUERY_Utils::process(receivedMessage, sender, "LoRa"));
            lastScreenOn = millis();
            show_display(firstLine, secondLine, thirdLine, fourthLine, fifthLine, "Callsign = " + sender, "TYPE --> QUERY", 0);
            return true;
        }
        else {
            return false;
        }
    }

    void processLoRaPacket(String packet) {
        if (espClient.connected() || modemLoggedToAPRSIS) {
            bool queryMessage = false;
            String aprsPacket, Sender, AddresseeAndMessage, Addressee;
            if (packet != "") {
                if ((packet.substring(0, 3) == "\x3c\xff\x01") && (packet.indexOf("TCPIP") == -1) && (packet.indexOf("NOGATE") == -1) && (packet.indexOf("RFONLY") == -1)) {
                    Sender = packet.substring(3, packet.indexOf(">"));
                    if (Sender != Config.callsign) {   // avoid listening yourself by digirepeating
                        STATION_Utils::updateLastHeard(Sender);
                        Utils::typeOfPacket(aprsPacket, "LoRa-APRS");
                        AddresseeAndMessage = packet.substring(packet.indexOf("::") + 2);
                        Addressee = AddresseeAndMessage.substring(0, AddresseeAndMessage.indexOf(":"));
                        Addressee.trim();
                        if (packet.indexOf("::") > 10 && Addressee == Config.callsign) {      // its a message for me!
                            queryMessage = processReceivedLoRaMessage(Sender, AddresseeAndMessage);
                        }
                        if (!queryMessage) {
                            aprsPacket = buildPacketToUpload(packet);
                            if (!Config.display.alwaysOn && Config.display.timeout != 0) {
                                display_toggle(true);
                            }
                            lastScreenOn = millis();
                            #ifdef ESP32_DIY_LoRa_A7670
                            stationBeacon = true;
                            A7670_Utils::uploadToAPRSIS(aprsPacket);
                            stationBeacon = false;
                            #else
                            upload(aprsPacket);
                            #endif
                            Utils::println("---> Uploaded to APRS-IS");
                            show_display(firstLine, secondLine, thirdLine, fourthLine, fifthLine, sixthLine, seventhLine, 0);
                        }
                    }
                }
            }
        }
    }

    void processAPRSISPacket(String packet) {
        String Sender, Addressee, receivedMessage;
        if (!packet.isEmpty() && !packet.startsWith("#")) {
            Utils::print("---> APRSIS Packet Rx    :");
            Utils::println(packet);
            Sender = packet.substring(0,packet.indexOf(">"));
            if (packet.indexOf("::") > 0) {
                String AddresseeAndMessage = packet.substring(packet.indexOf("::")+2);
                Addressee = AddresseeAndMessage.substring(0, AddresseeAndMessage.indexOf(":"));
                Addressee.trim();
                if (Addressee == Config.callsign) {             // its for me!
                    if (AddresseeAndMessage.indexOf("{")>0) {     // ack?
                        String ackMessage = "ack" + AddresseeAndMessage.substring(AddresseeAndMessage.indexOf("{")+1);
                        ackMessage.trim();
                        delay(4000);
                        for (int i = Sender.length(); i < 9; i++) {
                            Sender += ' ';
                        }
                        String ackPacket = Config.callsign + ">APLRG1,TCPIP,qAC::" + Sender + ":" + ackMessage;
                        #ifdef ESP32_DIY_LoRa_A7670
                        A7670_Utils::uploadToAPRSIS(ackPacket);
                        #else
                        upload(ackPacket);
                        #endif
                        receivedMessage = AddresseeAndMessage.substring(AddresseeAndMessage.indexOf(":") + 1, AddresseeAndMessage.indexOf("{"));
                    } else {
                        receivedMessage = AddresseeAndMessage.substring(AddresseeAndMessage.indexOf(":") + 1);
                    }
                    if (receivedMessage.indexOf("?") == 0) {
                        Utils::println("Received Query APRS-IS : " + packet);
                        String queryAnswer = QUERY_Utils::process(receivedMessage, Sender, "APRSIS");
                        //Serial.println("---> QUERY Answer : " + queryAnswer.substring(0,queryAnswer.indexOf("\n")));
                        if (!Config.display.alwaysOn && Config.display.timeout != 0) {
                            display_toggle(true);
                        }
                        lastScreenOn = millis();
                        delay(500);
                        #ifdef ESP32_DIY_LoRa_A7670
                        A7670_Utils::uploadToAPRSIS(queryAnswer);
                        #else
                        upload(queryAnswer);
                        #endif
                        SYSLOG_Utils::log("APRSIS Tx", queryAnswer, 0, 0, 0);
                        fifthLine = "APRS-IS ----> APRS-IS";
                        sixthLine = Config.callsign;
                        for (int j = sixthLine.length();j < 9;j++) {
                            sixthLine += " ";
                        }
                        sixthLine += "> " + Sender;
                        seventhLine = "QUERY = " + receivedMessage;
                    }
                } else {
                    Utils::print("Received from APRS-IS  : " + packet);

                    if (Config.aprs_is.toRF && STATION_Utils::wasHeard(Addressee)) {
                        STATION_Utils::addToOutputPacketBuffer(buildPacketToTx(packet));
                        display_toggle(true);
                        lastScreenOn = millis();
                        Utils::typeOfPacket(packet, "APRS-LoRa");
                    }
                }
                show_display(firstLine, secondLine, thirdLine, fourthLine, fifthLine, sixthLine, seventhLine, 0);
            } else {
                Addressee = packet.substring(0, packet.indexOf(":"));
                Addressee.trim();
                if (Sender != Config.callsign // It's not for me
                    && Addressee.indexOf("qAX") == -1 // Authorized login
                    && Addressee.indexOf("RFONLY") == -1 // RF allowed
                    && Addressee.indexOf("NOGATE") == -1  // RF allowed
                    && Addressee.indexOf("TCPXX") == -1) { // Packet is not for Internet
                    if (Config.aprs_is.toRF // TX to RF enabled
                        && (
                            (!Config.loramodule.rxActive && millis() - lastTxFromIs >= 30000) // LoRa RX disabled so we wait between 2 frames TX
                            || STATION_Utils::hasHeardSomeone() // Or we have heard someone
                    )) {
                        LoRa_Utils::sendNewPacket("APRS", LoRa_Utils::generatePacketSameContent(packet));
                        display_toggle(true);
                        lastScreenOn = millis();
                        lastTxFromIs = millis();
                        Utils::typeOfPacket(packet, "APRS-LoRa");
                    }
                }
            }
        }
    }

    void listenAPRSIS() {
        #ifdef ESP32_DIY_LoRa_A7670
        A7670_Utils::listenAPRSIS();
        #else
        if (espClient.connected()) {
            if (espClient.available()) {
                String aprsisPacket = espClient.readStringUntil('\r');
                aprsisPacket.trim();
                // Serial.println(aprsisPacket);
                processAPRSISPacket(aprsisPacket);
                lastRxTime = millis();
            }
        }
        #endif
    }

}
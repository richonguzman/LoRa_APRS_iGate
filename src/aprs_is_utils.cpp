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
extern bool                 modemLoggedToAPRSIS;
extern bool                 backUpDigiMode;

uint32_t    lastRxTime      = millis();
bool        passcodeValid   = false;

#ifdef HAS_A7670
    extern bool                 stationBeacon;
#endif


namespace APRS_IS_Utils {

    void upload(const String& line) {
        espClient.print(line + "\r\n");
    }

    void connect() {
        Serial.print("Connecting to APRS-IS ...     ");
        uint8_t count = 0;
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
        } else {
            Serial.println("Connected!\n(Server: " + String(Config.aprs_is.server) + " / Port: " + String(Config.aprs_is.port) + ")");
            // String filter = "t/m/" + Config.callsign + "/" + (String)Config.aprs_is.reportingDistance;
            String aprsAuth = "user ";
            aprsAuth += Config.callsign;
            aprsAuth += " pass ";
            aprsAuth += Config.aprs_is.passcode;
            aprsAuth += " vers CA2RXU_LoRa_iGate 2.0 filter ";
            aprsAuth += Config.aprs_is.filter;
            upload(aprsAuth);
        }
    }

    void checkStatus() {
        String wifiState, aprsisState;
        if (WiFi.status() == WL_CONNECTED) {
            wifiState = "OK";
        } else {
            if (backUpDigiMode || Config.digi.ecoMode) {
                wifiState = "--";
            } else {
                wifiState = "AP";
            }            
            if (!Config.display.alwaysOn && Config.display.timeout != 0) {
                displayToggle(true);
            }
            lastScreenOn = millis();
        }

        if (!Config.aprs_is.active) {
            aprsisState = "OFF";
        } else {
            #ifdef HAS_A7670
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
                displayToggle(true);
                lastScreenOn = millis();
            }
        }
        secondLine = "WiFi: ";
        secondLine += wifiState;
        secondLine += " APRS-IS: ";
        secondLine += aprsisState;
    }

    String checkForStartingBytes(const String& packet) {
        if (packet.indexOf("\x3c\xff\x01") != -1) {
            return packet.substring(0, packet.indexOf("\x3c\xff\x01"));
        } else {
            return packet;
        }
    }

    String buildPacketToUpload(const String& packet) {
        String buildedPacket = packet.substring(3, packet.indexOf(":"));
        if (!(Config.aprs_is.active && Config.digi.mode == 0)) { // Check if NOT only IGate
            buildedPacket += ",qAR,";
        } else {
            buildedPacket += ",qAO,";
        }
        buildedPacket += Config.callsign;
        buildedPacket += checkForStartingBytes(packet.substring(packet.indexOf(":")));
        return buildedPacket;
    }

    bool processReceivedLoRaMessage(const String& sender, const String& packet, bool thirdParty) {
        String receivedMessage;
        if (packet.indexOf("{") > 0) {     // ack?
            String ackMessage = "ack";
            ackMessage.concat(packet.substring(packet.indexOf("{") + 1));
            ackMessage.trim();
            //Serial.println(ackMessage);
            
            String addToBuffer = Config.callsign;
            addToBuffer += ">APLRG1";
            if (!thirdParty) addToBuffer += ",RFONLY";
            if (Config.beacon.path != "") {
                addToBuffer += ",";
                addToBuffer += Config.beacon.path;
            }
            addToBuffer += "::";

            String processedSender = sender;
            for (int i = sender.length(); i < 9; i++) {
                processedSender += ' ';
            }
            addToBuffer += processedSender;

            addToBuffer += ":";
            addToBuffer += ackMessage;
            STATION_Utils::addToOutputPacketBuffer(addToBuffer);
            receivedMessage = packet.substring(packet.indexOf(":") + 1, packet.indexOf("{"));
        } else {
            receivedMessage = packet.substring(packet.indexOf(":") + 1);
        }
        if (receivedMessage.indexOf("?") == 0) {
            if (!Config.display.alwaysOn && Config.display.timeout != 0) {
                displayToggle(true);
            }
            STATION_Utils::addToOutputPacketBuffer(QUERY_Utils::process(receivedMessage, sender, false, thirdParty));
            lastScreenOn = millis();
            displayShow(firstLine, secondLine, thirdLine, fourthLine, fifthLine, "Callsign = " + sender, "TYPE --> QUERY", 0);
            return true;
        }
        else {
            return false;
        }
    }

    void processLoRaPacket(const String& packet) {
        if (passcodeValid && (espClient.connected() || modemLoggedToAPRSIS)) {
            if (packet != "") {
                if ((packet.substring(0, 3) == "\x3c\xff\x01")  && (packet.indexOf("NOGATE") == -1) && (packet.indexOf("RFONLY") == -1)) {
                    int firstColonIndex = packet.indexOf(":");
                    if (firstColonIndex > 5 && firstColonIndex < (packet.length() - 1) && packet[firstColonIndex + 1] != '}' && packet.indexOf("TCPIP") == -1) {
                        const String& Sender = packet.substring(3, packet.indexOf(">"));
                        if (Sender != Config.callsign && Utils::checkValidCallsign(Sender) && !STATION_Utils::checkBlackList(Sender)) {
                            STATION_Utils::updateLastHeard(Sender);
                            Utils::typeOfPacket(packet.substring(3), 0);  // LoRa-APRS
                            const String& AddresseeAndMessage = packet.substring(packet.indexOf("::") + 2);
                            String Addressee = AddresseeAndMessage.substring(0, AddresseeAndMessage.indexOf(":"));
                            Addressee.trim();
                            bool queryMessage = false;
                            if (packet.indexOf("::") > 10 && Addressee == Config.callsign) {      // its a message for me!
                                queryMessage = processReceivedLoRaMessage(Sender, checkForStartingBytes(AddresseeAndMessage), false);
                            }
                            if (!queryMessage) {
                                const String& aprsPacket = buildPacketToUpload(packet);
                                if (!Config.display.alwaysOn && Config.display.timeout != 0) {
                                    displayToggle(true);
                                }
                                lastScreenOn = millis();
                                #ifdef HAS_A7670
                                    stationBeacon = true;
                                    A7670_Utils::uploadToAPRSIS(aprsPacket);
                                    stationBeacon = false;
                                #else
                                    upload(aprsPacket);
                                #endif
                                Utils::println("---> Uploaded to APRS-IS");
                                displayShow(firstLine, secondLine, thirdLine, fourthLine, fifthLine, sixthLine, seventhLine, 0);
                            }
                        }
                    }
                }
            }
        }
    }

    String buildPacketToTx(const String& aprsisPacket, uint8_t packetType) {
        String packet = aprsisPacket;
        packet.trim();
        String outputPacket = Config.callsign;
        outputPacket += ">APLRG1";
        if (Config.beacon.path != "") {
            outputPacket += ",";
            outputPacket += Config.beacon.path;
        }
        outputPacket += ":}";
        outputPacket += packet.substring(0, packet.indexOf(",")); // Callsign>Tocall
        outputPacket.concat(",TCPIP,");
        outputPacket.concat(Config.callsign);
        outputPacket.concat("*");
        switch (packetType) {
            case 0: // gps
                if (packet.indexOf(":=") > 0) {
                    outputPacket += packet.substring(packet.indexOf(":="));
                } else {
                    outputPacket += packet.substring(packet.indexOf(":!"));
                }
                break;
            case 1: // messages
                outputPacket += packet.substring(packet.indexOf("::"));
                break;
            case 2: // status
                outputPacket += packet.substring(packet.indexOf(":>"));
                break;
            case 3: // telemetry
                outputPacket += packet.substring(packet.indexOf("::"));
                break;
            case 4: // mic-e
                if (packet.indexOf(":`") > 0) {
                    outputPacket += packet.substring(packet.indexOf(":`"));
                } else {
                    outputPacket += packet.substring(packet.indexOf(":'"));
                }
                break;
            case 5: // object
                outputPacket += packet.substring(packet.indexOf(":;"));
                break;
        }
        return outputPacket;
    }

    void processAPRSISPacket(const String& packet) {
        if (!passcodeValid && packet.indexOf(Config.callsign) != -1) {
            if (packet.indexOf("unverified") != -1 ) {
                Serial.println("\n****APRS PASSCODE NOT VALID****\n");
                displayShow(firstLine, "", "    APRS PASSCODE", "    NOT VALID !!!", "", "", "", 0);
                while (1) {};
            } else if (packet.indexOf("verified") != -1 ) {
                passcodeValid = true;
            }
        }
        if (passcodeValid && !packet.startsWith("#")) {
            if (Config.aprs_is.messagesToRF && packet.indexOf("::") > 0) {
                String Sender = packet.substring(0, packet.indexOf(">"));
                const String& AddresseeAndMessage = packet.substring(packet.indexOf("::") + 2);
                String Addressee = AddresseeAndMessage.substring(0, AddresseeAndMessage.indexOf(":"));
                Addressee.trim();
                if (Addressee == Config.callsign) {                 // its for me!
                    String receivedMessage;
                    if (AddresseeAndMessage.indexOf("{") > 0) {     // ack?
                        String ackMessage = "ack";
                        ackMessage += AddresseeAndMessage.substring(AddresseeAndMessage.indexOf("{") + 1);
                        ackMessage.trim();
                        delay(4000);
                        for (int i = Sender.length(); i < 9; i++) {
                            Sender += ' ';
                        }

                        String ackPacket = Config.callsign;
                        ackPacket += ">APLRG1,TCPIP,qAC::";
                        ackPacket += Sender;
                        ackPacket += ":";
                        ackPacket += ackMessage;
                        #ifdef HAS_A7670
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
                        String queryAnswer = QUERY_Utils::process(receivedMessage, Sender, true, false);
                        //Serial.println("---> QUERY Answer : " + queryAnswer.substring(0,queryAnswer.indexOf("\n")));
                        if (!Config.display.alwaysOn && Config.display.timeout != 0) {
                            displayToggle(true);
                        }
                        lastScreenOn = millis();
                        delay(500);
                        #ifdef HAS_A7670
                            A7670_Utils::uploadToAPRSIS(queryAnswer);
                        #else
                            upload(queryAnswer);
                        #endif
                        SYSLOG_Utils::log(2, queryAnswer, 0, 0.0, 0); // APRSIS TX
                        fifthLine = "APRS-IS ----> APRS-IS";
                        sixthLine = Config.callsign;
                        for (int j = sixthLine.length();j < 9;j++) {
                            sixthLine += " ";
                        }
                        sixthLine += "> ";
                        sixthLine += Sender;
                        seventhLine = "QUERY = ";
                        seventhLine += receivedMessage;
                    }
                } else {
                    Utils::print("Received Message from APRS-IS  : " + packet);
                    if (STATION_Utils::wasHeard(Addressee) && packet.indexOf("EQNS.") == -1 && packet.indexOf("UNIT.") == -1 && packet.indexOf("PARM.") == -1) {
                        STATION_Utils::addToOutputPacketBuffer(buildPacketToTx(packet, 1));
                        displayToggle(true);
                        lastScreenOn = millis();
                        Utils::typeOfPacket(packet, 1); // APRS-LoRa
                    }
                }
                displayShow(firstLine, secondLine, thirdLine, fourthLine, fifthLine, sixthLine, seventhLine, 0);
            } else if (Config.aprs_is.objectsToRF && packet.indexOf(":;") > 0) {
                Utils::println("Received Object from APRS-IS  : " + packet);
                STATION_Utils::addToOutputPacketBuffer(buildPacketToTx(packet, 5));
                displayToggle(true);
                lastScreenOn = millis();
                Utils::typeOfPacket(packet, 1); // APRS-LoRa
            }
        }
    }

    void listenAPRSIS() {
        #ifdef HAS_A7670
            A7670_Utils::listenAPRSIS();
        #else
            if (espClient.connected()) {
                if (espClient.available()) {
                    String aprsisPacket = espClient.readStringUntil('\r');
                    // Serial.println(aprsisPacket);
                    processAPRSISPacket(aprsisPacket);
                    lastRxTime = millis();
                }
            }
        #endif
    }

    void firstConnection() {
        if (Config.aprs_is.active && (WiFi.status() == WL_CONNECTED) && !espClient.connected()) {
            connect();
            while (!passcodeValid) {
                listenAPRSIS();
            }
        }
    }

}
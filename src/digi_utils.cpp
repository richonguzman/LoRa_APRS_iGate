#include <WiFi.h>
#include "configuration.h"
#include "station_utils.h"
#include "aprs_is_utils.h"
#include "query_utils.h"
#include "digi_utils.h"
#include "wifi_utils.h"
#include "lora_utils.h"
#include "gps_utils.h"
#include "display.h"
#include "utils.h"

extern Configuration    Config;
extern uint32_t         lastScreenOn;
extern String           iGateBeaconPacket;
extern String           firstLine;
extern String           secondLine;
extern String           thirdLine;
extern String           fourthLine;
extern String           fifthLine;
extern String           sixthLine;
extern String           seventhLine;
extern bool             backUpDigiMode;


namespace DIGI_Utils {

    String buildPacket(const String& path, const String& packet, bool thirdParty, bool crossFreq) {
        if (!crossFreq) {
            String packetToRepeat = packet.substring(0, packet.indexOf(",") + 1);
            String tempPath = path;

            if (path.indexOf("WIDE1-1") != -1 && (Config.digi.mode == 2 || Config.digi.mode == 3)) {
                tempPath.replace("WIDE1-1", Config.callsign + "*");
            } else if (path.indexOf("WIDE2-") != -1 && Config.digi.mode == 3) {
                if (path.indexOf(",WIDE1*") != -1) {
                    tempPath.remove(path.indexOf(",WIDE1*"), 7);
                }
                if (path.indexOf("*") != -1) {
                    tempPath.remove(path.indexOf("*"), 1);
                }
                if (path.indexOf("WIDE2-1") != -1) {
                    tempPath.replace("WIDE2-1", Config.callsign + "*");
                } else if (path.indexOf("WIDE2-2") != -1) {
                    tempPath.replace("WIDE2-2", Config.callsign + "*,WIDE2-1");
                } else {
                    return "";
                }
            }
            packetToRepeat += tempPath;
            if (thirdParty) {
                packetToRepeat += APRS_IS_Utils::checkForStartingBytes(packet.substring(packet.indexOf(":}")));
            } else {
                packetToRepeat += APRS_IS_Utils::checkForStartingBytes(packet.substring(packet.indexOf(":")));
            }
            return packetToRepeat;
        } else {   // CrossFreq Digipeater
            String suffix = thirdParty ? ":}" : ":";
            String packetToRepeat = packet.substring(0, packet.indexOf(suffix));
            
            String terms[] = {",WIDE1*", ",WIDE2*", "*"};
            for (String term : terms) {
                int index = packetToRepeat.indexOf(term);
                if (index != -1) {
                    packetToRepeat.remove(index, term.length());
                }
            }
            packetToRepeat += ",";
            packetToRepeat += Config.callsign;
            packetToRepeat += "*";
            packetToRepeat += APRS_IS_Utils::checkForStartingBytes(packet.substring(packet.indexOf(suffix)));
            return packetToRepeat;
        }
    }

    String generateDigipeatedPacket(const String& packet, bool thirdParty){
        String temp;
        if (thirdParty) { // only header is used
            const String& header = packet.substring(0, packet.indexOf(":}"));
            temp = header.substring(header.indexOf(">") + 1);
        } else {
            temp = packet.substring(packet.indexOf(">") + 1, packet.indexOf(":"));
        }
        if (temp.indexOf(",") > 2) { // checks for path
            const String& path = temp.substring(temp.indexOf(",") + 1); // after tocall
            if (Config.digi.mode == 2 || backUpDigiMode) {
                if (path.indexOf("WIDE1-1") != - 1) {
                    return buildPacket(path, packet, thirdParty, false);
                } else if (path.indexOf("WIDE1-1") == -1 && (abs(Config.loramodule.txFreq - Config.loramodule.rxFreq) >= 125000)) { //  CrossFreq Digi
                    return buildPacket(path, packet, thirdParty, true);
                } else {
                    return "";
                }
            } else if (Config.digi.mode == 3) {
                if (path.indexOf("WIDE1-1") != -1 || path.indexOf("WIDE2-") != -1) {
                    int wide1Index = path.indexOf("WIDE1-1");
                    int wide2Index = path.indexOf("WIDE2-");

                    // WIDE1-1 && WIDE2-n   /   only WIDE1-1    /   only WIDE2-n
                    if ((wide1Index != -1 && wide2Index != -1 && wide1Index < wide2Index) || (wide1Index != -1 && wide2Index == -1) || (wide1Index == -1 && wide2Index != -1)) {
                        return buildPacket(path, packet, thirdParty, false);
                    }
                    return "";
                } else if (path.indexOf("WIDE1-1") == -1 && path.indexOf("WIDE2-") == -1 && (abs(Config.loramodule.txFreq - Config.loramodule.rxFreq) >= 125000)) {    //  CrossFreq Digi
                    return buildPacket(path, packet, thirdParty, true);
                } else {
                    return "";
                }
            } else {
                return "";
            }
        } else if (temp.indexOf(",") == -1 && (Config.digi.mode == 2 || backUpDigiMode || Config.digi.mode == 3) && (abs(Config.loramodule.txFreq - Config.loramodule.rxFreq) >= 125000)) {
            return buildPacket("", packet, thirdParty, true);
        } else {
            return "";
        }
    }

    void processLoRaPacket(const String& packet) {        
        if (packet != "") {
            if ((packet.substring(0, 3) == "\x3c\xff\x01") && (packet.indexOf("NOGATE") == -1)) {
                bool thirdPartyPacket = false;
                String temp, Sender;
                int firstColonIndex = packet.indexOf(":");
                if (firstColonIndex > 5 && firstColonIndex < (packet.length() - 1) && packet[firstColonIndex + 1] == '}' && packet.indexOf("TCPIP") > 0) {   // 3rd Party 
                    thirdPartyPacket = true;
                    temp    = packet.substring(packet.indexOf(":}") + 2);
                    Sender  = temp.substring(0, temp.indexOf(">"));
                } else {
                    temp    = packet.substring(3);
                    Sender  = packet.substring(3, packet.indexOf(">"));
                }
                if (Sender != Config.callsign && !STATION_Utils::checkBlackList(Sender)) {        // Avoid listening to own packets
                    if (!thirdPartyPacket && !Utils::checkValidCallsign(Sender)) {
                        return;
                    }
                    if (STATION_Utils::check25SegBuffer(Sender, temp.substring(temp.indexOf(":") + 2)) || Config.lowPowerMode) {
                        STATION_Utils::updateLastHeard(Sender);
                        Utils::typeOfPacket(temp, 2);    // Digi
                        bool queryMessage = false;
                        if (temp.indexOf("::") > 10) {   // it's a message
                            String AddresseeAndMessage  = temp.substring(temp.indexOf("::") + 2);
                            String Addressee            = AddresseeAndMessage.substring(0, AddresseeAndMessage.indexOf(":"));
                            Addressee.trim();
                            if (Addressee == Config.callsign) {     // it's a message for me!
                                queryMessage = APRS_IS_Utils::processReceivedLoRaMessage(Sender, AddresseeAndMessage, thirdPartyPacket);
                            }
                        }
                        if (!queryMessage) {
                            String loraPacket = generateDigipeatedPacket(packet.substring(3), thirdPartyPacket);
                            if (loraPacket != "") {
                                if (Config.lowPowerMode) {
                                    LoRa_Utils::sendNewPacket(loraPacket);
                                } else {
                                    STATION_Utils::addToOutputPacketBuffer(loraPacket);
                                }
                                displayToggle(true);
                                lastScreenOn = millis();
                            }
                        }
                    }
                }
            }
        }
    }

    void checkEcoMode() {
        if (Config.digi.ecoMode) {
            Config.display.alwaysOn     = false;
            Config.display.timeout      = 0;
            setCpuFrequencyMhz(10);
        }
    }

}
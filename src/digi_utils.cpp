#include <WiFi.h>
#include "configuration.h"
#include "station_utils.h"
#include "aprs_is_utils.h"
#include "query_utils.h"
#include "digi_utils.h"
#include "wifi_utils.h"
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

    String buildPacket(const String& path, const String& packet, bool thirdParty) {
        String packetToRepeat = packet.substring(0, packet.indexOf(",") + 1);
        String tempPath = path;
        if (Config.digi.mode == 2) {
            tempPath.replace("WIDE1-1", Config.callsign + "*");
        } else if (Config.digi.mode == 3) {
            if (path.indexOf("WIDE1-1") != -1) {
                tempPath.replace("WIDE1-1", Config.callsign + "*");
            }
            if (path.indexOf("WIDE2-1") != -1) {
                tempPath.replace("WIDE2-1", Config.callsign + "*");
            }
            if (path.indexOf(Config.callsign + "*," + Config.callsign + "*")) {
                tempPath.replace(Config.callsign + "*," + Config.callsign + "*", Config.callsign + "*");
            }
        }
        packetToRepeat += tempPath;
        if (thirdParty) {
            packetToRepeat += APRS_IS_Utils::checkForStartingBytes(packet.substring(packet.indexOf(":}")));
        } else {
            packetToRepeat += APRS_IS_Utils::checkForStartingBytes(packet.substring(packet.indexOf(":")));
        }
        return packetToRepeat;
    }

    String generateDigiRepeatedPacket(const String& packet, bool thirdParty){
        String temp;
        if (thirdParty) { // only header is used
            const String& header = packet.substring(0, packet.indexOf(":}"));
            temp = header.substring(header.indexOf(">") + 1);
        } else {
            temp = packet.substring(packet.indexOf(">") + 1, packet.indexOf(":"));
        }
        if (temp.indexOf(",") > 2) { // checks for path
            const String& path = temp.substring(temp.indexOf(",") + 1); // after tocall
            if (Config.digi.mode == 2 && path.indexOf("WIDE1-1") != - 1) {
                return buildPacket(path, packet, thirdParty);
            } else if (Config.digi.mode == 3) {
                int wide1Index = path.indexOf("WIDE1-1");
                int wide2Index = path.indexOf("WIDE2-1");

                if ((wide1Index != -1 && wide2Index == -1) || (wide1Index != -1 && wide2Index != -1 && wide1Index < wide2Index) || (wide1Index == -1 && wide2Index != -1)) {
                    return buildPacket(path, packet, thirdParty);
                } else {
                    return "";
                }
            } else {
                return "";
            }
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
                if (Sender != Config.callsign) {        // Avoid listening to own packets
                    if (!thirdPartyPacket && !Utils::checkValidCallsign(Sender)) {
                        return;
                    }
                    if (STATION_Utils::check25SegBuffer(Sender, temp.substring(temp.indexOf(":") + 2))) {
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
                            String loraPacket = generateDigiRepeatedPacket(packet.substring(3), thirdPartyPacket);
                            if (loraPacket != "") {
                                STATION_Utils::addToOutputPacketBuffer(loraPacket);
                                display_toggle(true);
                                lastScreenOn = millis();
                            }
                        }
                    }
                }
            }
        }
    }

}
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
        tempPath.replace(Config.beacon.path, Config.callsign + "*");
        packetToRepeat += tempPath;
        if (thirdParty) {
            packetToRepeat += APRS_IS_Utils::checkForStartingBytes(packet.substring(packet.indexOf(":}")));
        } else {
            packetToRepeat += APRS_IS_Utils::checkForStartingBytes(packet.substring(packet.indexOf(":")));
        }        
        return packetToRepeat;
    }


    String generateDigiRepeatedPacket(const String& packet, bool thirdParty){
        String temp, path;
        if (thirdParty) { // only header is used
            String header = packet.substring(0, packet.indexOf(":}"));
            temp = header.substring(header.indexOf(">") + 1);
        } else {
            temp = packet.substring(packet.indexOf(">") + 1, packet.indexOf(":"));
        }
        if (temp.indexOf(",") > 2) { // checks for path
            path = temp.substring(temp.indexOf(",") + 1);
            if (path.indexOf(Config.beacon.path) != -1) {
                return buildPacket(path, packet, thirdParty);
            } else {
                return "";
            }
        } else {
            return "";
        }
    }

    void processLoRaPacket(const String& packet) {
        bool queryMessage = false;
        String loraPacket, Sender, AddresseeAndMessage, Addressee;
        if (packet != "") {
            if ((packet.substring(0, 3) == "\x3c\xff\x01") && (packet.indexOf("NOGATE") == -1)) {
                if (packet.indexOf("}") > 0 && packet.indexOf("TCPIP") > 0) {   // 3rd Party 
                    String noHeaderPacket = packet.substring(packet.indexOf(":}") + 2);
                    Sender = noHeaderPacket.substring(0, noHeaderPacket.indexOf(">"));
                    if (Sender != Config.callsign) {                    // avoid processing own packets
                        if (STATION_Utils::check25SegBuffer(Sender, noHeaderPacket.substring(noHeaderPacket.indexOf(":") + 2))) {
                            STATION_Utils::updateLastHeard(Sender);
                            Utils::typeOfPacket(noHeaderPacket, 2);    // Digi
                            if (noHeaderPacket.indexOf("::") > 10) {   // it's a message
                                AddresseeAndMessage = noHeaderPacket.substring(noHeaderPacket.indexOf("::") + 2);
                                Addressee = AddresseeAndMessage.substring(0, AddresseeAndMessage.indexOf(":"));
                                Addressee.trim();
                                if (Addressee == Config.callsign) {     // it's a message for me!
                                    queryMessage = APRS_IS_Utils::processReceivedLoRaMessage(Sender, AddresseeAndMessage);
                                }
                            }
                            if (!queryMessage) {
                                loraPacket = generateDigiRepeatedPacket(packet.substring(3), true);
                                if (loraPacket != "") {
                                    STATION_Utils::addToOutputPacketBuffer(loraPacket);
                                    display_toggle(true);
                                    lastScreenOn = millis();
                                }
                            }
                        }
                    }
                } else {
                    Sender = packet.substring(3, packet.indexOf(">"));
                    if (Sender != Config.callsign && Utils::checkValidCallsign(Sender)) {
                        if (STATION_Utils::check25SegBuffer(Sender, packet.substring(packet.indexOf(":") + 2))) {
                            STATION_Utils::updateLastHeard(Sender);
                            Utils::typeOfPacket(packet.substring(3), 2);    // Digi
                            if (packet.indexOf("::") > 10) {                // it's a message
                                AddresseeAndMessage = packet.substring(packet.indexOf("::") + 2);
                                Addressee = AddresseeAndMessage.substring(0, AddresseeAndMessage.indexOf(":"));
                                Addressee.trim();
                                if (Addressee == Config.callsign) {         // its a message for me!
                                    queryMessage = APRS_IS_Utils::processReceivedLoRaMessage(Sender, AddresseeAndMessage);
                                }
                            }                            
                            if (!queryMessage) {
                                loraPacket = generateDigiRepeatedPacket(packet.substring(3), false);
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

}